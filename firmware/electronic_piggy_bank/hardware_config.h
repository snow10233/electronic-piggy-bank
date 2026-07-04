#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

namespace HardwareConfig {
constexpr int SERIAL_BAUD_RATE = 115200;

constexpr const char *WIFI_SSID = "your_id";
constexpr const char *WIFI_PASSWORD = "your_password";

constexpr const char *NTP_SERVER = "tw.pool.ntp.org";
constexpr long NTP_TIME_OFFSET_SECONDS = 28800;
constexpr unsigned long NTP_UPDATE_INTERVAL_MS = 60000;
constexpr unsigned long ONE_DAY_MS = 86400000;

constexpr int OLED_SCL_PIN = 2;
constexpr int OLED_SDA_PIN = 1;
constexpr unsigned long OLED_LOADING_INTERVAL_MS = 500;

constexpr int HX711_DT_PIN = 41;
constexpr int HX711_SCK_PIN = 42;
constexpr int HX711_SCALE_FACTOR = 730;
constexpr int COIN_SAMPLE_COUNT = 10;
constexpr float EMPTY_SCALE_THRESHOLD_GRAMS = 0.5;
constexpr float FACE_TRIGGER_WEIGHT_GRAMS = 30;

constexpr int BUZZER_PIN = 40;

constexpr int STEPPER_STEPS_PER_REVOLUTION = 2048;
constexpr int STEP_PIN_1 = 20;
constexpr int STEP_PIN_2 = 21;
constexpr int STEP_PIN_3 = 47;
constexpr int STEP_PIN_4 = 48;
constexpr int STEPPER_SPEED_RPM = 5;

constexpr int RIGHT_SERVO_PIN = 45;
constexpr int LEFT_SERVO_PIN = 39;
constexpr int TOP_SERVO_PIN = 38;
constexpr int SERVO_CENTER_ANGLE = 90;

constexpr int TT_PIN_1 = 14;
constexpr int TT_PIN_2 = 19;
constexpr unsigned long CONVEYOR_RUN_MS = 2125;

constexpr int SR04_TRIG_PIN = 3;
constexpr int SR04_ECHO_PIN = 46;
constexpr unsigned long DISTANCE_CHECK_INTERVAL_MS = 1000;
constexpr float BOX_CLOSED_DISTANCE_CM = 25;
constexpr float BOX_OPEN_DISTANCE_CM = 80;
constexpr int BOX_STATE_CONFIRM_COUNT = 5;
constexpr int BOX_OPEN_CONFIRM_COUNT = 10;

constexpr int CAMERA_D0_PIN = 11;
constexpr int CAMERA_D1_PIN = 9;
constexpr int CAMERA_D2_PIN = 8;
constexpr int CAMERA_D3_PIN = 10;
constexpr int CAMERA_D4_PIN = 12;
constexpr int CAMERA_D5_PIN = 18;
constexpr int CAMERA_D6_PIN = 17;
constexpr int CAMERA_D7_PIN = 16;
constexpr int CAMERA_XCLK_PIN = 15;
constexpr int CAMERA_PCLK_PIN = 13;
constexpr int CAMERA_VSYNC_PIN = 6;
constexpr int CAMERA_HREF_PIN = 7;
constexpr int CAMERA_SIOD_PIN = 4;
constexpr int CAMERA_SIOC_PIN = 5;
constexpr int CAMERA_PWDN_PIN = -1;
constexpr int CAMERA_RESET_PIN = -1;
constexpr int CAMERA_XCLK_FREQ_HZ = 20000000;
constexpr int CAMERA_JPEG_QUALITY = 10;
constexpr int CAMERA_FB_COUNT = 2;
}

#endif
