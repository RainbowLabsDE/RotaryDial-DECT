#include <Arduino.h>

#define interruptPin 2
#define buttonPressTime 100                     // time in ms to keep the phone button pressed
uint64_t lastDebounceTime = 0, lastTriggerTime; // the last time the output pin was toggled
uint64_t debounceDelay = 80;
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

void clearButtons() {
    for (byte i = 0; i < rowPinsLength; i++) {
        pinMode(rowPins[i], INPUT);
    }
    for (byte i = 0; i < colPinsLength; i++) {
        pinMode(colPins[i], INPUT);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println(F(">> Drehscheibentelefon to DECT <<<"));
    Serial.println(F("\nYou can use ASCII characters to control the phone, apart from dialing with the rotary dial."));
    Serial.println("\n\n> ");

    pinMode(interruptPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interruptPin), irq, CHANGE);

    for (byte i = 0; i < rowPinsLength; i++) {
        pinMode(rowPins[i], INPUT);
        digitalWrite(rowPins[i], LOW);
    }
    for (byte i = 0; i < colPinsLength; i++) {
        pinMode(colPins[i], INPUT);
        digitalWrite(colPins[i], HIGH);
    }
}

uint32_t lastButtonPressed = 0, lastButtonScheduling = 0;
bool buttonPressed = false;
char scheduledButtons[50];
bool buttonSchedulingInProgress = false;
uint8_t scheduledButtonCount = 0, scheduledButtonIdx = 0;

void loop() {
    unsigned long diff = millis() - lastDebounceTime;
    if (c > 0 && diff > 200 && diff < 500) // catch interrupt modified variable (underflow)
    {
        // Serial.println(String(millis()) + " " + String(lastDebounceTime) + " " + String(diff) + " " +  String(c));
        Serial.print(c % 10);
        c = 0;
    }

    // read stringed buttons to be pressed from serial into buffer
    while (!buttonSchedulingInProgress && Serial.available()) {
        char c = Serial.read();
        if (c != '\n') {
            scheduledButtons[scheduledButtonCount] = c;
            scheduledButtonCount++;
        } else {
            buttonSchedulingInProgress = true;
        }
        Serial.print(c);
    }

    // handle the actual pressing of the buttons
    if (buttonSchedulingInProgress) {
        if (lastButtonScheduling + (buttonPressTime * 2) < millis()) {
            lastButtonScheduling = millis();
            if (scheduledButtonCount > 0) {
                pressButton((button)scheduledButtons[scheduledButtonIdx]);
                scheduledButtonIdx++;
                if (scheduledButtonIdx >= scheduledButtonCount) {
                    buttonSchedulingInProgress = false;
                    Serial.print("\n> ");
                }
            }
        }
    }

    // stop pressing the button after the timeout
    if (buttonPressed && lastButtonPressed + buttonPressTime < millis()) {
        buttonPressed = false;
        clearButtons();
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

    // check that the corresponding pins have actually been found
    if (done) {
        digitalWrite(colPins[x], OUTPUT);
        digitalWrite(rowPins[y], OUTPUT);
        lastButtonPressed = millis();
        buttonPressed = true;
    }
}