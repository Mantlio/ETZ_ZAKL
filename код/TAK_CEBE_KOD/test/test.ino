#include <GyverStepper2.h>

GStepper2< STEPPER2WIRE> stepper_y(200, 46, 45, 44);
GStepper2< STEPPER2WIRE> stepper_r(200, 42, 43, 41);
GStepper2< STEPPER2WIRE> stepper_l(200, 40, 39, 38);

void draw_line(double x0, double y0, double x, double y){
  bool dirX = (x - x0) > 0, diry = (y - y0) > 0;
  double aX = abs(x - x0), aY = abs(y - y0), errX = 0, errY = 0;

  double maxT = max(aX, aY);

  for (int i = 1; i <= maxT; i++){
    errX += aX;
    errY += aY
  }
}

void setup() {
  stepper_l.reverse(true);
}

void loop() {

}
