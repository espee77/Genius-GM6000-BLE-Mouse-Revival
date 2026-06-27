#include "Encoders.h"
#include "Pins.h"

static volatile long xDelta = 0;
static volatile long yDelta = 0;
static volatile uint8_t lastXState = 0;
static volatile uint8_t lastYState = 0;

static const int8_t quadTable[16] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0
};

static void xISR() {
    uint8_t state = (digitalRead(X_A) << 1) | digitalRead(X_B);
    uint8_t transition = (lastXState << 2) | state;
    int8_t movement = quadTable[transition];
    if (movement != 0) xDelta += movement;
    lastXState = state;
}

static void yISR() {
    uint8_t state = (digitalRead(Y_A) << 1) | digitalRead(Y_B);
    uint8_t transition = (lastYState << 2) | state;
    int8_t movement = quadTable[transition];
    if (movement != 0) yDelta += movement;
    lastYState = state;
}

void encodersReset() {
    noInterrupts();
    xDelta = 0;
    yDelta = 0;
    lastXState = (digitalRead(X_A) << 1) | digitalRead(X_B);
    lastYState = (digitalRead(Y_A) << 1) | digitalRead(Y_B);
    interrupts();
}

void encodersBegin() {
    pinMode(X_A, INPUT_PULLUP);
    pinMode(X_B, INPUT_PULLUP);
    pinMode(Y_A, INPUT_PULLUP);
    pinMode(Y_B, INPUT_PULLUP);

    encodersReset();

    attachInterrupt(digitalPinToInterrupt(X_A), xISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(X_B), xISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(Y_A), yISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(Y_B), yISR, CHANGE);
}

void encodersConsume(long &rawX, long &rawY) {
    noInterrupts();
    rawX = xDelta;
    rawY = yDelta;
    xDelta = 0;
    yDelta = 0;
    interrupts();
}
