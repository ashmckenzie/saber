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
int MP3_BUSY_PIN = 12;
int POWER_PIN = 8;

int MP3_VOLUME = 20;

const int CLASH_THRESHOLD = 200;
const int SWING_THRESHOLD = 20;

const int MOVEMENT_COUNT_THRESHOLD = 2;

const int DEFAULT_DELAY_FOR_SABER_OFF = 500;
const int DEFAULT_DELAY_FOR_SABER_ON = 10;
const int POWER_STATE_COUNT_THRESHOLD_WHEN_ON = 30;
const int POWER_STATE_COUNT_THRESHOLD_WHEN_OFF = 1;

char str[1024];

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

// 1 - boot
// 2 - power on
// 3 - power off
// 4 - hum
// 5 - swing
// 6 - clash

SoftwareSerial SerialMP3(4, 5); // RX, TX

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

  if (playBootSound) {
    mp3_play_physical(1);
    delay(1890);
  }
}

void setupPowerSwitch() {
  pinMode(POWER_PIN, INPUT);
  digitalWrite(POWER_PIN, HIGH);
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

bool checkDiff(int threshold, int style) {
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

void playSound(int num) {
  if (!Serial.available()) {
    sprintf(str, "X=%d (%d), Y=%d (%d), Z=%d (%d)", XAxisValue, XAxisValueDiff, YAxisValue, YAxisValueDiff, ZAxisValue, ZAxisValueDiff);
    Serial.println(str);
  }
  playingSound = true;
  mp3_play_physical(num);
}

void repeatHum() {
  mp3_repeat_play(4);
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void setup() {
  analogReference(EXTERNAL);
  
  setupSerial();

  if (!Serial.available()) { Serial.println("Starting up.."); }

  setupLED();
  setupPowerSwitch();
  setupMP3();

  if (!Serial.available()) { Serial.println("Ready!"); }
}

void loop() {
  powerButtonState = digitalRead(POWER_PIN);

//  if (!Serial.available()) {
//    sprintf(str, "powerButtonState=%d, saberOn=%d, delayFor=%d", powerButtonState, saberOn, delayFor);
//    Serial.println(str);
//  }

  if (!powerButtonState) {
    if (powerButtonStateCount >= powerButtonStateCountThreshold) {      
      if (saberOn) {
        // Turn OFF
        saberOn = 0;
        delayFor = DEFAULT_DELAY_FOR_SABER_OFF;
        powerButtonStateCountThreshold = POWER_STATE_COUNT_THRESHOLD_WHEN_OFF;
        if (playPowerSounds) {
          mp3_play_physical(3);
          delay(1000);
          mp3_stop();
        }
      } else {
        // Turn ON
        saberOn = 1;        
        delayFor = DEFAULT_DELAY_FOR_SABER_ON;
        powerButtonStateCountThreshold = POWER_STATE_COUNT_THRESHOLD_WHEN_ON;        
        if (playPowerSounds) {
          mp3_play_physical(2);
          delay(1000);
          repeatHum();
        }
      }      
      powerButtonStateCount = 0;      
    } else {
      powerButtonStateCount++;
    }
  }
  
  if (saberOn) {
    
    XAxisValue = analogRead(xAxis);
    YAxisValue = analogRead(yAxis);
    ZAxisValue = analogRead(zAxis);
  
    if (XAxisValueLast > 0) { XAxisValueDiff = abs(XAxisValueLast - XAxisValue); }
    if (YAxisValueLast > 0) { YAxisValueDiff = abs(YAxisValueLast - YAxisValue); }
    if (ZAxisValueLast > 0) { ZAxisValueDiff = abs(ZAxisValueLast - ZAxisValue); }
  

    if (!playingSound) {
      if (checkDiff(CLASH_THRESHOLD, 1)) {
        if (!Serial.available()) { Serial.println("CLASH!"); }
        if (playMovementSounds) {
          playSound(6);
          t.after(1060, resetplayingSound);
        }
      } else if (checkDiff(SWING_THRESHOLD, 0)) {
        if (!Serial.available()) { Serial.println("SWING"); }
        if (playMovementSounds) {
          playSound(5);
          t.after(600, resetplayingSound);
        }
      }
    }   
  }

  XAxisValueLast = XAxisValue;
  YAxisValueLast = YAxisValue;
  ZAxisValueLast = ZAxisValue;
  
  delay(delayFor);
  t.update();
}
