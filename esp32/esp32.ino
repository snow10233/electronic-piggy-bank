#include <esp_camera.h>
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp_http_server.h"
#include "HX711.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h> //安裝ESP32Servo程式庫
#include <Stepper.h>
#include "happy.h"

//NTP網路時間
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "tw.pool.ntp.org", 28800, 60000);//所在國家的NTP伺服器 | 所在國家的時區 | 時間間隔
unsigned long lastNTPUpdate, ClkMillis;
int timer_hour, timer_minute, timer_second;//時 | 分 | 秒
const unsigned long oneDayMillis = 86400000;//一天抓取一次

//OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,U8X8_PIN_NONE,2,1);//SCL | SDA
int moneypoint_num, wifipoint_num;//OLED等待畫面...數量
unsigned long moneytimer,wifitimer;//OLED等待畫面...時間

//重量感測器
const int HX711_DTPin = 41;
const int HX711_SCKPin = 42;
const int scale_factor = 730;
const int piketimesSetup = 10;//硬幣數值抓取次數
int money = -1, moneyup, hx711state=1, faceTestState = 1, piketimesTimes, piketimesTimes_face;//金額總數 | 此筆增加金額 | 存錢感測器狀態 | 人臉辨識感測器狀態 | 存錢重量取得次數 | 人臉辨識重量取得次數
float coinweighttotal1, coinweighttotal2, coinweighttotal_face;//取平均前值 | 取平均後值 | 人臉辨識取平均值
HX711 scale;

//蜂鳴器
const int BUZZER_Pin = 40;

//步進馬達
const int STEP_Pin1 = 20;
const int STEP_Pin2 = 21;
const int STEP_Pin3 = 47;
const int STEP_Pin4 = 48;
Stepper CameraStep(2048, STEP_Pin1, STEP_Pin3, STEP_Pin2, STEP_Pin4);
int stepNum, stepState = 1;

//伺服馬達
const int right_servo = 45;
const int left_servo = 39;
const int top_servo = 38;
Servo right, left, top;

//TT直流馬達
const int TT_Pin1 = 14;
const int TT_Pin2 = 19;
int TTstate = 0;
unsigned long TTtimer;

//距離感測器
const int SR04_Pin1 = 3;
const int SR04_Pin2 = 46;
int closeTime, closeTimer, boxOpenChecktimes, boxCloseChecktimes_servoOn, boxCloseChecktimes_servoOff;

//網路
const char *ssid = "your_id";
const char *password = "your_password";

// HTTP 流媒體處理函數
esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t *_jpg_buf;
    char part_buf[64];
    bool is_jpeg_converted;

    // 設置返回類型為多部分傳輸流
    res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    if (res != ESP_OK) {
        return res;
    }

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE("CAMERA", "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        is_jpeg_converted = false;

        if (fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            is_jpeg_converted = true;
            esp_camera_fb_return(fb);
            fb = NULL;

            if (!jpeg_converted) {
                ESP_LOGE("CAMERA", "JPEG compression failed");
                res = ESP_FAIL;
                break;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        // 傳送圖片
        if (res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64,
                                   "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n", _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n", 2);
        }

        // 釋放記憶體
        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        } else if (is_jpeg_converted && _jpg_buf) {
            free(_jpg_buf);
        }

        if (res != ESP_OK) {
            break;
        }
    }

    return res;
}

// 攝像頭初始化函數
void cameraSetup() {
    camera_config_t config;
    config.pin_d0 = 11;
    config.pin_d1 = 9;
    config.pin_d2 = 8;
    config.pin_d3 = 10;
    config.pin_d4 = 12;
    config.pin_d5 = 18;
    config.pin_d6 = 17;
    config.pin_d7 = 16;
    config.pin_xclk = 15;
    config.pin_pclk = 13;
    config.pin_vsync = 6;
    config.pin_href = 7;
    config.pin_sccb_sda = 4;
    config.pin_sccb_scl = 5;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    // 初始化攝影機
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    startServer();
}

// 啟動 HTTP 服務器
void startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &stream_uri);
    }
}

//測量距離
float trig(){
    float cm, duration;
    digitalWrite(SR04_Pin1, HIGH); //給 Trig 高電位，持續 10微秒
    delayMicroseconds(10);
    digitalWrite(SR04_Pin1, LOW);
    duration = pulseIn(SR04_Pin2, HIGH); //收到高電位時的時間
    cm = duration*0.017;
    return cm;
}

//更新NTP時間
void updateNTPtime(){
    lastNTPUpdate = millis();
    ClkMillis = millis();
    timeClient.update();
    timer_hour = timeClient.getHours();
    timer_minute = timeClient.getMinutes();
    timer_second = timeClient.getSeconds();
}

//增加總金額
void addMoney(int up){
    save_servo(up);//改變存錢通道
    moneyup = up;
    money += up;//加錢
    Serial.printf("%d&", up);//傳送儲存命令
    mytone();//提示音
    TTstate = 1;
    TTtimer = millis();
    coinweighttotal1 = 0;
    coinweighttotal2 = 0;
    hx711state = 0;//關閉hx711
}

//改變分類通道
void save_servo(int save_money){
    switch(save_money){
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

//自拉蜂鳴器PWM信號
void mytone(){
    for(int i=0;i<=50;i++){
        digitalWrite(BUZZER_Pin,i%2);
        delayMicroseconds(1333);
    }
}

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_unifont_t_chinese1);
    WiFi.begin(ssid, password);
    timeClient.begin();
    scale.begin(HX711_DTPin, HX711_SCKPin);
    scale.set_scale(scale_factor);
    CameraStep.setSpeed(5);//速度調整
    pinMode(BUZZER_Pin,OUTPUT);
    pinMode(TT_Pin1,OUTPUT);
    pinMode(TT_Pin2,OUTPUT);
    digitalWrite(TT_Pin2, 0);
    pinMode(SR04_Pin1, OUTPUT);
    pinMode(SR04_Pin2, INPUT);
    digitalWrite(SR04_Pin1, LOW);
    right.attach(right_servo, 500, 2400);
    left.attach(left_servo, 500, 2400);
    top.attach(top_servo, 1000, 2400);
    delay(1000);
    right.write(90);
    left.write(90);
    top.write(90);
    mytone();//提示開始工作了
    //等待連接網路
    while (WiFi.status() != WL_CONNECTED) {
        if(millis() - 500 > wifitimer){
            wifitimer = millis();
            u8g2.firstPage();
            do{
                u8g2.setCursor(5,40);
                u8g2.print("網路連線中");
                for(int wifipoint = 0; wifipoint <= wifipoint_num; wifipoint++){
                    u8g2.setCursor(85+8*wifipoint, 40);
                    u8g2.print(".");
                }   
            }while (u8g2.nextPage());
            wifipoint_num = ++wifipoint_num % 5;
        }
    }
    cameraSetup();
    updateNTPtime();
    scale.tare();//歸零
}

void loop() {
    //等待後端傳入金額 預設money=-1
    while(money < 0) {
        if(stepState == 1){
            CameraStep.step(-9);
            stepNum += 9;
        }
        //讀取後端傳送數值
        if (Serial.available() > 0) {
            String moeny_string = Serial.readStringUntil('&'); // 讀取直到換行符
            moeny_string.trim();
            money = moeny_string.toInt();
            CameraStep.step(stepNum);
            stepNum = 0;
        }
        //畫面顯示...
        if(millis() - 500 > moneytimer){
            moneytimer = millis();
            u8g2.firstPage();
            do{
                u8g2.setCursor(5,40);
                u8g2.print("人臉辨識中");
                for(int moneypoint = 0; moneypoint <= moneypoint_num; moneypoint++){
                    u8g2.setCursor(85+8*moneypoint, 40);
                    u8g2.print(".");
                }   
            }while (u8g2.nextPage());
            moneypoint_num = ++moneypoint_num % 5;
        }        
        //按下後開始人臉辨識
        if(faceTestState == 1){
            if(piketimesTimes_face < piketimesSetup){
                coinweighttotal_face += scale.get_units(1);
                piketimesTimes_face++;
            }
            else{
                coinweighttotal_face /= piketimesSetup;
                if(coinweighttotal_face > 30){
                    Serial.println("face&");
                    faceTestState = 0;
                    stepState = 0;
                }
                coinweighttotal_face = 0;
                piketimesTimes_face = 0;
            }
        }
        else if(scale.get_units(1) < 0.5){
            faceTestState = 1;
        }
        if(millis() - 1000 > closeTimer){
            closeTimer = millis();
            if(trig() < 25){
                boxCloseChecktimes_servoOff = 0;
                boxCloseChecktimes_servoOn++;
            }
            else{
                boxCloseChecktimes_servoOn = 0;
                boxCloseChecktimes_servoOff++;
            }
            if(boxCloseChecktimes_servoOn == 5){
                stepState = 1;
                boxCloseChecktimes_servoOn = 0;
                boxCloseChecktimes_servoOff = 0;
            }
            if(boxCloseChecktimes_servoOff == 5){
                stepState = 0;
                boxCloseChecktimes_servoOn = 0;
                boxCloseChecktimes_servoOff = 0;
            }
        }
    }
    //根據距離關閉裝置
    if(millis() - 1000 > closeTimer){
        closeTimer = millis();
        if(trig() > 80){
            boxOpenChecktimes++;
        }
        else{
            boxOpenChecktimes = 0;
        }
        if(boxOpenChecktimes == 10){
            money = -1;
            boxOpenChecktimes = 0;
        }
    }
    //NTP刷新時間
    if (millis() - lastNTPUpdate > oneDayMillis) {
        updateNTPtime();
    }
    //millis刷新時間
    else {
        if (millis() - ClkMillis > 1000) {
            timer_second += (millis() - ClkMillis) / 1000;
            ClkMillis = millis();
            if (timer_second >= 60) {
                timer_minute += timer_second / 60;
                timer_second %= 60;
            }
            if (timer_minute >= 60) {
                timer_hour += timer_minute / 60;
                timer_minute %= 60;
            }
            if (timer_hour >= 24) {
                timer_hour %= 24;
            }
        }
    }
    //後端傳送扣錢數值
    if (Serial.available() > 0) {
        String costMoeny_string = Serial.readStringUntil('&'); // 讀取直到換行符
        costMoeny_string.trim();
        int costMoeny = costMoeny_string.toInt();
        money -= costMoeny;
    }
    //OLED主畫面 時間-HX711狀態-總金額
    u8g2.firstPage();
    do{
        //當前時間(第一行)
        u8g2.setCursor(0,14);
        char timer[30];
        sprintf(timer,"時間:%02d:%02d:%02d",timer_hour, timer_minute, timer_second);
        u8g2.print(timer);
        
        //重量感測器狀態(第二行)
        if(hx711state == 1){//(開)
            u8g2.setCursor(0,37); 
            u8g2.print("請放置硬幣");
        }
        else{//(關)
            u8g2.setCursor(0,37); 
            u8g2.print("請取下硬幣");
            //顯示存入金額
            u8g2.setCursor(90,37); 
            char up[10];
            sprintf(up,"+%d",moneyup);
            u8g2.print(up);
        }

        //現有金額(第三行)
        u8g2.setCursor(0,58);
        u8g2.print("現有金額:");
        u8g2.setCursor(73,58);
        char total[10];
        sprintf(total,"%d$",money);
        u8g2.print(total);
    }while (u8g2.nextPage());
    //啟用hx711 通過前後值做對比
    if(hx711state == 1){
        if(piketimesTimes < piketimesSetup){//抓取硬幣重量
            coinweighttotal1 += scale.get_units(1);
            piketimesTimes++;
        }
        else{//將重量進行計算並通過重量判斷金額
            coinweighttotal1 /= piketimesSetup;
            Serial.println(coinweighttotal1);
            if(coinweighttotal1 - coinweighttotal2 < 0.5 && coinweighttotal1 - coinweighttotal2 > -0.5){//|前-後| < 1 穩定
                if(coinweighttotal1 > 9 && coinweighttotal1 < 11) addMoney(50);//50$
                else if(coinweighttotal1 > 6 && coinweighttotal1 < 8.5) addMoney(10);//10$
                else if(coinweighttotal1 > 4.5 && coinweighttotal1 < 5) addMoney(5);//5$
                else if(coinweighttotal1 > 4 && coinweighttotal1 < 4.3) addMoney(1);//1$
            }
            coinweighttotal2 = coinweighttotal1;
            coinweighttotal1 = 0;
            piketimesTimes = 0;
        }
    }
    else if(scale.get_units(1) < 0.5){
        hx711state = 1;
    }
    if(TTstate == 1){
        if(millis() - 2125 < TTtimer){
            digitalWrite(TT_Pin1, 1);
        }
        else{
            digitalWrite(TT_Pin1, 0);
            TTstate = 0;
        }
    }
}
