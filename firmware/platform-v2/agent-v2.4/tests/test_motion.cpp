#include "../PhotonicsCarrierAgent/MotionController.h"
#include <assert.h>
#include <stdio.h>
using namespace carrier;
Observation good(double x=0,double y=0,float h=0,uint32_t origin=1){
  Observation p;p.x=x;p.y=y;p.heading=h;p.origin=origin;p.healthy=true;return p;
}
int main(){
  MotionController c;
  assert(!c.start(1,Kind::Move,0,20,good(),0));
  assert(!c.start(1,Kind::Move,501,20,good(),0));
  assert(!c.start(1,Kind::Move,50,41,good(),0));
  assert(!c.start(1,Kind::Turn,181,20,good(),0));
  assert(!c.start(1,Kind::Move,NAN,20,good(),0));
  auto bad=good();bad.healthy=false;
  assert(!c.start(1,Kind::Move,50,20,bad,0));
  assert(c.start(1,Kind::Move,100,20,good(),0));
  auto o=c.step(good(),10);assert(o.left>0&&o.right>0);
  assert(!c.start(2,Kind::Move,50,20,good(),20));
  o=c.step(good(50,0,0.05f),100);assert(o.left>o.right); // correct right
  o=c.step(good(96),200);assert(c.state==State::Completed&&!o.left&&!o.right);
  assert(!c.keepAlive(1,201));assert(!c.start(1,Kind::Move,100,20,good(),300));
  assert(c.start(2,Kind::Move,-100,20,good(),300));
  o=c.step(good(-30,0,-0.05f),400);assert(o.left<o.right&&o.right<0);
  c.step(good(-96),500);assert(c.state==State::Completed);
  // World heading rotated 90 degrees: forward progress is world +Y.
  assert(c.start(3,Kind::Move,100,20,good(0,0,MOTION_PI/2),600));
  c.step(good(0,96,MOTION_PI/2),700);assert(c.state==State::Completed);
  // Wrap-around and both turn signs.
  assert(c.start(4,Kind::Turn,30,20,good(0,0,170*MOTION_PI/180),800));
  o=c.step(good(0,0,175*MOTION_PI/180),900);assert(o.left<0&&o.right>0);
  c.step(good(0,0,-161*MOTION_PI/180),950);assert(c.state==State::Completed);
  assert(c.start(5,Kind::Turn,-30,20,good(0,0,-170*MOTION_PI/180),1000));
  c.step(good(0,0,161*MOTION_PI/180),1100);assert(c.state==State::Completed);
  assert(c.start(6,Kind::Move,100,20,good(),1200));
  o=c.step(bad,1250);assert(c.state==State::Fault&&!o.left&&!o.right);
  assert(c.start(7,Kind::Move,100,20,good(),1300));
  c.step(good(0,0,0,2),1350);assert(c.state==State::Fault);
  assert(c.start(8,Kind::Move,100,20,good(),1400));
  assert(!c.keepAlive(7,1450));c.step(good(),2400);assert(c.state==State::Fault);
  assert(!c.keepAlive(8,2401));
  assert(c.start(9,Kind::Move,100,20,good(),2500));
  assert(!c.keepAlive(9,3500));assert(c.state==State::Fault);
  assert(c.start(10,Kind::Move,100,20,good(),3600));
  for(unsigned t=3800;t<=6200;t+=200){c.keepAlive(10,t);c.step(good(),t);}
  assert(c.state==State::Fault); // stationary despite renewals
  assert(c.start(11,Kind::Move,500,20,good(),6300));
  for(unsigned i=1;i<=100;i++){unsigned t=6300+i*200;c.keepAlive(11,t);c.step(good(i),t);}
  assert(c.state==State::Fault); // total duration cap despite progress
  assert(c.start(12,Kind::Move,100,20,good(),27000));
  c.step(good(1,31),27100);assert(c.state==State::Fault);
  assert(c.start(13,Kind::Move,100,20,good(),27200));
  c.step(good(-11),27300);assert(c.state==State::Fault);
  assert(c.start(14,Kind::Turn,90,20,good(),27400));
  c.step(good(51),27500);assert(c.state==State::Fault);
  assert(c.start(15,Kind::Move,50,20,good(),27600));
  c.cancel("stop");o=c.step(good(),27700);assert(c.state==State::Cancelled&&!o.left&&!o.right);
  // millis() wraps after ~49 days; relative deadlines must still work.
  assert(c.start(16,Kind::Move,50,20,good(),UINT32_MAX-300));
  assert(c.keepAlive(16,100));o=c.step(good(10),200);assert(c.running()&&o.left>0);
  c.step(good(10),1100);assert(c.state==State::Fault);
  puts("Motion controller tests passed");
}
