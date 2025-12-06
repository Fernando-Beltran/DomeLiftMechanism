/*  DomeLift v1.1-fbn modificado por NandoMadrid
 *
 *  ESP32 Periscope bridge @9600
 *  Código limpio, sin debug innecesario
 */

// -----------------------------------------------------------------------------
// DEFINES
// -----------------------------------------------------------------------------
#define PRINTEDDROIDV12
#define BETTERDUINO_ADDRESS 50
#define USE_PERISCOPE_ESP32

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERIAL_PORT_SPEED 9600
#define SERIALBUFFERSIZE 64
char SerialBuffer[SERIALBUFFERSIZE];
unsigned int BufferIndex = 0;

// -----------------------------------------------------------------------------
// PINES
// -----------------------------------------------------------------------------
#define PERISC_LIGHT_PIN 40
#define FOG_MACHINE_PIN  41

#define PEIN1 2
#define PEIN2 3
#define BMIN1 4
#define BMIN2 5
#define ZIN1  6
#define ZIN2  7

#define LSIN1 44
#define LSIN2 45
#define LFIN1 46
#define LFIN2 47
#define DSIN1 49
#define DSIN2 48

#define PTop 22
#define PBot 23
#define BMTop 24
#define BMBot 25
#define ZTop 26
#define ZBot 27
#define LSTop 28
#define LSBot 29
#define LFTop 30
#define LFBot 31
#define DSTop 32
#define DSBot 33

#define buttonPin  39
#define buttonPin1 34
#define buttonPin2 37
#define buttonPin3 36
#define buttonPin4 35
#define buttonPin5 38

// -----------------------------------------------------------------------------
// VARIABLES
// -----------------------------------------------------------------------------
unsigned long currentMillis;

int PTopVal, PBotVal, BMTopVal, BMBotVal;
int ZTopVal, ZBotVal, LSTopVal, LSBotVal;
int LFTopVal, LFBotVal, DSTopVal, DSBotVal;

int buttonPushCounter  = 0;
int buttonPushCounter1 = 0;
int buttonPushCounter2 = 0;
int buttonPushCounter3 = 0;
int buttonPushCounter4 = 0;
int buttonPushCounter5 = 0;

int lastButtonState  = 0;
int lastButtonState1 = 0;
int lastButtonState2 = 0;
int lastButtonState3 = 0;
int lastButtonState4 = 0;
int lastButtonState5 = 0;

// Fog
bool fogActive = false;
unsigned long fogStart = 0;
const unsigned long FOG_DURATION = 5000;
bool lastBMTopVal = HIGH;

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup() {

    Serial.begin(SERIAL_PORT_SPEED);

#ifdef USE_PERISCOPE_ESP32
    Serial3.begin(9600);
    Serial.println("ESP32 bridge OK @9600");
#endif

    Wire.begin();
    pwm.begin();
    pwm.setPWMFreq(50);

    pinMode(PERISC_LIGHT_PIN, OUTPUT);
    pinMode(FOG_MACHINE_PIN, OUTPUT);
    digitalWrite(PERISC_LIGHT_PIN, LOW);
    digitalWrite(FOG_MACHINE_PIN, LOW);

    pinMode(PEIN1, OUTPUT); pinMode(PEIN2, OUTPUT);
    pinMode(BMIN1, OUTPUT); pinMode(BMIN2, OUTPUT);
    pinMode(ZIN1, OUTPUT);  pinMode(ZIN2, OUTPUT);
    pinMode(LSIN1, OUTPUT); pinMode(LSIN2, OUTPUT);
    pinMode(LFIN1, OUTPUT); pinMode(LFIN2, OUTPUT);
    pinMode(DSIN1, OUTPUT); pinMode(DSIN2, OUTPUT);

    pinMode(PTop, INPUT_PULLUP); pinMode(PBot, INPUT_PULLUP);
    pinMode(BMTop, INPUT_PULLUP); pinMode(BMBot, INPUT_PULLUP);
    pinMode(ZTop, INPUT_PULLUP);  pinMode(ZBot, INPUT_PULLUP);
    pinMode(LSTop, INPUT_PULLUP); pinMode(LSBot, INPUT_PULLUP);
    pinMode(LFTop, INPUT_PULLUP); pinMode(LFBot, INPUT_PULLUP);
    pinMode(DSTop, INPUT_PULLUP); pinMode(DSBot, INPUT_PULLUP);

    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    pinMode(buttonPin3, INPUT_PULLUP);
    pinMode(buttonPin4, INPUT_PULLUP);
    pinMode(buttonPin5, INPUT_PULLUP);

    lastBMTopVal = digitalRead(BMTop);
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------
void loop() {

    currentMillis = millis();

    // ---------------- SERIAL INPUT (MarcDuino) ----------------
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') return;

        SerialBuffer[BufferIndex++] = c;

        if (c == '\r' || BufferIndex >= SERIALBUFFERSIZE) {
            SerialBuffer[BufferIndex - 1] = 0;

            // PEQxx → ESP32 Qxx
            if (strncmp(SerialBuffer, ":PEQ", 4) == 0) {
                int mode = atoi(&SerialBuffer[4]);
                if (mode >= 0 && mode <= 20) {
                    Serial3.print("Q");
                    Serial3.println(mode);
                    Serial.print("PEQ -> Q");
                    Serial.println(mode);
                }
            }

            memset(SerialBuffer, 0, SERIALBUFFERSIZE);
            BufferIndex = 0;
        }
    }

    // ---------------- ESP32 OUTPUT (si habla) ----------------
#ifdef USE_PERISCOPE_ESP32
    if (Serial3.available()) {
        Serial.print("ESP32 -> ");
        while (Serial3.available()) Serial.write(Serial3.read());
        Serial.println();
    }
#endif

    // ---------------- LIMITS ----------------
    readlimits();

    // ---------------- CUSTOM LOGIC ----------------
    updatePeriscopeLight();
    updateFogMachine();

    // ---------------- BUTTONS ----------------
    handleButtons();
}

// -----------------------------------------------------------------------------
// FUNCIONES
// -----------------------------------------------------------------------------
void readlimits() {
    PBotVal = digitalRead(PBot);
    PTopVal = digitalRead(PTop);
    BMTopVal = digitalRead(BMTop);
    BMBotVal = digitalRead(BMBot);
    ZBotVal = digitalRead(ZBot);
    ZTopVal = digitalRead(ZTop);
    LSBotVal = digitalRead(LSBot);
    LSTopVal = digitalRead(LSTop);
    LFBotVal = digitalRead(LFBot);
    LFTopVal = digitalRead(LFTop);
    DSBotVal = digitalRead(DSBot);
    DSTopVal = digitalRead(DSTop);
}

// Luz periscopio ON salvo en fondo
void updatePeriscopeLight() {
    digitalWrite(PERISC_LIGHT_PIN, (PBotVal == HIGH));
}

// Fog al llegar arriba (flanco)
void updateFogMachine() {

    if (!fogActive && BMTopVal == LOW && lastBMTopVal == HIGH) {
        fogActive = true;
        fogStart = millis();
        digitalWrite(FOG_MACHINE_PIN, HIGH);
    }

    if (fogActive) {
        if (millis() - fogStart >= FOG_DURATION || BMTopVal == HIGH) {
            fogActive = false;
            digitalWrite(FOG_MACHINE_PIN, LOW);
        }
    }

    lastBMTopVal = BMTopVal;
}

void handleButtons() {

    int bs  = digitalRead(buttonPin);
    int bs1 = digitalRead(buttonPin1);
    int bs2 = digitalRead(buttonPin2);
    int bs3 = digitalRead(buttonPin3);
    int bs4 = digitalRead(buttonPin4);
    int bs5 = digitalRead(buttonPin5);

    if (bs  != lastButtonState  && bs  == LOW) buttonPushCounter++;
    if (bs1 != lastButtonState1 && bs1 == LOW) buttonPushCounter1++;
    if (bs2 != lastButtonState2 && bs2 == LOW) buttonPushCounter2++;
    if (bs3 != lastButtonState3 && bs3 == LOW) buttonPushCounter3++;
    if (bs4 != lastButtonState4 && bs4 == LOW) buttonPushCounter4++;
    if (bs5 != lastButtonState5 && bs5 == LOW) buttonPushCounter5++;

    lastButtonState  = bs;
    lastButtonState1 = bs1;
    lastButtonState2 = bs2;
    lastButtonState3 = bs3;
    lastButtonState4 = bs4;
    lastButtonState5 = bs5;
}
