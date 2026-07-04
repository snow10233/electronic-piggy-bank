void renderWifiConnectingScreen() {
    u8g2.firstPage();
    do {
        u8g2.setCursor(5, 40);
        u8g2.print("網路連線中");
        for (int wifipoint = 0; wifipoint <= wifipoint_num; wifipoint++) {
            u8g2.setCursor(85 + 8 * wifipoint, 40);
            u8g2.print(".");
        }
    } while (u8g2.nextPage());
    wifipoint_num = ++wifipoint_num % 5;
}

void renderFaceRecognitionWaitingScreen() {
    u8g2.firstPage();
    do {
        u8g2.setCursor(5, 40);
        u8g2.print("人臉辨識中");
        for (int moneypoint = 0; moneypoint <= moneypoint_num; moneypoint++) {
            u8g2.setCursor(85 + 8 * moneypoint, 40);
            u8g2.print(".");
        }
    } while (u8g2.nextPage());
    moneypoint_num = ++moneypoint_num % 5;
}

void renderMainScreen() {
    u8g2.firstPage();
    do {
        u8g2.setCursor(0, 14);
        char timer[30];
        sprintf(timer, "時間:%02d:%02d:%02d", timer_hour, timer_minute, timer_second);
        u8g2.print(timer);

        if (hx711state == 1) {
            u8g2.setCursor(0, 37);
            u8g2.print("請放置硬幣");
        } else {
            u8g2.setCursor(0, 37);
            u8g2.print("請取下硬幣");
            u8g2.setCursor(90, 37);
            char up[10];
            sprintf(up, "+%d", moneyup);
            u8g2.print(up);
        }

        u8g2.setCursor(0, 58);
        u8g2.print("現有金額:");
        u8g2.setCursor(73, 58);
        char total[10];
        sprintf(total, "%d$", money);
        u8g2.print(total);
    } while (u8g2.nextPage());
}
