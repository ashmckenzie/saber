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

int MP3_VOLUME = 10;

const int CLASH_THRESHOLD = 220;
const int SWING_THRESHOLD = 70;

char str[1024];
int playingSound = 0;

SoftwareSerial SerialMP3(4, 5); // RX, TX

// mp3/0001.wav = poweron
// mp3/0002.wav = poweroff
// mp3/0003.wav = hum
// mp3/0004.wav = swing
// mp3/0005.wav = clash

// advert/0004.wav = swing
// advert/0005.wav = clash

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
  mp3_set_volume(MP3_VOLUME);
  delay(10);
//  mp3_set_EQ(5);
  mp3_play(1);
  delay(1500);
  mp3_repeat_play(5);
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

bool checkDiff(int threshold) {
  if (XAxisValueDiff > threshold && YAxisValueDiff > threshold && ZAxisValueDiff > threshold) {
//  if (XAxisValueDiff > threshold || YAxisValueDiff > threshold || ZAxisValueDiff > threshold) {
    sprintf(str, "threshold=%d, X=%d, Y=%d, Z=%d", threshold, XAxisValueDiff, YAxisValueDiff, ZAxisValueDiff);
    Serial.println(str);
    return true;
  } else {
    return false;
  }
}

void resetplayingSound() {
  playingSound = 0;
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

  if (playingSound != 1 && checkDiff(CLASH_THRESHOLD)) {
    Serial.println("CLASH!");
    mp3_advert_play(5);
    playingSound = 1;
    t.after(1000, resetplayingSound);
  } else if (playingSound == 0 && checkDiff(SWING_THRESHOLD)) {
    Serial.println("SWING");
    mp3_advert_play(4);
    playingSound = 2;
    t.after(100, resetplayingSound);
  }
  
  XAxisValueLast = XAxisValue;
  YAxisValueLast = YAxisValue;
  ZAxisValueLast = ZAxisValue;

  delay(50);

  t.update();
}
