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

int MP3_VOLUME = 15;

const int CLASH_THRESHOLD = 400;
const int SWING_THRESHOLD = 140;

char str[1024];
int playSound = 1;

SoftwareSerial SerialMP3(4, 5); // RX, TX

// mp3/0001.mp3 = poweron
// mp3/0002.mp3 = poweroff
// mp3/0003.mp3 = hum
// mp3/0004.mp3 = swing
// mp3/0005.mp3 = clash

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
  mp3_set_volume(MP3_VOLUME);
  delay(10);
//  mp3_set_EQ(5);
  mp3_play(1);
  delay(1200);
  mp3_repeat_play(3);
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

bool checkDiff(int threshold) {
  if (XAxisValueDiff > threshold || YAxisValueDiff > threshold || ZAxisValueDiff > threshold) {
    sprintf(str, "threshold=%d, X=%d, Y=%d, Z=%d", threshold, XAxisValueDiff, YAxisValueDiff, ZAxisValueDiff);
    Serial.println(str);
    return true;
  } else {
    return false;
  }
}

void resetplaySound() {
  playSound = 1;
  mp3_repeat_play(3);
}

void setup() {
  analogReference(EXTERNAL);
  
  setupSerial();
  Serial.println("Starting up..");
  
  setupLED();
  setupMP3();

  Serial.println("Ready!");
}

void loop() {
  XAxisValue = analogRead(xAxis);
  YAxisValue = analogRead(yAxis);
  ZAxisValue = analogRead(zAxis);

  if (XAxisValueLast > 0) { XAxisValueDiff = abs(XAxisValueLast - XAxisValue); }
  if (YAxisValueLast > 0) { YAxisValueDiff = abs(YAxisValueLast - YAxisValue); }
  if (ZAxisValueLast > 0) { ZAxisValueDiff = abs(ZAxisValueLast - ZAxisValue); }

//  sprintf(str, "X=%d (%d), Y=%d (%d), Z=%d (%d)", XAxisValue, XAxisValueDiff, YAxisValue, YAxisValueDiff, ZAxisValue, ZAxisValueDiff);
//  Serial.println(str);

  if (playSound) {
    if (checkDiff(CLASH_THRESHOLD)) {
      playSound = 0;
      Serial.println("CLASH!");
      mp3_play(5);
      t.after(1000, resetplaySound);
    } else if (checkDiff(SWING_THRESHOLD)) {
      playSound = 0;
      Serial.println("SWING");
      mp3_play(4);
      t.after(800, resetplaySound);
    }
  }
  
  XAxisValueLast = XAxisValue;
  YAxisValueLast = YAxisValue;
  ZAxisValueLast = ZAxisValue;

  delay(100);

  t.update();
}
