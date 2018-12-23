#include <Arduino.h>

const byte interruptPin = 2;
volatile byte state = LOW;
int ledState = HIGH;                                 // the current state of the output pin
int buttonState;                                     // the current reading from the input pin
int lastButtonState = LOW;                           // the previous reading from the input pin
unsigned long lastDebounceTime = 0, lastTriggerTime; // the last time the output pin was toggled
unsigned long debounceDelay = 80;
volatile int c = 0;

byte colPins[] = {4, 5, 6, 7, 8};
byte rowPins[] = {9, 10, 11, 12};

#define colPinsLength sizeof(colPins) / sizeof(colPins[0])
#define rowPinsLength sizeof(rowPins) / sizeof(rowPins[0])

// this enum also defines the ASCII codes needed to be sent over serial
enum button {
    B_0 = '0',
    B_1 = '1',
    B_2 = '2',
    B_3 = '3',
    B_4 = '4',
    B_5 = '5',
    B_6 = '6',
    B_7 = '7',
    B_8 = '8',
    B_9 = '9',
    B_STAR = '*',
    B_HASH = '#',
    B_CALL = 'C',
    B_CANCEL = 'X',
    B_SPEAKER = 'S',
    B_MESSAGE = 'E',
    B_BACK = '-',
    B_MENU = 'M',
    B_DOWN = 'v',
    B_UP = '^',
    B_LEFT = '<',
    B_RIGHT = '>',
    B_ENTER = '!'
};

button buttonMapping[rowPinsLength][colPinsLength] = {{}};

void irq() {
    noInterrupts();
    /*Serial.print(millis() % 1000);
    Serial.print(" ");
    Serial.print(digitalRead(interruptPin));*/
    if (digitalRead(interruptPin) && lastDebounceTime + debounceDelay < millis()) {
        // Serial.print(" x");
        c++;
        lastDebounceTime = millis();
    }
    // Serial.println();
    interrupts();
}

void setup() {
    Serial.begin(115200);

    pinMode(interruptPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interruptPin), irq, CHANGE);
}

void loop() {
    unsigned long diff = millis() - lastDebounceTime;
    if (c > 0 && diff > 200 && diff < 500) // catch interrupt modified variable (underflow)
    {
        // Serial.println(String(millis()) + " " + String(lastDebounceTime) + " " + String(diff) + " " +  String(c));
        Serial.print(c % 10);
        c = 0;
    }
}

void pressButton(button btn) {
    byte y = 0, x = 0;
    bool done = false;
    for (y = 0; y < rowPinsLength; y++) {
        for (x = 0; x < colPinsLength; x++) {
            if (buttonMapping[y][x] == btn) {
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }
    }
}