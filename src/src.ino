/** ToDo:
 *    sleep mode (only sleep when handset is down)
 *    start call when no number input happens for a time
 */

#include <Arduino.h>

#define rotaryDialPin 2        // has to be an interrupt pin
#define cradlePin 3            // signal of the phone cradle, has to be an interrupt pin
#define cradleDebounceDelay 50 // debounce time for the cradle switch
#define buttonPressTime 100    // time in ms to keep the phone button pressed
uint32_t lastDialPulse = 0;    // the last time a rotary dial pulse has been detected
#define dialDebounceDelay 80   // debounce time for the rotary dial pulses
volatile int c = 0;

// command chain for connection to a base station
char* dectLogonCommand = "XXXXX!^^!vvv!vv!!0000!"; 

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

// interrupt each time the rotary dial generates a pulse
void dialIRQ() {
    noInterrupts();
    if (digitalRead(rotaryDialPin) && lastDialPulse + dialDebounceDelay < millis()) {
        c++;
        lastDialPulse = millis();
    }
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
    Serial.println(F("\n>> Drehscheibentelefon to DECT <<<"));
    Serial.println(F("\nYou can use ASCII characters to control the phone, apart from dialing with the rotary dial."));
    Serial.print("\n\n> ");

    pinMode(cradlePin, INPUT_PULLUP);
    pinMode(rotaryDialPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(rotaryDialPin), dialIRQ, CHANGE);

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
bool lastCradleState = false;
uint32_t lastCradleChange = 0;

void loop() {
    // handle the end of a dial rotation
    unsigned long diff = millis() - lastDialPulse;
    if (c > 0 && diff > 200 && diff < 500) // catch interrupt modified variable (underflow)
    {
        // handle the 0 on the rotary dial, as it is located at the end of the wheel
        if (c == 10) {
            c = 0;
        }
        Serial.print(c);                // repeat the dialed digit to the Serial terminal
        pressButton((button)(c + '0')); // convert the detected number into an ASCII char
        c = 0;
    }

    // handle the cradle switch
    bool cradleState = digitalRead(cradlePin);
    if (lastCradleState != cradleState && lastCradleChange + cradleDebounceDelay < millis()) {
        lastCradleChange = millis();

        if (cradleState) { // handset has been put down
            Serial.print((char)B_CANCEL);
            pressButton(B_CANCEL);
        } else { // handset has been lifted up
            // deactivate sleep count-down
        }

        lastCradleState = cradleState;
    }

    // read stringed buttons to be pressed from serial into buffer
    while (!buttonSchedulingInProgress && Serial.available()) {
        char c = Serial.read();

        if (c != '\n') {
            // put all received chars in buffer
            scheduledButtons[scheduledButtonCount] = c;
            scheduledButtonCount++;
        } else {
            // upon receiving a newline, trigger the button pressing
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
            } else {
                buttonSchedulingInProgress = false;
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