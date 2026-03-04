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
#define speed 575
#define work_led 47
#define line analogRead(A0)
ServoSmooth z;
GStepper2< STEPPER2WIRE> stepper_y(400, 46, 45, 44);
GStepper2< STEPPER2WIRE> stepper_r(3200, 42, 43, 41);
GStepper2< STEPPER2WIRE> stepper_l(3200, 40, 39, 38);
GPlanner2< STEPPER2WIRE, 3, 100> planner;
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
EncButton enc(18, 19, 2, INPUT_PULLUP);
GyverMenu menu(20, 8);
GyverMenu smenu(20, 8);
GyverMenu tmenu(20, 8);

double start_x = 0;
const double STEPS_X = 231.48;
const double STEPS_Y = 500.0;

struct Point {
  double x;
  double y;
  int s = speed;
  Point(double nx, double ny, int ns = speed)
    : x(nx), y(ny), s(ns) {}
};

int xp, yp;

int vali = 180;

void print(double t) {
  oled.clear();
  oled.home();
  oled.print(t);
  oled.update();
}

void up() {
  z.setTargetDeg(40);
  while (abs(z.getCurrentDeg() - 40) > 1) {
    handler();
  }
}

void down() {
  z.setTargetDeg(vali);
  while (abs(z.getCurrentDeg() - vali) > 2) {
    handler();
  }
}

void release() {
  z.setTargetDeg(0);
  while (abs(z.getCurrentDeg() - 0) > 1) {
    handler();
  }
}

double px = 0, py = 0;

bool add(double x, double y, int s = speed, bool t = 1) {
  if (!t && x == 0 && y == 0) return false;

  if (t && xp == x && yp == y) return false;

  planner.brake();
  planner.setMaxSpeed(speed * 5);
  planner.setAcceleration(500 * 5 * 1.5);
  planner.start();

  xp = t ? x : xp + x;
  yp = t ? y : yp + y;

  int32_t point[3];
  point[0] = (int32_t)cm_x(x);
  point[1] = (int32_t)cm_y(y);
  point[2] = (int32_t)cm_x(x);

  planner.addTarget(point, 3, t ? ABSOLUTE : RELATIVE);
  return true;
}


void wait() {
  for (int i = 0; i < 1; i++) {
    while (planner.getStatus() == 1) {
      handler();
    }
    while (planner.getStatus() == 2) {
      handler();
    }
  }
}

void move(double x, double y, int s = speed, bool type = 1) {
  if (!add(x, y, s, type)) return;
  wait();
}

int bx = 0, by = 0;

void move_b(double x, double y) {
  bool nx = 0, ny = 0;
  if (x < 0) {
    nx = 1;
    stepper_l.reverse(1);
    stepper_r.reverse(1);
  }
  if (y < 0) {
    ny = 1;
    stepper_y.reverse(1);
  }
  int aX = abs(cm_x(x)), aY = abs(cm_y(y));
  int errx = 0, erry = 0;

  int maxT = max(aX, aY);

  for (int i = 1; i < maxT; i++) {
    errx += aX;
    erry += aY;
    if (errx >= maxT) {
      stepper_l.step();
      stepper_r.step();
      errx -= maxT;
    }
    if (erry >= maxT) {
      stepper_y.step();
      erry -= maxT;
    }
  }
}

void draw_line_b(int x0, int y0, int x, int y) {
  if (bx == x0 && by == y0) {
    down();
  } else {
    up();
    move_b(x0, y0);
    down();
  }
  move_b(x, y);
}

int smes = 0;

void disable() {
  stepper_r.disable();
  stepper_l.disable();
  stepper_y.disable();
}

void enable() {
  stepper_r.enable();
  stepper_l.enable();
  stepper_y.enable();
}

void setup() {
  pinMode(A0, INPUT);
  stepper_l.reverse(true);
  planner.addStepper(0, stepper_r);
  planner.addStepper(2, stepper_l);
  planner.addStepper(1, stepper_y);
  int start[3] = { 0, 0, 0 };
  planner.addTarget(start, 3);
  planner.setAcceleration(0);
  planner.start();
  oled.init();
  Serial.begin(9);
  z.attach(11, 500, 2400);
  z.setSpeed(1000);
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
      enable();
      digitalWrite(work_led, 1);
      f_1();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 2", []() {
      enable();
      digitalWrite(work_led, 1);
      f_2();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 3", []() {
      enable();
      digitalWrite(work_led, 1);
      f_3();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 4", []() {
      enable();
      digitalWrite(work_led, 1);
      f_4();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 5", []() {
      enable();
      digitalWrite(work_led, 1);
      f_5();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 6", []() {
      enable();
      digitalWrite(work_led, 1);
      f_6();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 7", []() {
      enable();
      digitalWrite(work_led, 1);
      f_7();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 8", []() {
      enable();
      digitalWrite(work_led, 1);
      f_8();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 9", []() {
      enable();
      digitalWrite(work_led, 1);
      f_9();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
    });
    b.Button("Задание 10", []() {
      enable();
      digitalWrite(work_led, 1);
      f_10();
      up();
      move(start_x, 0);
      digitalWrite(work_led, 0);
      disable();
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
      enable();
      calib();
      disable();
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
    b.ValueInt<int>("ValueInt", &vali, 0, 180, 5, DEC, "*", [](int vo) {
      down();
    });
    b.Button("Смещение + 1 мм по x", []() {
      enable();
      move(0.1, 0);
      reset();
      disable();
    });
    b.Button("Смещение - 1 мм по x", []() {
      enable();
      move(-0.1, 0);
      reset();
      disable();
    });
    b.Button("Смещение + 1 мм по y", []() {
      enable();
      move(0, 0.1);
      reset();
      disable();
    });
    b.Button("Смещение - 1 мм по y", []() {
      enable();
      move(0, -0.1);
      reset();
      disable();
    });
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
      enable();
      move(10, 0, speed, 0);
      disable();
    });
    b.Button("Проезд на 5 см вперед y", []() {
      enable();
      move(0, 5, speed, 0);
      disable();
    });
    b.Button("Проезд на 10 см назад x", []() {
      enable();
      move(-10, 0, speed, 0);
      disable();
    });
    b.Button("Проезд на 5 см назад y", []() {
      enable();
      move(0, -5, speed, 0);
      disable();
    });
    b.Button("вернуть", []() {
      enable();
      move(px, py, 1000);
      disable();
    });
    b.Button("на старт", []() {
      enable();
      move(start_x, 0, 1000);
      disable();
    });
  });

  menu.setFastCursor(false);
  smenu.setFastCursor(false);
  tmenu.setFastCursor(false);
  menu.refresh();
  OS.attach(0, handler);
  disable();
}

static int settings = 0;

void refresh() {
  if (settings == 0) menu.refresh();
  if (settings == 1) smenu.refresh();
  if (settings == 2) tmenu.refresh();
}

void set_current(int x = get_cm_x(), int y = get_cm_y()) {
  int32_t point[3];
  point[0] = (int32_t)cm_x(x);
  point[1] = (int32_t)cm_y(y);
  point[2] = (int32_t)cm_x(x);
  planner.setCurrent(point);
}

double cm_x(double n) {
  return n * STEPS_X;
}

double step_x(double n) {
  return n / STEPS_X;
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
  stepper_l.setSpeed(s);
  stepper_r.setSpeed(s);
}

void stop_x() {
  stepper_l.brake();
  stepper_r.brake();
}

int get_step_x() {
  return stepper_l.pos;
}

double get_cm_x() {
  return step_x(get_step_x());
}

double go_line_x(int s, double f = 1) {
  set_speed_x(s);
  while (line < 800) {
    handler();
  }
  long t = millis();
  if (f) {
    while (line > 700) {
      handler();
    }
  }
  stop_x();
  set_current();
  return millis() - t;
}

double cm_y(double n) {
  return n * STEPS_Y;
}

double step_y(double n) {
  return n / STEPS_Y;
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
  stepper_y.setSpeed(s);
}

void stop_y() {
  stepper_y.brake();
}

int get_step_y() {
  return stepper_y.pos;
}

double get_cm_y() {
  return step_y(get_step_y());
}

double go_line_y(int s, bool f = 0) {
  set_speed_y(s);
  while (line < 800) {
    handler();
  }
  long t = millis();
  if (f) {
    while (line > 500) {
      handler();
    }
  }
  stop_y();
  set_current();
  return millis() - t;
}

void reset() {
  xp = 0;
  yp = 0;
  planner.reset();
  stepper_r.pos = 0;
  stepper_l.pos = 0;
  stepper_y.pos = 0;
}


void stop() {
  stop_y();
  stop_x();
}

void calib() {
  reset();
  go_line_x(4000);
  start_x = -get_cm_x() - 4;
  print(start_x);
  go_line_y(-speed);
  stop();
  reset();
  move(4, -1);
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
  } else if (enc.left()) {
    if (settings == 0) menu.up();
    if (settings == 1) smenu.up();
    if (settings == 2) tmenu.up();
  } else if (enc.hasClicks(1)) {
    if (settings == 0) menu.set();
    if (settings == 1) smenu.set();
    if (settings == 2) tmenu.set();
  } else if (enc.hasClicks(2)) {
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

void draw_line(double x0, double y0, double x, double y, bool f = 1, int s = speed, bool type = 1) {
  if (!f) {
    down();
  } else {
    up();
    move(x0, y0, s, type);
    down();
  }
  move(x, y, s, type);
}

void draw_circle(double cx, double cy, double r, int s = speed) {
  double segments = 80.0;
  up();
  move(cx + r, cy);
  down();
  double step = 2 * M_PI / segments;
  for (int i = 1; i <= segments + 1; i++) {
    add(cx + r * cos(i * step), cy + r * sin(i * step), speed);
  }
  for (int i = 1; i <= segments + 1; i++) {
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

void grid_search(double x0, double y0, double x = 23.0, double y = 16.0, int angle = 0) {
  x0 -= 4;
  x -= 4;
  move(x0, y0, speed, 1);

  const double step_y = 0.5;
  double current_y = y0;
  bool found = false;
  bool move_right = true;

  while (current_y <= y0 + y && !found) {
    if (move_right) {
      set_speed_x(speed);
      while (get_cm_x() < x0 + x && !found) {
        handler();
        if (line > 800) {
          found = true;
        }
      }
    } else {
      set_speed_x(-speed);
      while (get_cm_x() > x0 && !found) {
        handler();
        if (line > 800) {
          found = true;
        }
      }
    }

    if (found) {
      if (!move_right) {
        while (line > 700) {
          handler();
        }
        stop_x();
      } else {
        stop_x();
      }
      return;
    }

    xp = get_cm_x();
    yp = current_y;
    set_current();


    current_y += step_y;
    if (current_y > y0 + y) break;

    move(0, 1, speed, 0);
    if (found) break;

    xp = get_cm_x();
    yp = get_cm_y();
    int32_t curr_y[3] = { stepper_l.getCurrent(), stepper_y.getCurrent(), stepper_l.getCurrent() };
    planner.setCurrent(curr_y);

    move_right = !move_right;
  }
}

void loop() {
  OS.tick();
}

void f_1() {
  draw_circle(17.000, 9.000, 7.071);
}

void f_2() {
  grid_search(2, 2, 15, 15);
  move(-1.5, 0, speed, 0);
  double y0 = get_cm_y();
  go_line_x(speed, 0);
  double x0 = get_cm_x();
  go_line_x(speed, 1);
  double d = get_cm_x() - x0;
  y0 = y0 + d / 2 - 1.5;
  x0 += d / 2 + 4 - 0.5;

  oled.clear();
  oled.home();
  oled.print(x0);
  oled.setCursor(0, 1);
  oled.print(y0);
  oled.setCursor(0, 2);
  oled.print(d);
  oled.update();
  draw_circle(x0, y0 + 0.2, d / 2 * sqrt(2));
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