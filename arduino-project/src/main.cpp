#include <Arduino.h>
#define LED_PIN 9;
#define SS_PIN 2;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SS_PIN, INPUT);
  Serial.begin(9600);
  Serial.setTimeout(50);
}

int num = 0;
int prev = 0;

void loop() {
  if (Serial.available()) {
    int tmp = Serial.parseInt();
	if (tmp != 0 || lastValidNum == 0) {
        num = tmp;
        prev = num;
    }
  }
  int PWM = num;
  int val = digitalRead(SS_PIN);
  if (val == LOW) {
    PWM += 60;
    if (PWM > 255) PWM = 255;
  }
  analogWrite(PWM_PIN, PWM);
  delay(50);
}