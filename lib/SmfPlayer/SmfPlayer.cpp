#include "SmfPlayer.h"
#include "SmfSeq.h"
#include "IntervalCheckMicros.h"

#define D_PLAY_SPEED_RATE   (1.0) // 演奏スピードの調整レート

SMF_SEQ_TABLE  *pSeqTbl;    // SMFシーケンサハンドル
HardwareSerial *pSerial;    // MIDI用シリアルポート

// 処理周期
IntervalCheckMicros sTickProcInterval( (int)((double)(ZTICK * 1000) * D_PLAY_SPEED_RATE), true );

// 初期化する
void SmfPlayer::begin(HardwareSerial* serial, int rx_pin, int tx_pin)
{
    // クローズしてオープンし直すとMIDIメッセージ送信がおかしくなるので、
    // 一度オープンしたらそのまま使い続ける
    pSerial = serial;
    if (rx_pin != -1 && tx_pin != -1) {
        pSerial->begin(MIDI_PORT_BPS, SERIAL_8N1, rx_pin, tx_pin);
    } else {
        pSerial->begin(MIDI_PORT_BPS); // use default pins
    }
}

// メインループから呼び出す関数
bool SmfPlayer::loop()
{
    int Ret;

    // 周期処理
    if( sTickProcInterval.check() == true ){
        if( SmfSeqGetStatus( pSeqTbl ) != SMF_STAT_STOP ){
            //状態が演奏停止中以外の場合
            //定期処理を実行
            bool need_delay = false;
            Ret = SmfSeqTickProc( pSeqTbl );   
            // 間に合っていない場合のリカバリ、追いつくまで進める
            while(sTickProcInterval.check() == true)
            {
              Ret = SmfSeqTickProc( pSeqTbl );   
            }
            
            // 再生終了したとき
            if( SmfSeqGetStatus( pSeqTbl )  == SMF_STAT_STOP )
            {
              // ファイルクローズ
              SmfSeqEnd( pSeqTbl );
              
              if(onPlaybackFinished != nullptr) onPlaybackFinished();
            }
        }
        return true;
    }else{
        return false;
    }
}

// SMFファイルを読み込む
// filename: SMFファイル名（SDカード上のパス）
// 戻り値: true=成功、false=失敗
bool SmfPlayer::load(const char* filename)
{
    int Ret;

    // SMFシーケンサハンドルの初期化
    pSeqTbl = SmfSeqInit( ZTICK );
    if ( pSeqTbl == NULL ) return false;
  
    //SMFファイル読込
    SmfSeqFileLoadWithChNoOffset( pSeqTbl, (char*)filename, chNoOffset );
    //GMリセット送信
    Ret = SmfSeqGMReset();
    Ret = SmfSeqSetVolume(masterVolume);

    return true;
}

// 再生開始/再開
void SmfPlayer::play()
{
    SmfSeqStart( pSeqTbl );
}

// 一時停止
void SmfPlayer::pause()
{
    if( getStatus() != SMF_STAT_STOP ){
        SmfSeqStop( pSeqTbl );
    }

    // 発音中全キーノートオフ
//  int Ret = SmfSeqAllNoteOff( pSeqTbl );
}

// 一時停止してから先頭に戻る
void SmfPlayer::rewind()
{
    pause();

    // トラック初期化(頭出し)
    int Ret = SmfSeqInitTrkTbl( pSeqTbl );
}

// 再生を停止する
void SmfPlayer::stop()
{
    pause();

    //トラックテーブルリセット
//  SmfSeqPlayResetTrkTbl( pSeqTbl );

    // ファイルクローズ
    SmfSeqEnd( pSeqTbl );

    for(int x=0; x<5; x++)
    {
      SmfSeqTickProc( pSeqTbl );
      delay(100);
    }
}

// 音量を上げる
void SmfPlayer::upVolume()
{
    masterVolume += volumeStep;
    if( masterVolume > 127 ) {
      masterVolume = 127;
    }
    setVolume(masterVolume);
}

// 音量を下げる
void SmfPlayer::downVolume()
{
    if( masterVolume == 127) masterVolume = 128;
    
    masterVolume -= volumeStep;
    if( masterVolume < 0 ) {
      masterVolume = 0;
    }
    setVolume(masterVolume);
}

// 音量を設定する
// volume: 0-127の範囲で指定
void SmfPlayer::setVolume(int volume)
{
    if (volume < 0) {
        volume = 0;
    } else if (volume > 127) {
        volume = 127;
    }
    masterVolume = volume;

    int Ret = SmfSeqSetVolume(masterVolume);
}

// 音量を取得する
// 戻り値: 0-127の範囲で返す
int SmfPlayer::getVolume()
{
    return masterVolume;
}

// シーケンサの状態を取得する
// 戻り値: SMF_STAT_STOP, SMF_STAT_PLAY, SMF_STAT_PAUSE, SMF_STAT_WAITのいずれか
int SmfPlayer::getStatus()
{
    return SmfSeqGetStatus( pSeqTbl );
}
