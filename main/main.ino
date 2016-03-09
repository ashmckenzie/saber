#include <Arduino.h>

#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>

#include <OneButton.h>
#include <EEPROM.h>

#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

#include <Timer.h>

Timer t;
MPU6050 mpu;

// PINS
//
const int LED_PIN = 13;

const int MP3_RX_PIN = 6;
const int MP3_TX_PIN = 7;
const int MP3_BUSY_PIN = 12;

const int XAXIS_PIN = A0;
const int YAXIS_PIN = A1;
const int ZAXIS_PIN = A2;

const unsigned int BUTTON_MAIN_PIN = 8;
const unsigned int BUTTON_AUX_PIN = 9;

OneButton buttonMain(BUTTON_MAIN_PIN, true);
OneButton buttonAux(BUTTON_AUX_PIN, true);

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

// TUNABLES
//
const int SOUND_VOLUME = 5;
 
/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

#define SWING_SUPPRESS 7
#define SWING          1200

uint8_t swingSuppress = SWING_SUPPRESS * 2;

uint16_t packetSize;
uint8_t fifoBuffer[64];
uint16_t mpuFifoCount;
uint8_t mpuIntStatus;

volatile bool mpuInterrupt = false;

Quaternion quaternion;
VectorInt16 aaWorld;
static Quaternion quaternion_last;
static Quaternion quaternion_reading;
static VectorInt16 aaWorld_last;
static VectorInt16 aaWorld_reading;
unsigned long magnitude = 0;

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

// TOGGLES
//
const bool DEBUG = true;

const bool PLAY_BOOT_SOUND = true;
const bool PLAY_POWER_ON_OFF_SOUND = true;
const bool PLAY_MOVEMENT_SOUNDS = true;

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

const unsigned int SOUND_FONT_MIN = 2;
const unsigned int SOUND_FONT_MAX = 4;

const unsigned int BOOT_SOUND_NUM = 1;
const unsigned int POWER_ON_SOUND_NUM = 2;
const unsigned int POWER_OFF_SOUND_NUM = 3;
const unsigned int HUM_SOUND_NUM = 4;
const unsigned int SWING_SOUND_NUM = 5;
const unsigned int CLASH_SOUND_NUM = 6;

char str[1024];

unsigned int currentSoundFontFolder = 2;
unsigned int movementCount = 0;

bool saberOn = false;

SoftwareSerial SerialMP3(MP3_RX_PIN, MP3_TX_PIN);

unsigned int soundPlaying = 0;

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void setupAuxSwitch() {
  buttonAux.attachClick(auxButtonPress);
  buttonAux.attachLongPressStart(auxButtonLongPress);
}

void setupPowerSwitch() {
  buttonMain.attachClick(mainButtonPress);
  buttonMain.attachLongPressStart(mainButtonLongPress);
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void setupSerial() {
  Serial.begin(115200);
}

void setupAccelerometer() {
  Wire.begin();
  mpu.initialize();
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  mpu.dmpInitialize();

//  Sensor readings with offsets:  2 -9  16372 0 -1  0
//  Your offsets: -762  583 1254  84  -47 87
//
//  Data is printed as: acelX acelY acelZ giroX giroY giroZ
//  Check that your sensor readings are close to 0 0 16384 0 0 0
//  If calibration was succesful write down your offsets so you can set them in your projects using something similar to mpu.setXAccelOffset(youroffset)

  mpu.setXAccelOffset(-762);
  mpu.setYAccelOffset(583);
  mpu.setZAccelOffset(1254);
  
  mpu.setXGyroOffset(84);
  mpu.setYGyroOffset(-47);
  mpu.setZGyroOffset(87);

  mpu.setDMPEnabled(true);

  attachInterrupt(0, dmpDataReady, RISING);
  mpu.getIntStatus();
  packetSize = mpu.dmpGetFIFOPacketSize();
}

void setupMP3() {
  unsigned int currentSoundFont = 2;
  
  pinMode(MP3_BUSY_PIN, INPUT);

  SerialMP3.begin(9600);
  
  mp3_set_serial(SerialMP3);
  delay(10); 
  
  mp3_set_volume(SOUND_VOLUME); 
  delay(10);
  
  mp3_set_EQ(5);
  delay(10);

  currentSoundFont = EEPROM.read(0);

  if (currentSoundFont == 0) {
    currentSoundFont = SOUND_FONT_MIN;
  }

  selectSoundFont(currentSoundFont);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void setLED(int value) {
  digitalWrite(LED_PIN, value);  
}

void fireLED(int d) {
  digitalWrite(LED_PIN, HIGH);
  delay(d);
  digitalWrite(LED_PIN, LOW);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void vPrint(const __FlashStringHelper *str) {
  if (!Serial.available()) { Serial.println(str); }
}

void dPrint(const __FlashStringHelper *str) {
  if (DEBUG) { vPrint(str); }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void selectSoundFont(unsigned int num) {
  currentSoundFontFolder = num;
  EEPROM.write(0, num);
  if (PLAY_BOOT_SOUND) { 
    playSound(BOOT_SOUND_NUM);
  }
}

void repeatHum() {
  vPrint(F("Repeating hum"));
  repeatSound(HUM_SOUND_NUM);
}

void repeatSound(unsigned int num) {
  mp3_repeat_from_folder(num, currentSoundFontFolder);
}

void playSound(unsigned int num) {
  mp3_play_from_folder(num, currentSoundFontFolder);
}

void changeSoundFont() {
  unsigned int newSoundFontFolder = 0;
  if (!saberOn) {
    if (currentSoundFontFolder == SOUND_FONT_MAX) {
      newSoundFontFolder = SOUND_FONT_MIN;
    } else {
      newSoundFontFolder = currentSoundFontFolder + 1;
    }
    selectSoundFont(newSoundFontFolder);
  }
}

void playMovementSound(unsigned int num) {
  if (PLAY_MOVEMENT_SOUNDS) { playSound(num); }
}

void ensureHum() {
  if (saberOn && !isSoundPlaying()) { repeatHum(); }
}

bool isSoundPlaying() {
  if (digitalRead(MP3_BUSY_PIN) == 0) {
    return true;
  } else {
    return false;
  }
}

void waitUntilSoundStopped() {
  delay(150);
  if (isSoundPlaying()) {
    vPrint(F("Waiting for sound to finish.."));
  }
  
  while (isSoundPlaying()) { }
  
  vPrint(F("Sound finished!"));
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void processButtonClicks() {
  buttonMain.tick();
  buttonAux.tick();
}

void mainButtonPress() {
  if (saberOn) {
    vPrint(F("MAIN button press!"));
    playSound(CLASH_SOUND_NUM);
  } else {
    turnSaberOn();    
  }
}

void mainButtonLongPress() {
  turnSaberOff();
}

void auxButtonPress() {
  if (saberOn) {
    vPrint(F("AUX button press!"));
    playSound(SWING_SOUND_NUM);
  } else {
    vPrint(F("Changing sound font.."));
    changeSoundFont();
  }
}

void auxButtonLongPress() {
  enterConfigMode();
}

void enterConfigMode() {
  vPrint(F("Entering config mode!"));
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void processMovements() {      
  if (swingDetected()) {
    vPrint(F("SWING"));
    playMovementSound(SWING_SOUND_NUM);
  }
}

inline void dmpDataReady() {
  if (saberOn) { mpuInterrupt = true; }
}

void captureAccelerometerValues() {
  long multiplier = 100000;
  VectorInt16 aa;
  VectorInt16 aaReal;

  VectorFloat gravity;

  while (!mpuInterrupt && mpuFifoCount < packetSize) { }

  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();
  mpuFifoCount = mpu.getFIFOCount();

  if ((mpuIntStatus & 0x10) || mpuFifoCount == 1024) {
    mpu.resetFIFO();
  } else if (mpuIntStatus & 0x02) {
    while (mpuFifoCount < packetSize)
    
    mpuFifoCount = mpu.getFIFOCount();
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    mpuFifoCount -= packetSize;

    quaternion_last = quaternion_reading;
    aaWorld_last = aaWorld_reading;

    mpu.dmpGetQuaternion(&quaternion_reading, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &quaternion_reading);
    mpu.dmpGetAccel(&aa, fifoBuffer);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &quaternion_reading);

    quaternion.w = quaternion_reading.w * multiplier - quaternion_last.w * multiplier;
    quaternion.x = quaternion_reading.x * multiplier - quaternion_last.x * multiplier;
    quaternion.y = quaternion_reading.y * multiplier - quaternion_last.y * multiplier;
    quaternion.z = quaternion_reading.z * multiplier - quaternion_last.z * multiplier;

    magnitude = 1000 * sqrt(sq(aaWorld.x / 1000) + sq(aaWorld.y / 1000) + sq(aaWorld.z / 1000));
  }
}

bool swingDetected() {
  if (abs(quaternion.w) > SWING and !swingSuppress
      and aaWorld.z < 0
      and abs(quaternion.z) < (9 / 2) * SWING
      and (abs(quaternion.x) > 3 * SWING
          or abs(quaternion.y) > 3 * SWING)
     ) {
       return true;
     } else {
       return false;
     }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void turnSaberOff() {
  if (saberOn) {    
    vPrint(F("Powering OFF.."));
    if (PLAY_POWER_ON_OFF_SOUND) {
      mp3_stop();
      delay(10);
      playSound(POWER_OFF_SOUND_NUM);
    }
    vPrint(F("Powered OFF!"));
    setLED(LOW);
    saberOn = false;
  }
}

void turnSaberOn() {
  if (!saberOn) {
    vPrint(F("Powering ON.."));
    if (PLAY_POWER_ON_OFF_SOUND) { playSound(POWER_ON_SOUND_NUM); }
    vPrint(F("Powered ON!"));
    setLED(HIGH);
    saberOn = true;
  }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void setup() {
  setupSerial();
  vPrint(F("Starting up.."));

  setupLED();
  setupPowerSwitch();
  setupAuxSwitch();
  setupMP3();
  setupAccelerometer();

  t.every(100, ensureHum);
//  t.every(100, processButtonClicks);

  vPrint(F("Ready!"));
}

void loop() { 
  processButtonClicks();
  
  if (saberOn) {
    captureAccelerometerValues();
    processMovements();
  }

  t.update();
}
