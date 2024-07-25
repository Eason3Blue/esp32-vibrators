#include <Arduino.h>
#include <ESP32Servo.h>
#include <SSD1306Wire.h>

const int sda = 26, scl = 27;
SSD1306Wire display(0x3c,sda,scl);
Servo s;
int count = 1;

String IntToString(int x){
  String y, z;
  while (x)
  {
    y += (char)(x % 10 + '0');
    x /= 10;
  }
  for (int i = y.length() - 1; i >= 0; i--)
  {
    z += y[i];
  }
  return z;
}

void My_Println(int count, int d, int mode){
  Serial.print(
      "The " + IntToString(count) + (mode == 1 ? " Start " : " Relax "));
  if (mode == 1)
    Serial.print( "dgree is " + IntToString(d) );
  display.print(
    "The " + IntToString(count) + (mode == 1 ? " Start " : " Relax ")
  );
  if(mode == 1)
    display.print( "dgree is " + IntToString(d) );
  display.print("\n");
  Serial.print("\n");
  return;
}

void setup() {
  Serial.begin(115200);

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  s.attach(13);
  s.write(0);
  return;
}

void loop() {

  int mode = esp_random() % 2;
  if(mode == 1){
    int d = esp_random() % 131 + 50;
    My_Println(count++, d, 1);
    s.write(d);
  }else{
    My_Println(count++, 0, 0);
    s.write(0);
  }

  int t = esp_random() % 9 + 2;
  delay(t * 1000);
  return;
}
