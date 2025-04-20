#pragma once
#include <Arduino.h>
#include <SD.h>
#include "SmfSeq.h"
#include "IntervalCheck.h"

// SMF Player class
class SmfPlayer
{
public:
  SmfPlayer(){}

  void begin(HardwareSerial* serial, int rx_pin=-1, int tx_pin=-1);
  bool loop();
  bool load(const char* filename);
  void play();
  void pause();
  void rewind();
  void stop();
  void upVolume();
  void downVolume();
  void setVolume(int volume);
  int  getVolume();
  int  getStatus();

  // 再生終了時のコールバック
  void (*onPlaybackFinished)() = nullptr;
  
  // CH番号をずらす（eVY1のCH0が音声合成専用のため、GM音源として使用する場合CH0を使用しない）
  int chNoOffset = 0;

private:
  int masterVolume = 127; // 音量
  int volumeStep   = 8;   // 音量調整ステップ

 //   HardwareSerial* _serial;
 //   SMF_SEQ_TABLE *pseqTbl;     //SMFシーケンサハンドル
};
