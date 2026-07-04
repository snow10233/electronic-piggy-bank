void addMoney(int up) {
    save_servo(up);
    moneyup = up;
    money += up;
    Serial.printf("%d&", up);
    mytone();
    TTstate = 1;
    TTtimer = millis();
    coinweighttotal1 = 0;
    coinweighttotal2 = 0;
    hx711state = 0;
}

void save_servo(int save_money) {
    switch (save_money) {
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

void mytone() {
    for (int i = 0; i <= 50; i++) {
        digitalWrite(HardwareConfig::BUZZER_PIN, i % 2);
        delayMicroseconds(1333);
    }
}
