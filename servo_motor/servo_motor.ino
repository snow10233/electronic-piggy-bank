#include <ESP32Servo.h> //請先安裝ESP32Servo程式庫

Servo right, left, top; //建立一個伺服馬達物件
const int right_servo = 45;
const int left_servo = 39;
const int top_servo = 38;

void save_servo(int save_money) {
  switch(save_money) {
    case 1:
      top.write(20);
      left.write(125);
      break;
    case 5:
      top.write(20);
      left.write(60);
      break;
    case 10:
      top.write(140);
      right.write(0);
      break;
    case 50:
      top.write(140);
      right.write(115);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  right.attach(right_servo, 500, 2400);
  left.attach(left_servo, 500, 2400);
  top.attach(top_servo, 1000, 2400);
  delay(1000);
  right.write(90);
  left.write(90);
  top.write(90);
  Serial.println("ok");
}

void loop() {
  if (Serial.available() > 0) {
      String moeny_string = Serial.readStringUntil('&'); // 讀取直到換行符
      moeny_string.trim();
      int money = moeny_string.toInt();
      Serial.println(money);
      if(money == 50) {
        Serial.println(money);
        save_servo(50);//50$
      }
      if(money == 10) {
        Serial.println(money);
        save_servo(10);//10$
      }
      if(money == 5) {
        Serial.println(money);
        save_servo(5);//5$
      }
      if(money == 1) {
        Serial.println(money);
        save_servo(1);//1$
      }
  }
}
