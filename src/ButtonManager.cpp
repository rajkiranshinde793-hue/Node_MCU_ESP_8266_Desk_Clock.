#include "ButtonManager.h"

ButtonManager buttonManager;

ButtonManager::ButtonManager() {
    btn1State = BTN1_IDLE;
    btn1PressStart = 0;
    btn1ReleaseTime = 0;
    lastBtn2State = HIGH;
    lastBtn3State = HIGH;
    lastBtn4State = HIGH;
}

void ButtonManager::init() {
    pinMode(BUTTON_1_PIN, INPUT_PULLUP);
    pinMode(BUTTON_2_PIN, INPUT_PULLUP);
    pinMode(BUTTON_3_PIN, INPUT_PULLUP);
    pinMode(BUTTON_4_PIN, INPUT_PULLUP);
}

ButtonEvent ButtonManager::update() {
    ButtonEvent eventToReturn = EV_NONE;
    unsigned long now = millis();

    // --- BUTTON 1 STATE MACHINE ---
    bool curBtn1 = digitalRead(BUTTON_1_PIN);

    switch (btn1State) {
        case BTN1_IDLE:
            if (curBtn1 == LOW) {
                btn1PressStart = now;
                btn1State = BTN1_PRESSED_1;
            }
            break;

        case BTN1_PRESSED_1:
            if (curBtn1 == HIGH) {
                unsigned long duration = now - btn1PressStart;
                if (duration >= LONG_PRESS_MS) {
                    btn1State = BTN1_IDLE;
                    eventToReturn = EV_BTN1_LONG;
                } else if (duration >= DEBOUNCE_MS) {
                    btn1ReleaseTime = now;
                    btn1State = BTN1_WAIT_SECOND;
                } else {
                    btn1State = BTN1_IDLE; // Mechanical bounce
                }
            }
            break;

        case BTN1_WAIT_SECOND:
            if (curBtn1 == LOW) {
                btn1PressStart = now;
                btn1State = BTN1_PRESSED_2;
            } else if (now - btn1ReleaseTime >= DOUBLE_CLICK_WINDOW_MS) {
                btn1State = BTN1_IDLE;
                eventToReturn = EV_BTN1_SINGLE;
            }
            break;

        case BTN1_PRESSED_2:
            if (curBtn1 == HIGH) {
                unsigned long duration = now - btn1PressStart;
                btn1State = BTN1_IDLE;
                if (duration >= DEBOUNCE_MS) {
                    eventToReturn = EV_BTN1_DOUBLE;
                }
            }
            break;
    }

    if (eventToReturn != EV_NONE) {
        return eventToReturn;
    }

    // --- BUTTON 2 (CHIRAG Button 1) ---
    bool curBtn2 = digitalRead(BUTTON_2_PIN);
    if (curBtn2 == LOW && lastBtn2State == HIGH) {
        lastBtn2State = curBtn2;
        return EV_BTN2_PRESS;
    }
    lastBtn2State = curBtn2;

    // --- BUTTON 3 (CHIRAG Button 2) ---
    bool curBtn3 = digitalRead(BUTTON_3_PIN);
    if (curBtn3 == LOW && lastBtn3State == HIGH) {
        lastBtn3State = curBtn3;
        return EV_BTN3_PRESS;
    }
    lastBtn3State = curBtn3;

    // --- BUTTON 4 (CHIRAG Button 3) ---
    bool curBtn4 = digitalRead(BUTTON_4_PIN);
    if (curBtn4 == LOW && lastBtn4State == HIGH) {
        lastBtn4State = curBtn4;
        return EV_BTN4_PRESS;
    }
    lastBtn4State = curBtn4;

    return EV_NONE;
}
