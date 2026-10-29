"""Compile actual firmware handlers against a tiny fake ESP32/HTTP environment."""
from pathlib import Path
import subprocess
import os

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "PhotonicsCarrierAgent/PhotonicsCarrierAgent.ino").read_text(encoding="utf-8")

def section(start, end):
    return source[source.index(start):source.index(end, source.index(start))]

prefix = r'''
#include <assert.h>
#include <string>
#include <map>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../PhotonicsCarrierAgent/MotionController.h"
using String=std::string;
struct Server {
  std::map<std::string,std::string> args;
  int code=0;std::string body;
  bool hasArg(const char* n){return args.count(n);}
  String arg(const char* n){return args[n];}
  void sendHeader(const char*,const char*){}
  void send(int c,const char*,const char* b){code=c;body=b;}
} server;
uint32_t tick=1000;
uint32_t millis(){return tick;}
uint32_t micros(){return tick*1000;}
int leftMotorCommand=0,rightMotorCommand=0,requestedLeft=0,requestedRight=0;
void delay(int n){tick+=n;leftMotorCommand=requestedLeft;rightMotorCommand=requestedRight;}
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
template<class T>T constrain(T x,T lo,T hi){return std::min(hi,std::max(lo,x));}
carrier::MotionController motion;
uint32_t bootId=42,actionMotorEpoch=0,motorEpoch=1,motorSequence=0,lastMotorCommandMs=0,motorTimeouts=0;
constexpr uint32_t MOTOR_TIMEOUT_MS=750,MAX_OPTICAL_GAP_US=100000;
bool motorTimerReady=true,invertLeft=false,invertRight=false;
float motorScaleLeft=1,motorScaleRight=1,bodyTiltDeg=0;
int speedPercent=30;
struct Pose {double x=0,y=0;float heading=0;bool ready=true,valid=true,incomplete=false,pending=false;uint32_t originId=1;} pose;
struct Config {int minSqual=10;float maxTiltDeg=15;} cfg;
bool freshHeading=true,pawFound=true,pawConnected=true,pawLifted=false;
int pawSqual=50;uint32_t lastPawGoodUs=1000000;
bool headingFresh(uint32_t){return freshHeading;}
float degreesF(float r){return r*180/carrier::MOTION_PI;}
const char* jb(bool x){return x?"true":"false";}
void fresh(uint32_t t){tick=t;lastPawGoodUs=micros();}
'''

parts = [
    section("int scaleMotorCommand", "int setMotor"),
    section("void invalidateMotorLease", "// Runs in ESP32's"),
    section("bool automationTrackingHealthy", "const char *statusText"),
    section("void reply(int", "void handleRoot"),
    section("void handleActionStatus", "void handleInvert"),
]
# Arduino String::length/operator[]/c_str match std::string; no other adapter needed.
suffix = r'''
void request(uint32_t id,const char* kind="move",const char* value="100",const char* speed="20"){
 server.args={{"boot_id","42"},{"origin_id","1"},{"id",std::to_string(id)},
              {"kind",kind},{"value",value},{"speed",speed}};
}
int main(){
 // Independent motor factors are applied exactly once, including inversion.
 motorScaleLeft=0.5f;motorScaleRight=1.5f;invertRight=true;
 request(1);handleActionStart();assert(server.code==200&&motion.running());
 updateAutomation();assert(requestedLeft==26&&requestedRight==-77);
 // Duplicate start neither changes the starting pose nor renews the heartbeat.
 auto hb=motion.heartbeat;fresh(1100);pose.x=20;
 request(1);handleActionStart();assert(server.code==200&&motion.startX==0&&motion.heartbeat==hb);
 request(1,"move","200");handleActionStart();assert(server.code==409);
 server.args={{"cmd","F"},{"epoch",std::to_string(motorEpoch)},{"seq","1"}};
 handleMotor();assert(server.code==409);
 handleArm();assert(server.code==409);
 // A tracking gap cancels before another motor output can be issued.
 pose.incomplete=true;updateAutomation();assert(motion.state==carrier::State::Fault&&!requestedLeft&&!requestedRight);
 request(2);handleActionStart();assert(server.code==409);
 pose.incomplete=false;pose.x=0;request(2);handleActionStart();assert(server.code==200);
 fresh(1850);checkMotorTimeout(); // watchdog expiry must never be auto-rearmed
 updateAutomation();assert(motion.state==carrier::State::Fault&&!requestedLeft&&!requestedRight);
 request(1);handleActionStart();assert(server.code==409);
 // Heartbeat expiry and stale boot reject revival.
 fresh(2000);request(3);handleActionStart();updateAutomation();
 fresh(3000);server.args={{"boot_id","42"},{"id","3"}};handleActionHeartbeat();
 assert(motion.state==carrier::State::Fault&&!requestedLeft);
 server.args={{"boot_id","41"},{"id","3"}};handleActionHeartbeat();assert(server.code==409);
 // Changing scalers stops the action and retains supplied factors.
 fresh(3100);request(4);handleActionStart();updateAutomation();
 server.args={{"left","0.8"},{"right","1.2"}};handleMotorScale();
 assert(server.code==200&&!motion.running()&&!requestedLeft&&!requestedRight);
 assert(fabsf(motorScaleLeft-.8f)<1e-6f&&fabsf(motorScaleRight-1.2f)<1e-6f);
 // Replaying cancelled action is read-only; STOP works without an epoch.
 request(4);handleActionStart();assert(server.code==200&&motion.state==carrier::State::Cancelled);
 request(5);handleActionStart();updateAutomation();
 server.args={{"cmd","S"}};handleMotor();assert(server.code==200&&!motion.running()&&!requestedLeft);
 request(6);server.args["boot_id"]="41";handleActionStart();assert(server.code==409);
 request(6);server.args["origin_id"]="2";handleActionStart();assert(server.code==409);
 request(6);server.args["id"]="-1";handleActionStart();assert(server.code==400);
 request(6);server.args["value"]="nan";handleActionStart();assert(server.code==400);
 request(6);server.args["speed"]="20.5";handleActionStart();assert(server.code==400);
 motorScaleLeft=0;request(6);handleActionStart();assert(server.code==409);motorScaleLeft=.8f;
 // A cancel for an older job must not interrupt the current job.
 request(6);handleActionStart();updateAutomation();
 server.args={{"boot_id","42"},{"id","5"}};handleActionCancel();assert(server.code==409&&motion.running());
 server.args={{"boot_id","42"},{"id","6"}};handleActionCancel();assert(server.code==200&&!motion.running());
 // Existing manual lease + scale flow still works.
 fresh(3200);server.args.clear();handleArm();assert(server.code==200);
 server.args={{"cmd","B"},{"epoch",std::to_string(motorEpoch)},{"seq","1"}};
 handleMotor();assert(server.code==200&&requestedLeft==-61&&requestedRight==91);
 fresh(3950);checkMotorTimeout();assert(!requestedLeft&&!requestedRight);
 assert(scaleMotorCommand(200,2)==255&&scaleMotorCommand(-200,2)==-255);
 puts("Firmware API, watchdog, replay and motor-scale integration tests passed");
}
'''
build = ROOT / ".build-tools"
build.mkdir(exist_ok=True)
cpp = build / "firmware_handlers_test.cpp"
cpp.write_text(prefix + "\n".join(parts) + suffix, encoding="utf-8")
exe = build / "firmware_handlers_test.exe"
subprocess.run([os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra",
                str(cpp), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)

# Syntax check the actual embedded browser script.
js = source.split("<script>", 1)[1].split("</script>", 1)[0]
js_path = build / "firmware_ui.js"
js_path.write_text(js, encoding="utf-8")
subprocess.run([os.environ.get("NODE", "node"), "--check", str(js_path)], check=True)
print("Embedded web UI JavaScript syntax passed")
