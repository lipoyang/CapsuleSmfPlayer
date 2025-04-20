/***********************************************************************
<ユーザ設定関数>

 <csrc.c>

                    catsin
        MIDIシーケンサユーザ設定関数
***********************************************************************/
#include "MidiFunc.h"
#include "MidiPort.h" //MIDIポートアクセス関数定義

int midiOutOpen(){
  int            Status;

  /* オプションポートをオープン */
  /* オプションの値（下記コメントのかっこ内はその設定値）は適宜変更してください */
  /* /B 転送速度       (31250bps) */
  /* /P パリティ       (なし)     */
  /* /L データ長       (8ビット)  */
  /* /S ストップビット (1ビット)  */
  /* /C ハードフロー   (行わない) */
  /* /X XON/XOFFフロー (行わない) */
  /* 他のオプションはデフォルト値を使用 */
  MidiPort_open();
  
  /* エラー処理（アラートボックスの表示）*/
  Status = MIDI_OK;

  return( Status );
}

int midiOutClose(){
  int            Status;

  Status = MIDI_OK;

  /* オプションポートのクローズ */
  MidiPort_close();
    
  return( Status );
}

int midiOutShortMsg( UCHAR status, UCHAR data1, UCHAR data2 ){
  int          Status;
  UCHAR        Buf[3];
  ULONG        Len;

  Status = MIDI_OK;

  Buf[0] = status;
  Buf[1] = data1;
  Buf[2] = data2;

  Len = 3;
  MidiPort_writeBuffer( Buf, Len );

  return( Status );
}

int midiOutLongMsg( UCHAR *Buf, ULONG Len ){
  int            Status;
  static const UCHAR GMResetData[]  = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
  static const UCHAR GMVolumeData[] = {0xF0, 0x7F, 0x7F, 0x04, 0x01/*, 0x00, volume, 0xF7*/};
  static const UCHAR GSResetData[]  = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7};
  static const UCHAR GSVolumeData[] = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x04/*, volume, checksum, 0xF7*/};
  static const UCHAR XGResetData[]  = {0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7};
  static const UCHAR XGVolumeData[] = {0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x04/*, volume, 0xF7*/};
  static const UCHAR *IGNORE_DATA[] = {
    GMResetData, GMVolumeData, GSResetData, GSVolumeData, XGResetData, XGVolumeData
  };
  static const int IGNORE_DATA_SIZE[] = {
    sizeof(GMResetData), sizeof(GMVolumeData),
    sizeof(GSResetData), sizeof(GSVolumeData),
    sizeof(XGResetData), sizeof(XGVolumeData)
  };

  Status = MIDI_OK;

  if(Buf[0] == 0xF0){
    #ifdef DEBUG_LOG
    for(int i=0; i<Len; i++){ DPRINTF("%02X ", Buf[i]); }
    DPRINT("\n");
    #endif
    for(int i=0; i<sizeof(IGNORE_DATA)/sizeof(IGNORE_DATA[0]); i++){
      if(memcmp( Buf, IGNORE_DATA[i], IGNORE_DATA_SIZE[i] ) == 0 ){
        DPRINTF("SysEx Passed! %d\n", i);
        return MIDI_OK;
      }
    }
  }

  MidiPort_writeBuffer( Buf, Len );

  return( Status );
}

int midiOutGMReset(){
  int          Status;
  UCHAR        GMResetData[] = {0xF0,0x7E,0x7F,0x09,0x01,0xF7};

  Status = MIDI_OK;

  MidiPort_writeBuffer( (UCHAR *)GMResetData, (ULONG)sizeof(GMResetData) );

  return( Status );
}

int midiOutSetMaterVolume(UCHAR volume){
  int          Status;
  UCHAR        GMVolumeData[] = {0xF0, 0x7F, 0x7F, 0x04, 0x01, 0x00, volume, 0xF7};

  Status = MIDI_OK;

  MidiPort_writeBuffer( (UCHAR *)GMVolumeData, (ULONG)sizeof(GMVolumeData) );

  return( Status );
}
