#include <GyverMenu.h>
// #define GS_NO_ACCEL
#include <ServoSmooth.h>
#include <GyverStepper2.h>
#include <GyverPlanner2.h>
#include <GyverOLED.h>
#include <EncButton.h>
#include <GyverOS.h>

struct Point {
  double x;
  double y;
};

GyverOS<20> OS;
#define draw_path(pts, f) _draw_path_internal(pts, sizeof(pts) / sizeof(Point), f)
#define speed 1000
#define work_led 47
#define line analogRead(A0)
ServoSmooth z;
GStepper2< STEPPER2WIRE> stepper_y(200, 46, 45, 44);
GStepper2< STEPPER2WIRE> stepper_r(200, 42, 43, 41);
GStepper2< STEPPER2WIRE> stepper_l(200, 40, 39, 38);
GPlanner2< STEPPER2WIRE, 3, 100> planner;
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
EncButton enc(18, 19, 2, INPUT_PULLUP);
GyverMenu menu(30, 8);
GyverMenu smenu(30, 8);
int xp, yp;

void up() {
  planner.stop();
  z.setTargetDeg(100);
  while (abs(z.getCurrentDeg() - 100) > 1) {
    handler();
  }
  planner.start();
}

void down() {
  planner.stop();
  z.setTargetDeg(180);
  while (abs(z.getCurrentDeg() - 180) > 1) {
    handler();
  }
  planner.start();
}

void release() {
  planner.stop();
  z.setTargetDeg(0);
  while (abs(z.getCurrentDeg() - 0) > 1) {
    handler();
  }
  planner.start();
}

void setup() {
  pinMode(A0, INPUT);
  stepper_l.reverse(true);
  planner.addStepper(0, stepper_r);
  planner.addStepper(2, stepper_l);
  planner.addStepper(1, stepper_y);
  planner.setMaxSpeed(speed);
  int start[3] = { 0, 0, 0 };
  planner.addTarget(start, 3);
  planner.setAcceleration(0);
  planner.start();
  oled.init();
  Serial.begin(9600);
  z.attach(11, 500, 2400);
  z.setAccel(0);
  z.setSpeed(100);
  pinMode(work_led, OUTPUT);
  release();
  planner.setMaxSpeed(2500);
  menu.onPrint([](const char* str, size_t len) {
    if (str) oled.Print::write(str, len);
    else oled.update();
  });
  menu.onCursor([](uint8_t row, bool chosen, bool active) -> uint8_t {
    oled.setCursor(0, row);
    oled.invertText(chosen);
    return 0;
  });

  menu.onBuild([](gm::Builder& b) {
    b.Button("Задание 1", []() {
      digitalWrite(work_led, 1);
      f_1();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 2", []() {
      digitalWrite(work_led, 1);
      f_2();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 3", []() {
      digitalWrite(work_led, 1);
      f_3();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 4", []() {
      digitalWrite(work_led, 1);
      f_4();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 5", []() {
      digitalWrite(work_led, 1);
      f_5();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 6", []() {
      digitalWrite(work_led, 1);
      f_6();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 7", []() {
      digitalWrite(work_led, 1);
      f_7();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 8", []() {
      digitalWrite(work_led, 1);
      f_8();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 9", []() {
      digitalWrite(work_led, 1);
      f_9();
      digitalWrite(work_led, 0);
    });
    b.Button("Задание 10", []() {
      digitalWrite(work_led, 1);
      f_10();
      digitalWrite(work_led, 0);
    });
  });

  smenu.onPrint([](const char* str, size_t len) {
    if (str) oled.Print::write(str, len);
    else oled.update();
  });
  smenu.onCursor([](uint8_t row, bool chosen, bool active) -> uint8_t {
    oled.setCursor(0, row);
    oled.invertText(chosen);
    return 0;
  });

  smenu.onBuild([](gm::Builder& b) {
    b.Button("Показания датчика линии", []() {
      while (true) {
        enc.tick();
        if (enc.click()) {
          smenu.refresh();
          break;
        }
      }
      enc.reset();
    });
    b.Button("Калибровка линии", []() {
    });
    b.Button("Взять маркер", []() {
      up();
    });
    b.Button("Отпустить маркер", []() {
      release();
    });
    b.Button("Опустить маркер", []() {
      down();
    });
  });

  menu.setFastCursor(false);
  smenu.setFastCursor(false);
  menu.refresh();
  OS.attach(0, handler);
}

static bool settings = 0;

double cm_x(double n) {
  return 10 * n * (3200.0 / (44.0 * M_PI));
}

void reset_x(){
  int32_t point[3];
  point[0] = 0;
  point[1] = planner.getCurrent(1);
  point[2] = 0;
  planner.setCurrent(point);
  xp = 0;
}

void set_speed_x(int s){
  planner.setSpeed(0, s);
  planner.setSpeed(2, s);
}

void stop_x(){
  planner.setSpeed(0, 0);
  planner.setSpeed(2, 0);
}

double go_line_x(int s){
  set_speed_x(s);
  while (line > 500){
    handler();
  }
  long t = millis();
  while (line < 500){
    handler();
  }
  stop_x();
  return millis() - t;
}

double cm_y(double n) {
  return n * 3200.0 / 8.0 * 10.0;
}

void reset_y(){
  int32_t point[3];
  point[0] = planner.getCurrent(0);
  point[1] = 0;
  point[2] = planner.getCurrent(2);
  planner.setCurrent(point);
  yp = 0;
}

void set_speed_y(int s){
  planner.setSpeed(1, s);
}

void stop_y(){
  planner.setSpeed(1, 0);
}

double go_line_y(int s){
  set_speed_y(s);
  while (line > 500){
    handler();
  }
  long t = millis();
  while (line < 500){
    handler();
  }
  stop_y();
  return millis() - t;
}

void reset(){
  planner.reset();
}

void handler() {
  z.tick();
  enc.tick();
  stepper_r.tick();
  stepper_l.tick();
  stepper_y.tick();
  planner.tick();
  if (enc.right()) {
    if (settings) {
      smenu.down();
    } else {
      menu.down();
    }
  } else if (enc.left()) {
    if (settings) {
      smenu.up();
    } else {
      menu.up();
    }
  } else if (enc.hasClicks(1)) {
    if (settings) {
      smenu.set();
    } else {
      menu.set();
    }
  } else if (enc.hasClicks(2)) {
    settings = !settings;
    if (settings) {
      smenu.refresh();
    } else {
      menu.refresh();
    }
  }
}

bool add(double x, double y) {
  if (xp == x && yp == y){
    return false;
  }
  xp = x;
  yp = y;
  int32_t point[3];
  point[0] = (int32_t)cm_x(x);
  point[1] = (int32_t)cm_y(y);
  point[2] = (int32_t)cm_x(x);

  planner.addTarget(point, 3, ABSOLUTE);
  return true;
}

void wait() {
  while (planner.getStatus() == 1) {
    handler();
  }
  while (planner.getStatus() == 2) {
    handler();
  }
}

void move(double x, double y) {
  if (!add(x, y)) return;
  wait();
}

void draw_line(double x0, double y0, double x, double y, bool f = 0){
  if (!f){
    down();
  }
  else{
    up();
    move(x0, y0);
    down();
  }
  move(x, y);
}

void draw_circle(double cx, double cy, double r){
  double segments = 80.0;
  up();
  move(cx + r, cy);
  down();
  double step = 2 * M_PI / segments;
  for (int i = 0; i <= segments; i++){
    add(cx + r * cos(i * step), cy + r * sin(i * step));
  }
  for (int i = 0; i < segments; i++){
    wait();
  }
  up();
}

void draw_polygon(int cx, int cy, int r, int n){
  up();
  move(cx + r, cy);
  down();
  double step = 2 * M_PI / n;
  for (int i = 0; i <= n; i++){
    add(cx + r * cos(i * step), cy + r * sin(i * step));
  }
  for (int i = 0; i < n; i++){
    wait();
  }
  up();
}

void _draw_path_internal(Point* pts, int size, bool zam) {
  up();
  move(pts[0].x, pts[0].y);
  down();
  for (int i = 1; i < size; i++) {
    move(pts[i].x, pts[i].y);
  }
  if (zam){
    move(pts[0].x, pts[0].y);
  }
  up();
}

void loop() {
  OS.tick();
}

void f_1() {

}

void f_2() {
}

void f_3() {
}

void f_4() {
}

void f_5() {
}

void f_6() {
}

void f_7() {
}

void f_8() {
}

void f_9() {
}

void f_10() {
}