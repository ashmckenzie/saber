#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>
#include "Timer.h"

const int xAxis = A0;
const int yAxis = A1;
const int zAxis = A2;
int XAxisValue = 0;
int YAxisValue = 0;
int ZAxisValue = 0;
int XAxisValueLast = 0;
int YAxisValueLast = 0;
int ZAxisValueLast = 0;
int XAxisValueDiff = 0;
int YAxisValueDiff = 0;
int ZAxisValueDiff = 0;

Timer t;

int LED_PIN = 13;
int SERIAL_RX_PIN = 4;
int SERIAL_TX_PIN = 5;
int MP3_BUSY_PIN = 12;
int POWER_ON_OFF_PIN = 8;
int SELECT_SOUND_FONT_PIN = 9;

int MP3_VOLUME = 5;

const int MAX_SOUND_FONT_FOLDERS = 2;
const int SOUNDS_PER_SOUND_FONT_FOLDER = 6;

const int CLASH_THRESHOLD = 300;
const int SWING_THRESHOLD = 20;

const int MOVEMENT_COUNT_THRESHOLD = 4;

const int DEFAULT_DELAY_FOR_SABER_OFF = 250;
const int DEFAULT_DELAY_FOR_SABER_ON = 10;
const int POWER_STATE_COUNT_THRESHOLD_WHEN_ON = 30;
const int POWER_STATE_COUNT_THRESHOLD_WHEN_OFF = 1;

const int BOOT_SOUND_NUM = 1;
const int POWER_ON_SOUND_NUM = 2;
const int POWER_OFF_SOUND_NUM = 3;
const int HUM_SOUND_NUM = 4;
const int SWING_SOUND_NUM = 5;
const int CLASH_SOUND_NUM = 6;

char str[1024];

int currentSoundFontFolder = 0;

int movementCount = 0;

int playBootSound = 1;
int playPowerSounds = 1;
int playMovementSounds = 1;

int saberOn = 0;
bool playingSound = false;
int powerButtonState = 0;
int powerButtonStateCount = 0;
int powerButtonStateCountThreshold = POWER_STATE_COUNT_THRESHOLD_WHEN_OFF;
int delayFor = DEFAULT_DELAY_FOR_SABER_OFF;

int selectSoundFontButtonState = 0;
int selectSoundFontButtonStateCount = 0;
int selectSoundFontButtonStateCountThreshold = 6;

// Black Star
//
// 1 - boot
// 2 - power on
// 3 - power off
// 4 - hum
// 5 - swing
// 6 - clash

// EP IV
//
// 7  - boot
// 8  - power on
// 9  - power off
// 10  - hum
// 11 - swing
// 12 - clash

SoftwareSerial SerialMP3(SERIAL_RX_PIN, SERIAL_TX_PIN);

void selectSoundFont(int num) {
  if (num >= MAX_SOUND_FONT_FOLDERS) { num = 0; }
  currentSoundFontFolder = num;
  if (playBootSound) { playSound(BOOT_SOUND_NUM, 1890); }
}

void fireLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

void setupSerial() {
  Serial.begin(115200);
}

void setupMP3() {
  pinMode(MP3_BUSY_PIN, INPUT);
  SerialMP3.begin(9600);
  mp3_set_serial(SerialMP3);
  delay(10);
  
  mp3_set_device(1);
  delay(10);
  
  mp3_set_volume(MP3_VOLUME);
  delay(10);
  
  mp3_set_EQ(5);
  delay(10);

  selectSoundFont(currentSoundFontFolder);
}

void playSound(int num, int del) {
  num = num + (SOUNDS_PER_SOUND_FONT_FOLDER * currentSoundFontFolder);
  mp3_play_physical(num);
  if (del > 0) { delay(del); }
}

void setupPowerSwitch() {
  pinMode(POWER_ON_OFF_PIN, INPUT);
  digitalWrite(POWER_ON_OFF_PIN, HIGH);
}

void setupChangeSoundFontSwitch() {
  pinMode(SELECT_SOUND_FONT_PIN, INPUT);
  digitalWrite(SELECT_SOUND_FONT_PIN, HIGH);
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

bool checkDiff(int threshold, int style) {
  //
  // 0 = all
  // 1 = any
  //
  bool movement = false;

  if (style == 0) {
    if (XAxisValueDiff > threshold && YAxisValueDiff > threshold && ZAxisValueDiff > threshold) { movement = true; }
  } else if (style == 1) {
    if (XAxisValueDiff > threshold || YAxisValueDiff > threshold || ZAxisValueDiff > threshold) { movement = true; }
  }
  
  if (movement) {
    movementCount++;
    if (style == 1 || movementCount >= MOVEMENT_COUNT_THRESHOLD) { 
      movementCount = 0;
      return true;
    }
  }
  
  return false;
}

void resetplayingSound() {  
  repeatHum();
  playingSound = false;
}

void playMovementSound(int num, int d) {
  if (!Serial.available()) {
    sprintf(str, "X=%d (%d), Y=%d (%d), Z=%d (%d)", XAxisValue, XAxisValueDiff, YAxisValue, YAxisValueDiff, ZAxisValue, ZAxisValueDiff);
    Serial.println(str);
  }

  if (playMovementSounds) {
    playingSound = true;
    playSound(num, 0);  
    t.after(d, resetplayingSound);
  }
}

void repeatHum() {
  int num = HUM_SOUND_NUM + (SOUNDS_PER_SOUND_FONT_FOLDER * currentSoundFontFolder);
  mp3_repeat_play(num);
}

void processPowerButton() {
  powerButtonState = digitalRead(POWER_ON_OFF_PIN);

  if (!powerButtonState) {
    if (powerButtonStateCount >= powerButtonStateCountThreshold) {      
      if (saberOn) {
        // Turn OFF
        saberOn = 0;
        delayFor = DEFAULT_DELAY_FOR_SABER_OFF;
        powerButtonStateCountThreshold = POWER_STATE_COUNT_THRESHOLD_WHEN_OFF;
        if (playPowerSounds) {
          playSound(POWER_OFF_SOUND_NUM, 1000);
          mp3_stop();
        }
        if (!Serial.available()) { Serial.println("Power OFF!"); }
      } else {
        // Turn ON
        saberOn = 1;        
        delayFor = DEFAULT_DELAY_FOR_SABER_ON;
        powerButtonStateCountThreshold = POWER_STATE_COUNT_THRESHOLD_WHEN_ON;        
        if (playPowerSounds) {
          playSound(POWER_ON_SOUND_NUM, 1000);
          repeatHum();
        }
        if (!Serial.available()) { Serial.println("Power ON!"); }
      }      
      powerButtonStateCount = 0;      
    } else {
      powerButtonStateCount++;
    }
  }
}

void processChangeSoundFont() {
  selectSoundFontButtonState = digitalRead(SELECT_SOUND_FONT_PIN);

  if (!selectSoundFontButtonState) {
    if (selectSoundFontButtonStateCount >= selectSoundFontButtonStateCountThreshold) {      
      if (!Serial.available()) { Serial.println("Changing sound font!"); }
      selectSoundFont(currentSoundFontFolder + 1);
      selectSoundFontButtonStateCount = 0;      
    } else {
      selectSoundFontButtonStateCount++;
    }
  }
}

void processMovements() {      
  if (XAxisValueLast > 0) { XAxisValueDiff = abs(XAxisValueLast - XAxisValue); }
  if (YAxisValueLast > 0) { YAxisValueDiff = abs(YAxisValueLast - YAxisValue); }
  if (ZAxisValueLast > 0) { ZAxisValueDiff = abs(ZAxisValueLast - ZAxisValue); }

  if (!playingSound) {
    if (checkDiff(CLASH_THRESHOLD, 1)) {
      if (!Serial.available()) { Serial.println("CLASH!"); }
      playMovementSound(CLASH_SOUND_NUM, 1060);
    } else if (checkDiff(SWING_THRESHOLD, 0)) {
      if (!Serial.available()) { Serial.println("SWING"); }
       playMovementSound(SWING_SOUND_NUM, 600);      
    }
  }   
}

void captureAccelerometerValues() {
  XAxisValue = analogRead(xAxis);
  YAxisValue = analogRead(yAxis);
  ZAxisValue = analogRead(zAxis);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void setup() {
  analogReference(EXTERNAL);
  
  setupSerial();

  if (!Serial.available()) { Serial.println("Starting up.."); }

  setupLED();
  setupPowerSwitch();
  setupChangeSoundFontSwitch();
  setupMP3();

  if (!Serial.available()) { Serial.println("Ready!"); }
}

void loop() {
  captureAccelerometerValues();

  processPowerButton();
  
  if (saberOn) {
    processMovements();
  } else {
    processChangeSoundFont();  
  }

  XAxisValueLast = XAxisValue;
  YAxisValueLast = YAxisValue;
  ZAxisValueLast = ZAxisValue;
  
  delay(delayFor);
  t.update();
}
