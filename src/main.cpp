#include <Arduino.h>
#include <Wire.h>
#include <MLX.h>

MLX mlx(&Wire);

void setup()
{
  Serial.begin(1500000);
  Serial2.begin(115200);

  mlx.init();
}

void push_serial()
{
  static long last_push = 0;

  if (millis() - last_push < 1000)
    return;

  Serial2.printf("%f, %f\n", 1.0, -1.0);
  // Serial.printf("%f, %f\n", 1.0, -1.0);

  last_push = millis();
}

void loop()
{
  static long last_img = millis();

  push_serial();

  if (mlx.has_img())
  {
    // Serial.printf("Frame time %d; Read time %d; Therm[0] %d\n", millis() - last_img, mlx.read_img(), mlx.img[0]);
    mlx.print_img();
    last_img = millis();
  }
  delay(5);
}