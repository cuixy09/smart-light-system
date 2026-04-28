#include <Arduino.h>
#define LED_PIN 7
#define PHOTO_PIN A0
#define IR_PIN A5

int computedBrightness(int aqi)
{
  int sr = 360;
  int ss = 1080;

  int cur_min = (millis() / 1000) / 60;

  bool is_night = (cur_min >= ss || cur_min < sr);
  if (!is_night)
    return 0;

  int elapsed;
  if (cur_min >= ss)
    elapsed = cur_min - ss;
  else
    elapsed = (24 * 60 - ss) + cur_min;

  int night_time = (24 * 60 - ss) + sr;
  double lambda = elapsed * 1.0 / night_time;
  double base = 255.0 * (1.0 - 2.0 * abs(lambda - 0.5));

  double aqi_fac = 1.0 + (aqi - 50) / 200.0;
  if (aqi_fac < 0.5)
    aqi_fac = 0.5;
  if (aqi_fac > 2.0)
    aqi_fac = 2.0;

  int pwm = (int)(base * aqi_fac);
  if (pwm > 255)
    pwm = 255;
  if (pwm < 0)
    pwm = 0;
  return pwm;
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);
  Serial.begin(9600);
}

int aqi = 50;

void loop()
{
  int irValue = digitalRead(IR_PIN);
  int brightness;

  int photoValue = analogRead(PHOTO_PIN);
  int photoBrightness = map(photoValue, 0, 1023, 255, 0);

  if (irValue == LOW)
  {
    brightness = photoBrightness;
  }
  else
  {
    brightness = 0;
  }

  analogWrite(LED_PIN, brightness);
  Serial.print("IR:");
  Serial.print(irValue);
  Serial.print(" brightness:");
  Serial.println(brightness);
  Serial.println(photoBrightness);
  delay(1000);
}