// SMF(Standard Midi File) Player for M5Capsule
// plays SMF files "playdat0.mid" - "playdat9.mid" in the SD card

// [NOTE]
// 5V pin of Grove connector is modified. 
// through 5Vin instead of 5Vout.

#include <M5Unified.h>
#include <SD.h>
#include <EspEasyLED.h>
#include "Button.h"
#include "SmfPlayer.h"

// Pin assignments
#define PIN_RX           GPIO_NUM_15
#define PIN_TX           GPIO_NUM_13
#define PIN_BUTTON_PREV  GPIO_NUM_1
#define PIN_BUTTON_NEXT  GPIO_NUM_3
#define PIN_BUTTON_UP    GPIO_NUM_5
#define PIN_BUTTON_DOWN  GPIO_NUM_7
#define PIN_BUTTON_PLAY  GPIO_NUM_42 // It's the button on M5Capsule
#define PIN_RGBLED       GPIO_NUM_9  // It's not the LED on M5Capsule
#define SERIAL_MIDI      Serial2

// SMFプレイヤー
SmfPlayer smfPlayer;

// ボタン
Button btnPlay;   // 再生/停止ボタン (本体ボタン)
Button btnPrev;   // << ボタン
Button btnNext;   // >> ボタン
Button btnUp;     // + ボタン
Button btnDown;   // - ボタン
// ステータスLED
EspEasyLED *rgbled;

// 周期処理用
IntervalCheck sButtonCheckInterval( 100, true );  // ボタンチェック周期
IntervalCheck sUpdateScreenInterval( 500, true ); // 画面更新周期

// ボリューム初期値 (0-127)
const int INIT_VOLUME = 64;

// SMFファイルのディレクトリとファイル名
char dirname[20] = {0};
char filename[100] = {0};

//SMFファイル名生成
// playdat0.mid～playdat9.midの文字列を順次返す。
#define D_PLAY_NEXT (0)         //次の曲
#define D_PLAY_PREV (1)         //前の曲
#define D_PLAY_DATA_NUM     10  //SDカード格納ＳＭＦファイル数
char * makeFilename(int next_prev = D_PLAY_NEXT)
{
  static int playdataCnt = -1; //選曲番号

  int cnt = 0;
  for( cnt=0; cnt < D_PLAY_DATA_NUM; cnt++ ){
    if( next_prev == D_PLAY_NEXT ){
      playdataCnt++;
      if( playdataCnt >= D_PLAY_DATA_NUM ){
        playdataCnt = 0;
      }
    } else {
      playdataCnt--;
      if( playdataCnt < 0 ){
        playdataCnt = D_PLAY_DATA_NUM - 1;
      }
    }
    sprintf(filename,"%s/playdat%01d.mid", dirname, playdataCnt);
    if( SD.exists(filename) == true ){
      break;
    }
  }
  return( filename );
}

// 表示更新
// ※ M5Capsuleには画面がないので、とりあえずUSBシリアルに出力
void updateScreen()
{
  static String last_filename = "";
  static int    last_status = -1;
  static String status_name[] = {
    F("File load failed."), F("stop."), F("playing."), F("pause."), F("wait.")
  };

  // SMFプレイヤーの状態取得
  int status = smfPlayer.getStatus();

  if((last_filename != filename)||(last_status != status)) {
    Serial.print("Filename: ");
    Serial.println(filename);
    Serial.print("Status: ");
    Serial.println(status_name[status]);
    last_filename = filename;
    last_status = status;
  } else {
    switch(status) {
    case SMF_STAT_PLAY:
      // 演奏中
      // 画面があれば表示更新周期でアニメーション表示など
      break;
    }
  }
  
  // バッテリー残量表示
  // int battery = M5.Power.getBatteryLevel();
}

// 再生終了時のコールバック
void onPlaybackFinished()
{
  rgbled->showColor(EspEasyLEDColor::CYAN);

  // 次の曲を読み込んで再生開始
  smfPlayer.load(makeFilename());
  smfPlayer.play();

  rgbled->showColor(EspEasyLEDColor::BLUE);
}

// 初期化
void setup()
{
  M5.begin();
//M5.Power.begin();

  btnPlay.begin(PIN_BUTTON_PLAY, true, 10); // (ピン番号, 負論理, デバウンス時間)
  btnPrev.begin(PIN_BUTTON_PREV, true, 10);
  btnNext.begin(PIN_BUTTON_NEXT, true, 10);
  btnUp.begin  (PIN_BUTTON_UP,   true, 10);
  btnDown.begin(PIN_BUTTON_DOWN, true, 10);

  rgbled = new EspEasyLED(PIN_RGBLED, 1, 10); // (ピン番号, RGBLEDの数, LEDの明るさ)
  rgbled->showColor(EspEasyLEDColor::YELLOW);

  Serial.begin( 115200 );
  Serial.println("M5Stack MIDI Player");

  // SDカードの初期化
  Serial.println("Initializing SD card...");
  int pin_sck  = M5.getPin(m5::sd_spi_sclk);
  int pin_miso = M5.getPin(m5::sd_spi_miso);
  int pin_mosi = M5.getPin(m5::sd_spi_mosi);
  int pin_cs   = M5.getPin(m5::sd_spi_cs);
  // Serial.printf("SCK=%d MISO=%d MOSI=%d CS=%d\n", pin_sck, pin_miso, pin_mosi, pin_cs);

  SPI.begin(pin_sck, pin_miso, pin_mosi);
  if (!SD.begin(pin_cs))
  {
    rgbled->showColor(EspEasyLEDColor::RED);
    Serial.println("SD card failed, or not present.");
    Serial.println("Reset and retry after 10 seconds.");
    // 10秒待ってからリセットしてリトライ
    delay(10000);
    esp_restart();
    return;
  }
  DPRINTLN(F("SD card initialized."));
  delay(2000); // すぐにファイルアクセスするとフォーマット破壊することがあったため待ち

#if 0
  // 起動時にボタンが押されていた場合、使用するディレクトリを変更する
  int dirno = 0;
  if(btnPrev.isPressed()) dirno += 01;
  if(btnNext.isPressed()) dirno += 02;
  if(btnDown.isPressed()) dirno += 04;
  if(btnUp.isPressed())   dirno += 08;
  if(dirno > 0){
    sprintf(dirname, "/dir%01d", dirno);
  }
#endif

  // SMFプレイヤーの初期化
  smfPlayer.begin(&SERIAL_MIDI, PIN_RX, PIN_TX);
  rgbled->showColor(EspEasyLEDColor::CYAN);

  // SMFプレイヤーのボリューム初期値
  smfPlayer.setVolume(INIT_VOLUME);

  // SMFファイルの読み込み
  if(smfPlayer.load(makeFilename()) == false )
  {
    rgbled->showColor(EspEasyLEDColor::RED);
    Serial.println("SMF Player initialization failed.");
    while(1){ delay(1000); }
  }

  // 演奏停止時のコールバック (1曲終わったとき)
  smfPlayer.onPlaybackFinished = onPlaybackFinished;
  
  rgbled->showColor(EspEasyLEDColor::GREEN);

#if 0
  //演奏開始
  smfPlayer.play();
  rgbled->showColor(EspEasyLEDColor::BLUE);
#endif
}

// メインループ
void loop()
{
  // 周期処理
  if(smfPlayer.loop() == true)
  {
    // ボタン操作処理
    if( sButtonCheckInterval.check() == true ){
      M5.update();
      btnPlay.read();
      btnPrev.read();
      btnNext.read();
      btnUp.read();
      btnDown.read();
    
      // 再生/停止ボタン
      if( btnPlay.wasSingleClicked() ){
        rgbled->showColor(EspEasyLEDColor::CYAN);
        if( smfPlayer.getStatus() == SMF_STAT_STOP ){
          Serial.println("Play");
          smfPlayer.play();
          rgbled->showColor(EspEasyLEDColor::BLUE);
        }else{
          Serial.println("Pause");
          smfPlayer.pause();
          rgbled->showColor(EspEasyLEDColor::GREEN);
        }
      }
      if( btnPlay.wasDoubleClicked() ){
        Serial.println("Rewind");
        rgbled->showColor(EspEasyLEDColor::CYAN);
        smfPlayer.rewind();
        rgbled->showColor(EspEasyLEDColor::GREEN);
      }
      // >> ボタン
      if( btnNext.wasPressed()  ){    
        Serial.println(">> button pressed.");
        rgbled->showColor(EspEasyLEDColor::CYAN);

        bool  playing = ( smfPlayer.getStatus() != SMF_STAT_STOP ) ? true : false;
        smfPlayer.stop();
        smfPlayer.load(makeFilename(D_PLAY_NEXT));

        if( playing == true ){
          smfPlayer.play();
          rgbled->showColor(EspEasyLEDColor::BLUE);
        }else{
          rgbled->showColor(EspEasyLEDColor::GREEN);
        }
      }
      // << ボタン
      if( btnPrev.wasPressed()  ){
        Serial.println("<< button pressed.");
        rgbled->showColor(EspEasyLEDColor::CYAN);

        bool  playing = ( smfPlayer.getStatus() != SMF_STAT_STOP ) ? true : false;
        smfPlayer.stop();
        smfPlayer.load(makeFilename(D_PLAY_PREV));

        if( playing == true ){
          smfPlayer.play();
          rgbled->showColor(EspEasyLEDColor::BLUE);
        }else{
          rgbled->showColor(EspEasyLEDColor::GREEN);
        }
      }
      // + ボタン
      if( btnUp.wasPressed() ){
        smfPlayer.upVolume();
        Serial.printf("+ button pressed. vol = %d\n", smfPlayer.getVolume());
      }
      // - ボタン
      if( btnDown.wasPressed() ){
        smfPlayer.downVolume();
        Serial.printf("- button pressed. vol = %d\n", smfPlayer.getVolume());
      }
#if 0
      // パワーオフ
      if( btnPower.wasPressed()  ){
        Serial.println("Power button pressed.");

        if( smfPlayer.getStatus() != SMF_STAT_STOP ){
          smfPlayer.stop(); //演奏中なら演奏停止
        }
        M5.powerOFF();
      }
#endif
    }
    // 状態表示更新
    else if( sUpdateScreenInterval.check() == true ) {
      updateScreen();
    }
  }
}
