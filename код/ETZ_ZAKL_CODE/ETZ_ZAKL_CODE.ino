#include <GyverMenu.h>
// #define GS_NO_ACCEL
#define GS_FAST_PROFILE 10
#include <ServoSmooth.h>
#include <GyverStepper2.h>
#include <GyverPlanner2.h>
#include <GyverOLED.h>
#include <EncButton.h>
#include <GyverOS.h>

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
GyverMenu menu(20, 8);
GyverMenu smenu(20, 8);
GyverMenu tmenu(20, 8);

double start_x = 0;

struct Point {
  double x;
  double y;
  int s = speed;
  Point(double nx, double ny, int ns = speed)
    : x(nx), y(ny), s(ns) {}
};

int xp, yp;

int vali = 165;

void print(int t){
  oled.clear();
  oled.home();
  oled.print(t);
  oled.update();
}

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
  z.setTargetDeg(vali);
  while (abs(z.getCurrentDeg() - vali) > 1) {
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

double px = 0, py = 0;

bool add(double x, double y, int s = speed) {
  planner.brake();
  planner.setMaxSpeed(s);
  planner.start();
  if (xp == x && yp == y) {
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
  print(0);
  while (planner.getStatus() == 1) {
    handler();
  }
  print(1);
  while (planner.getStatus() == 2) {
    handler();
  }
  print(2);
}

void move(double x, double y, int s = speed) {
  if (!add(x, y, s)) return;
  wait();
}

void setup() {
  pinMode(A0, INPUT);
  stepper_l.reverse(true);
  planner.addStepper(0, stepper_r);
  planner.addStepper(2, stepper_l);
  planner.addStepper(1, stepper_y);
  //planner.setMaxSpeed(speed);
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
        print(line);
        if (enc.click()) {
          smenu.refresh();
          break;
        }
      }
      enc.reset();
    });
    b.Button("Калибровка линии", []() {
      calib();
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
    b.ValueInt<int>("ValueInt", &vali, 0, 180, 5, DEC, "*", [](int vo) { down(); });
  });

  tmenu.onPrint([](const char* str, size_t len) {
    if (str) oled.Print::write(str, len);
    else oled.update();
  });
  tmenu.onCursor([](uint8_t row, bool chosen, bool active) -> uint8_t {
    oled.setCursor(0, row);
    oled.invertText(chosen);
    return 0;
  });

  tmenu.onBuild([](gm::Builder& b) {
    b.Button("Проезд на 10 см вперед x", []() {
      px += 10;
      move(px, py);
    });
    b.Button("Проезд на 5 см вперед y", []() {
      py += 5;
      move(px, py, 25000);
    });
    b.Button("Проезд на 10 см назад x", []() {
      px -= 10;
      move(px, py);
    });
    b.Button("Проезд на 5 см назад y", []() {
      py -= 5;
      move(px, py, 25000);
    });
    b.Button("вернуть", []() {
      py = 0;
      px = 0;
      move(px, py, 10000);
    });
  });

  menu.setFastCursor(false);
  smenu.setFastCursor(false);
  tmenu.setFastCursor(false);
  menu.refresh();
  OS.attach(0, handler);
}

static int settings = 0;

void refresh() {
  if (settings == 0) menu.refresh();
  if (settings == 1) smenu.refresh();
  if (settings == 2) tmenu.refresh();
}

double cm_x(double n) {
  return 10 * (n * 3200.0 / (44.0 * M_PI));
}

void reset_x() {
  int32_t point[3];
  point[0] = 0;
  point[1] = planner.getCurrent(1);
  point[2] = 0;
  planner.setCurrent(point);
  xp = 0;
}

void set_speed_x(int s) {
  planner.setSpeed(0, s);
  planner.setSpeed(2, s);
}

double go_line_x(int s, double f = 1) {
  set_speed_x(s);
  while (line < 750) {
    handler();
  }
  long t = millis();
  if (f) {
    while (line > 600) {
      handler();
    }
  }
  stop();
  return millis() - t;
}

double cm_y(double n) {
  return n * 3200.0 / 8.0 * 10.0;
}

void reset_y() {
  int32_t point[3];
  point[0] = planner.getCurrent(0);
  point[1] = 0;
  point[2] = planner.getCurrent(2);
  planner.setCurrent(point);
  yp = 0;
}

void set_speed_y(int s) {
  planner.setSpeed(1, s);
}

double go_line_y(int s, bool f = 0) {
  set_speed_y(s);
  while (line < 750) {
    handler();
  }
  long t = millis();
  if (f) {
    while (line > 500) {
      handler();
    }
  }
  stop();
  return millis() - t;
}

void reset() {
  xp = 0;
  yp = 0;
  planner.reset();
}

void stop() {
  planner.brake();
}

void calib(){
  int sp = 5000;
  long t = millis();
  go_line_x(sp, 1);
  double time = (millis() - t) / 1000.0;
  start_x = -cm_x(sp * time) / 100000 + 6;
  print(start_x);
  go_line_y(-20000, 0);
  stop();
  reset();
  move(-6, 0.5);
  reset();
  move(start_x, 0);
}

void handler() {
  z.tick();
  enc.tick();
  stepper_r.tick();
  stepper_l.tick();
  stepper_y.tick();
  planner.tick();
  if (enc.right()) {
    if (settings == 0) menu.down();
    if (settings == 1) smenu.down();
    if (settings == 2) tmenu.down();
  }
  else if (enc.rightH()) {
    if (settings == 0) menu.right();
    if (settings == 1) smenu.right();
    if (settings == 2) tmenu.right();
  }
  else if (enc.left()) {
    if (settings == 0) menu.up();
    if (settings == 1) smenu.up();
    if (settings == 2) tmenu.up();
  } else if (enc.leftH()) {
    if (settings == 0) menu.left();
    if (settings == 1) smenu.left();
    if (settings == 2) tmenu.left();
  } else if (enc.hasClicks(1)) {
    if (settings == 0) menu.set();
    if (settings == 1) smenu.set();
    if (settings == 2) tmenu.set();
  }
   else if (enc.hasClicks(2)) {
    if (settings == 0) settings = 1;
    else if (settings == 1) settings = 0;
    else if (settings == 2) settings = 1;
    refresh();
  } else if (enc.hasClicks(3)) {
    if (settings == 0) settings = 2;
    else if (settings == 1) settings = 2;
    else if (settings == 2) settings = 0;
    refresh();
  }
}

void draw_line(double x0, double y0, double x, double y, bool f = 1) {
  if (!f) {
    down();
  } else {
    up();
    move(x0, y0);
    down();
  }
  move(x, y);
}

void draw_circle(double cx, double cy, double r) {
  double segments = 80.0;
  up();
  move(cx + r, cy);
  down();
  double step = 2 * M_PI / segments;
  for (int i = 0; i <= segments; i++) {
    add(cx + r * cos(i * step), cy + r * sin(i * step));
  }
  for (int i = 0; i < segments; i++) {
    wait();
  }
  up();
}

void draw_polygon(int cx, int cy, int r, int n) {
  up();
  move(cx + r, cy);
  down();
  double step = 2 * M_PI / n;
  for (int i = 0; i <= n; i++) {
    add(cx + r * cos(i * step), cy + r * sin(i * step));
  }
  for (int i = 0; i < n; i++) {
    wait();
  }
  up();
}

void _draw_path_internal(Point* pts, int size, bool zam) {
  up();
  move(pts[0].x, pts[0].y, pts[0].s);
  down();
  for (int i = 1; i < size; i++) {
    move(pts[i].x, pts[i].y, pts[i].s);
  }
  if (zam) {
    move(pts[0].x, pts[0].y, pts[0].s);
  }
  up();
}

void draw_point(double x, double y) {
  up();
  move(x, y);
  down();
  up();
}

void draw_rect(double x0, double y0, double x1, double y1, int s = speed) {
  draw_line(x0, y0, x1, y0);
  draw_line(x1, y0, x1, y1, 0);
  draw_line(x1, y1, x0, y1, 0);
  draw_line(x0, y1, x0, y0, 0);
  up();
}

void loop() {
  OS.tick();
}

void f_1() {
  Point lom1[] = { { 5.000, 3.500, 5000 }, { 15.000, 3.500, 5000 }, { 15.000, 13.500, 20000 }, { 5.000, 13.500, 5000 }, { 5.000, 3.500, 20000 } };
  draw_path(lom1, 0);
  draw_line(5.000, 3.500, 15.000, 13.500, 0);
  draw_line(5.000, 13.500, 15.000, 3.500);
  draw_circle(10.000, 8.500, 5.000);
  draw_circle(10.000, 8.500, 7.071);
}

void f_2() {
up();
move(15, 0);
Point lom1[] = { { 1.000, 1.000, 10000 }, { 1.000, 16.000, 26000 }, { 16.000, 16.000, 1500 } };
draw_path(lom1, 0);
move(start_x, 0, 20000);
}

void f_3() {
Point poly1[] = { { 10.000, 11.500, 5000}, { 12.600, 7.000, 5000}, { 7.400, 7.000, 5000}, { 10.000, 11.500, 5000} };
draw_path(poly1, 0);
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