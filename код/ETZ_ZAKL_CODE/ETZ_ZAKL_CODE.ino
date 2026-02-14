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

void print(String s){
  oled.clear();
  oled.home();
  oled.print(s);
  oled.update();
}

void up() {
  z.setTargetDeg(40);
  while (abs(z.getCurrentDeg() - 40) > 1) {
    handler(); // Обязательно! Чтобы работал z.tick()
  }
}

void down() {
  z.setTargetDeg(180);
  while (abs(z.getCurrentDeg() - 180) > 1) {
    handler(); 
  }
  delay(100); // Небольшая пауза для фиксации маркера
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
      OS.start(1);
    });
    b.Button("Задание 2", []() {
      digitalWrite(work_led, 1);
      OS.start(2);
    });
    b.Button("Задание 3", []() {
      digitalWrite(work_led, 1);
      OS.start(3);
    });
    b.Button("Задание 4", []() {
      digitalWrite(work_led, 1);
      OS.start(4);
    });
    b.Button("Задание 5", []() {
      digitalWrite(work_led, 1);
      OS.start(5);
    });
    b.Button("Задание 6", []() {
      digitalWrite(work_led, 1);
      OS.start(6);
    });
    b.Button("Задание 7", []() {
      digitalWrite(work_led, 1);
      OS.start(7);
    });
    b.Button("Задание 8", []() {
      digitalWrite(work_led, 1);
      OS.start(8);
    });
    b.Button("Задание 9", []() {
      digitalWrite(work_led, 1);
      OS.start(9);
    });
    b.Button("Задание 10", []() {
      digitalWrite(work_led, 1);
      OS.start(10);
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
      while (true){
        enc.tick();
        print(String(analogRead(A0)));
        if (enc.click()){
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

double cm_x(double n){
  return 10 * n * (800.0 / (11.0 * M_PI));
}

void go_cm_x(int mm){
  stepper_l.setTarget(cm_x(mm));
  stepper_r.setTarget(cm_x(mm));
}

bool ready_x(){
  return stepper_l.getStatus() == 0 && stepper_r.getStatus() == 0;
}

double cm_y(double n){
  return n * 500;
}

void go_cm_y(int mm){
  stepper_y.setTarget(cm_y(mm));
}

bool ready_y(){
  return stepper_y.getStatus() == 0;
}

bool ready(){
  return ready_x() && ready_y();
}

void handler() {
  z.tick();
  enc.tick();
  stepper_r.tick();
  stepper_l.tick();
  stepper_y.tick();
  if (enc.right()) {
    if (settings){
      smenu.down();
    }
    else{
      menu.down();
    }
  } else if (enc.left()) {
    if (settings){
      smenu.up();
    }
    else{
      menu.up();
    }
  } else if (enc.hasClicks(1)) {
    if (settings){
      smenu.set();
    }
    else{
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

void stop(){
  stepper_l.brake();
  stepper_r.brake();
}

void reset_pos(){
  stepper_l.reset();
  stepper_r.reset();
}

void calib(){
  stepper_l.setSpeed(5000);
  stepper_r.setSpeed(5000);
  while (line > 500) {
    stepper_l.tick();
    stepper_r.tick();
  }
  while (line < 500) {
    stepper_l.tick();
    stepper_r.tick();
  }
  reset_pos();
  stop();

}

int calc_speed(int x, int y, int speed_x = 2000){
  return speed_x * y / x;
}

int c = 0;

bool draw_line(int x0, int y0, int x, int y, bool f = 1){
  // 1. Подьем (если нужно) и переход в начало
  if (f) up();
  
  go_cm_x(x0);
  go_cm_y(y0);
  while (!ready()) handler();

  // 2. Опускаем и ЖДЕМ
  down();

  // 3. Расчет пропорциональной скорости
  int xd = x - x0;
  int yd = y - y0;
  
  if (xd != 0) {
    double ratio = (double)abs(yd) / abs(xd);
    // Коэффициент пересчета шагов Y к X (500 к ~23.1)
    double stepRatio = 500.0 / (800.0 / (1.1 * M_PI)); 
    long calculatedSpeed = (long)(speed * ratio * stepRatio);
    
    stepper_y.setMaxSpeed(constrain(calculatedSpeed, 100, 2500));
  } else {
    stepper_y.setMaxSpeed(speed); // Для вертикальных линий
  }

  // 4. Движение в конечную точку
  go_cm_x(x);
  go_cm_y(y);
  
  while (!ready()) handler();

  // 5. Подьем (если нужно)
  if (f) up();
  
  // Возвращаем стандартную скорость Y для следующих маневров
  stepper_y.setMaxSpeed(1000); 
  return true;
}


void f_1(){
  draw_line(0, 0, 10, 0, 0);
  draw_line(10, 0, 10, 10, 0);
  draw_line(10, 10, 0, 10, 0);
  draw_line(0, 10, 0, 0, 0);
  draw_line(0, 0, 10, 10, 0);
  draw_line(0, 10, 10, 0);
  digitalWrite(work_led, 0);
  OS.stop(1);
}

void f_2(){
  draw_line(0, 0, 10, 10);
  OS.stop(2);
}

void f_3(){
  
}

void f_4(){
  
}

void f_5(){
  
}

void f_6(){
  
}

void f_7(){
  
}

void f_8(){
  
}

void f_9(){
  
}

void f_10(){
  
}
