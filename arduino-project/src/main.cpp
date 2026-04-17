#include <Arduino.h>
#define LED_PIN 7

// 后续接入计算函数
int computedBrightness(int);

void setup()
{
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  // 呼吸灯效果：使用正弦函数实现平滑亮度变化
  float brightness = (sin(millis() / 500.0) + 1) * 127.5;
  analogWrite(LED_PIN, (int)brightness);
  delay(30);
}