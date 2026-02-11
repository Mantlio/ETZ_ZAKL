#include <ServoSmooth.h>
#define GS_NO_ACCEL
#include <GyverStepper2.h>
#include <GyverOLED.h>
#include <EncButton.h>

ServoSmooth z;
GStepper2< STEPPER2WIRE> stepper_y(200, 46, 45, 44);
GStepper2< STEPPER2WIRE> stepper_r(200, 42, 43, 41);
GStepper2< STEPPER2WIRE> stepper_l(200, 40, 39, 38);
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
EncButton enc(18, 19, 2, INPUT_PULLUP);

#define work_led 47

void up(){
  z.setTargetDeg(20);
}

void down(){
  z.setTargetDeg(180);
}

void release(){
  z.setTargetDeg(0);
}

volatile long c = 0;

void setup() {
  oled.init();
  Serial.begin(9600);
  stepper_l.reverse(true);
  z.attach(11, 500, 2400);
  pinMode(work_led, OUTPUT);
  digitalWrite(work_led, 1);
  release();
  stepper_r.setSpeed(500);
  stepper_l.setSpeed(500);
  oled.clear();
  oled.home();
  oled.print("hello");
  oled.update();
}


void loop() {
  enc.tick();
  stepper_r.tick();
  stepper_l.tick();
  if (enc.right()){
    Serial.println("right");
  }
  if (enc.left()){
    Serial.println("left");
  }
  if (enc.click()){
    Serial.println("click");
  }
}
