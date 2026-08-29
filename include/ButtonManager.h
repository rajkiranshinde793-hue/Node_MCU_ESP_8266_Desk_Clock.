#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "config.h"

enum ButtonEvent {
    EV_NONE,
    EV_BTN1_SINGLE,
    EV_BTN1_DOUBLE,
    EV_BTN1_LONG,
    EV_BTN2_PRESS,
    EV_BTN3_PRESS,
    EV_BTN4_PRESS
};

class ButtonManager {
public:
    ButtonManager();
    void init();
    ButtonEvent update();

private:
    enum Button1State {
        BTN1_IDLE,
        BTN1_PRESSED_1,
        BTN1_WAIT_SECOND,
        BTN1_PRESSED_2
    };

    Button1State btn1State;
    unsigned long btn1PressStart;
    unsigned long btn1ReleaseTime;

    bool lastBtn2State;
    bool lastBtn3State;
    bool lastBtn4State;
};

extern ButtonManager buttonManager;

#endif // BUTTON_MANAGER_H
