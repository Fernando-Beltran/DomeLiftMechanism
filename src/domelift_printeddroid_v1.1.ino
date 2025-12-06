/*  DomeLift v1.1-fbn modificado por NandoMadrid
 *
 *  Mejoras añadidas:
 *  -------------------------------------------
 *  ✓ Switch 1 (pin 40) → Luz del periscopio
 *      - ON mientras el periscopio NO está en PBot
 *      - OFF al tocar PBot
 *
 *  ✓ Switch 2 (pin 41) → Máquina de humo (Fog Machine)
 *      - Se activa cuando el zapper llega al TOP (ZTopVal == LOW)
 *      - Permanece 5 segundos
 *      - Se desactiva antes si el zapper baja (ZTopVal vuelve HIGH)
 *
 *  Todo el código original se mantiene intacto.
 */


//05/01/2021 Modified by Printed-Droid.com for use with Marcduino and wireless (beta)
// Code for R2-D2 Dome mechanism by Matthew Zwarts
// Version 1 released 14th September 2019
// Items used Arduino Mega 2560, L298N Dual channel motor driver, PCA9685 16 channel servo driver,
// LM2596 DC-DC converter
// servo driver controlled via i2c
// Refer Thingiverse username: Matteous78 for drawing of parts layout, Dome Lift Mechanism Part 7
// NOTE: READ THIS
// I suggest you focus on connecting one mechanism first, the Lifeform Scanner or Periscope, to learn
// how the code will interact and fault find switch errors
// Wire up the components required and keep the belt tension on the pulleys firm, but not too tight,
// make sure the lifts slide easily before adding the belt tension
// Run this code to the serial monitor on your computer while plugged in to the Arduino Mega 2560,
// Baud rate 57600, to show the limit switches ZBOT, ZTOP for example.
// Trigger the switches by hand to check they are connected to the Arduino correctly and the value will
// change from 1 to 0, or vice versa
// The button triggers have three states, button count=1, move up , Button count=2, move down, button
// count=3, reset.
// Add power to the motor driver board and see if when the Button is pressed, PIN 34 on the Arduino to
// ground,it lifts the motor belt up or down
// If it moves the wrong direction, then just swap the 2 motor IN1 and IN2 pins on the motor driver
// Add one mechanism at a time to avoid having to try and fiugre out the complex wiring later
// Hope you can get it all moving...
// Regards, Matt Zwarts, Melbourne, Australia


/********************************************************************************************
    PERISCOPE LIGHTSHOW INTEGRATION (ESP32) – DOME LIFT BRIDGE
    -----------------------------------------------------------------------------------------
    Este módulo permite controlar la placa de iluminación del periscopio (ESP32 Periscope
    Lightshow de Printed-Droid) a través de comandos enviados desde Shadow / BetterDuino.

    FLUJO DE COMUNICACIÓN
    -----------------------------------------------------------------------------------------
    Shadow  →  BetterDuino  →  DomeLift  →  ESP32 (periscope lightshow)

    1. Shadow envía comandos personalizados del estilo:
        %PEQ0   %PEQ5   %PEQ14   %PEQ20

    2. BetterDuino los convierte al formato MarcDuino extendido:
        :PEQ0   :PEQ5   :PEQ14   :PEQ20

    3. El DomeLift recibe el comando en el parser principal (Serial / NRF),
       interpreta el valor y lo reenvía al ESP32 mediante el puerto UART Serial3:

        Serial3.println("Q0");
        Serial3.println("Q14");
        Serial3.println("Q20");

    4. El ESP32 ejecuta automáticamente el efecto Q correspondiente:
        Q0  = R2D2 Classic
        Q1  = Party Mode
        Q2  = Bright Pulse
        ...
        Q20 = Demo Mode

    PUERTO UTILIZADO PARA EL ESP32
    -----------------------------------------------------------------------------------------
    Se utiliza el puerto UART hardware Serial3 del Arduino Mega 2560:

        Mega TX3 (pin 14)  →  ESP32 RX
        Mega RX3 (pin 15)  →  ESP32 TX
        Mega 5V            →  ESP32 5V
        Mega GND           →  ESP32 GND

    DEFINES NECESARIOS
    -----------------------------------------------------------------------------------------
    Se utilizan dos defines para seleccionar el modo de funcionamiento del puerto Serial3:

        #define USE_MARCDUINO_SERIAL3
            → Activa el modo original de Printed-Droid (MarcDuino cableado a 9600 baud)

        #define USE_PERISCOPE_ESP32
            → Activar este para enviar comandos Q0–Q20 al ESP32 del periscopio (115200 baud)

    NOTA: Solo debe estar activo UNO de los dos defines al mismo tiempo.

    CONFIGURACIÓN DEL PUERTO SERIAL3
    -----------------------------------------------------------------------------------------
    En setup():

        #ifdef USE_MARCDUINO_SERIAL3
            Serial3.begin(9600);
        #elif defined(USE_PERISCOPE_ESP32)
            Serial3.begin(115200);
        #endif

    PARSEADOR DE COMANDOS :PEQxx
    -----------------------------------------------------------------------------------------
    Dentro del parser principal de comandos MarcDuino-like:

        :PEQ0  → envía Q0 al ESP32
        :PEQ5  → envía Q5 al ESP32
        :PEQ20 → envía Q20 al ESP32

    Se usa atoi(&SerialBuffer[4]) para convertir el número ASCII en entero.

    EJEMPLO:
        SerialBuffer = ":PEQ14"
        &SerialBuffer[4] = "14"
        atoi("14") = 14

    Solo se permiten valores entre 0 y 20 (efectos oficiales soportados).

    BENEFICIOS DE ESTA ARQUITECTURA
    -----------------------------------------------------------------------------------------
    ✔ Mantiene intacto el comportamiento original del DomeLift y MarcDuino
    ✔ Permite efectos avanzados del periscopio sin cargar el Mega 2560
    ✔ Mantiene compatibilidad total con Shadow y BetterDuino
    ✔ Comunicación UART hardware estable (115200 baud)
    ✔ Modular, limpia y fácil de extender

*********************************************************************************************/

// Notation abbreviations…
// P = periscope
// BM = Bad Motivator
// Z = Dome Zapper
// LS = Lightsaber
// LF = Lifeform Scanner
// DS = Drink Server

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERIAL_PORT_SPEED 9600
uint8_t servonum = 0;

//Version 1.2 with the NRF Module
#define PRINTEDDROIDV12
#define BETTERDUINO_ADDRESS 50

//#define USENRF
//#define USESERIAL3
//#define USE_MARCDUINO_SERIAL3
#define USE_PERISCOPE_ESP32   // <-- Usamos el domelift de pasarela al ESP32

#ifdef PRINTEDDROIDV12
  #define NRF_CE  9
  #define NRF_CSN 10
  #ifdef USENRF
    #include <SPI.h>
    #include <nRF24L01.h>
    #include <RF24.h>
    RF24 nrf_radio(NRF_CE, NRF_CSN);
    const byte nrf_address[6] = "R2NRF";
  #endif
#endif

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

#define ZLEDSERVOMIN 200
#define ZLEDSERVOMAX 500
#define BMLEDSERVOMIN 200
#define BMLEDSERVOMAX 500

// --------------------- MOTORES ---------------------
#define PEIN1 2
#define PEIN2 3
#define BMIN1 4
#define BMIN2 5
#define ZIN1 6
#define ZIN2 7

#ifdef PRINTEDDROIDV12
  #define LSIN1 44
  #define LSIN2 45
  #define LFIN1 46
  #define LFIN2 47
  #define DSIN1 49
  #define DSIN2 48
#else
  #define LSIN1 8
  #define LSIN2 9
  #define LFIN1 10
  #define LFIN2 11
  #define DSIN1 12
  #define DSIN2 13
#endif

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

// --------------------- BUTTON PINS ---------------------
#define buttonPin 39
#define buttonPin1 34
#define buttonPin2 37
#define buttonPin3 36
#define buttonPin4 35
#define buttonPin5 38


#define PERISC_LIGHT_PIN 40   // Switch 1 → Luz Periscopio
#define FOG_MACHINE_PIN  41   // Switch 2 → Fog Machine

// --------------------- TIMERS Y VARIABLES ---------------------
unsigned long currentMillis;
unsigned long zappreviousMillis;
unsigned long zapinterval = 30;
unsigned long zapturnpreviousMillis;
unsigned long zapturninterval = 2000;
unsigned long zapturninterval2 = 4000;
unsigned long zappause;
byte zapflashcount = 0;
byte zapperturncount = 0;
unsigned long bmpreviousMillis;
unsigned long bminterval = 200;
unsigned long pturnpreviousMillis;
unsigned long pturninterval = 1200;
byte pturncount = 0;
unsigned long lfturnpreviousMillis;
unsigned long lfturninterval = 600;
byte lfturncount = 0;
const unsigned long lfledinterval = 500;
unsigned long lfledpreviousmillis = 0;
unsigned long dspreviousMillis;
long dsinterval = 4000;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
// sates to select from the different case functions
static enum { ZAP_MOVE_TOP, ZAP_TOP } statezapup;     //zapper up
static enum { ZAP_MOVE_BOT, ZAP_BOT } statezapdown;   // zapper down
static enum { P_MOVE_TOP,   P_TOP } statepup;         //periscope up
static enum { P_MOVE_BOT,   P_BOT } statepdown;       //periscope down

static enum { LF_MOVE_TOP,  LF_TOP } statelfup;       //lifeform scanner up
static enum { LF_MOVE_BOT,  LF_BOT } statelfdown;     //lifeform scanner down
static enum { BM_MOVE_TOP,  BM_TOP } statebmup;       //Bad Motivator up
static enum { BM_MOVE_BOT,  BM_BOT } statebmdown;     //Bad Motivator down
static enum { LS_MOVE_TOP,  LS_TOP } statelsup;       //Lightsaber up
static enum { LS_MOVE_BOT,  LS_BOT } statelsdown;     //Lightsaber down
static enum { DS_MOVE_TOP,  DS_TOP } statedsup;       //Drink Server up
static enum { DS_MOVE_BOT,  DS_BOT } statedsdown;     //Drink Server down

int statez;    //zapper arm lift and turn
int statezl;   //zapper led
int statept;   //periscope turn
int statelf;   //lifeform scanner
int statelft;  //lifeform scanner turn
int statebml;  //bad motivator led
int stateds;   //drink server arm

// storage for limit switch values
int PTopVal = LOW;
int PBotVal = LOW;
int BMTopVal = LOW;
int BMBotVal = LOW;
int ZTopVal = LOW;
int ZBotVal = LOW;
int LSTopVal = LOW;
int LSBotVal = LOW;
int LFTopVal = LOW;
int LFBotVal = LOW;
int DSTopVal = LOW;
int DSBotVal = LOW;

// input button
int buttonPushCounter = 0;
int buttonPushCounter1 = 0;
int buttonPushCounter2 = 0;
int buttonPushCounter3 = 0;
int buttonPushCounter4 = 0;
int buttonPushCounter5 = 0;
int ledState = LOW;
int ledState1 = LOW;
int ledState2 = LOW;
int buttonState = 0;
int buttonState1 = 0;
int buttonState2 = 0;
int buttonState3 = 0;
int buttonState4 = 0;
int buttonState5 = 0;

int lastButtonState = 0;
int lastButtonState1 = 0;
int lastButtonState2 = 0;
int lastButtonState3 = 0;
int lastButtonState4 = 0;
int lastButtonState5 = 0;

#define SERIALBUFFERSIZE 64

char SerialBuffer[SERIALBUFFERSIZE];
unsigned int BufferIndex = 0;
bool lastBMTopVal = HIGH;   // estado anterior BadMotivator, para maquina de humo
// ---------------------- NUEVAS VARIABLES PARA FOG ----------------------
bool fogActive = false;
unsigned long fogStart = 0;
const unsigned long FOG_DURATION = 5000; // 5 segundos de humo


void setup()
{
    Serial.begin(SERIAL_PORT_SPEED); // serial communication

    Serial.println();
    Serial.println("=== DomeLift boot ===");
    Serial.println("Version: v1.1-fbn");
    Serial.print("ESP32 bridge: ");
    #ifdef USE_PERISCOPE_ESP32
    Serial.println("ENABLED (Serial3 @9600)");
    #else
    Serial.println("DISABLED");
    #endif

    Wire.begin();
    pwm.begin();
    pwm.setPWMFreq(50); // standard for analog servos

  // --- CONFIGURACIÓN DEL PUERTO Serial3 ---
    #ifdef USE_MARCDUINO_SERIAL3
        Serial3.begin(SERIAL_PORT_SPEED);     // Modo antiguo MarcDuino
    #elif defined(USE_PERISCOPE_ESP32)
        Serial3.begin(SERIAL_PORT_SPEED);   // Modo ESP32 Periscopio
      Serial.println("Init Serial for periscope 3");
        
    #endif

    #ifdef PRINTEDDROIDV12
      #ifdef USENRF
        if (!nrf_radio.begin()){
          Serial.println("radio.begin failed");
        }
        if (!nrf_radio.isChipConnected()){
          Serial.println("nrf chip is not connected");
        }
        nrf_radio.setPALevel(RF24_PA_MAX);
        nrf_radio.openReadingPipe(0, nrf_address); // set the address
        nrf_radio.startListening(); // set module as receiver
      #endif

      #ifdef USESERIAL3
        Serial3.begin(9600); //Serial to receive from marcduino slave board the "%" commands
      #endif
    #endif

    //output pins
    pinMode(PEIN1, OUTPUT);
    pinMode(PEIN2, OUTPUT);
    pinMode(BMIN1, OUTPUT);
    pinMode(BMIN2, OUTPUT);
    pinMode(ZIN1, OUTPUT);
    pinMode(ZIN2, OUTPUT);
    pinMode(LSIN1, OUTPUT);
    pinMode(LSIN2, OUTPUT);
    pinMode(LFIN1, OUTPUT);
    pinMode(LFIN2, OUTPUT);
    pinMode(DSIN1, OUTPUT);
    pinMode(DSIN2, OUTPUT);

    // input pins
    pinMode(PTop, INPUT_PULLUP);
    pinMode(PBot, INPUT_PULLUP);
    pinMode(BMTop, INPUT_PULLUP);
    pinMode(BMBot, INPUT_PULLUP);
    pinMode(ZTop, INPUT_PULLUP);
    pinMode(ZBot, INPUT_PULLUP);
    pinMode(LSTop, INPUT_PULLUP);
    pinMode(LSBot, INPUT_PULLUP);
    pinMode(DSTop, INPUT_PULLUP);
    pinMode(DSBot, INPUT_PULLUP);
    pinMode(LFTop, INPUT_PULLUP);
    pinMode(LFBot, INPUT_PULLUP);

    pinMode(buttonPin,  INPUT_PULLUP);
    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    pinMode(buttonPin3, INPUT_PULLUP);
    pinMode(buttonPin4, INPUT_PULLUP);
    pinMode(buttonPin5, INPUT_PULLUP);

    // NUEVOS: salidas para los transistores IRLZ44N
    pinMode(PERISC_LIGHT_PIN, OUTPUT);   // Luz del periscopio
    pinMode(FOG_MACHINE_PIN,  OUTPUT);   // Máquina de humo
    digitalWrite(PERISC_LIGHT_PIN, LOW);
    digitalWrite(FOG_MACHINE_PIN,  LOW);

    // Escribir los motores a LOW al inicio
    digitalWrite(PEIN1, LOW);
    digitalWrite(PEIN2, LOW);
    digitalWrite(BMIN1, LOW);
    digitalWrite(BMIN2, LOW);
    digitalWrite(ZIN1, LOW);
    digitalWrite(ZIN2, LOW);
    digitalWrite(LSIN1, LOW);
    digitalWrite(LSIN2, LOW);
    digitalWrite(LFIN1, LOW);
    digitalWrite(LFIN2, LOW);
    digitalWrite(DSIN1, LOW);
    digitalWrite(DSIN2, LOW);
    lastBMTopVal = digitalRead(BMTop);
    //Set the servos to their start positions
    servoSetup();
    delay(1000);
}
void loop()
{
    currentMillis = millis();

    #ifdef PRINTEDDROIDV12
      #ifdef USENRF

        if(nrf_radio.available()) {
          char nrf_buffer[32] = {0};
          nrf_radio.read(&nrf_buffer, sizeof(nrf_buffer));
          Serial.print("nrf received:");
          Serial.println(nrf_buffer);
          if (strlen(nrf_buffer)>=2){
            if (strcmp(nrf_buffer,"PE")==0){
              buttonPushCounter1++;
            }
            if (strcmp(nrf_buffer,"LF")==0){
              buttonPushCounter2++;
            }
            if (strcmp(nrf_buffer,"LS")==0){
              buttonPushCounter4++;
            }
            if (strcmp(nrf_buffer,"ZA")==0){
              buttonPushCounter++;
            }
            if (strcmp(nrf_buffer,"BM")==0){
              buttonPushCounter3++;
            }
          }
          else {
            nrf_buffer[0]=0;
          }
        }
      #endif

      #ifdef USESERIAL3
      char buffer[32] = {0};
      unsigned char index = 0;
      while (Serial3.available()>0){
        char c = Serial3.read();
        if (c == '\r'){
          Serial.print("received on serial3:");
          Serial.println(buffer);
          if (sizeof(buffer) >=2){
            if (strcmp(buffer,"PE") == 0){ buttonPushCounter1++; }
            else if (strcmp(buffer,"LF") == 0){ buttonPushCounter2++; }
            else if (strcmp(buffer,"LS") == 0){ buttonPushCounter4++; }
            else if (strcmp(buffer,"BM") == 0){ buttonPushCounter3++; }
            else if (strcmp(buffer,"ZA") == 0){ buttonPushCounter++; }
            buffer[0] = 0;
            index = 0;
          }
        }
        else {
          if (index < 31) {
            buffer[index++] = c;
            buffer[index] = 0;
          }
        }
      }
      #endif
    #endif


    // ------------------- INPUT SERIAL MARCDUINO -------------------
    if (Serial.available())
    {
        char c = Serial.read();

        Serial.print("RX CHAR -> ");
        if (c == '\r') Serial.println("\\r");
        else if (c == '\n') Serial.println("\\n");
        else Serial.println(c);

        if (c == '\n')
            return;
        SerialBuffer[BufferIndex++] = c;
        if ((c == '\r') || (BufferIndex == SERIALBUFFERSIZE))
        {
            SerialBuffer[BufferIndex-1] = 0x00;

            if(BufferIndex>1)
            {
                if (strcmp(SerialBuffer, ":LI00") == 0)
                {
                    buttonPushCounter = 1;
                    buttonPushCounter1= 1;
                    buttonPushCounter2= 1;
                    buttonPushCounter3= 1;
                    buttonPushCounter4= 1;
                    buttonPushCounter5= 1;
                }
                else if (strcmp(SerialBuffer, ":LI07") == 0){ buttonPushCounter3++; }
                else if (strcmp(SerialBuffer, ":LI08") == 0){ buttonPushCounter++; }
                else if (strcmp(SerialBuffer, ":LI09") == 0){ buttonPushCounter4++; }
                else if (strcmp(SerialBuffer, ":LI10") == 0){ buttonPushCounter2++; }
                else if (strcmp(SerialBuffer, ":LI11") == 0){ buttonPushCounter1++; }

                if (strcmp(SerialBuffer, ":LI99") == 0)
                {
                    buttonPushCounter  = 2;
                    buttonPushCounter1 = 2;
                    buttonPushCounter2 = 2;
                    buttonPushCounter3 = 2;
                    buttonPushCounter4 = 2;
                    buttonPushCounter5 = 2;
                }
                else if (strcmp(SerialBuffer, ":L?") == 0)
                {
                    Wire.beginTransmission(BETTERDUINO_ADDRESS);

                    Wire.write(":L?01\r");
                    Wire.endTransmission();
                }
                else if (strcmp(SerialBuffer, ":L?") == 0)    // Check Module Presence
                {
                    Wire.beginTransmission(BETTERDUINO_ADDRESS);
                    Wire.write(":L?01\r");
                    Wire.endTransmission();        
                }
                else if (strncmp(SerialBuffer, ":PEQ", 4) == 0) {
                  int mode = atoi(&SerialBuffer[4]);
                  if (mode >= 0 && mode <= 20) {
                      Serial3.print("Q");
                      Serial3.println(mode);
                      Serial.print("Serial3 TX -> Q");
                      Serial.println(mode);
                  }
              }
            }

            memset(SerialBuffer, 0x00, SERIALBUFFERSIZE);
            BufferIndex = 0;
        }
    }


    // ------------------- READ LIMITS -------------------
    readlimits();

    // ------------------- NEW: UPDATE PERISCOPE LIGHT -------------------
    updatePeriscopeLight();

    // ------------------- NEW: UPDATE FOG MACHINE -----------------------
    updateFogMachine();


    // ------------------- READ BUTTON STATES -------------------
    buttonState =  digitalRead(buttonPin);
    buttonState1 = digitalRead(buttonPin1);
    buttonState2 = digitalRead(buttonPin2);
    buttonState3 = digitalRead(buttonPin3);
    buttonState4 = digitalRead(buttonPin4);
    buttonState5 = digitalRead(buttonPin5);

    if (buttonState != lastButtonState)     if (buttonState == LOW)  buttonPushCounter++;
    if (buttonState1 != lastButtonState1)   if (buttonState1 == LOW) buttonPushCounter1++;
    if (buttonState2 != lastButtonState2)   if (buttonState2 == LOW) buttonPushCounter2++;
    if (buttonState3 != lastButtonState3)   if (buttonState3 == LOW) buttonPushCounter3++;
    if (buttonState4 != lastButtonState4)   if (buttonState4 == LOW) buttonPushCounter4++;
    if (buttonState5 != lastButtonState5)   if (buttonState5 == LOW) buttonPushCounter5++;


    // ----------- ZAPPER LÓGICA ORIGINAL ----------------
    if (buttonPushCounter == 1)
    {
        DomeZapperUp();
        if (ZTopVal == LOW && ZBotVal == HIGH)
            DomeZapper();
    }
    else if (buttonPushCounter == 2)
    {
        pwm.setPWM(5, 0, ZAPTURNSERVOMIN);
        pwm.setPWM(4, 0, ZAPSERVOMIN);
        pwm.setPWM(8, 0, 4096);
        DomeZapperDown();
    }
    else
    {
        buttonPushCounter = 0;
        statezapup   = ZAP_MOVE_TOP;
        statezapdown = ZAP_MOVE_BOT;
        statez = 1;
        statezl = 0;
    }


    // ----------- PERISCOPE LÓGICA ORIGINAL ----------------
    if (buttonPushCounter1 == 1)
    {
        PeriscopeUp();
        if (PTopVal == LOW && PBotVal == HIGH)
            PeriscopeTurn();
    }
    else if (buttonPushCounter1 == 2)
    {
        pwm.setPWM(6, 0, PETURNSERVOMIN);
        PeriscopeDown();
    }
    else
    {
        buttonPushCounter1 = 0;
        statepup = P_MOVE_TOP;
        statepdown = P_MOVE_BOT;
        statept = 0;
    }


    // ----------- LIFEFORM SCANNER ----------------
    if (buttonPushCounter2 == 1)
    {
        LifeformUp();
        if (currentMillis - lfledpreviousmillis >= lfledinterval)
        {
            pwm.setPWM(10, 4096, 0);
            lfledpreviousmillis = currentMillis;
        }
        else
        {
            pwm.setPWM(10, 0, 4096);
        }

        if (LFTopVal == LOW && LFBotVal == HIGH)
            LFTurn();
    }
    else if (buttonPushCounter2 == 2)
    {
        pwm.setPWM(7, 0, LFTURNSERVOMIN);
        LifeformDown();
    }
    else
    {
        buttonPushCounter2 = 0;
        statelfup = LF_MOVE_TOP;
        statelfdown = LF_MOVE_BOT;
        statelft = 0;
        lfturncount = 0;
    }
    // ----------- BAD MOTIVATOR ----------------
    if (buttonPushCounter3 == 1)
    {
        BadMotivatorUp();
        pwm.setPWM(9, 4096, 0);
    }
    else if (buttonPushCounter3 == 2)
    {
        BadMotivatorDown();
        pwm.setPWM(9, 0, 4096);
    }
    else
    {
        buttonPushCounter3 = 0;
        statebmup = BM_MOVE_TOP;
        statebmdown = BM_MOVE_BOT;
    }


    // ----------- LIGHTSABER ----------------
    if (buttonPushCounter4 == 1)
    {
        LightsaberUp();
    }
    else if (buttonPushCounter4 == 2)
    {
        LightsaberDown();
    }
    else
    {
        buttonPushCounter4 = 0;
        statelsup   = LS_MOVE_TOP;
        statelsdown = LS_MOVE_BOT;
    }


    // ----------- DRINK SERVER ----------------
    if (buttonPushCounter5 == 1)
    {
        DrinkServerUp();
    }
    else if (buttonPushCounter5 == 2)
    {
        if (DSTopVal == LOW && DSBotVal == HIGH)
            pwm.setPWM(11, 0, DRINKSERVOMAX);
    }
    else if (buttonPushCounter5 == 3)
    {
        if (DSTopVal == LOW && DSBotVal == HIGH)
            pwm.setPWM(11, 0, DRINKSERVOMIN);
    }
    else if (buttonPushCounter5 == 4)
    {
        DrinkServerDown();
    }
    else
    {
        buttonPushCounter5 = 0;
        statedsup   = DS_MOVE_TOP;
        statedsdown = DS_MOVE_BOT;
    }


    // ------------------- RESET STATES -------------------
    lastButtonState  = buttonState;
    lastButtonState1 = buttonState1;
    lastButtonState2 = buttonState2;
    lastButtonState3 = buttonState3;
    lastButtonState4 = buttonState4;
    lastButtonState5 = buttonState5;
}

void DomeZapperUp()
{
    switch (statezapup) {
    case ZAP_MOVE_TOP:
        if (ZTopVal != LOW) {
            if (digitalRead(ZTop) == HIGH && digitalRead(ZBot) == LOW) {
                pwm.setPWM(1, 0, ZSERVOMAX);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":OP08\r");
                Wire.write(":LK08\r");
                Wire.endTransmission();
            }
            digitalWrite(ZIN1, HIGH);
            digitalWrite(ZIN2, LOW);
            statezapup = ZAP_TOP;
        }
        break;

    case ZAP_TOP:
        if (ZTopVal == LOW) {
            digitalWrite(ZIN1, LOW);
            digitalWrite(ZIN2, LOW);
        }
        break;
    }
}

void DomeZapperDown()
{
    switch (statezapdown) {
    case ZAP_MOVE_BOT:
        if (ZBotVal != LOW) {
            digitalWrite(ZIN1, LOW);
            digitalWrite(ZIN2, HIGH);
            statezapdown = ZAP_BOT;
        }
        break;

    case ZAP_BOT:
        if (ZBotVal == LOW) {
            digitalWrite(ZIN1, LOW);
            digitalWrite(ZIN2, LOW);

            if (digitalRead(ZBot) == LOW && digitalRead(ZTop) == HIGH) {
                pwm.setPWM(1, 0, ZSERVOMIN);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":UL08\r");
                Wire.write(":CL08\r");
                Wire.endTransmission();
                buttonPushCounter++;
            }
        }
        break;
    }
}

void DomeZapper()
{
    switch (statez) {
    case 1:
        currentMillis = millis();
        pwm.setPWM(4, 0, ZAPSERVOMAX);
        if (currentMillis - zapturnpreviousMillis >= zapturninterval2) {
            statez = 2;
            zapturnpreviousMillis = currentMillis;
        }
        break;

    case 2:
        currentMillis = millis();
        pwm.setPWM(5, 0, ZAPTURNSERVOMAX);
        ZapLed();
        if (currentMillis - zapturnpreviousMillis >= zapturninterval2) {
            statez = 3;
            zapturnpreviousMillis = currentMillis;
        }
        break;

    case 3:
        currentMillis = millis();
        if (currentMillis - zapturnpreviousMillis >= zapturninterval2) {
            pwm.setPWM(5, 0, ZAPTURNSERVOMIN);
            statez = 0;
            zapturnpreviousMillis = currentMillis;
        }
        break;
    }
}

void ZapLed()
{
    switch (statezl) {
    case 0:
        currentMillis = millis();
        pwm.setPWM(8, 4096, 0);
        if (currentMillis - zappreviousMillis >= zapinterval) {
            zappreviousMillis = currentMillis;
            statezl = 1;
        }
        break;

    case 1:
        currentMillis = millis();
        pwm.setPWM(8, 0, 4096);
        if (currentMillis - zappreviousMillis >= zapinterval) {
            zappreviousMillis = currentMillis;
            statezl = 2;
        }
        break;

    case 2:
        currentMillis = millis();
        zapflashcount++;
        if (zapflashcount == 80) {
            statezl = 3;
            zapflashcount = 0;
        }
        else {
            statezl = 0;
        }
        break;
    }
}


// -------------------- PERISCOPE --------------------

void PeriscopeUp()
{
    switch (statepup) {
    case P_MOVE_TOP:
        if (PTopVal != LOW) {
            digitalWrite(PEIN1, HIGH);
            digitalWrite(PEIN2, LOW);
            statepup = P_TOP;
        }
        break;

    case P_TOP:
        if (PTopVal == LOW) {
            digitalWrite(PEIN1, LOW);
            digitalWrite(PEIN2, LOW);
        }
        break;
    }
}

void PeriscopeDown()
{
    switch (statepdown) {
    case P_MOVE_BOT:
        if (PBotVal != LOW) {
            digitalWrite(PEIN1, LOW);
            digitalWrite(PEIN2, HIGH);
            statepdown = P_BOT;
        }
        break;

    case P_BOT:
        if (PBotVal == LOW) {
            digitalWrite(PEIN1, LOW);
            digitalWrite(PEIN2, LOW);
        }
        break;
    }
}

void PeriscopeTurn()
{
    switch (statept) {
    case 0:
        currentMillis = millis();
        if (currentMillis - pturnpreviousMillis >= pturninterval) {
            pwm.setPWM(6, 0, PETURNSERVOMAX);
            pturnpreviousMillis = currentMillis;
            statept = 1;
        }
        break;

    case 1:
        currentMillis = millis();
        if (currentMillis - pturnpreviousMillis >= pturninterval) {
            pwm.setPWM(6, 0, PETURNSERVOMIN);
            pturnpreviousMillis = currentMillis;
            statept = 2;
        }
        break;

    case 2:
        currentMillis = millis();
        pturncount++;
        if (pturncount == 3) {
            statept = 3;
            pturncount = 0;
        }
        else {
            statept = 0;
        }
        break;
    }
}


// -------------------- LIFEFORM SCANNER --------------------

void LifeformUp()
{
    switch (statelfup) {
    case LF_MOVE_TOP:
        if (LFTopVal != LOW) {
            if (digitalRead(LFTop) == HIGH && digitalRead(LFBot) == LOW) {
                pwm.setPWM(3, 0, LFSERVOMAX);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":OP10\r");
                Wire.write(":LK10\r");
                Wire.endTransmission();
            }
            digitalWrite(LFIN1, HIGH);
            digitalWrite(LFIN2, LOW);
            statelfup = LF_TOP;
        }
        break;

    case LF_TOP:
        if (LFTopVal == LOW) {
            digitalWrite(LFIN1, LOW);
            digitalWrite(LFIN2, LOW);
        }
        break;
    }
}

void LifeformDown()
{
    statelft = 0;
    lfturncount = 0;

    switch (statelfdown) {
    case LF_MOVE_BOT:
        if (LFBotVal != LOW) {
            digitalWrite(LFIN1, LOW);
            digitalWrite(LFIN2, HIGH);
            statelfdown = LF_BOT;
        }
        break;

    case LF_BOT:
        if (LFBotVal == LOW) {
            digitalWrite(LFIN1, LOW);
            digitalWrite(LFIN2, LOW);

            if (digitalRead(LFBot) == LOW && digitalRead(LFTop) == HIGH) {
                pwm.setPWM(3, 0, LFSERVOMIN);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":UL10\r");
                Wire.write(":CL10\r");
                Wire.endTransmission();
                buttonPushCounter2++;
            }
        }
        break;
    }
}

void LFTurn()
{
    switch (statelft) {
    case 0:
        currentMillis = millis();
        if (currentMillis - lfturnpreviousMillis >= lfturninterval) {
            pwm.setPWM(7, 0, LFTURNSERVOMAX);
            lfturnpreviousMillis = currentMillis;
            statelft = 1;
        }
        break;

    case 1:
        currentMillis = millis();
        if (currentMillis - lfturnpreviousMillis >= lfturninterval) {
            pwm.setPWM(7, 0, LFTURNSERVOMIN);
            lfturnpreviousMillis = currentMillis;
            statelft = 2;
        }
        break;

    case 2:
        currentMillis = millis();
        lfturncount++;
        if (lfturncount == 6) {
            statelft = 3;
            lfturncount = 0;
        }
        else {
            statelft = 0;
        }
        break;
    }
}
void BadMotivatorUp()
{
    switch (statebmup) {

    case BM_MOVE_TOP:
        if (BMTopVal != LOW) {
            if (digitalRead(BMTop) == HIGH && digitalRead(BMBot) == LOW) {
                pwm.setPWM(0, 0, BMSERVOMAX);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":OP07\r");
                Wire.write(":LK07\r");
                Wire.endTransmission();
            }
            digitalWrite(BMIN1, HIGH);
            digitalWrite(BMIN2, LOW);
            statebmup = BM_TOP;
        }
        break;

    case BM_TOP:
        if (BMTopVal == LOW) {
            digitalWrite(BMIN1, LOW);
            digitalWrite(BMIN2, LOW);
        }
        break;
    }
}

void BadMotivatorDown()
{
    switch (statebmdown) {
    case BM_MOVE_BOT:
        if (BMBotVal != LOW) {
            digitalWrite(BMIN1, LOW);
            digitalWrite(BMIN2, HIGH);
            statebmdown = BM_BOT;
        }
        break;

    case BM_BOT:
        if (BMBotVal == LOW) {

            digitalWrite(BMIN1, LOW);
            digitalWrite(BMIN2, LOW);

            if (digitalRead(BMBot) == LOW && digitalRead(BMTop) == HIGH) {
                pwm.setPWM(0, 0, BMSERVOMIN);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":UL07\r");
                Wire.write(":CL07\r");
                Wire.endTransmission();
                buttonPushCounter3++;
            }
        }
        break;
    }
}



void LightsaberUp()
{
    switch (statelsup) {
    case LS_MOVE_TOP:
        if (LSTopVal != LOW) {
            if (digitalRead(LSTop) == HIGH && digitalRead(LSBot) == LOW) {
                pwm.setPWM(2, 0, LSSERVOMAX);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":OP09\r");
                Wire.write(":LK09\r");
                Wire.endTransmission();
            }
            digitalWrite(LSIN1, HIGH);
            digitalWrite(LSIN2, LOW);
            statelsup = LS_TOP;
        }
        break;

    case LS_TOP:
        if (LSTopVal == LOW) {
            digitalWrite(LSIN1, LOW);
            digitalWrite(LSIN2, LOW);
        }
        break;
    }
}

void LightsaberDown()
{
    switch (statelsdown) {
    case LS_MOVE_BOT:
        if (LSBotVal != LOW) {
            digitalWrite(LSIN1, LOW);
            digitalWrite(LSIN2, HIGH);
            statelsdown = LS_BOT;
        }
        break;

    case LS_BOT:
        if (LSBotVal == LOW) {
            digitalWrite(LSIN1, LOW);
            digitalWrite(LSIN2, LOW);

            if (digitalRead(LSBot) == LOW && digitalRead(LSTop) == HIGH) {
                pwm.setPWM(2, 0, LSSERVOMIN);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":UL09\r");
                Wire.write(":CL09\r");
                Wire.endTransmission();
                buttonPushCounter4++;
            }
        }
        break;
    }
}



void DrinkServerUp()
{
    switch (statedsup) {
    case DS_MOVE_TOP:
        if (DSTopVal != LOW) {
            if (digitalRead(DSTop) == HIGH && digitalRead(DSBot) == LOW) {
                pwm.setPWM(12, 0, DSSERVOMAX);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":OP11\r");
                Wire.write(":LK11\r");
                Wire.endTransmission();
            }
            digitalWrite(DSIN1, HIGH);
            digitalWrite(DSIN2, LOW);
            statedsup = DS_TOP;
        }
        break;

    case DS_TOP:
        if (DSTopVal == LOW) {
            digitalWrite(DSIN1, LOW);
            digitalWrite(DSIN2, LOW);
        }
        break;
    }
}

void DrinkServerDown()
{
    switch (statedsdown) {
    case DS_MOVE_BOT:
        if (DSBotVal != LOW) {
            digitalWrite(DSIN1, LOW);
            digitalWrite(DSIN2, HIGH);
            statedsdown = DS_BOT;
        }
        break;

    case DS_BOT:
        if (DSBotVal == LOW) {

            digitalWrite(DSIN1, LOW);
            digitalWrite(DSIN2, LOW);

            if (digitalRead(DSBot) == LOW && digitalRead(DSTop) == HIGH) {
                pwm.setPWM(12, 0, DSSERVOMIN);
                Wire.beginTransmission(BETTERDUINO_ADDRESS);
                Wire.write(":UL11\r");
                Wire.write(":CL11\r");
                Wire.endTransmission();
                buttonPushCounter5++;
            }
        }
        break;
    }
}


// -------------------- readlimits() --------------------

void readlimits()
{
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


// -------------------- servoSetup() --------------------

void servoSetup()
{
    pwm.setPWM(0, 0, BMSERVOMIN);
    pwm.setPWM(1, 0, ZSERVOMIN);
    pwm.setPWM(2, 0, LSSERVOMIN);
    pwm.setPWM(3, 0, LFSERVOMIN);
    pwm.setPWM(4, 0, ZAPSERVOMIN);
    pwm.setPWM(5, 0, ZAPTURNSERVOMIN);
    pwm.setPWM(6, 0, PETURNSERVOMIN);
    pwm.setPWM(7, 0, LFTURNSERVOMIN);

    pwm.setPWM(8, 0, 4096);   // Zapper LED
    pwm.setPWM(9, 0, 4096);   // Bad Motivator LED
    pwm.setPWM(10, 0, 4096);  // Lifeform LED

    pwm.setPWM(11, 0, DRINKSERVOMIN);
    pwm.setPWM(12, 0, DSSERVOMIN);
}
/* ---------------------------------------------------------
   NUEVAS FUNCIONES PERSONALIZADAS
   --------------------------------------------------------- */

// ---------------------- PERISCOPE LIGHT CONTROL ----------------------
// Luz ON cuando el periscopio NO está en la posición inferior (PBotVal == HIGH)
// Luz OFF cuando el periscopio toca el final inferior (PBotVal == LOW)
void updatePeriscopeLight() {

    if (PBotVal == HIGH) {
        // El periscopio está subiendo o arriba → luz encendida
        digitalWrite(PERISC_LIGHT_PIN, HIGH);
    } else {
        // Toca fondo → apagamos luz
        digitalWrite(PERISC_LIGHT_PIN, LOW);
    }
}



// ---------------------- FOG MACHINE CONTROL --------------------------
// Enciende la máquina de humo cuando el Zapper llega al TOP (ZTopVal == LOW).
// La mantiene ON durante 5 segundos y la apaga si baja antes.
void updateFogMachine() {

    // Disparo SOLO al llegar al TOP (flanco)
    if (!fogActive && BMTopVal == LOW && lastBMTopVal == HIGH) {
        fogActive = true;
        fogStart = millis();
        digitalWrite(FOG_MACHINE_PIN, HIGH);
    }

    if (fogActive) {

        // Apagado por tiempo
        if (millis() - fogStart >= FOG_DURATION) {
            fogActive = false;
            digitalWrite(FOG_MACHINE_PIN, LOW);
        }
        // Apagado si empieza a bajar
        else if (BMTopVal == HIGH) {
            fogActive = false;
            digitalWrite(FOG_MACHINE_PIN, LOW);
        }
    }

    // Memorizar estado para el siguiente loop
    lastBMTopVal = BMTopVal;
}


// ---------------------- SerialOut (debug) ----------------------------
void SerialOut()
{
    Serial.print("B:");  Serial.print(buttonPushCounter);  Serial.print("\t");
    Serial.print("B1:"); Serial.print(buttonPushCounter1); Serial.print("\t");
    Serial.print("B2:"); Serial.print(buttonPushCounter2); Serial.print("\t");
    Serial.print("B3:"); Serial.print(buttonPushCounter3); Serial.print("\t");
    Serial.print("B4:"); Serial.print(buttonPushCounter4); Serial.print("\t");
    Serial.print("B5:"); Serial.print(buttonPushCounter5); Serial.print("\t");

    Serial.print("ZBot:"); Serial.print(ZBotVal); Serial.print("\t");
    Serial.print("ZTop:"); Serial.print(ZTopVal); Serial.print("\t");
    Serial.print("PBot:"); Serial.print(PBotVal); Serial.print("\t");
    Serial.print("PTop:"); Serial.print(PTopVal); Serial.print("\t");
    Serial.print("BMBot:"); Serial.print(BMBotVal); Serial.print("\t");
    Serial.print("BMTop:"); Serial.print(BMTopVal); Serial.print("\t");
    Serial.print("LFBot:"); Serial.print(LFBotVal); Serial.print("\t");
    Serial.print("LFTop:"); Serial.print(LFTopVal); Serial.print("\t");
    Serial.print("LSBot:"); Serial.print(LSBotVal); Serial.print("\t");
    Serial.print("LSTop:"); Serial.print(LSTopVal); Serial.print("\t");
    Serial.print("DSBot:"); Serial.print(DSBotVal); Serial.print("\t");
    Serial.print("DSTop:"); Serial.print(DSTopVal); Serial.print("\t");

    Serial.print("Millis:"); Serial.print(currentMillis);
    Serial.println("\t");
}

/* ---------------------------------------------------------
   FIN DEL ARCHIVO
   --------------------------------------------------------- */

