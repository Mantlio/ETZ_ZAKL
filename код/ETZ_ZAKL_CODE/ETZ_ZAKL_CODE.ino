#include <GyverMenu.h>
#define GS_NO_ACCEL
#include <ServoSmooth.h>
#include <GyverStepper2.h>
#include <GyverOLED.h>
#include <EncButton.h>
#include <GyverOS.h>

GyverOS<20> OS;
#define speed 1000
#define work_led 47
#define line analogRead(A0)
ServoSmooth z;
GStepper2< STEPPER2WIRE> stepper_y(200, 46, 45, 44);
GStepper2< STEPPER2WIRE> stepper_r(200, 42, 43, 41);
GStepper2< STEPPER2WIRE> stepper_l(200, 40, 39, 38);
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
EncButton enc(18, 19, 2, INPUT_PULLUP);
GyverMenu menu(30, 8);
GyverMenu smenu(30, 8);

int start_x = 0;

void print(String s) {
  oled.clear();
  oled.home();
  oled.print(s);
  oled.update();
}

void up() {
  z.setTargetDeg(100);
  while (abs(z.getCurrentDeg() - 100) > 1) {
    handler();
  }
}

void down() {
  z.setTargetDeg(180);
  while (abs(z.getCurrentDeg() - 180) > 1) {
    handler();
  }
  delay(100);
}

void release() {
  z.setTargetDeg(0);
  while (abs(z.getCurrentDeg() - 0) > 1) {
    handler();
  }
}

void setup() {

  pinMode(A0, INPUT);
  stepper_r.setMaxSpeed(speed);
  stepper_l.setMaxSpeed(speed);
  stepper_y.setMaxSpeed(1000);
  oled.init();
  Serial.begin(9600);
  stepper_l.reverse(true);
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
    });
    b.Button("Задание 2", []() {
      digitalWrite(work_led, 1);
      f_2();
    });
    b.Button("Задание 3", []() {
      digitalWrite(work_led, 1);
      f_3();
    });
    b.Button("Задание 4", []() {
      digitalWrite(work_led, 1);
      f_4();
    });
    b.Button("Задание 5", []() {
      digitalWrite(work_led, 1);
      f_5();
    });
    b.Button("Задание 6", []() {
      digitalWrite(work_led, 1);
      f_6();
    });
    b.Button("Задание 7", []() {
      digitalWrite(work_led, 1);
      f_7();
    });
    b.Button("Задание 8", []() {
      digitalWrite(work_led, 1);
      f_8();
    });
    b.Button("Задание 9", []() {
      digitalWrite(work_led, 1);
      f_9();
    });
    b.Button("Задание 10", []() {
      digitalWrite(work_led, 1);
      f_10();
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
        print(String(analogRead(A0)));
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
  });

  menu.setFastCursor(false);
  smenu.setFastCursor(false);
  menu.refresh();
  OS.attach(0, handler);
  OS.attach(1, f_1);
  OS.stop(1);
  OS.attach(2, f_2);
  OS.stop(2);
  OS.attach(3, f_3);
  OS.stop(3);
  OS.attach(4, f_4);
  OS.stop(4);
  OS.attach(5, f_5);
  OS.stop(5);
  OS.attach(6, f_6);
  OS.stop(6);
  OS.attach(7, f_7);
  OS.stop(7);
  OS.attach(8, f_8);
  OS.stop(8);
  OS.attach(9, f_9);
  OS.stop(9);
  OS.attach(10, f_10);
  OS.stop(10);
}

static bool settings = 0;

double cm_x(double n) {
  return 10 * n * (800.0 / (11.0 * M_PI));
}

void go_cm_x(double mm) {
  stepper_l.setTarget(cm_x(mm));
  stepper_r.setTarget(cm_x(mm));
}

bool ready_x() {
  return stepper_l.getStatus() == 0 && stepper_r.getStatus() == 0;
}

void set_speed_x(int s) {
  stepper_l.setMaxSpeed(s);
  stepper_r.setMaxSpeed(s);
}

void reset_pos_x() {
  stepper_l.reset();
  stepper_r.reset();
}

int go_to_line_x(bool f = 1, bool f0 = 1) {
  if (f) {
    set_speed_x(speed + 2000);
  } else {
    set_speed_x(-speed - 2000);
  }
  while (line > 500) handler();
  long t = millis();
  if (f0) {
    while (line > 500) handler();
  }
  if (!f0) {
    return 0;
  } else {
    return millis() - t;
  }
  stop();
}

void set_speed_y(int s) {
  stepper_y.setMaxSpeed(s);
}

double cm_y(double n) {
  return n * 500;
}

void go_cm_y(double mm) {
  stepper_y.setTarget(cm_y(mm));
}

bool ready_y() {
  return stepper_y.getStatus() == 0;
}

void reset_pos_y() {
  stepper_y.reset();
}

int go_to_line_y(bool f = 0, bool f0 = 0) {
  if (f) {
    set_speed_y(speed + 2000);
  } else {
    set_speed_y(-speed - 2000);
  }

  while (line > 500) handler();
  long t = millis();
  if (f0) {
    while (line > 500) handler();
  }
  if (f0) {
    return millis() - t;
  } else {
    return 0;
  }
  stop();
}

bool ready() {
  return ready_x() && ready_y();
}

void reset() {
  reset_pos_y();
  reset_pos_x();
}

void handler() {
  z.tick();
  enc.tick();
  stepper_r.tick();
  stepper_l.tick();
  stepper_y.tick();
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

void loop() {
  OS.tick();
}

void stop() {
  stepper_l.brake();
  stepper_r.brake();
}

void calib() {
  go_to_line_x();
  start_x = -cm_x(stepper_r.getCurrent());
  reset_pos_x();
  go_cm_x(1);
  go_to_line_y();
  go_cm_x(start_x);
}

void move(double x, double y) {
  go_cm_x(x);
  go_cm_y(y);
  while (!ready()) {
    handler();
  }
}

void draw_point(int x, int y) {
  move(x, y);
  down();
  up();
}

bool draw_line(double x0, double y0, double x, double y, bool f = 1) {
  if (f) up();

  move(x0, y0);

  down();

  double xd = x - x0;
  double yd = y - y0;

  if (xd != 0) {
    double ratio = (double)abs(yd) / abs(xd);
    double stepRatio = cm_y(1.0) / cm_x(1.0);
    long calculatedSpeed = (long)(speed * ratio * stepRatio);

    stepper_y.setMaxSpeed(constrain(calculatedSpeed, 100, 2500));
  } else {
    stepper_y.setMaxSpeed(speed);
  }

  move(x, y);

  if (f) up();

  stepper_y.setMaxSpeed(speed);
  return true;
}

void draw_circle(double x0, double y0, double r) {
  int segments = 80;
  double angleStep = 2.0 * M_PI / segments;
  double x = x0 + r * cos(0);
  double y = y0 + r * sin(0);
  double x1, y1;
  move(x, y);
  for (int i = 0; i < segments; i++) {
    x = x0 + r * cos(i * angleStep);
    y = y0 + r * sin(i * angleStep);
    x1 = x0 + r * cos((i + 1) * angleStep);
    y1 = y0 + r * sin((i + 1) * angleStep);
    draw_line(x, y, x1, y1, 0);
  }
  up();
}

void draw_polygon(double x0, double y0, double r, double n) {
  double angleStep = 2.0 * M_PI / n;
  double x = x0 + r * cos(0);
  double y = y0 + r * sin(0);
  double x1, y1;
  move(x, y);
  while (!ready()) handler();
  for (int i = 0; i < n; i++) {
    x = x0 + r * cos(i * angleStep);
    y = y0 + r * sin(i * angleStep);
    x1 = x0 + r * cos((i + 1) * angleStep);
    y1 = y0 + r * sin((i + 1) * angleStep);
    draw_line(x, y, x1, y1, 0);
  }
  up();
}

void draw_quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
  draw_line(x0, y0, x1, y1);
  draw_line(x1, y1, x2, y2, 0);
  draw_line(x2, y2, x3, y3, 0);
  draw_line(x3, y3, x0, y0, 0);
  up();
}

void draw_rectangle(int x0, int y0, int x1, int y1) {
  draw_line(x0, y0, x1, y0, 0);
  draw_line(x1, y0, x1, y1, 0);
  draw_line(x1, y1, x0, y1, 0);
  draw_line(x0, y1, x0, y0, 0);
  up();
}

void f_1() {
  draw_rectangle(0, 0, 10, 10);
  draw_line(0, 0, 10, 10, 0);
  draw_line(10, 0, 0, 10);
  draw_circle(5, 5, 5);
  draw_circle(5, 5, 5 * sqrt(2));
}

void f_2() {
  for (double i = -4; i < 4.1; i += 0.1) {
    double y0 = 0.05 * i * i;
    double y1 = 0.05 * pow((i + 0.1), 2);
    draw_line(i + 4, y0, i + 4.1, y1, 0);
  }
  up();
}

void f_3() {
  for (double i = -4; i < 4.1; i += 0.1) {
    double y0 = 0.000625 * pow(i, 3);
    double y1 = 0.000625 * pow((i + 0.1), 3);
    draw_line(i + 4, y0, i + 4.1, y1, 0);
  }
  up();
}

void f_4() {
  draw_rectangle(3, 5, 8, 1);
}

void f_5() {
  draw_polygon(5, 5, 5, 6);
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
