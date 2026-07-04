float trig() {
    float duration;
    digitalWrite(HardwareConfig::SR04_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(HardwareConfig::SR04_TRIG_PIN, LOW);
    duration = pulseIn(HardwareConfig::SR04_ECHO_PIN, HIGH);
    return duration * 0.017;
}

void updateNTPtime() {
    lastNTPUpdate = millis();
    ClkMillis = millis();
    timeClient.update();
    timer_hour = timeClient.getHours();
    timer_minute = timeClient.getMinutes();
    timer_second = timeClient.getSeconds();
}

void updateClock() {
    if (millis() - lastNTPUpdate > HardwareConfig::ONE_DAY_MS) {
        updateNTPtime();
        return;
    }

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
