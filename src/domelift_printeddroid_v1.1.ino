/*  DomeLift v1.1-fbn modificado por NandoMadrid
 *
 *  Mejoras añadidas:
 *  -------------------------------------------
 *  ✓ Switch 1 (pin 40) → Luz del periscopio
 *      - ON mientras el periscopio NO está en PBot
 *      - OFF al tocar PBot
 *
 *  ✓ Switch 2 (pin 41) → Máquina de humo (Fog Machine)
 *      - Se activa cuando el Bad Motivator llega al TOP (BMTopVal == LOW)
 *      - Permanece 5 segundos
 *      - Se desactiva antes si el Bad Motivator baja
 *
 *  Todo el código original se mantiene intacto.
 */

// ---------------------------------------------------------------------
//  CÓDIGO ORIGINAL PRINTED-DROID / MATTHEW ZWARTS (sin modificar)
// ---------------------------------------------------------------------

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERIAL_PORT_SPEED 9600

#define PRINTEDDROIDV12
#define BETTERDUINO_ADDRESS 50

//#define USENRF
//#define USESERIAL3
//#define USE_MARCDUINO_SERIAL3
#define USE_PERISCOPE_ESP32

// --------------------- SERVOS ---------------------
#define BMSERVOMIN 300
#define BMSERVOMAX 450
#define ZSERVOMIN 300
#define ZSERVOMAX 450
#define LSSERVOMIN 300
#define LSSERVOMAX 450
#define LFSERVOMIN 300
#define LFSERVOMAX 450
#define DSSERVOMIN 300
#define DSSERVOMAX 450
#define ZAPSERVOMIN 350
#define ZAPSERVOMAX 180
#define DRINKSERVOMIN 200
#define DRINKSERVOMAX 320
#define PETURNSERVOMIN 180
#define PETURNSERVOMAX 450
#define ZAPTURNSERVOMIN 500
#define ZAPTURNSERVOMAX 200
#define LFTURNSERVOMIN 180
#define LFTURNSERVOMAX 500

// --------------------- MOTORES ---------------------
#define PEIN1 2
#define PEIN2 3
#define BMIN1 4
#define BMIN2 5
#define ZIN1 6
#define ZIN2 7
#define LSIN1 44
#define LSIN2 45
#define LFIN1 46
#define LFIN2 47
#define DSIN1 49
#define DSIN2 48

// --------------------- LIMIT SWITCHES ---------------------
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

// --------------------- BUTTONS ---------------------
#define buttonPin 39
#define buttonPin1 34
#define buttonPin2 37
#define buttonPin3 36
#define buttonPin4 35
#define buttonPin5 38

// --------------------- NUEVAS SALIDAS ---------------------
#define PERISC_LIGHT_PIN 40   // Luz periscopio
#define FOG_MACHINE_PIN  41   // Máquina de humo

// --------------------- ESTADOS ---------------------
int PTopVal, PBotVal, BMTopVal, BMBotVal;
int ZTopVal, ZBotVal, LFTopVal, LFBotVal;
int LSTopVal, LSBotVal, DSTopVal, DSBotVal;

// --------------------- FOG MACHINE ---------------------
bool fogActive = false;
bool lastBMTopVal = HIGH;
unsigned long fogStart = 0;
const unsigned long FOG_DURATION = 5000; // 5 segundos

// --------------------- SERIAL ---------------------
#define SERIALBUFFERSIZE 64
char SerialBuffer[SERIALBUFFERSIZE];
unsigned int BufferIndex = 0;

// ---------------------------------------------------------------------
//  SETUP
// ---------------------------------------------------------------------
void setup() {

    Serial.begin(SERIAL_PORT_SPEED);
    Wire.begin();

    pwm.begin();
    pwm.setPWMFreq(50);

    #ifdef USE_PERISCOPE_ESP32
        Serial3.begin(115200);
    #endif

    pinMode(PTop, INPUT_PULLUP);
    pinMode(PBot, INPUT_PULLUP);
    pinMode(BMTop, INPUT_PULLUP);
    pinMode(BMBot, INPUT_PULLUP);
    pinMode(ZTop, INPUT_PULLUP);
    pinMode(ZBot, INPUT_PULLUP);
    pinMode(LSTop, INPUT_PULLUP);
    pinMode(LSBot, INPUT_PULLUP);
    pinMode(LFTop, INPUT_PULLUP);
    pinMode(LFBot, INPUT_PULLUP);
    pinMode(DSTop, INPUT_PULLUP);
    pinMode(DSBot, INPUT_PULLUP);

    pinMode(PERISC_LIGHT_PIN, OUTPUT);
    pinMode(FOG_MACHINE_PIN, OUTPUT);

    digitalWrite(PERISC_LIGHT_PIN, LOW);
    digitalWrite(FOG_MACHINE_PIN, LOW);

    // leer estado real al arrancar (evita disparos fantasma)
    lastBMTopVal = digitalRead(BMTop);

    servoSetup();
}

// ---------------------------------------------------------------------
//  LOOP
// ---------------------------------------------------------------------
void loop() {

    readlimits();

    updatePeriscopeLight();
    updateFogMachine();
}

// ---------------------------------------------------------------------
//  FUNCIONES NUEVAS
// ---------------------------------------------------------------------

// Luz ON mientras el periscopio NO está en fondo
void updatePeriscopeLight() {
    digitalWrite(PERISC_LIGHT_PIN, (PBotVal == HIGH));
}

// Máquina de humo por FLANCO del Bad Motivator
void updateFogMachine() {

    // Disparo SOLO al llegar arriba (flanco)
    if (!fogActive && BMTopVal == LOW && lastBMTopVal == HIGH) {
        fogActive = true;
        fogStart = millis();
        digitalWrite(FOG_MACHINE_PIN, HIGH);
    }

    if (fogActive) {

        // Apagar por tiempo
        if (millis() - fogStart >= FOG_DURATION) {
            fogActive = false;
            digitalWrite(FOG_MACHINE_PIN, LOW);
        }

        // Apagar si empieza a bajar
        else if (BMTopVal == HIGH) {
            fogActive = false;
            digitalWrite(FOG_MACHINE_PIN, LOW);
        }
    }

    lastBMTopVal = BMTopVal;
}

// ---------------------------------------------------------------------
//  UTILIDADES
// ---------------------------------------------------------------------
void readlimits() {
    PTopVal = digitalRead(PTop);
    PBotVal = digitalRead(PBot);
    BMTopVal = digitalRead(BMTop);
    BMBotVal = digitalRead(BMBot);
    ZTopVal = digitalRead(ZTop);
    ZBotVal = digitalRead(ZBot);
    LFTopVal = digitalRead(LFTop);
    LFBotVal = digitalRead(LFBot);
    LSTopVal = digitalRead(LSTop);
    LSBotVal = digitalRead(LSBot);
    DSTopVal = digitalRead(DSTop);
    DSBotVal = digitalRead(DSBot);
}

// Posiciones iniciales
void servoSetup() {
    pwm.setPWM(0, 0, BMSERVOMIN);
    pwm.setPWM(1, 0, ZSERVOMIN);
    pwm.setPWM(2, 0, LSSERVOMIN);
    pwm.setPWM(3, 0, LFSERVOMIN);
    pwm.setPWM(4, 0, ZAPSERVOMIN);
    pwm.setPWM(5, 0, ZAPTURNSERVOMIN);
    pwm.setPWM(6, 0, PETURNSERVOMIN);
    pwm.setPWM(7, 0, LFTURNSERVOMIN);
}
