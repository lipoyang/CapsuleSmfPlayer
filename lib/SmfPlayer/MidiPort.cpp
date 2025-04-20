//MIDIポートアクセス関数定義
// MidiFunc.c/hから呼び出されるMIDI I/Fアクセス関数の実体を記述する。
// 以下ではハードウェアシリアルを使用した場合の例を記述。
#include "MidiPort.h"

// MIDI用シリアルポート
extern HardwareSerial *pSerial;
// シリアル送信バッファ
static QueueHandle_t s_midi_port_output_queue;

void midiPortOutputTask(void * arg)
{
  while(1)
  {
    unsigned char data;
    while(xQueueReceive(s_midi_port_output_queue, &data, 0) == pdTRUE)
    {
      while(pSerial->write( data ) == 0)
      {
        // もし書き込みが失敗していたらリトライ
        vTaskDelay(1);
        EPRINTLN("MIDI OUT:Write failed.");
      }
    }
    vTaskDelay(1);
  }
}

bool  s_midiPortOpen = false;
int   MidiPort_open()
{
  if(s_midiPortOpen == false)
  {
    s_midi_port_output_queue = xQueueCreate(1024, sizeof(unsigned char));
    xTaskCreatePinnedToCore(midiPortOutputTask, "midiPortOutputTask", 2048, NULL, 1, NULL, PRO_CPU_NUM);
    s_midiPortOpen = true;
  }
  return(0);
}

void  MidiPort_close()
{
// ここではクローズしない
//  pSerial->end();
}

int   MidiPort_write( UCHAR data )
{
  if(s_midiPortOpen == true)
  {
    // もしキューバッファがいっぱいの場合、空くまで待つ。通常の演奏では溢れることはない
    int ret = xQueueSend(s_midi_port_output_queue, &data, portMAX_DELAY );
    if(ret != pdTRUE)
    {
      EPRINTLN("MIDI OUT:Queue buffer overflow.");
    }
  }
  return( 1 );
}

int   MidiPort_writeBuffer( UCHAR * pData, ULONG Len )
{
  UCHAR size = Len;
  if(s_midiPortOpen == true)
  {
    for( int i=0; i<Len; i++ )
    {
      // もしキューバッファがいっぱいの場合、空くまで待つ。通常の演奏では溢れることはない
      int ret = xQueueSend(s_midi_port_output_queue, &pData[i], portMAX_DELAY );
      if(ret != pdTRUE)
      {
        EPRINTLN("MIDI OUT:Queue buffer overflow.");
      }
    }
  }
  return( Len );
}


