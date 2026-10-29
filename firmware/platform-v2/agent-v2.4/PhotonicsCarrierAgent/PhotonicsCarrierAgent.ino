/*
  Photonics Carrier - IMU + PAW3395 planar odometry, v2.4 agent control
  ESP32-WROOM / ESP32 Dev Module. Serial 115200.
  Libraries: Adafruit BNO08x (and its Library Manager dependencies).
  No encoders, camera, flash settings, asynchronous server or user RTOS tasks.

  Fusion: BNO086 rotation vector (original working report) + optical displacement.
  Body axes: +X forward, +Y left, +Z up. Positive heading is counterclockwise.
  World axes are the body axes at origin reset. Position is relative, not GPS.
  Acceleration is displayed, NOT integrated into position. No invented Z.
  Optical mounting/sign/scale and sensor offset MUST be calibrated on hardware.
  On each boot, place flat and still, then SET LEVEL & RESET ORIGIN in the UI.
  IMU location: 15 mm behind the centre, 37 mm above the ground. These are
  NOT optical sensor offsets; translation does not change rigid-body heading.
  GPIO12 is MR2; external circuitry must not pull it HIGH at power-on.
*/
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Adafruit_BNO08x.h>
#include <esp_timer.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>
#include "MotionController.h"

carrier::MotionController motion;
uint32_t actionMotorEpoch=0;

const int IMU_RESET=26, IMU_CS=27, IMU_INT=14;
const int PAW_MOTION=4, PAW_CS=17;
const float IMU_FORWARD_OFFSET_MM=-15.0f, IMU_HEIGHT_ABOVE_GROUND_MM=37.0f;
const int MOTOR_L_IN1=13, MOTOR_L_IN2=33, MOTOR_R_IN1=32, MOTOR_R_IN2=12;
const uint16_t PAW_CPI=1600;
const SPISettings PAW_SPI(1000000, MSBFIRST, SPI_MODE3);
const char *AP_SSID="PhotonicsCarrier", *AP_PASSWORD="carrier123";
const uint32_t MOTOR_TIMEOUT_MS=750;
const uint32_t PAW_PERIOD_US=10000, MAX_OPTICAL_GAP_US=100000;
const uint32_t IMU_STALE_US=200000, GYRO_STALE_US=100000;
const uint32_t IMU_INTERVAL_US=50000; // Original working 20 Hz report rate.
const float PI_F=3.14159265358979323846f;

WebServer server(80);
bool invertLeft=false, invertRight=false;
int speedPercent=30;
float motorScaleLeft=1.0f,motorScaleRight=1.0f; // RAM only; 0..2, PWM capped at 255.
volatile int leftMotorCommand=0,rightMotorCommand=0,requestedLeft=0,requestedRight=0;
volatile uint32_t lastMotorCommandMs=0,motorEpoch=1,motorSequence=0,motorTimeouts=0;
uint32_t bootId=0,sampleSequence=0;
portMUX_TYPE motorMux=portMUX_INITIALIZER_UNLOCKED;
esp_timer_handle_t motorTimer=nullptr;
bool motorTimerReady=false;
uint32_t lastDebugMs=0, lastSnapshotMs=0, maxLoopUs=0;
bool imuFound=false, imuResetPending=false, reportsPending=false;
uint8_t reportMask=0, reportAttempt=0;
uint32_t reportAttemptMs=0, imuResets=0, imuDecodeErrors=0;
bool pawFound=false, pawConnected=false, pawLifted=false, pawMoving=false;
uint8_t pawSqual=0, pawMotionByte=0, pawObservation=0, pawRawSum=0, pawRawMax=0, pawRawMin=0;
uint16_t pawShutter=0;
int16_t pawDx=0,pawDy=0;
int64_t pawTotalX=0,pawTotalY=0;
uint32_t pawReads=0, pawInvalid=0, lastPawUs=0, lastPawGoodUs=0, lastPawIdMs=0;
uint8_t pawProduct=0,pawInverse=0;
uint32_t accelUs=0,gyroUs=0,quatUs=0;
uint64_t imuTimestampUs=0;
uint32_t accelReports=0,gyroReports=0,quatReports=0,imuSequenceGaps=0;
uint8_t accelAccuracy=0,gyroAccuracy=0,quatAccuracy=0;
bool haveAccel=false,haveGyro=false,haveQuat=false;
float accelX=0,accelY=0,accelZ=0,gyroX=0,gyroY=0,gyroZ=0;
float qw=1,qx=0,qy=0,qz=0,roll=0,pitch=0,yaw=0,yawRate=0;
float bodyTiltDeg=0;
uint32_t imuStillSinceMs=0;

// BEGIN FUSION CORE (also compiled by the host mathematical tests).
struct Configuration {
  float scaleX=1,scaleY=1,opticalAngleDeg=0;
  float offsetXmm=0,offsetYmm=0;
  float imuRollDeg=0,imuPitchDeg=0,imuYawDeg=0;
  bool swapXY=false,invertX=false,invertY=false;
  int minSqual=10;
  float maxSpeedMmS=500,maxTiltDeg=15;
};
Configuration cfg;
bool levelCalibrated=false;
float levelW=1,levelX=0,levelY=0,levelZ=0; // Mount correction, RAM only.
struct Pose {
  double x=0,y=0,distance=0;
  float vx=0,vy=0,heading=0,originYaw=0,previousHeading=0;
  bool ready=false,pending=true,previousValid=false,incomplete=false,valid=false;
  uint32_t originId=0,accepted=0,rejected=0;
};
Pose pose;
enum TrackingStatus {WAIT_ORIGIN,NEED_LEVEL,TRACKING,NO_IMU,NO_OPTICAL,LIFTED,LOW_QUALITY,
                     TILTED,TIME_GAP,OUTLIER,HEADING_JUMP,REBASE};
TrackingStatus trackingStatus=WAIT_ORIGIN;
float wrapPi(float a) { return atan2f(sinf(a),cosf(a)); }
float radiansF(float d) { return d*PI_F/180.0f; }
float degreesF(float r) { return r*180.0f/PI_F; }

void quatMultiply(float aw,float ax,float ay,float az,float bw,float bx,float by,float bz,
                  float &w,float &x,float &y,float &z) {
  w=aw*bw-ax*bx-ay*by-az*bz; x=aw*bx+ax*bw+ay*bz-az*by;
  y=aw*by-ax*bz+ay*bw+az*bx; z=aw*bz+ax*by-ay*bx+az*bw;
}
// Mount angles describe the IMU axes relative to body axes (ZYX Euler).
// Convert q_world_imu to q_world_body using inverse(q_body_imu).
void configuredMountingQuaternion(float &w,float &x,float &y,float &z) {
  float cr=cosf(radiansF(cfg.imuRollDeg)*0.5f),sr=sinf(radiansF(cfg.imuRollDeg)*0.5f);
  float cp=cosf(radiansF(cfg.imuPitchDeg)*0.5f),sp=sinf(radiansF(cfg.imuPitchDeg)*0.5f);
  float cy=cosf(radiansF(cfg.imuYawDeg)*0.5f),sy=sinf(radiansF(cfg.imuYawDeg)*0.5f);
  w=cr*cp*cy+sr*sp*sy; x=sr*cp*cy-cr*sp*sy;
  y=cr*sp*cy+sr*cp*sy; z=cr*cp*sy-sr*sp*cy;
}
void mountingQuaternion(float &w,float &x,float &y,float &z) {
  configuredMountingQuaternion(w,x,y,z);
  if(levelCalibrated)quatMultiply(levelW,levelX,levelY,levelZ,w,x,y,z,w,x,y,z);
}
void updateBodyOrientation() {
  float mw,mx,my,mz,bw,bx,by,bz;mountingQuaternion(mw,mx,my,mz);
  quatMultiply(qw,qx,qy,qz,mw,-mx,-my,-mz,bw,bx,by,bz);
  roll=atan2f(2*(bw*bx+by*bz),1-2*(bx*bx+by*by));
  pitch=asinf(constrain(2*(bw*by-bz*bx),-1.0f,1.0f));
  yaw=atan2f(2*(bw*bz+bx*by),1-2*(by*by+bz*bz));
  bodyTiltDeg=degreesF(acosf(constrain(1-2*(bx*bx+by*by),-1.0f,1.0f)));
}
void captureLevelReference() {
  // Right-side mount correction, NOT a left-side world rotation or an Euler
  // subtraction. This keeps a flat carrier level through every yaw angle,
  // including when the IMU is vertical or upside down on the carrier.
  float mw,mx,my,mz,bw,bx,by,bz;configuredMountingQuaternion(mw,mx,my,mz);
  quatMultiply(qw,qx,qy,qz,mw,-mx,-my,-mz,bw,bx,by,bz);
  float h=atan2f(2*(bw*bz+bx*by),1-2*(by*by+bz*bz));
  quatMultiply(cosf(h*0.5f),0,0,-sinf(h*0.5f),bw,bx,by,bz,levelW,levelX,levelY,levelZ);
  float n=sqrtf(levelW*levelW+levelX*levelX+levelY*levelY+levelZ*levelZ);
  levelW/=n;levelX/=n;levelY/=n;levelZ/=n;
  levelCalibrated=true;updateBodyOrientation();
}
void rotateVector(float w,float x,float y,float z,float a,float b,float c,
                  float &ox,float &oy,float &oz) {
  ox=(1-2*(y*y+z*z))*a+2*(x*y-w*z)*b+2*(x*z+w*y)*c;
  oy=2*(x*y+w*z)*a+(1-2*(x*x+z*z))*b+2*(y*z-w*x)*c;
  oz=2*(x*z-w*y)*a+2*(y*z+w*x)*b+(1-2*(x*x+y*y))*c;
}
void invalidatePose(TrackingStatus reason) {
  trackingStatus=reason; pose.valid=false; pose.previousValid=false;
  pose.vx=pose.vy=0;
  if(pose.ready) pose.incomplete=true;
  ++pose.rejected;
}
void requestOrigin() {
  pose.pending=true; pose.previousValid=false; pose.valid=false;
  pose.vx=pose.vy=0; trackingStatus=WAIT_ORIGIN;
}
// One displacement accumulated over dt; heading supplied at this interval end.
// Optical counts are rotated at the interval's midpoint heading. Exact rigid
// offset endpoint correction removes the optical sensor's arc about body origin.
bool fuseOptical(int16_t dx,int16_t dy,float headingNow,float dt) {
  if(!isfinite(headingNow)||!isfinite(dt)||dt<=0||dt>0.1f) {
    invalidatePose(TIME_GAP); return false;
  }
  if(pose.pending) {
    uint32_t id=pose.originId+1;
    pose=Pose(); pose.pending=false; pose.ready=true; pose.previousValid=true;
    pose.originId=id; pose.originYaw=headingNow; pose.valid=true;
    trackingStatus=TRACKING;
    return true; // Discard pre-reset counts: origin begins at this sample.
  }
  if(!pose.ready) return false;
  float h=wrapPi(headingNow-pose.originYaw);
  pose.heading=h;
  if(!pose.previousValid) {
    pose.previousHeading=h; pose.previousValid=true; pose.valid=false;
    trackingStatus=REBASE; return false; // Never bridge a rejected interval.
  }
  float dh=wrapPi(h-pose.previousHeading);
  if(fabsf(dh)>radiansF(15)+8.0f*dt) {
    invalidatePose(HEADING_JUMP); return false;
  }
  float u=dx,v=dy;
  if(cfg.swapXY) {float t=u;u=v;v=t;}
  if(cfg.invertX)u=-u; if(cfg.invertY)v=-v;
  u*=25.4f/PAW_CPI*cfg.scaleX; v*=25.4f/PAW_CPI*cfg.scaleY;
  float a=radiansF(cfg.opticalAngleDeg);
  float bx=cosf(a)*u-sinf(a)*v,by=sinf(a)*u+cosf(a)*v;
  float mid=pose.previousHeading+dh*0.5f;
  double wx=cosf(mid)*bx-sinf(mid)*by;
  double wy=sinf(mid)*bx+cosf(mid)*by;
  // r is the sensor location relative to the selected body reference point.
  wx-=(cosf(h)-cosf(pose.previousHeading))*cfg.offsetXmm
       -(sinf(h)-sinf(pose.previousHeading))*cfg.offsetYmm;
  wy-=(sinf(h)-sinf(pose.previousHeading))*cfg.offsetXmm
       +(cosf(h)-cosf(pose.previousHeading))*cfg.offsetYmm;
  double ds=hypot(wx,wy);
  // Allow 3 raw counts of quantization in addition to configured max speed.
  float allowance=3*25.4f/PAW_CPI*fmaxf(cfg.scaleX,cfg.scaleY);
  if(!isfinite(ds)||ds>cfg.maxSpeedMmS*dt+allowance) {
    invalidatePose(OUTLIER);return false;
  }
  pose.x+=wx; pose.y+=wy;pose.distance+=ds;
  // 80 ms low-pass ONLY for displayed velocity; position preserves increments.
  float alpha=dt/(0.08f+dt);
  pose.vx+=alpha*((float)(wx/dt)-pose.vx);
  pose.vy+=alpha*((float)(wy/dt)-pose.vy);
  pose.previousHeading=h;pose.valid=true;trackingStatus=TRACKING;++pose.accepted;
  return true;
}
// END FUSION CORE

int scaleMotorCommand(int command,float factor) {
  // Scale once before inversion; round both directions symmetrically.
  command=constrain(command,-255,255);
  int scaled=(int)lroundf(command*factor);
  return constrain(scaled,-255,255);
}
int setMotor(int in1,int in2,int command,bool inverted) {
  command=constrain(command,-255,255);if(inverted)command=-command;
  if(command>=0){analogWrite(in2,0);analogWrite(in1,command);}
  else {analogWrite(in1,0);analogWrite(in2,-command);}
  return command;
}
void setMotors(int l,int r) {
  l=scaleMotorCommand(l,motorScaleLeft);r=scaleMotorCommand(r,motorScaleRight);
  if(invertLeft)l=-l;if(invertRight)r=-r;
  portENTER_CRITICAL(&motorMux);requestedLeft=l;requestedRight=r;portEXIT_CRITICAL(&motorMux);
}
void stopMotors(){setMotors(0,0);}
void invalidateMotorLease(){
  motion.cancel("stopped_or_settings_changed");
  portENTER_CRITICAL(&motorMux);
  requestedLeft=0;requestedRight=0;motorEpoch=motorEpoch+1;motorSequence=0;
  portEXIT_CRITICAL(&motorMux);
}
void checkMotorTimeout() {
  portENTER_CRITICAL(&motorMux);
  if((requestedLeft||requestedRight)&&(uint32_t)(millis()-lastMotorCommandMs)>=MOTOR_TIMEOUT_MS){
    requestedLeft=0;requestedRight=0;motorEpoch=motorEpoch+1;motorSequence=0;motorTimeouts=motorTimeouts+1;
  }
  portEXIT_CRITICAL(&motorMux);
}
// Runs in ESP32's existing esp_timer service, NOT an ISR or a new user task.
// Sole PWM writer after setup: a slow HTTP client cannot extend motor commands.
// No SPI, Serial, network, memory allocation or critical-section I/O here.
void motorTimerCallback(void*) {
  checkMotorTimeout();
  portENTER_CRITICAL(&motorMux);int l=requestedLeft,r=requestedRight;portEXIT_CRITICAL(&motorMux);
  if(l!=leftMotorCommand)leftMotorCommand=setMotor(MOTOR_L_IN1,MOTOR_L_IN2,l,false);
  if(r!=rightMotorCommand)rightMotorCommand=setMotor(MOTOR_R_IN1,MOTOR_R_IN2,r,false);
}
void setupMotorTimer(){
  // Allocate all four PWM channels once, before the timer can use them.
  bool attached=true;
  const int pins[]={MOTOR_L_IN1,MOTOR_L_IN2,MOTOR_R_IN1,MOTOR_R_IN2};
  for(int pin:pins)if(!ledcAttach(pin,1000,8))attached=false;
  if(!attached){Serial.println("PWM allocation FAILED; motor requests disabled");return;}
  setMotor(MOTOR_L_IN1,MOTOR_L_IN2,0,false);setMotor(MOTOR_R_IN1,MOTOR_R_IN2,0,false);
  esp_timer_create_args_t a={};a.callback=motorTimerCallback;a.name="motor_cutoff";
  a.dispatch_method=ESP_TIMER_TASK;
  if(esp_timer_create(&a,&motorTimer)==ESP_OK)
    motorTimerReady=esp_timer_start_periodic(motorTimer,10000)==ESP_OK;
  if(!motorTimerReady)Serial.println("Motor timer FAILED; motor requests disabled");
}


// v2.1: restore the standard driver used by the working basic firmware.
// Do not replace its SPI HAL, initialization, or report callback.
Adafruit_BNO08x bno08x(IMU_RESET);
sh2_SensorValue_t imuSensorValue; // Persistent storage for the library callback.
bool imuFrameChanged=false;
uint32_t imuFirstQuatMs=0;
uint8_t imuSeq[3]={0,0,0};
bool imuSeqSeen[3]={false,false,false};
// Transport/queue counters from v2.0 are no longer available with the stock
// driver; telemetry uses null instead of pretending they were measured.

void scheduleReports(){reportsPending=true;reportAttempt=0;reportMask=0;reportAttemptMs=millis()-100;}
void enableIMUReports(){
  // Three configuration attempts; SPI timing is owned by Adafruit.
  // The library may block during a faulty transfer; PWM cutoff is independent.
  if(!reportsPending||(uint32_t)(millis()-reportAttemptMs)<100)return;
  reportAttemptMs=millis();++reportAttempt;
  if(!(reportMask&1)&&bno08x.enableReport(SH2_ACCELEROMETER,IMU_INTERVAL_US))reportMask|=1;
  if(!(reportMask&2)&&bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED,IMU_INTERVAL_US))reportMask|=2;
  if(!(reportMask&4)&&bno08x.enableReport(SH2_ROTATION_VECTOR,IMU_INTERVAL_US))reportMask|=4;
  if(reportMask==7||reportAttempt>=3){
    reportsPending=false;Serial.printf("IMU reports mask: %u (7 = all enabled)\n",reportMask);
  }
}
void setupIMU(){
  Serial.println("Starting BNO086 (standard Adafruit SPI, CS27 INT14 RST26)");
  digitalWrite(PAW_CS,HIGH);
  imuFound=bno08x.begin_SPI(IMU_CS,IMU_INT,&SPI);
  Serial.println(imuFound?"BNO086 FOUND":"BNO086 NOT FOUND");
  imuResetPending=false;
  if(imuFound){
    bno08x.wasReset(); // Clear the initial reset, as in the original firmware.
    scheduleReports();enableIMUReports();
  }

}
bool finite3(float x,float y,float z){return isfinite(x)&&isfinite(y)&&isfinite(z);}
void readIMU(){
  if(!imuFound)return;
  if(bno08x.wasReset())imuResetPending=true;
  if(imuResetPending){
    imuResetPending=false;++imuResets;invalidateMotorLease();
    if(pose.ready){imuFrameChanged=true;invalidatePose(NO_IMU);}
    haveQuat=haveAccel=haveGyro=false;imuFirstQuatMs=0;imuStillSinceMs=0;
    for(int i=0;i<3;++i)imuSeqSeen[i]=false;
    scheduleReports();
    Serial.println("IMU reset: reports requested; reset origin before continuing tracking");
  }
  enableIMUReports();
  // INT-low gate and ONE standard API call, exactly as in the working sketch.
  if(digitalRead(IMU_INT)!=LOW)return;
  digitalWrite(PAW_CS,HIGH);
  if(!bno08x.getSensorEvent(&imuSensorValue))return;
  if(bno08x.wasReset()){imuResetPending=true;return;}
  sh2_SensorValue_t &v=imuSensorValue;
  uint32_t received=micros();
  imuTimestampUs=v.timestamp;
  int index=v.sensorId==SH2_ACCELEROMETER?0:v.sensorId==SH2_GYROSCOPE_CALIBRATED?1:v.sensorId==SH2_ROTATION_VECTOR?2:-1;
  if(index>=0){
    uint8_t difference=(uint8_t)(v.sequence-imuSeq[index]);
    if(imuSeqSeen[index]&&difference>1)imuSequenceGaps+=difference-1;
    imuSeq[index]=v.sequence;imuSeqSeen[index]=true;
  }
  if(v.sensorId==SH2_ACCELEROMETER){
    if(!finite3(v.un.accelerometer.x,v.un.accelerometer.y,v.un.accelerometer.z)){++imuDecodeErrors;return;}
    accelX=v.un.accelerometer.x;accelY=v.un.accelerometer.y;accelZ=v.un.accelerometer.z;
    accelUs=received;haveAccel=true;accelAccuracy=v.status&3;++accelReports;
  }else if(v.sensorId==SH2_GYROSCOPE_CALIBRATED){
    if(!finite3(v.un.gyroscope.x,v.un.gyroscope.y,v.un.gyroscope.z)){++imuDecodeErrors;return;}
    gyroX=v.un.gyroscope.x;gyroY=v.un.gyroscope.y;gyroZ=v.un.gyroscope.z;
    gyroUs=received;haveGyro=true;gyroAccuracy=v.status&3;++gyroReports;
  }else if(v.sensorId==SH2_ROTATION_VECTOR){
    float w=v.un.rotationVector.real,x=v.un.rotationVector.i;
    float y=v.un.rotationVector.j,z=v.un.rotationVector.k;
    float n=sqrtf(w*w+x*x+y*y+z*z);
    if(!isfinite(n)||n<0.5f||n>1.5f){++imuDecodeErrors;return;}
    qw=w/n;qx=x/n;qy=y/n;qz=z/n;
    updateBodyOrientation();
    if(!haveQuat){
      imuFirstQuatMs=millis();
      Serial.println("BNO086 ORIENTATION STREAMING (rotation vector, 20 Hz)");
    }
    quatUs=received;haveQuat=true;quatAccuracy=v.status&3;++quatReports;
  }
}
bool headingFresh(uint32_t now){
  return levelCalibrated&&haveQuat&&(uint32_t)(now-quatUs)<=IMU_STALE_US&&!imuFrameChanged
      &&(uint32_t)(millis()-imuFirstQuatMs)>=2000;
}
float currentHeading(uint32_t now){
  float mw,mx,my,mz,bx,by,bz;mountingQuaternion(mw,mx,my,mz);
  rotateVector(mw,mx,my,mz,gyroX,gyroY,gyroZ,bx,by,bz);
  yawRate=(sinf(roll)*by+cosf(roll)*bz)/fmaxf(cosf(pitch),0.1f);
  float dt=fminf((uint32_t)(now-quatUs)*1e-6f,0.02f);
  if(!haveGyro||(uint32_t)(now-gyroUs)>GYRO_STALE_US)dt=0;
  // Gyro interpolates only the last <=20 ms beyond the fused quaternion.
  return wrapPi(yaw+yawRate*dt);
}
void updateIMUStillness(){
  uint32_t now=micros();
  float a2=accelX*accelX+accelY*accelY+accelZ*accelZ;
  float g2=gyroX*gyroX+gyroY*gyroY+gyroZ*gyroZ;
  bool still=imuFound&&haveQuat&&haveAccel&&haveGyro
    &&(uint32_t)(now-quatUs)<=IMU_STALE_US&&(uint32_t)(now-accelUs)<=IMU_STALE_US
    &&(uint32_t)(now-gyroUs)<=GYRO_STALE_US&&(uint32_t)(millis()-imuFirstQuatMs)>=2000
    &&isfinite(a2)&&isfinite(g2)&&a2>=8.5f*8.5f&&a2<=11.0f*11.0f&&g2<=0.1f*0.1f
    &&!requestedLeft&&!requestedRight&&!leftMotorCommand&&!rightMotorCommand;
  if(!still)imuStillSinceMs=0;
  else if(!imuStillSinceMs)imuStillSinceMs=millis();
}
bool levelCaptureReady(){return imuStillSinceMs&&(uint32_t)(millis()-imuStillSinceMs)>=1000;}


const uint8_t PAW_POWER_UP[][2] = {
  {0x7F, 0x07}, {0x40, 0x41}, {0x7F, 0x00}, {0x40, 0x80},
  {0x7F, 0x0E}, {0x55, 0x0D}, {0x56, 0x1B}, {0x57, 0xE8},
  {0x58, 0xD5}, {0x7F, 0x14}, {0x42, 0xBC}, {0x43, 0x74},
  {0x4B, 0x20}, {0x4D, 0x00}, {0x53, 0x0E}, {0x7F, 0x05},
  {0x44, 0x04}, {0x4D, 0x06}, {0x51, 0x40}, {0x53, 0x40},
  {0x55, 0xCA}, {0x5A, 0xE8}, {0x5B, 0xEA}, {0x61, 0x31},
  {0x62, 0x64}, {0x6D, 0xB8}, {0x6E, 0x0F}, {0x70, 0x02},
  {0x4A, 0x2A}, {0x60, 0x26}, {0x7F, 0x06}, {0x6D, 0x70},
  {0x6E, 0x60}, {0x6F, 0x04}, {0x53, 0x02}, {0x55, 0x11},
  {0x7A, 0x01}, {0x7D, 0x51}, {0x7F, 0x07}, {0x41, 0x10},
  {0x42, 0x32}, {0x43, 0x00}, {0x7F, 0x08}, {0x71, 0x4F},
  {0x7F, 0x09}, {0x62, 0x1F}, {0x63, 0x1F}, {0x65, 0x03},
  {0x66, 0x03}, {0x67, 0x1F}, {0x68, 0x1F}, {0x69, 0x03},
  {0x6A, 0x03}, {0x6C, 0x1F}, {0x6D, 0x1F}, {0x51, 0x04},
  {0x53, 0x20}, {0x54, 0x20}, {0x71, 0x0C}, {0x72, 0x07},
  {0x73, 0x07}, {0x7F, 0x0A}, {0x4A, 0x14}, {0x4C, 0x14},
  {0x55, 0x19}, {0x7F, 0x14}, {0x4B, 0x30}, {0x4C, 0x03},
  {0x61, 0x0B}, {0x62, 0x0A}, {0x63, 0x02}, {0x7F, 0x15},
  {0x4C, 0x02}, {0x56, 0x02}, {0x41, 0x91}, {0x4D, 0x0A},
  {0x7F, 0x0C}, {0x4A, 0x10}, {0x4B, 0x0C}, {0x4C, 0x40},
  {0x41, 0x25}, {0x55, 0x18}, {0x56, 0x14}, {0x49, 0x0A},
  {0x42, 0x00}, {0x43, 0x2D}, {0x44, 0x0C}, {0x54, 0x1A},
  {0x5A, 0x0D}, {0x5F, 0x1E}, {0x5B, 0x05}, {0x5E, 0x0F},
  {0x7F, 0x0D}, {0x48, 0xDD}, {0x4F, 0x03}, {0x52, 0x49},
  {0x51, 0x00}, {0x54, 0x5B}, {0x53, 0x00}, {0x56, 0x64},
  {0x55, 0x00}, {0x58, 0xA5}, {0x57, 0x02}, {0x5A, 0x29},
  {0x5B, 0x47}, {0x5C, 0x81}, {0x5D, 0x40}, {0x71, 0xDC},
  {0x70, 0x07}, {0x73, 0x00}, {0x72, 0x08}, {0x75, 0xDC},
  {0x74, 0x07}, {0x77, 0x00}, {0x76, 0x08}, {0x7F, 0x10},
  {0x4C, 0xD0}, {0x7F, 0x00}, {0x4F, 0x63}, {0x4E, 0x00},
  {0x52, 0x63}, {0x51, 0x00}, {0x54, 0x54}, {0x5A, 0x10},
  {0x77, 0x4F}, {0x47, 0x01}, {0x5B, 0x40}, {0x64, 0x60},
  {0x65, 0x06}, {0x66, 0x13}, {0x67, 0x0F}, {0x78, 0x01},
  {0x79, 0x9C}, {0x40, 0x00}, {0x55, 0x02}, {0x23, 0x70},
  {0x22, 0x01},
};

uint8_t pawReadRegister(uint8_t reg) {
  digitalWrite(IMU_CS, HIGH);
  SPI.beginTransaction(PAW_SPI);
  digitalWrite(PAW_CS, LOW);
  delayMicroseconds(1);
  SPI.transfer(reg & 0x7F);
  delayMicroseconds(2); // PAW3395 address-to-data delay, minimum 2 us.
  uint8_t value = SPI.transfer(0x00);
  delayMicroseconds(1);
  digitalWrite(PAW_CS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(2);
  return value;
}

void pawWriteRegister(uint8_t reg, uint8_t value) {
  digitalWrite(IMU_CS, HIGH);
  SPI.beginTransaction(PAW_SPI);
  digitalWrite(PAW_CS, LOW);
  delayMicroseconds(1);
  SPI.transfer(reg | 0x80);
  SPI.transfer(value);
  delayMicroseconds(1);
  digitalWrite(PAW_CS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(5);
}

void setupPAW() {
  Serial.println("Starting PAW3395");
  pawFound = false;
  delay(50); // Power-up wait, only in setup().
  digitalWrite(PAW_CS, HIGH);
  delayMicroseconds(2);
  digitalWrite(PAW_CS, LOW);
  delayMicroseconds(2);
  digitalWrite(PAW_CS, HIGH);
  pawWriteRegister(0x3A, 0x5A); // Software reset; no reset GPIO needed.
  delay(5);

  uint8_t product = pawReadRegister(0x00);
  uint8_t inverse = pawReadRegister(0x5F);
  Serial.printf("PAW3395 ID: 0x%02X | inverse: 0x%02X (expected 0x51 / 0xAE)\n",
                (unsigned)product, (unsigned)inverse);
  if (product != 0x51 || inverse != 0xAE) {
    Serial.println("PAW3395 NOT FOUND");
    return; // IMU, Wi-Fi and motor control can still run.
  }

  for (size_t i = 0; i < sizeof(PAW_POWER_UP) / sizeof(PAW_POWER_UP[0]); ++i)
    pawWriteRegister(PAW_POWER_UP[i][0], PAW_POWER_UP[i][1]);

  // Datasheet: at most 60 startup reads, spaced 1 ms apart.
  bool ready = false;
  delayMicroseconds(1000);
  for (uint8_t attempt = 0; attempt < 60; ++attempt) {
    uint32_t started = micros();
    if (pawReadRegister(0x6C) == 0x80) {
      ready = true;
      break;
    }
    // Includes the SPI read time; this bounded wait is only during setup.
    while ((uint32_t)(micros() - started) < 1000) { }
  }
  if (!ready) {
    // This fallback is part of the manufacturer's initialization sequence.
    pawWriteRegister(0x7F, 0x14);
    pawWriteRegister(0x6C, 0x00);
    pawWriteRegister(0x7F, 0x00);
    Serial.println("PAW3395: using datasheet startup fallback");
  }
  pawWriteRegister(0x22, 0x00);
  pawWriteRegister(0x55, 0x00);
  pawWriteRegister(0x7F, 0x07);
  pawWriteRegister(0x40, 0x40);
  pawWriteRegister(0x7F, 0x00);

  // Required initial clearing of motion registers, regardless of MOTION.
  for (uint8_t reg = 0x02; reg <= 0x06; ++reg) pawReadRegister(reg);

  // Keep the sensor awake for this bench test, so SQUAL stays meaningful.
  pawWriteRegister(0x40, 0x80);
  pawWriteRegister(0x5C, 0x00); // Active-low MOTION; X resolution applies to both axes.
  const uint16_t resolution = PAW_CPI / 50 - 1;
  pawWriteRegister(0x48, resolution & 0xFF);
  pawWriteRegister(0x49, resolution >> 8);
  pawWriteRegister(0x4A, resolution & 0xFF);
  pawWriteRegister(0x4B, resolution >> 8);
  pawWriteRegister(0x47, 0x01); // Apply resolution.

  uint16_t readback = pawReadRegister(0x48);
  readback |= (uint16_t)pawReadRegister(0x49) << 8;
  if (readback != resolution || pawReadRegister(0x00) != 0x51 ||
      pawReadRegister(0x5F) != 0xAE) {
    Serial.println("PAW3395 INIT FAILED: register readback mismatch");
    return;
  }

  pawFound = true;
  lastPawUs = micros();
  Serial.printf("PAW3395 FOUND | CPI: %u\n", (unsigned)PAW_CPI);
}



void readPAW(){
  if(!pawFound)return;
  uint32_t now=micros(),elapsed=now-lastPawUs;
  if(elapsed<PAW_PERIOD_US)return;
  lastPawUs=now;
  // Poll at 100 Hz even when MOTION is high: quality and lift stay current.
  // MOTION is still displayed for wiring diagnostics; loss of that wire alone
  // cannot stop position updates.
  uint8_t b[12];digitalWrite(IMU_CS,HIGH);SPI.beginTransaction(PAW_SPI);
  digitalWrite(PAW_CS,LOW);delayMicroseconds(1);SPI.transfer(0x16);delayMicroseconds(2);
  for(unsigned i=0;i<12;++i)b[i]=SPI.transfer(0);
  delayMicroseconds(1);digitalWrite(PAW_CS,HIGH);SPI.endTransaction();delayMicroseconds(2);
  ++pawReads;
  bool allFF=true;for(uint8_t v:b)if(v!=0xFF)allFF=false;
  pawMotionByte=b[0];pawObservation=b[1];pawSqual=b[6];
  pawRawSum=b[7];pawRawMax=b[8];pawRawMin=b[9];pawShutter=((b[10]&0x0F)<<8)|b[11];
  pawLifted=b[0]&8;pawMoving=b[0]&0x80;
  pawDx=pawMoving?(int16_t)((uint16_t)b[2]|((uint16_t)b[3]<<8)):0;
  pawDy=pawMoving?(int16_t)((uint16_t)b[4]|((uint16_t)b[5]<<8)):0;
  if((uint32_t)(millis()-lastPawIdMs)>=1000){
    lastPawIdMs=millis();pawProduct=pawReadRegister(0);pawInverse=pawReadRegister(0x5F);
    pawConnected=pawProduct==0x51&&pawInverse==0xAE;
  }
  if(allFF||!pawConnected){++pawInvalid;invalidatePose(NO_OPTICAL);return;}
  lastPawGoodUs=now;
  pawTotalX+=pawDx;pawTotalY+=pawDy; // Includes rejected surface counts for diagnosis.
  if(!levelCalibrated){invalidatePose(NEED_LEVEL);return;}
  if(!headingFresh(now)){invalidatePose(NO_IMU);return;}
  if(pawLifted){invalidatePose(LIFTED);return;}
  if(pawSqual<cfg.minSqual){invalidatePose(LOW_QUALITY);return;}
  if(bodyTiltDeg>cfg.maxTiltDeg){invalidatePose(TILTED);return;}
  if(elapsed>MAX_OPTICAL_GAP_US){invalidatePose(TIME_GAP);return;}
  fuseOptical(pawDx,pawDy,currentHeading(now),elapsed*1e-6f);
}
void checkTrackingHealth(){
  uint32_t now=micros();
  if(pose.ready&&haveQuat&&levelCalibrated)pose.heading=wrapPi(currentHeading(now)-pose.originYaw);
  if(pose.valid&&!headingFresh(now))invalidatePose(NO_IMU);
  else if(pose.valid&&(!pawFound||!pawConnected||(uint32_t)(now-lastPawGoodUs)>MAX_OPTICAL_GAP_US))invalidatePose(NO_OPTICAL);
}

bool automationTrackingHealthy(){
  uint32_t now=micros();
  return pose.ready&&pose.valid&&!pose.incomplete&&!pose.pending&&headingFresh(now)
    &&pawFound&&pawConnected&&!pawLifted&&pawSqual>=cfg.minSqual
    &&(uint32_t)(now-lastPawGoodUs)<=MAX_OPTICAL_GAP_US
    &&bodyTiltDeg<=cfg.maxTiltDeg;
}
carrier::Observation motionObservation(){
  carrier::Observation p;
  p.x=pose.x;p.y=pose.y;p.heading=pose.heading;
  p.healthy=automationTrackingHealthy();p.origin=pose.originId;return p;
}
void updateAutomation(){
  if(!motion.running())return;
  uint32_t now=millis();
  carrier::Output output=motion.step(motionObservation(),now);
  if(!motion.running()){invalidateMotorLease();return;}
  int l=scaleMotorCommand(output.left,motorScaleLeft);
  int r=scaleMotorCommand(output.right,motorScaleRight);
  if(invertLeft)l=-l;if(invertRight)r=-r;
  portENTER_CRITICAL(&motorMux);
  // Never resume an action after the independent motor watchdog expired.
  bool accepted=motorTimerReady&&motorEpoch==actionMotorEpoch
      &&(uint32_t)(now-lastMotorCommandMs)<MOTOR_TIMEOUT_MS;
  if(accepted){requestedLeft=l;requestedRight=r;lastMotorCommandMs=now;}
  portEXIT_CRITICAL(&motorMux);
  if(!accepted){motion.finish(carrier::State::Fault,"motor_watchdog");invalidateMotorLease();}
}
const char *statusText(){
  if(imuFrameChanged)return "IMU reset: RESET ORIGIN required";
  if(!imuFound)return "BNO086 not found";
  if(!pawFound)return "PAW3395 not found";
  if(haveQuat&&(uint32_t)(millis()-imuFirstQuatMs)<2000)return "IMU settling: keep still";
  if(!levelCalibrated)return "Place flat and still, then SET LEVEL";
  switch(trackingStatus){
    case WAIT_ORIGIN:return "Waiting for valid origin";
    case NEED_LEVEL:return "Place flat and still, then SET LEVEL";
    case TRACKING:return "Tracking";
    case NO_IMU:return "IMU orientation missing/stale";
    case NO_OPTICAL:return "Optical sensor missing/stale";
    case LIFTED:return "Lifted: position paused";
    case LOW_QUALITY:return "Low surface quality: paused";
    case TILTED:return "Too much tilt: paused";
    case TIME_GAP:return "Sampling gap: paused";
    case OUTLIER:return "Implausible displacement rejected";
    case HEADING_JUMP:return "Heading discontinuity rejected";
    case REBASE:return "Resuming after tracking gap";
  }
  return "Unknown";
}

// Telemetry is a fixed-size snapshot updated at 10 Hz, never a sensor read
// inside an HTTP handler. CSV logging is in the browser, not ESP32 flash/RAM.
char telemetry[6144];size_t telemetryUsed=0;bool telemetryOverflow=false;
void jsonAppend(const char *format,...){
  if(telemetryOverflow)return;
  va_list args;va_start(args,format);
  int n=vsnprintf(telemetry+telemetryUsed,sizeof(telemetry)-telemetryUsed,format,args);va_end(args);
  if(n<0||(size_t)n>=sizeof(telemetry)-telemetryUsed){telemetryOverflow=true;return;}
  telemetryUsed+=(size_t)n;
}
const char *jb(bool b){return b?"true":"false";}
long ageMs(bool present,uint32_t t,uint32_t now){return present?(long)((uint32_t)(now-t)/1000):-1;}
void makeSnapshot(){
  uint32_t now=micros();telemetryUsed=0;telemetryOverflow=false;++sampleSequence;
  jsonAppend("{\"schema\":3,\"firmware\":\"2.4\",\"boot_id\":%lu,\"sample\":%lu,\"uptime_ms\":%llu,",
    (unsigned long)bootId,(unsigned long)sampleSequence,(unsigned long long)(esp_timer_get_time()/1000));
  jsonAppend("\"pose\":{\"status\":\"%s\",\"valid\":%s,\"incomplete\":%s,\"ready\":%s,\"origin_pending\":%s,\"origin_id\":%lu,"
    "\"x_mm\":%.5f,\"y_mm\":%.5f,\"heading_deg\":%.4f,\"vx_mm_s\":%.4f,\"vy_mm_s\":%.4f,\"distance_mm\":%.5f,\"accepted\":%lu,\"rejected\":%lu},",
    statusText(),jb(pose.valid),jb(pose.incomplete),jb(pose.ready),jb(pose.pending),(unsigned long)pose.originId,
    pose.x,pose.y,degreesF(pose.heading),pose.vx,pose.vy,pose.distance,(unsigned long)pose.accepted,(unsigned long)pose.rejected);
  jsonAppend("\"imu\":{\"found\":%s,\"heading_fresh\":%s,\"frame_changed\":%s,\"mode\":\"rotation_vector\","
    "\"roll_deg\":%.4f,\"pitch_deg\":%.4f,\"yaw_deg\":%.4f,\"yaw_rate_rad_s\":%.5f,"
    "\"ax_m_s2\":%.5f,\"ay_m_s2\":%.5f,\"az_m_s2\":%.5f,\"gx_rad_s\":%.6f,\"gy_rad_s\":%.6f,\"gz_rad_s\":%.6f,"
    "\"qw\":%.7f,\"qx\":%.7f,\"qy\":%.7f,\"qz\":%.7f,"
    "\"accel_accuracy\":%u,\"gyro_accuracy\":%u,\"quat_accuracy\":%u,\"accel_age_ms\":%ld,\"gyro_age_ms\":%ld,\"quat_age_ms\":%ld,"
    "\"sensor_timestamp_us\":%llu,\"accel_reports\":%lu,\"gyro_reports\":%lu,\"quat_reports\":%lu,\"sequence_gaps\":%lu,"
    "\"queue_drops\":null,\"decode_errors\":%lu,\"transport_errors\":null,\"resets\":%lu,\"reports_mask\":%u,"
    "\"level_calibrated\":%s,\"level_capture_ready\":%s,\"tilt_deg\":%.4f,\"tilt_limit_deg\":%.2f,"
    "\"level_qw\":%.7f,\"level_qx\":%.7f,\"level_qy\":%.7f,\"level_qz\":%.7f,"
    "\"forward_offset_mm\":%.1f,\"height_above_ground_mm\":%.1f},",
    jb(imuFound),jb(headingFresh(now)),jb(imuFrameChanged),degreesF(roll),degreesF(pitch),degreesF(yaw),yawRate,
    accelX,accelY,accelZ,gyroX,gyroY,gyroZ,qw,qx,qy,qz,accelAccuracy,gyroAccuracy,quatAccuracy,
    ageMs(haveAccel,accelUs,now),ageMs(haveGyro,gyroUs,now),ageMs(haveQuat,quatUs,now),(unsigned long long)imuTimestampUs,
    (unsigned long)accelReports,(unsigned long)gyroReports,(unsigned long)quatReports,(unsigned long)imuSequenceGaps,
    (unsigned long)imuDecodeErrors,(unsigned long)imuResets,reportMask,
    jb(levelCalibrated),jb(levelCaptureReady()),bodyTiltDeg,cfg.maxTiltDeg,levelW,levelX,levelY,levelZ,
    IMU_FORWARD_OFFSET_MM,IMU_HEIGHT_ABOVE_GROUND_MM);
  jsonAppend("\"paw\":{\"found\":%s,\"connected\":%s,\"product_id\":%u,\"inverse_id\":%u,\"cpi\":%u,"
    "\"dx_counts\":%d,\"dy_counts\":%d,\"total_x_counts\":%lld,\"total_y_counts\":%lld,"
    "\"squal\":%u,\"lifted\":%s,\"moving\":%s,\"motion_pin_low\":%s,\"motion_byte\":%u,\"observation\":%u,"
    "\"raw_sum\":%u,\"raw_max\":%u,\"raw_min\":%u,\"shutter\":%u,\"age_ms\":%ld,\"reads\":%lu,\"invalid\":%lu},",
    jb(pawFound),jb(pawConnected),pawProduct,pawInverse,PAW_CPI,pawDx,pawDy,(long long)pawTotalX,(long long)pawTotalY,
    pawSqual,jb(pawLifted),jb(pawMoving),jb(digitalRead(PAW_MOTION)==LOW),pawMotionByte,pawObservation,
    pawRawSum,pawRawMax,pawRawMin,pawShutter,ageMs(pawReads&&pawConnected,lastPawGoodUs,now),(unsigned long)pawReads,(unsigned long)pawInvalid);
  jsonAppend("\"motor\":{\"left_pwm\":%d,\"right_pwm\":%d,\"speed_percent\":%d,\"invert_left\":%s,\"invert_right\":%s,"
    "\"scale_left\":%.4f,\"scale_right\":%.4f,"
    "\"epoch\":%lu,\"command_age_ms\":%lu,\"timeouts\":%lu,\"timer_ready\":%s},",
    leftMotorCommand,rightMotorCommand,speedPercent,jb(invertLeft),jb(invertRight),motorScaleLeft,motorScaleRight,(unsigned long)motorEpoch,
    (unsigned long)(millis()-lastMotorCommandMs),(unsigned long)motorTimeouts,jb(motorTimerReady));
  jsonAppend("\"system\":{\"free_heap_bytes\":%lu,\"min_free_heap_bytes\":%lu,\"max_loop_us\":%lu,\"wifi_clients\":%u},",
    (unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getMinFreeHeap(),(unsigned long)maxLoopUs,WiFi.softAPgetStationNum());
  jsonAppend("\"action\":{\"id\":%lu,\"state\":\"%s\",\"kind\":\"%s\",\"reason\":\"%s\",\"target\":%.3f,\"progress\":%.3f},",
    (unsigned long)motion.id,motion.stateName(),motion.kindName(),motion.reason,motion.target,motion.progress);
  jsonAppend("\"config\":{\"scaleX\":%.6f,\"scaleY\":%.6f,\"opticalAngleDeg\":%.3f,\"offsetXmm\":%.3f,\"offsetYmm\":%.3f,"
    "\"imuRollDeg\":%.3f,\"imuPitchDeg\":%.3f,\"imuYawDeg\":%.3f,\"swapXY\":%s,\"invertX\":%s,\"invertY\":%s,"
    "\"minSqual\":%d,\"maxSpeedMmS\":%.3f,\"maxTiltDeg\":%.3f}}",
    cfg.scaleX,cfg.scaleY,cfg.opticalAngleDeg,cfg.offsetXmm,cfg.offsetYmm,cfg.imuRollDeg,cfg.imuPitchDeg,cfg.imuYawDeg,
    jb(cfg.swapXY),jb(cfg.invertX),jb(cfg.invertY),cfg.minSqual,cfg.maxSpeedMmS,cfg.maxTiltDeg);
  if(telemetryOverflow)strcpy(telemetry,"{\"error\":\"telemetry buffer full\"}");
}


const char PAGE[] PROGMEM=R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Photonics Carrier — Position</title><style>
*{box-sizing:border-box}body{font:16px system-ui,sans-serif;background:#f3f5f7;color:#17202a;margin:0;padding:16px}
main{max-width:1050px;margin:auto}h1{font-size:25px}h2{font-size:19px;margin-top:0}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(290px,1fr));gap:14px}
section,details{background:white;border:1px solid #dce1e5;border-radius:10px;padding:16px;margin:12px 0}
button,input,select{font:inherit}button{padding:12px;border:1px solid #b4bcc5;border-radius:7px;background:#edf1f5;cursor:pointer}
button:disabled{opacity:.5;cursor:default}.pad button{touch-action:none;user-select:none;-webkit-user-select:none;min-height:58px}
.pad{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}.row{display:flex;gap:8px;flex-wrap:wrap;margin:12px 0}
.stop{background:#bd202b;color:white;font-weight:bold}#status{padding:12px;background:#fff4d5;border-radius:6px}
.metrics{display:grid;grid-template-columns:1fr 1fr;gap:12px}.metrics b{display:block;font-size:24px;font-variant-numeric:tabular-nums}
small,.muted{color:#586573}canvas{width:100%;height:280px;background:#fafbfc;border:1px solid #e0e5ea}
table{width:100%;font-size:13px;border-collapse:collapse}td{padding:5px;border-bottom:1px solid #edf0f2;overflow-wrap:anywhere}
td:last-child{text-align:right;font-variant-numeric:tabular-nums}label{display:block;margin:10px 0}
input[type=number]{width:120px;float:right}input[type=range]{width:100%}summary{cursor:pointer;font-weight:600}
#message{min-height:24px;color:#ad2933}a{color:#205aa5}.warn{color:#a33300}
</style></head><body><main>
<h1>Photonics Carrier</h1><p class="muted">BNO086 + PAW3395 · Relative 2D odometry · Units: mm and degrees</p>
<div id="status">Connecting…</div><div id="message" role="status"></div>
<div class="grid"><section><h2>Position</h2><div class="metrics">
<div>X <b id="x">—</b></div><div>Y <b id="y">—</b></div>
<div>Heading <b id="heading">—</b></div><div>Distance <b id="distance">—</b></div>
</div><p id="velocity">Velocity: —</p>
<button id="level">SET LEVEL & RESET ORIGIN</button>
<button id="zero">STOP & RESET ORIGIN</button>
<p id="levelInfo">Level reference: not set</p>
<p><small>After every restart: place the carrier flat on its normal driving surface, leave it still for 3–5 seconds, then SET LEVEL. This corrects a rotated or upside-down IMU mount; saved in RAM only. Use RESET ORIGIN later without changing level. X=forward and Y=left at reset; positive heading=counterclockwise. Position can drift. A tracking gap requires a new origin for a complete path.</small></p>
<canvas id="path" width="680" height="360"></canvas><small>Recent path; +X right, +Y up. Blue arrow = robot heading.</small>
</section><section><h2>Motor control</h2><p>Hold to move. Release to stop.</p>
<div class="pad"><span></span><button data-cmd="F">FORWARD</button><span></span>
<button data-cmd="L">LEFT</button><button id="stop" class="stop">STOP</button><button data-cmd="R">RIGHT</button>
<span></span><button data-cmd="B">BACKWARD</button><span></span></div>
<label>Speed: <span id="percent">30</span>%<input id="speed" type="range" min="0" max="100" value="30"></label>
<div class="row"><button id="invL">Invert left</button><button id="invR">Invert right</button></div>
<form id="motorScales">
<label>Left power factor <input id="motorScaleLeft" type="number" min="0" max="2" step="0.01" value="1.00" required></label>
<label>Right power factor <input id="motorScaleRight" type="number" min="0" max="2" step="0.01" value="1.00" required></label>
<button type="submit">STOP & APPLY MOTOR FACTORS</button>
</form>
<p id="appliedScales">Applied factors L/R: —</p>
<small>1.00 = normal; 0.80 = 80% of the speed command; 0 = off. Range 0–2; output capped at 255. RAM only, resets to 1.00 on restart. Apply stops motors; hold a direction again to test.</small>
<p id="motor">PWM: —</p><small>Command heartbeat: 200 ms. Automatic cutoff: 750 ms, checked every 10 ms independently of the web server.</small>
<h2 style="margin-top:24px">Measured movements</h2>
<p>Uses both motor power factors above. Start with a short move on a clear floor.</p>
<label>Distance (mm) <input id="autoDistance" type="number" min="-500" max="500" value="50" step="10"></label>
<label>Turn (degrees) <input id="autoAngle" type="number" min="-180" max="180" value="30" step="5"></label>
<label>Action power (%) <input id="autoPower" type="number" min="15" max="40" value="20" step="1"></label>
<div class="row"><button id="autoMove">MOVE DISTANCE</button><button id="autoTurn">TURN ANGLE</button></div>
<p id="autoStatus">Action: idle</p>
<small>Positive distance = forward; positive angle = left. Keep this page visible for a page-started action. STOP cancels any movement. Agent-started actions are maintained by the laptop tool. Opening or leaving this control page sends STOP.</small>
<h2 style="margin-top:24px">Export data</h2><div class="row">
<button id="record">START CSV RECORDING</button><button id="download">Download CSV</button>
<button id="clear">Clear recording</button><button id="json">Download JSON</button></div>
<p id="recordInfo">0 rows · Not recording</p>
<small>CSV records all telemetry fields at up to 10 Hz while this page is open. Maximum 10,000 rows; download before closing. Nothing is saved to ESP32 flash. Raw deltas are the latest 100 Hz sample; cumulative counts preserve total movement.</small>
<p><a href="/api/telemetry" target="_blank" rel="noopener">Open JSON endpoint</a></p>
</section></div>
<details><summary>Calibration & mounting — RAM only</summary>
<p>Apply stops motors and resets the origin. SET LEVEL corrects the installed IMU orientation. Changing IMU mount angles requires setting level again. Validate optical axes by hand before driving.</p>
<form id="settings"><div class="grid"><div>
<label>Optical scale X <input id="scaleX" type="number" min="0.1" max="10" step="0.000001" required></label>
<label>Optical scale Y <input id="scaleY" type="number" min="0.1" max="10" step="0.000001" required></label>
<label>Optical rotation (deg) <input id="opticalAngleDeg" type="number" min="-180" max="180" step="0.1" required></label>
<label><input id="swapXY" type="checkbox"> Swap optical X/Y first</label>
<label><input id="invertX" type="checkbox"> Invert optical X after swap</label>
<label><input id="invertY" type="checkbox"> Invert optical Y after swap</label>
<small>Then scales are applied, then rotation into body axes. For scale: new = old × known distance / measured distance.</small>
</div><div>
<label>Optical forward offset (mm) <input id="offsetXmm" type="number" min="-500" max="500" step="0.1" required></label>
<label>Optical left offset (mm) <input id="offsetYmm" type="number" min="-500" max="500" step="0.1" required></label>
<label>IMU mount roll (deg) <input id="imuRollDeg" type="number" min="-180" max="180" step="0.1" required></label>
<label>IMU mount pitch (deg) <input id="imuPitchDeg" type="number" min="-180" max="180" step="0.1" required></label>
<label>IMU mount yaw (deg) <input id="imuYawDeg" type="number" min="-180" max="180" step="0.1" required></label>
<small>Offsets above are for the PAW3395, not the IMU. Zero offsets track the optical sensor point. Your IMU is 15 mm behind the centre and 37 mm above ground; this does not shift the heading-based position calculation. IMU angles describe its axes relative to the body (ZYX); SET LEVEL corrects residual tilt.</small>
</div><div>
<label>Minimum SQUAL <input id="minSqual" type="number" min="0" max="100" step="1" required></label>
<label>Max speed (mm/s) <input id="maxSpeedMmS" type="number" min="10" max="5000" step="1" required></label>
<label>Max tilt (deg) <input id="maxTiltDeg" type="number" min="1" max="30" step="1" required></label>
<small>SQUAL is a surface-quality count, not a probability. The default threshold 10 is a starting point, not a calibrated guarantee.</small>
</div></div><button type="submit">APPLY & RESET ORIGIN</button></form></details>
<div class="grid"><details open><summary>IMU — acceleration / gyro / quaternion</summary><table id="imuData"></table></details>
<details open><summary>Optical sensor — counts / surface / timing</summary><table id="pawData"></table></details>
<details><summary>Pose & system diagnostics</summary><table id="systemData"></table></details></div>
<p class="muted">No camera or encoders. Rotation-vector mode matches the original working firmware. Nearby magnetic fields can affect heading. Keep the sensor at its correct working height on a suitable textured surface.</p>
</main><script>
'use strict';
const $=id=>document.getElementById(id);
let latest=null,lastReceived=0,pollBusy=false,settingsLoaded=false,adminBusy=false;
let held='',pointer=null,generation=0,lease=null,seq=0,moveBusy=false;
let autoLease=null,autoBusy=false;
let recording=false,rows=[],columns=[],lastSample='',pathPoints=[],pathFrame='';
const configKeys=['scaleX','scaleY','opticalAngleDeg','offsetXmm','offsetYmm','imuRollDeg','imuPitchDeg','imuYawDeg','swapXY','invertX','invertY','minSqual','maxSpeedMmS','maxTiltDeg'];
const boolKeys=['swapXY','invertX','invertY'];
function message(s){$('message').textContent=s;}
async function request(url,post=false,timeout=1200){
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),timeout);
  try{const r=await fetch(url,{method:post?'POST':'GET',cache:'no-store',signal:controller.signal,keepalive:post});
    if(!r.ok)throw new Error(await r.text());return await r.json();
  }finally{clearTimeout(timer);}
}
function stop(){
  autoLease=null;
  held='';pointer=null;lease=null;++generation;
  return request('/api/motor?cmd=S',true).catch(()=>{});
}
async function moveTick(){
  if(!held||lease===null||moveBusy)return;
  moveBusy=true;const g=generation,cmd=held,token=lease;
  try{await request('/api/motor?cmd='+cmd+'&epoch='+token+'&seq='+(++seq),true,800);}
  catch(e){message('Movement stopped: '+e.message);stop();}
  finally{moveBusy=false;if(g!==generation)request('/api/motor?cmd=S',true).catch(()=>{});}
}
document.querySelectorAll('[data-cmd]').forEach(button=>{
  button.addEventListener('pointerdown',async e=>{
    if(pointer!==null||adminBusy||e.button!==0)return;
    e.preventDefault();pointer=e.pointerId;button.setPointerCapture(pointer);held=button.dataset.cmd;
    const g=++generation;lease=null;
    try{const a=await request('/api/arm',true,800);
      if(g!==generation||!held){request('/api/motor?cmd=S',true).catch(()=>{});return;}
      lease=a.epoch;seq=0;message('');moveTick();
    }catch(err){message(err.message);stop();}
  });
  button.addEventListener('lostpointercapture',e=>{if(e.pointerId===pointer)stop();});
});
['pointerup','pointercancel'].forEach(t=>document.addEventListener(t,e=>{if(e.pointerId===pointer)stop();}));
document.addEventListener('contextmenu',e=>{if(e.target.closest('.pad'))e.preventDefault();});
$('stop').onpointerdown=stop;$('stop').onclick=stop;
window.addEventListener('blur',stop);
window.addEventListener('pagehide',()=>{held='';lease=null;++generation;navigator.sendBeacon('/api/motor?cmd=S');});
document.addEventListener('visibilitychange',()=>{if(document.hidden)stop();});
setInterval(moveTick,200);
async function startAuto(kind){
  if(adminBusy||autoBusy)return;adminBusy=true;
  try{
    const stopped=stop(),g=generation;
    await stopped;await new Promise(resolve=>setTimeout(resolve,40));
    if(g!==generation||document.hidden)return;
    const d=await request('/api/telemetry');
    const s=await request('/api/action');
    if(g!==generation||document.hidden)return;
    const p=new URLSearchParams({boot_id:d.boot_id,origin_id:d.pose.origin_id,id:s.id+1,
      kind,value:$(kind==='move'?'autoDistance':'autoAngle').value,speed:$('autoPower').value});
    const a=await request('/api/action?'+p.toString(),true,600);
    if(g!==generation){await stop();return;}
    if(a.state==='running')autoLease={boot:a.boot_id,id:a.id};
    message('Action '+a.state+': '+a.reason);
  }catch(e){message(e.message);await stop();}finally{adminBusy=false;}
}
$('autoMove').onclick=()=>startAuto('move');$('autoTurn').onclick=()=>startAuto('turn');
async function autoTick(){
  if(!autoLease||autoBusy||document.hidden)return;
  autoBusy=true;const token=autoLease;
  try{
    const a=await request('/api/action/heartbeat?boot_id='+token.boot+'&id='+token.id,true,600);
    if(autoLease===token&&a.state!=='running'){autoLease=null;message('Action '+a.state+': '+a.reason);}
  }catch(e){message('Action stopped: '+e.message);await stop();}finally{autoBusy=false;}
}
setInterval(autoTick,200);
async function action(url){
  if(adminBusy)return;adminBusy=true;
  try{await stop();const r=await request(url,true);message(r.message||'Applied');}
  catch(e){message(e.message);}finally{adminBusy=false;}
}
$('speed').oninput=()=>{$('percent').textContent=$('speed').value;};
$('speed').onchange=()=>request('/api/speed?value='+$('speed').value,true).catch(e=>message(e.message));
$('invL').onclick=()=>action('/api/invert?side=L');$('invR').onclick=()=>action('/api/invert?side=R');
$('motorScales').onsubmit=e=>{
  e.preventDefault();
  const p=new URLSearchParams({left:$('motorScaleLeft').value,right:$('motorScaleRight').value});
  action('/api/motor-scale?'+p.toString());
};
$('zero').onclick=()=>action('/api/zero');
$('level').onclick=()=>action('/api/level');
$('settings').onsubmit=e=>{
  e.preventDefault();const p=new URLSearchParams();
  configKeys.forEach(k=>p.set(k,boolKeys.includes(k)?($(k).checked?'1':'0'):$(k).value));
  action('/api/config?'+p.toString());
};
function flatten(o,p='',out={}){
  Object.entries(o).forEach(([k,v])=>{const key=p?p+'.'+k:k;if(v!==null&&typeof v==='object')flatten(v,key,out);else out[key]=v;});return out;
}
function csvCell(v){return '"'+String(v===undefined?'':v).replace(/"/g,'""')+'"';}
function recordSample(d){
  const key=d.boot_id+':'+d.sample;if(key===lastSample)return;lastSample=key;
  if(!recording)return;
  const flat=flatten(d);flat.browser_utc=new Date().toISOString();
  if(!columns.length)columns=Object.keys(flat);
  rows.push(columns.map(k=>csvCell(flat[k])).join(','));
  if(rows.length>=10000){recording=false;message('Recording limit reached. Download CSV, then clear to start again.');}
  recordLabel();
}
function recordLabel(){
  $('record').textContent=recording?'STOP CSV RECORDING':'START CSV RECORDING';
  $('recordInfo').textContent=rows.length+' rows · '+(recording?'Recording':'Not recording');
}
function save(name,text,type){
  const url=URL.createObjectURL(new Blob([text],{type})),a=document.createElement('a');
  a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),10000);
}
$('record').onclick=()=>{if(!recording&&rows.length>=10000){message('Download and clear the full recording first.');return;}recording=!recording;recordLabel();};
$('download').onclick=()=>{if(!rows.length){message('Record some samples first.');return;}save('carrier-telemetry.csv',columns.map(csvCell).join(',')+'\r\n'+rows.join('\r\n')+'\r\n','text/csv;charset=utf-8');};
$('clear').onclick=()=>{if(recording){message('Stop recording before clearing.');return;}rows=[];columns=[];recordLabel();};
$('json').onclick=()=>{if(latest)save('carrier-snapshot.json',JSON.stringify(latest,null,2),'application/json');};
function table(id,obj){
  const fragment=document.createDocumentFragment();
  Object.entries(obj).forEach(([k,v])=>{const tr=document.createElement('tr'),a=document.createElement('td'),b=document.createElement('td');
    a.textContent=k.replace(/_/g,' ');b.textContent=String(v);tr.append(a,b);fragment.appendChild(tr);});
  $(id).replaceChildren(fragment);
}
function plot(d){
  const frame=d.boot_id+':'+d.pose.origin_id;
  if(frame!==pathFrame){pathFrame=frame;pathPoints=[];}
  pathPoints.push({x:d.pose.x_mm,y:d.pose.y_mm,valid:d.pose.valid});if(pathPoints.length>1500)pathPoints.shift();
  const c=$('path'),g=c.getContext('2d'),w=c.width,h=c.height;
  g.clearRect(0,0,w,h);let minX=0,maxX=0,minY=0,maxY=0;
  pathPoints.forEach(p=>{minX=Math.min(minX,p.x);maxX=Math.max(maxX,p.x);minY=Math.min(minY,p.y);maxY=Math.max(maxY,p.y);});
  const scale=Math.min((w-60)/Math.max(100,maxX-minX),(h-60)/Math.max(100,maxY-minY));
  const cx=(minX+maxX)/2,cy=(minY+maxY)/2,X=x=>w/2+(x-cx)*scale,Y=y=>h/2-(y-cy)*scale;
  g.strokeStyle='#dce1e5';g.beginPath();g.moveTo(X(0),0);g.lineTo(X(0),h);g.moveTo(0,Y(0));g.lineTo(w,Y(0));g.stroke();
  g.strokeStyle='#2465b8';g.lineWidth=2;g.beginPath();let prev=false;
  pathPoints.forEach(p=>{if(p.valid&&prev)g.lineTo(X(p.x),Y(p.y));else g.moveTo(X(p.x),Y(p.y));prev=p.valid;});g.stroke();
  const p=d.pose,angle=-p.heading_deg*Math.PI/180;
  g.save();g.translate(X(p.x_mm),Y(p.y_mm));g.rotate(angle);g.fillStyle=p.valid?'#2465b8':'#b85921';
  g.beginPath();g.moveTo(12,0);g.lineTo(-8,-6);g.lineTo(-8,6);g.closePath();g.fill();g.restore();
  g.fillStyle='#586573';g.font='13px sans-serif';g.fillText('Visible width: '+(w/scale).toFixed(0)+' mm',10,h-10);
}
function render(d){
  const p=d.pose;$('x').textContent=p.x_mm.toFixed(2)+' mm';$('y').textContent=p.y_mm.toFixed(2)+' mm';
  $('heading').textContent=p.heading_deg.toFixed(2)+'°';$('distance').textContent=p.distance_mm.toFixed(2)+' mm';
  $('velocity').textContent='Velocity X/Y: '+p.vx_mm_s.toFixed(1)+' / '+p.vy_mm_s.toFixed(1)+' mm/s';
  $('levelInfo').textContent=(d.imu.level_calibrated?'Tilt from calibrated level: '+d.imu.tilt_deg.toFixed(1)+'° / limit '+d.imu.tilt_limit_deg.toFixed(0)+'°':'Level reference not set')
    +' · '+(d.imu.level_capture_ready?'Ready to SET LEVEL':'Stop and keep flat/still to set level');
  $('status').textContent=p.status+(p.incomplete?' · POSITION INCOMPLETE — tracking was lost':'')+(p.origin_pending?' · Origin reset pending':'');
  $('status').style.background=p.valid&&!p.incomplete?'#e4f4e9':'#fff0d5';
  $('motor').textContent='Applied PWM L/R: '+d.motor.left_pwm+' / '+d.motor.right_pwm;
  $('appliedScales').textContent='Applied factors L/R: '+d.motor.scale_left.toFixed(2)+' / '+d.motor.scale_right.toFixed(2);
  if(d.action)$('autoStatus').textContent='Action '+d.action.id+': '+d.action.state+' · '+d.action.kind+' '+d.action.progress.toFixed(1)+' / '+d.action.target+' · '+d.action.reason;
  $('invL').textContent='Invert left: '+(d.motor.invert_left?'ON':'OFF');$('invR').textContent='Invert right: '+(d.motor.invert_right?'ON':'OFF');
  if(!settingsLoaded){configKeys.forEach(k=>{if(boolKeys.includes(k))$(k).checked=d.config[k];else $(k).value=d.config[k];});
    $('motorScaleLeft').value=d.motor.scale_left.toFixed(2);$('motorScaleRight').value=d.motor.scale_right.toFixed(2);
    $('speed').value=d.motor.speed_percent;$('percent').textContent=d.motor.speed_percent;settingsLoaded=true;}
  table('imuData',d.imu);table('pawData',d.paw);table('systemData',flatten({pose:d.pose,system:d.system,motor:d.motor,boot_id:d.boot_id,sample:d.sample,uptime_ms:d.uptime_ms}));
  plot(d);recordSample(d);
}
async function poll(){
  if(pollBusy||document.hidden)return;pollBusy=true;
  try{const d=await request('/api/telemetry');if(d.error)throw new Error(d.error);latest=d;lastReceived=Date.now();render(d);}
  catch(e){$('status').textContent='Disconnected / telemetry unavailable';if(held||autoLease)stop();}
  finally{pollBusy=false;}
}
setInterval(()=>{if(lastReceived&&Date.now()-lastReceived>1500){$('status').textContent='Telemetry stale — check connection';if(held||autoLease)stop();}},500);
stop();poll();setInterval(poll,100);
</script></body></html>
)HTML";


void reply(int code,const char *text){
  server.sendHeader("Cache-Control","no-store");server.send(code,"application/json",text);
}
bool parseNumber(const char *name,float minimum,float maximum,float &value){
  if(!server.hasArg(name))return false;
  String s=server.arg(name);char *end=nullptr;errno=0;
  float n=strtof(s.c_str(),&end);
  if(s.length()==0||end==s.c_str()||*end||errno||!isfinite(n)||n<minimum||n>maximum)return false;
  value=n;return true;
}
bool parseUnsigned(const char *name,uint32_t &value){
  if(!server.hasArg(name))return false;
  String s=server.arg(name);if(s.length()==0||s.length()>10)return false;
  for(unsigned i=0;i<s.length();++i)if(s[i]<'0'||s[i]>'9')return false;
  char *end=nullptr;errno=0;unsigned long long n=strtoull(s.c_str(),&end,10);
  if(errno||*end||n>UINT32_MAX)return false;value=(uint32_t)n;return true;
}
void handleRoot(){
  invalidateMotorLease();server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",PAGE);
}
void handleTelemetry(){reply(200,telemetry);}
void handleActionStatus(){
  char b[1200];
  snprintf(b,sizeof(b),"{\"boot_id\":%lu,\"id\":%lu,\"state\":\"%s\",\"kind\":\"%s\",\"reason\":\"%s\","
    "\"target\":%.3f,\"progress\":%.3f,\"units\":\"%s\",\"speed_percent\":%d,\"origin_id\":%lu,"
    "\"elapsed_ms\":%lu,\"motor_stopped\":%s,\"pose\":{\"x_mm\":%.5f,\"y_mm\":%.5f,\"heading_deg\":%.4f,\"healthy\":%s}}",
    (unsigned long)bootId,(unsigned long)motion.id,motion.stateName(),motion.kindName(),motion.reason,
    motion.target,motion.progress,motion.kind==carrier::Kind::Move?"mm":"deg",motion.speed,
    (unsigned long)motion.origin,(unsigned long)(motion.id?(uint32_t)(millis()-motion.started):0),
    jb(!requestedLeft&&!requestedRight&&!leftMotorCommand&&!rightMotorCommand),
    pose.x,pose.y,degreesF(pose.heading),jb(automationTrackingHealthy()));
  reply(200,b);
}
void handleActionStart(){
  uint32_t boot,origin,id;float value,power;
  String kind=server.arg("kind");
  if(!parseUnsigned("boot_id",boot)||!parseUnsigned("origin_id",origin)||!parseUnsigned("id",id)||!id
      ||!(kind=="move"||kind=="turn")||!parseNumber("value",-500,500,value)
      ||!parseNumber("speed",15,40,power)||floorf(power)!=power){
    reply(400,"{\"error\":\"Need boot_id, origin_id, positive id, kind=move|turn, signed value, speed=15..40 integer\"}");return;
  }
  carrier::Kind k=kind=="move"?carrier::Kind::Move:carrier::Kind::Turn;
  if(!carrier::MotionController::validRequest(k,value,(int)power)){
    reply(400,"{\"error\":\"Move magnitude 10..500 mm; turn magnitude 5..180 degrees\"}");return;
  }
  if(boot!=bootId){reply(409,"{\"error\":\"Robot restarted; read telemetry again\"}");return;}
  // Same request is replay-safe, including after completion/cancel/failure.
  if(id==motion.id){
    if(k!=motion.kind||value!=motion.target||(int)power!=motion.speed||origin!=motion.origin){
      reply(409,"{\"error\":\"Action id already used with different parameters\"}");return;
    }
    handleActionStatus();return;
  }
  if(id<motion.id){reply(409,"{\"error\":\"Old action id; never replay an earlier movement\"}");return;}
  if(motion.running()||requestedLeft||requestedRight||leftMotorCommand||rightMotorCommand){
    reply(409,"{\"error\":\"Movement active; wait or STOP\"}");return;
  }
  if(!motorTimerReady||motorScaleLeft<=0||motorScaleRight<=0){
    reply(409,"{\"error\":\"Motor timer and two nonzero power factors required\"}");return;
  }
  if(origin!=pose.originId||!automationTrackingHealthy()){
    reply(409,"{\"error\":\"Need calibrated, fresh, complete tracking in the requested origin\"}");return;
  }
  invalidateMotorLease();
  uint32_t now=millis();
  if(!motion.start(id,k,value,(int)power,motionObservation(),now)){
    reply(409,"{\"error\":\"Action could not start\"}");return;
  }
  portENTER_CRITICAL(&motorMux);actionMotorEpoch=motorEpoch;lastMotorCommandMs=now;portEXIT_CRITICAL(&motorMux);
  handleActionStatus();
}
void handleActionHeartbeat(){
  uint32_t boot,id;
  if(!parseUnsigned("boot_id",boot)||!parseUnsigned("id",id)||boot!=bootId||!id||id!=motion.id){
    reply(409,"{\"error\":\"Heartbeat does not match this boot and action\"}");return;
  }
  if(motion.running()&&!motion.keepAlive(id,millis()))invalidateMotorLease();
  handleActionStatus();
}
void handleActionCancel(){
  uint32_t boot,id;
  if(!parseUnsigned("boot_id",boot)||!parseUnsigned("id",id)||boot!=bootId||!id||id!=motion.id){
    reply(409,"{\"error\":\"Cancel does not match this boot and action\"}");return;
  }
  if(motion.running())invalidateMotorLease();
  handleActionStatus();
}
void handleArm(){
  if(motion.running()){reply(409,"{\"error\":\"Automated action active; STOP first\"}");return;}
  if(!motorTimerReady){reply(503,"{\"error\":\"Motor timer unavailable\"}");return;}
  bool busy;uint32_t epoch;
  portENTER_CRITICAL(&motorMux);
  busy=requestedLeft||requestedRight;
  if(!busy){motorEpoch=motorEpoch+1;motorSequence=0;lastMotorCommandMs=millis();}
  epoch=motorEpoch;portEXIT_CRITICAL(&motorMux);
  if(busy){reply(409,"{\"error\":\"Another movement is active; STOP first\"}");return;}
  char b[64];snprintf(b,sizeof(b),"{\"epoch\":%lu}",(unsigned long)epoch);reply(200,b);
}
void handleMotor(){
  String cmd=server.arg("cmd");
  if(cmd=="S"){invalidateMotorLease();reply(200,"{\"ok\":true}");return;}
  if(motion.running()){reply(409,"{\"error\":\"Automated action active; STOP first\"}");return;}
  uint32_t epoch,seq;
  if(!parseUnsigned("epoch",epoch)||!parseUnsigned("seq",seq)||!(cmd=="F"||cmd=="B"||cmd=="L"||cmd=="R")){
    reply(400,"{\"error\":\"Invalid motor command\"}");return;
  }
  if(!motorTimerReady){reply(503,"{\"error\":\"Motor timer unavailable\"}");return;}
  int pwm=speedPercent*255/100,l=0,r=0;
  if(cmd=="F"){l=pwm;r=pwm;}if(cmd=="B"){l=-pwm;r=-pwm;}
  if(cmd=="L"){l=-pwm;r=pwm;}if(cmd=="R"){l=pwm;r=-pwm;}
  l=scaleMotorCommand(l,motorScaleLeft);r=scaleMotorCommand(r,motorScaleRight);
  if(invertLeft)l=-l;if(invertRight)r=-r;
  portENTER_CRITICAL(&motorMux);
  bool accepted=epoch==motorEpoch&&seq>motorSequence&&(uint32_t)(millis()-lastMotorCommandMs)<MOTOR_TIMEOUT_MS;
  if(accepted){motorSequence=seq;lastMotorCommandMs=millis();requestedLeft=l;requestedRight=r;}
  portEXIT_CRITICAL(&motorMux);
  reply(accepted?200:409,accepted?"{\"ok\":true}":"{\"error\":\"Expired movement; release and press again\"}");
}
void handleSpeed(){
  float value;if(!parseNumber("value",0,100,value)||floorf(value)!=value){reply(400,"{\"error\":\"Speed must be integer 0..100\"}");return;}
  if(motion.running())invalidateMotorLease();
  speedPercent=(int)value;reply(200,"{\"ok\":true}");
}
bool stopBeforeChange(){
  invalidateMotorLease();uint32_t started=millis();
  while((leftMotorCommand||rightMotorCommand)&&(uint32_t)(millis()-started)<30)delay(1);
  return leftMotorCommand==0&&rightMotorCommand==0;
}
void handleMotorScale(){
  float left,right;
  if(!parseNumber("left",0,2,left)||!parseNumber("right",0,2,right)){
    reply(400,"{\"error\":\"Both motor factors must be numbers from 0 to 2\"}");return;
  }
  if(!stopBeforeChange()){reply(503,"{\"error\":\"Motor stop not yet confirmed\"}");return;}
  motorScaleLeft=left;motorScaleRight=right;
  reply(200,"{\"message\":\"Motor factors applied; hold a direction to test\"}");
}
void handleInvert(){
  if(!stopBeforeChange()){reply(503,"{\"error\":\"Motor stop not yet confirmed\"}");return;}
  String side=server.arg("side");
  if(side=="L")invertLeft=!invertLeft;else if(side=="R")invertRight=!invertRight;
  else{reply(400,"{\"error\":\"Side must be L or R\"}");return;}
  reply(200,"{\"message\":\"Motor stopped; inversion toggled\"}");
}
void handleLevel(){
  if(!stopBeforeChange()){reply(503,"{\"error\":\"Motor stop not yet confirmed\"}");return;}
  updateIMUStillness();
  if(!levelCaptureReady()){
    reply(409,"{\"error\":\"Place flat, keep still for 3-5 seconds, then press SET LEVEL again; fresh IMU readings are required\"}");return;
  }
  captureLevelReference();imuFrameChanged=false;requestOrigin();
  Serial.printf("LEVEL SET | corrected tilt: %.2f deg | IMU: 15 mm rear, 37 mm above ground\n",bodyTiltDeg);
  reply(200,"{\"message\":\"Level calibrated in RAM; origin resets at the next valid optical sample\"}");
}
void handleZero(){
  if(!stopBeforeChange()){reply(503,"{\"error\":\"Motor stop not yet confirmed\"}");return;}
  if(!levelCalibrated){reply(409,"{\"error\":\"Place flat and use SET LEVEL first\"}");return;}
  imuFrameChanged=false;requestOrigin();
  reply(202,"{\"message\":\"Keep still: origin resets at the next valid sensor sample\"}");
}
void handleConfig(){
  if(!stopBeforeChange()){reply(503,"{\"error\":\"Motor stop not yet confirmed\"}");return;}
  Configuration c=cfg;float squal;
  bool ok=parseNumber("scaleX",0.1,10,c.scaleX)&&parseNumber("scaleY",0.1,10,c.scaleY)
    &&parseNumber("opticalAngleDeg",-180,180,c.opticalAngleDeg)
    &&parseNumber("offsetXmm",-500,500,c.offsetXmm)&&parseNumber("offsetYmm",-500,500,c.offsetYmm)
    &&parseNumber("imuRollDeg",-180,180,c.imuRollDeg)&&parseNumber("imuPitchDeg",-180,180,c.imuPitchDeg)
    &&parseNumber("imuYawDeg",-180,180,c.imuYawDeg)&&parseNumber("minSqual",0,100,squal)
    &&floorf(squal)==squal&&parseNumber("maxSpeedMmS",10,5000,c.maxSpeedMmS)
    &&parseNumber("maxTiltDeg",1,30,c.maxTiltDeg);
  const char *keys[]={"swapXY","invertX","invertY"};
  for(const char *k:keys)if(!server.hasArg(k)||(server.arg(k)!="0"&&server.arg(k)!="1"))ok=false;
  if(!ok){reply(400,"{\"error\":\"Invalid/missing calibration; no settings changed\"}");return;}
  c.minSqual=(int)squal;c.swapXY=server.arg("swapXY")=="1";
  c.invertX=server.arg("invertX")=="1";c.invertY=server.arg("invertY")=="1";
  bool mountingChanged=c.imuRollDeg!=cfg.imuRollDeg||c.imuPitchDeg!=cfg.imuPitchDeg||c.imuYawDeg!=cfg.imuYawDeg;
  if(mountingChanged){levelCalibrated=false;levelW=1;levelX=levelY=levelZ=0;}
  cfg=c;haveQuat=false;imuStillSinceMs=0;imuFrameChanged=false;requestOrigin();
  reply(200,levelCalibrated?"{\"message\":\"Settings applied in RAM; keep still for the new origin\"}":
    "{\"message\":\"Settings applied in RAM; place flat, wait 3-5 seconds, then SET LEVEL\"}");
}
void setup(){
  Serial.begin(115200); // No while(!Serial): battery operation never waits for a PC.
  Serial.println("\n==================================\nPHOTONICS CARRIER POSITION v2.3\n==================================");
  const int pins[]={MOTOR_L_IN1,MOTOR_L_IN2,MOTOR_R_IN1,MOTOR_R_IN2};
  for(int pin:pins){pinMode(pin,OUTPUT);digitalWrite(pin,LOW);}
  setupMotorTimer();Serial.println("Motor pins initialized (MR2 = GPIO12)");
  bootId=esp_random();motorEpoch=bootId?bootId:1;
  pinMode(IMU_CS,OUTPUT);digitalWrite(IMU_CS,HIGH);
  pinMode(PAW_CS,OUTPUT);digitalWrite(PAW_CS,HIGH);pinMode(PAW_MOTION,INPUT);
  SPI.begin(18,19,23,27);
  // Restore the original startup order: sensors first, then the Wi-Fi radio.
  setupIMU();setupPAW();pawConnected=pawFound;
  if(pawFound){pawProduct=0x51;pawInverse=0xAE;}
  Serial.println("Starting WiFi");WiFi.mode(WIFI_AP);WiFi.setSleep(false);
  if(!WiFi.softAP(AP_SSID,AP_PASSWORD))Serial.println("WiFi AP start FAILED");
  Serial.print("SSID: ");Serial.println(AP_SSID);Serial.print("IP: ");Serial.println(WiFi.softAPIP());
  server.on("/",HTTP_GET,handleRoot);server.on("/api/telemetry",HTTP_GET,handleTelemetry);
  server.on("/imu",HTTP_GET,handleTelemetry);server.on("/api/arm",HTTP_POST,handleArm);
  server.on("/api/motor",HTTP_POST,handleMotor);server.on("/api/speed",HTTP_POST,handleSpeed);
  server.on("/api/motor-scale",HTTP_POST,handleMotorScale);
  server.on("/api/invert",HTTP_POST,handleInvert);server.on("/api/zero",HTTP_POST,handleZero);
  server.on("/api/level",HTTP_POST,handleLevel);
  server.on("/api/config",HTTP_POST,handleConfig);
  server.on("/api/action",HTTP_GET,handleActionStatus);
  server.on("/api/action",HTTP_POST,handleActionStart);
  server.on("/api/action/heartbeat",HTTP_POST,handleActionHeartbeat);
  server.on("/api/action/cancel",HTTP_POST,handleActionCancel);
  server.onNotFound([](){reply(404,"{\"error\":\"Not found\"}");});
  makeSnapshot();server.begin();Serial.println("Web server started");
  makeSnapshot();Serial.println("Place flat and still for 3-5 seconds, then SET LEVEL & RESET ORIGIN in the web UI.");
}
void loop(){
  uint32_t start=micros();checkMotorTimeout();
  readIMU();updateIMUStillness();readPAW();checkTrackingHealth();
  updateAutomation();server.handleClient();checkMotorTimeout();updateAutomation();
  uint32_t now=millis();
  if((uint32_t)(now-lastSnapshotMs)>=100){lastSnapshotMs=now;checkTrackingHealth();makeSnapshot();}
  if((uint32_t)(now-lastDebugMs)>=500){
    lastDebugMs=now;
    Serial.printf("POS | X/Y mm: %.2f, %.2f | H: %.2f | %s%s | SQUAL: %u | MOTOR: %d, %d\n",
      pose.x,pose.y,degreesF(pose.heading),statusText(),pose.incomplete?" [INCOMPLETE]":"",pawSqual,leftMotorCommand,rightMotorCommand);
    Serial.printf("IMU | RPY: %.2f, %.2f, %.2f | ACC: %.3f, %.3f, %.3f | GYRO: %.4f, %.4f, %.4f | PAW total: %lld, %lld\n",
      degreesF(roll),degreesF(pitch),degreesF(yaw),accelX,accelY,accelZ,gyroX,gyroY,gyroZ,(long long)pawTotalX,(long long)pawTotalY);
  }
  static uint32_t lastImuDiagnosticMs=0;
  if((uint32_t)(now-lastImuDiagnosticMs)>=2000){
    lastImuDiagnosticMs=now;
    Serial.printf("IMU STATUS | found=%d INT=%d mask=%u | reports A/G/Q=%lu/%lu/%lu | Q age=%ld ms | level=%d ready=%d tilt=%.2f/%.1f deg\n",
      imuFound,digitalRead(IMU_INT),reportMask,(unsigned long)accelReports,
      (unsigned long)gyroReports,(unsigned long)quatReports,ageMs(haveQuat,quatUs,micros()),
      levelCalibrated,levelCaptureReady(),bodyTiltDeg,cfg.maxTiltDeg);
  }
  uint32_t duration=micros()-start;if(duration>maxLoopUs)maxLoopUs=duration;
}
