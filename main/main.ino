#include <Arduino.h>

#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>

#include <OneButton.h>
#include <EEPROM.h>
//
//unsigned int XAxisValue = 0;
//unsigned int YAxisValue = 0;
//unsigned int ZAxisValue = 0;
//unsigned int XAxisValueLast = 0;
//unsigned int YAxisValueLast = 0;
//unsigned int ZAxisValueLast = 0;
//unsigned int XAxisValueDiff = 0;
//unsigned int YAxisValueDiff = 0;
//unsigned int ZAxisValueDiff = 0;

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
const int SOUND_VOLUME = 20;
 
const unsigned int SWING_THRESHOLD = 80;
const unsigned int SWING_MOVEMENT_COUNT_THRESHOLD = 2;

const unsigned int CLASH_THRESHOLD = 200;
const unsigned int CLASH_MOVEMENT_COUNT_THRESHOLD = 2;

//const unsigned int DEFAULT_DELAY_FOR_SABER_OFF = 250;
//const unsigned int DEFAULT_DELAY_FOR_SABER_ON = 100;
const unsigned int DEFAULT_DELAY_FOR_SABER_OFF = 10;
const unsigned int DEFAULT_DELAY_FOR_SABER_ON = 10;

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
bool playingMovementSound = false;
unsigned int delayFor = DEFAULT_DELAY_FOR_SABER_OFF;

SoftwareSerial SerialMP3(MP3_RX_PIN, MP3_TX_PIN);

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void fireLED(int d) {
  digitalWrite(LED_PIN, HIGH);
  delay(d);
  digitalWrite(LED_PIN, LOW);
}

void setLED(int value) {
  digitalWrite(LED_PIN, value);  
}

void setupSerial() {
  Serial.begin(115200);
}

void setupAccelerometer() {
//  analogReference(EXTERNAL);
}

void setupMP3() {
  unsigned int currentSoundFont = 2;
  
  pinMode(MP3_BUSY_PIN, INPUT);

  SerialMP3.begin(9600);
  
  mp3_set_serial(SerialMP3);
  delay(10); 
  
  mp3_set_volume(SOUND_VOLUME); 
  delay(10);
  
//  mp3_set_EQ(5);
//  delay(10);

  currentSoundFont = EEPROM.read(0);

  if (currentSoundFont == 0) {
    currentSoundFont = SOUND_FONT_MIN;
  }

  selectSoundFont(currentSoundFont);
}

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

bool playingSound() {
  if (digitalRead(MP3_BUSY_PIN) == 0) {
    return true;
  } else {
    return false;
  }
}

void waitUntilSoundStopped() {
  delay(100);
  if (playingSound()) {
    vPrint(F("Waiting for sound to finish.."));
  }
  
  while (playingSound()) { }
  
  vPrint(F("Sound finished!"));
}

void setupPowerSwitch() {
  buttonMain.attachClick(mainButtonPress);
  buttonMain.attachLongPressStart(mainButtonLongPress);
}

void mainButtonPress() {
  if (saberOn) {
    vPrint(F("MAIN button press!"));
  } else {
    turnSaberOn();    
  }
}

void mainButtonLongPress() {
  turnSaberOff();
}

void setupAuxSwitch() {
  buttonAux.attachClick(auxButtonPress);
  buttonAux.attachLongPressStart(auxButtonLongPress);
}

void auxButtonPress() {
  if (saberOn) {
    vPrint(F("AUX button press!"));
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

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

//bool swingDetected() {
////  checkForMovement(SWING_THRESHOLD, 1, SWING_MOVEMENT_COUNT_THRESHOLD)
//
//  bool movement = false;
//
//  if (XAxisValueDiff > SWING_THRESHOLD) { movement = true; }
//
//  if (movement) {
//    movementCount++;
//    if (movementCount >= SWING_MOVEMENT_COUNT_THRESHOLD) { 
//      movementCount = 0;
//      return true;
//    }
//  }
//  
//  return false;
//}

//bool clashDetected() {
////  if (checkForMovement(CLASH_THRESHOLD, 1, CLASH_MOVEMENT_COUNT_THRESHOLD)) {
//
//  bool movement = false;
//
//  if (XAxisValueDiff > CLASH_THRESHOLD && YAxisValueDiff > CLASH_THRESHOLD) { movement = true; }
//
//  if (movement) {
//    movementCount++;
//    if (movementCount >= CLASH_MOVEMENT_COUNT_THRESHOLD) { 
//      movementCount = 0;
//      return true;
//    }
//  }
//  
//  return false;
//}

//bool checkForMovement(int threshold, int style, int movementCountThreshold) {
//  //
//  // 1 = all
//  // 2 = any
//  // 3 = all except Z
//  // 4 = any except Z
//  // 5 = just Z
//  //
//  bool movement = false;
//
//  if (style == 1) {
//    if (XAxisValueDiff > threshold && YAxisValueDiff > threshold && ZAxisValueDiff > threshold) { movement = true; }
//  } else if (style == 2) {
//    if (XAxisValueDiff > threshold || YAxisValueDiff > threshold || ZAxisValueDiff > threshold) { movement = true; }
//  } else if (style == 3) {
//    if (XAxisValueDiff > threshold || YAxisValueDiff > threshold) { movement = true; }
//  } else if (style == 4) {
//    if (XAxisValueDiff > threshold && YAxisValueDiff > threshold) { movement = true; }
//  } else if (style == 5) {
//    if (ZAxisValueDiff > threshold) { movement = true; }
//  }
//  
//  if (movement) {
//    movementCount++;
//    if (movementCount >= movementCountThreshold) { 
//      movementCount = 0;
//      return true;
//    }
//  }
//  
//  return false;
//}

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
//  sprintf(str, "X=%d (%d), Y=%d (%d), Z=%d (%d)", XAxisValue, XAxisValueDiff, YAxisValue, YAxisValueDiff, ZAxisValue, ZAxisValueDiff);
//  vPrint(str);

  if (PLAY_MOVEMENT_SOUNDS) {
    playingMovementSound = true;
    playSound(num);  
    
    resetPlayingMovementSound();
  }
}

void resetPlayingMovementSound() {  
  repeatHum();
  playingMovementSound = false;
}

void turnSaberOff() {
  if (saberOn) {
    saberOn = false;
    vPrint(F("Powering OFF.."));
    if (PLAY_POWER_ON_OFF_SOUND) {
      playSound(POWER_OFF_SOUND_NUM);
    }
    vPrint(F("Powered OFF!"));
    setLED(LOW);
  }
}

void turnSaberOn() {
  if (!saberOn) {
    saberOn = true;
    vPrint(F("Powering ON.."));
    if (PLAY_POWER_ON_OFF_SOUND) {
      playSound(POWER_ON_SOUND_NUM);
    }
    vPrint(F("Powered ON!"));
    setLED(HIGH);
    waitUntilSoundStopped();
    repeatHum();
  }
}

//void processMovements() {      
//  if (XAxisValueLast > 0) { XAxisValueDiff = abs(XAxisValueLast - XAxisValue); }
//  if (YAxisValueLast > 0) { YAxisValueDiff = abs(YAxisValueLast - YAxisValue); }
//  if (ZAxisValueLast > 0) { ZAxisValueDiff = abs(ZAxisValueLast - ZAxisValue); }
//
////  sprintf(str, "X=%d (%d), Y=%d (%d), Z=%d (%d)", XAxisValue, XAxisValueDiff, YAxisValue, YAxisValueDiff, ZAxisValue, ZAxisValueDiff);
////  dPrint(str);
//  
//  if (!playingMovementSound) {
////    if (clashDetected()) {
////      vPrint("CLASH!");
//////      playMovementSound(CLASH_SOUND_NUM);
////      playMovementSound(CLASH_SOUND_NUM);
////    } else 
//    if (swingDetected()) {
//      vPrint("SWING");
//      playMovementSound(SWING_SOUND_NUM);
//    }
//  }
//}

void processButtonClicks() {
  buttonMain.tick();
  buttonAux.tick();
}

//void captureAccelerometerValues() {
//  XAxisValue = analogRead(XAXIS_PIN);
//  YAxisValue = analogRead(YAXIS_PIN);
//  ZAxisValue = analogRead(ZAXIS_PIN);
//}

void vPrint(const __FlashStringHelper *str) {
  if (!Serial.available()) { Serial.println(str); }
}

void dPrint(const __FlashStringHelper *str) {
  if (DEBUG) { vPrint(str); }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void setup() {
  setupSerial();
  vPrint(F("Starting up.."));

  setupLED();
  setupMP3();
//  setupAccelerometer();
  setupPowerSwitch();
  setupAuxSwitch();

  vPrint(F("Ready!"));
}

void loop() {  
  processButtonClicks();

//  captureAccelerometerValues();
  
//  if (saberOn) {
//    processMovements();
//  }
//
//  XAxisValueLast = XAxisValue;
//  YAxisValueLast = YAxisValue;
//  ZAxisValueLast = ZAxisValue;
  
  delay(delayFor);
}
