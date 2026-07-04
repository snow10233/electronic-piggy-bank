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
#include <ESP32Servo.h>
#include <Stepper.h>

#include "hardware_config.h"
#include "happy.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, HardwareConfig::NTP_SERVER, HardwareConfig::NTP_TIME_OFFSET_SECONDS, HardwareConfig::NTP_UPDATE_INTERVAL_MS);
unsigned long lastNTPUpdate, ClkMillis;
int timer_hour, timer_minute, timer_second;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, HardwareConfig::OLED_SCL_PIN, HardwareConfig::OLED_SDA_PIN);
int moneypoint_num, wifipoint_num;
unsigned long moneytimer, wifitimer;

int money = -1, moneyup, hx711state = 1, faceTestState = 1, piketimesTimes, piketimesTimes_face;
float coinweighttotal1, coinweighttotal2, coinweighttotal_face;
HX711 scale;

Stepper CameraStep(
    HardwareConfig::STEPPER_STEPS_PER_REVOLUTION,
    HardwareConfig::STEP_PIN_1,
    HardwareConfig::STEP_PIN_3,
    HardwareConfig::STEP_PIN_2,
    HardwareConfig::STEP_PIN_4
);
int stepNum, stepState = 1;

Servo right, left, top;

int TTstate = 0;
unsigned long TTtimer;

int closeTimer, boxOpenChecktimes, boxCloseChecktimes_servoOn, boxCloseChecktimes_servoOff;

void setup() {
    initializeHardware();
    connectWifi();
    cameraSetup();
    updateNTPtime();
    scale.tare();
}

void loop() {
    waitForUserIdentification();
    handleBoxOpenReset();
    updateClock();
    readCostFromBackend();
    renderMainScreen();
    detectCoinDeposit();
    updateConveyorMotor();
}

void initializeHardware() {
    Serial.begin(HardwareConfig::SERIAL_BAUD_RATE);
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_unifont_t_chinese1);

    WiFi.begin(HardwareConfig::WIFI_SSID, HardwareConfig::WIFI_PASSWORD);
    timeClient.begin();

    scale.begin(HardwareConfig::HX711_DT_PIN, HardwareConfig::HX711_SCK_PIN);
    scale.set_scale(HardwareConfig::HX711_SCALE_FACTOR);

    CameraStep.setSpeed(HardwareConfig::STEPPER_SPEED_RPM);

    pinMode(HardwareConfig::BUZZER_PIN, OUTPUT);
    pinMode(HardwareConfig::TT_PIN_1, OUTPUT);
    pinMode(HardwareConfig::TT_PIN_2, OUTPUT);
    digitalWrite(HardwareConfig::TT_PIN_2, LOW);

    pinMode(HardwareConfig::SR04_TRIG_PIN, OUTPUT);
    pinMode(HardwareConfig::SR04_ECHO_PIN, INPUT);
    digitalWrite(HardwareConfig::SR04_TRIG_PIN, LOW);

    right.attach(HardwareConfig::RIGHT_SERVO_PIN, 500, 2400);
    left.attach(HardwareConfig::LEFT_SERVO_PIN, 500, 2400);
    top.attach(HardwareConfig::TOP_SERVO_PIN, 1000, 2400);

    delay(1000);
    right.write(HardwareConfig::SERVO_CENTER_ANGLE);
    left.write(HardwareConfig::SERVO_CENTER_ANGLE);
    top.write(HardwareConfig::SERVO_CENTER_ANGLE);

    mytone();
}

void connectWifi() {
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - HardwareConfig::OLED_LOADING_INTERVAL_MS > wifitimer) {
            wifitimer = millis();
            renderWifiConnectingScreen();
        }
    }
}

void waitForUserIdentification() {
    while (money < 0) {
        if (stepState == 1) {
            CameraStep.step(-9);
            stepNum += 9;
        }

        if (Serial.available() > 0) {
            String moneyString = Serial.readStringUntil('&');
            moneyString.trim();
            money = moneyString.toInt();
            CameraStep.step(stepNum);
            stepNum = 0;
        }

        if (millis() - HardwareConfig::OLED_LOADING_INTERVAL_MS > moneytimer) {
            moneytimer = millis();
            renderFaceRecognitionWaitingScreen();
        }

        updateFaceRecognitionTrigger();
        updateCameraScanState();
    }
}

void updateFaceRecognitionTrigger() {
    if (faceTestState == 1) {
        if (piketimesTimes_face < HardwareConfig::COIN_SAMPLE_COUNT) {
            coinweighttotal_face += scale.get_units(1);
            piketimesTimes_face++;
        } else {
            coinweighttotal_face /= HardwareConfig::COIN_SAMPLE_COUNT;
            if (coinweighttotal_face > HardwareConfig::FACE_TRIGGER_WEIGHT_GRAMS) {
                Serial.println("face&");
                faceTestState = 0;
                stepState = 0;
            }
            coinweighttotal_face = 0;
            piketimesTimes_face = 0;
        }
    } else if (scale.get_units(1) < HardwareConfig::EMPTY_SCALE_THRESHOLD_GRAMS) {
        faceTestState = 1;
    }
}

void updateCameraScanState() {
    if (millis() - HardwareConfig::DISTANCE_CHECK_INTERVAL_MS > closeTimer) {
        closeTimer = millis();
        if (trig() < HardwareConfig::BOX_CLOSED_DISTANCE_CM) {
            boxCloseChecktimes_servoOff = 0;
            boxCloseChecktimes_servoOn++;
        } else {
            boxCloseChecktimes_servoOn = 0;
            boxCloseChecktimes_servoOff++;
        }
        if (boxCloseChecktimes_servoOn == HardwareConfig::BOX_STATE_CONFIRM_COUNT) {
            stepState = 1;
            boxCloseChecktimes_servoOn = 0;
            boxCloseChecktimes_servoOff = 0;
        }
        if (boxCloseChecktimes_servoOff == HardwareConfig::BOX_STATE_CONFIRM_COUNT) {
            stepState = 0;
            boxCloseChecktimes_servoOn = 0;
            boxCloseChecktimes_servoOff = 0;
        }
    }
}

void handleBoxOpenReset() {
    if (millis() - HardwareConfig::DISTANCE_CHECK_INTERVAL_MS > closeTimer) {
        closeTimer = millis();
        if (trig() > HardwareConfig::BOX_OPEN_DISTANCE_CM) {
            boxOpenChecktimes++;
        } else {
            boxOpenChecktimes = 0;
        }
        if (boxOpenChecktimes == HardwareConfig::BOX_OPEN_CONFIRM_COUNT) {
            money = -1;
            boxOpenChecktimes = 0;
        }
    }
}

void readCostFromBackend() {
    if (Serial.available() > 0) {
        String costMoneyString = Serial.readStringUntil('&');
        costMoneyString.trim();
        int costMoney = costMoneyString.toInt();
        money -= costMoney;
    }
}

void detectCoinDeposit() {
    if (hx711state == 1) {
        if (piketimesTimes < HardwareConfig::COIN_SAMPLE_COUNT) {
            coinweighttotal1 += scale.get_units(1);
            piketimesTimes++;
        } else {
            coinweighttotal1 /= HardwareConfig::COIN_SAMPLE_COUNT;
            Serial.println(coinweighttotal1);
            if (coinweighttotal1 - coinweighttotal2 < 0.5 && coinweighttotal1 - coinweighttotal2 > -0.5) {
                if (coinweighttotal1 > 9 && coinweighttotal1 < 11) addMoney(50);
                else if (coinweighttotal1 > 6 && coinweighttotal1 < 8.5) addMoney(10);
                else if (coinweighttotal1 > 4.5 && coinweighttotal1 < 5) addMoney(5);
                else if (coinweighttotal1 > 4 && coinweighttotal1 < 4.3) addMoney(1);
            }
            coinweighttotal2 = coinweighttotal1;
            coinweighttotal1 = 0;
            piketimesTimes = 0;
        }
    } else if (scale.get_units(1) < HardwareConfig::EMPTY_SCALE_THRESHOLD_GRAMS) {
        hx711state = 1;
    }
}

void updateConveyorMotor() {
    if (TTstate == 1) {
        if (millis() - HardwareConfig::CONVEYOR_RUN_MS < TTtimer) {
            digitalWrite(HardwareConfig::TT_PIN_1, HIGH);
        } else {
            digitalWrite(HardwareConfig::TT_PIN_1, LOW);
            TTstate = 0;
        }
    }
}
