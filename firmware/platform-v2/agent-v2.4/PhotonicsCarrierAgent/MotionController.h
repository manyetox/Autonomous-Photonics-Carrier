#pragma once
#include <stdint.h>
#include <math.h>

// Hardware-independent controller; shared by the sketch and host tests.
// All motor values here are BEFORE the existing per-wheel scales/inversion.
namespace carrier {
constexpr float MOTION_PI = 3.14159265358979323846f;
constexpr uint32_t CLIENT_TIMEOUT_MS = 1000;
constexpr uint32_t ACTION_TIMEOUT_MS = 20000;
constexpr uint32_t STALL_TIMEOUT_MS = 2500;
inline float wrap(float a) { return atan2f(sinf(a), cosf(a)); }
inline float clamp(float x, float lo, float hi) { return fminf(hi, fmaxf(lo, x)); }
enum class Kind { Move, Turn };
enum class State { Idle, Running, Completed, Cancelled, Fault };
struct Observation {
  double x=0, y=0;
  float heading=0;
  bool healthy=false;
  uint32_t origin=0;
};
struct Output { int left=0, right=0; };
class MotionController {
public:
  State state=State::Idle;
  Kind kind=Kind::Move;
  uint32_t id=0, origin=0, started=0, heartbeat=0, progressAt=0;
  float target=0, progress=0, best=0, startHeading=0, lastHeading=0;
  float crossTrack=0, headingError=0;
  double startX=0, startY=0;
  int speed=20;
  const char* reason="idle";
  bool running() const { return state==State::Running; }
  const char* stateName() const {
    switch(state) {
      case State::Idle:return "idle";
      case State::Running:return "running";
      case State::Completed:return "completed";
      case State::Cancelled:return "cancelled";
      default:return "fault";
    }
  }
  const char* kindName() const { return kind==Kind::Move?"move":"turn"; }
  static bool validRequest(Kind k,float value,int power) {
    return isfinite(value)&&power>=15&&power<=40&&
      (k==Kind::Move?(fabsf(value)>=10&&fabsf(value)<=500):
                      (fabsf(value)>=5&&fabsf(value)<=180));
  }
  bool start(uint32_t newId,Kind k,float value,int power,const Observation& p,uint32_t now) {
    if(running()||!newId||newId<=id||!p.healthy||!validRequest(k,value,power)
        ||!isfinite(p.x)||!isfinite(p.y)||!isfinite(p.heading))return false;
    id=newId;kind=k;target=value;speed=power;origin=p.origin;
    startX=p.x;startY=p.y;startHeading=lastHeading=p.heading;
    progress=best=crossTrack=headingError=0;
    started=heartbeat=progressAt=now;state=State::Running;reason="running";
    return true;
  }
  void finish(State s,const char* why) { state=s;reason=why; }
  void cancel(const char* why) { if(running())finish(State::Cancelled,why); }
  bool keepAlive(uint32_t requestId,uint32_t now) {
    if(!running()||requestId!=id)return false;
    // An overdue heartbeat cannot revive an expired action.
    if((uint32_t)(now-heartbeat)>=CLIENT_TIMEOUT_MS) {
      finish(State::Fault,"client_timeout");return false;
    }
    heartbeat=now;return true;
  }
  Output step(const Observation& p,uint32_t now) {
    Output out;
    if(!running())return out;
    if(!p.healthy||!isfinite(p.x)||!isfinite(p.y)||!isfinite(p.heading)) {
      finish(State::Fault,"tracking_unreliable");return out;
    }
    if(p.origin!=origin){finish(State::Fault,"origin_changed");return out;}
    if((uint32_t)(now-heartbeat)>=CLIENT_TIMEOUT_MS){finish(State::Fault,"client_timeout");return out;}
    if((uint32_t)(now-started)>=ACTION_TIMEOUT_MS){finish(State::Fault,"action_timeout");return out;}
    const float sign=target>0?1.0f:-1.0f;
    double dx=p.x-startX,dy=p.y-startY;
    float tolerance=kind==Kind::Move?5.0f:2.0f;
    if(kind==Kind::Move) {
      progress=(float)(cosf(startHeading)*dx+sinf(startHeading)*dy);
      crossTrack=(float)(-sinf(startHeading)*dx+cosf(startHeading)*dy);
      headingError=wrap(startHeading-p.heading);
      if(fabsf(crossTrack)>30||fabsf(headingError)>20*MOTION_PI/180) {
        finish(State::Fault,"path_deviation");return out;
      }
    } else {
      progress+=wrap(p.heading-lastHeading)*180/MOTION_PI;
      lastHeading=p.heading;
      if(hypot(dx,dy)>50){finish(State::Fault,"turn_translation");return out;}
    }
    const float directed=sign*progress;
    if(directed < -10){finish(State::Fault,"wrong_direction");return out;}
    const float remaining=fabsf(target)-directed;
    if(remaining<=tolerance){finish(State::Completed,"target_reached");return out;}
    if(directed>best+0.5f){best=directed;progressAt=now;}
    if((uint32_t)(now-progressAt)>=STALL_TIMEOUT_MS){finish(State::Fault,"no_progress");return out;}
    // Reduce power near the endpoint; wheel-scale calibration still applies.
    const float slowZone=kind==Kind::Move?60.0f:30.0f;
    float power=15+(speed-15)*clamp(remaining/slowZone,0,1);
    float pwm=power*255/100;
    if(kind==Kind::Turn) {
      out.left=(int)lroundf(-sign*pwm);out.right=(int)lroundf(sign*pwm);
    } else {
      // Differential correction works in reverse as well: positive yaw = R-L.
      float correction=clamp(headingError*100,-pwm*0.35f,pwm*0.35f);
      float cap=speed*255.0f/100;
      out.left=(int)lroundf(clamp(sign*pwm-correction,-cap,cap));
      out.right=(int)lroundf(clamp(sign*pwm+correction,-cap,cap));
    }
    return out;
  }
};
}
