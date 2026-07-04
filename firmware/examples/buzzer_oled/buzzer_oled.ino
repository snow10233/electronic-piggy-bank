#include <Wire.h>
#include <U8g2lib.h>
#include "happy.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,U8X8_PIN_NONE,2,1);

const int BUZZER_Pin = 40;
int musicNum, awardtimes;
unsigned long oledTimer, musicTimer;

void setup() {
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_unifont_t_chinese1);
  musicTimer = millis();
  oledTimer = millis();
}

void loop() {
    for(awardtimes=0;awardtimes<140;awardtimes++){
      awardtimes %= 28;
      if (millis() - oledTimer >= 50) {
        oledTimer = millis();
        u8g2.firstPage();
        do {
            u8g2.clearBuffer();
            u8g2.setCursor(3, 14);
            u8g2.print("恭喜存入");
            u8g2.setCursor(3,35);
            char total[10];
            sprintf(total,"超過%d元!!", 100);
            u8g2.print(total);
            u8g2.drawXBMP(80, 7, 50, 50, epd_bitmap_allArray1[awardtimes]);
            u8g2.sendBuffer();
        } while (u8g2.nextPage());
      }
      if (millis() - musicTimer > 190) {
          tone(BUZZER_Pin, melody[musicNum]);
          musicTimer = millis();
          musicNum = (musicNum + 1) % 17; // 循環播放
      }
    }
    
}

//獎勵
        // if(money >= awardMoney[awardMoneyState] && awardMoney[awardMoneyState] > 0){
        //     musicTimer = millis();
        //     oledTimer = millis();
        //     for(int awardtimes=0;awardtimes<140;awardtimes++){
        //         awardtimes %= 28;
        //         if (millis() - oledTimer >= 50) {
        //           oledTimer = millis();
        //           u8g2.firstPage();
        //           do {
        //               u8g2.clearBuffer();
        //               u8g2.setCursor(3, 14);
        //               u8g2.print("恭喜存入");
        //               u8g2.setCursor(3,35);
        //               char total[10];
        //               sprintf(total,"超過%d元!!", awardMoney[awardMoneyState]);
        //               u8g2.print(total);
        //               u8g2.drawXBMP(80, 7, 50, 50, epd_bitmap_allArray1[awardtimes]);
        //               u8g2.sendBuffer();
        //           } while (u8g2.nextPage());
        //         }
        //         if (millis() - musicTimer > 150) {
        //             tone(BUZZER_Pin, melody[musicNum]);
        //             musicTimer = millis();
        //             musicNum = (musicNum + 1) % 17; // 循環播放
        //         }
        //     }
        //     awardMoneyState++;
        // }
