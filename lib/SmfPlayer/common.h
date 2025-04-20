#ifndef DEF_COMMON_H
#define DEF_COMMON_H

#include <Arduino.h>
#include <SD.h>

typedef unsigned char UCHAR;  //1バイト符号なしデータ（SMFファイルアクセスに使用）
typedef unsigned long ULONG;  //4バイト符号なしデータ（SMFファイルアクセスに使用）

#define ZTICK           (1)        //[ms]定期更新処理起動間隔
#define MIDI_PORT_BPS   (31250)    //標準MIDI

//#define DEBUG_LOG
#ifdef  DEBUG_LOG
#define DPRINT      Serial.print
#define DPRINTLN    Serial.println
#define DPRINTF     Serial.printf
#else
#define DPRINT
#define DPRINTLN
#define DPRINTF
#endif

#define ERROR_LOG
#ifdef  ERROR_LOG
#define EPRINT      Serial.print
#define EPRINTLN    Serial.println
#define EPRINTF     Serial.printf
#else
#define EPRINT
#define EPRINTLN
#define EPRINTF
#endif

#define INFO_LOG
#ifdef  INFO_LOG
#define IPRINT      Serial.print
#define IPRINTLN    Serial.println
#define IPRINTF     Serial.printf
#else
#define IPRINT
#define IPRINTLN
#define IPRINTF
#endif

#endif  //DEF_COMMON_H
