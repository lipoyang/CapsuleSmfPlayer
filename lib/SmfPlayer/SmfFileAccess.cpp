//SMFファイルアクセス関数定義
// SmfSeq.c/hから呼び出されるSMFファイルへのアクセス関数の実体を記述する。
// 以下ではSDカードシールドライブラリに対し、ファイルポインタで直接読み出し位置を指定する方法での例を記述。
// ライブラリ自体の初期化はsetup関数に記述している。
#include "SmfFileAccess.h"

// ヒープに大サイズの領域を分割して保持するための管理クラス
class MemoryManager
{
public:
  unsigned long data_size_ = 0;                     // 取り扱っている領域のサイズ
  unsigned long block_num_ = 0;                     // 使用しているメモリブロックの数
  unsigned long last_pos_ = 0;                      // シーケンシャルアクセス時のポインタ

  static const unsigned long block_num_max_ = 100;  // 最大で扱うメモリブロックの数
  unsigned long block_size_ = 10240;                // メモリブロックのサイズ
  unsigned char * block_list_[block_num_max_];      // メモリブロック管理リスト

public:
  // コンストラクタ
  MemoryManager()
  {
    release();
  }
  // デストラクタ
  ~MemoryManager()
  {
    release();
  }
  // 領域確保
  bool  alloc(unsigned long data_size)
  {
    bool  result = true;

    // 新たに領域確保する場合、過去に確保したものは破棄する
    release();

    // データブロック数を算出
    data_size_ = data_size;
    block_num_ = data_size_ / block_size_;
    if(data_size_ % block_size_ > 0)
    {
      block_num_++;
    }
    // データブロックをヒープから確保
    if(block_num_ < block_num_max_)
    {
      for(int i=0; i< block_num_; i++)
      {
        // 一度確保したものは使い続けるようにしておく
        if(block_list_[i] == NULL)
        {
          block_list_[i] = (unsigned char *)malloc(block_size_);
        }
//        block_list_[i] = (unsigned char *)malloc(block_size_);
        if(block_list_[i] == NULL)
        {
          result = false;
          break;
        }
      }
    }
    else
    {
      result = false;
    }
    
    if(result == false)
    {
      release();
    }
    return result;
  }
  // 領域解放
  void  release()
  {
// 一度確保したものは使い続けるようにしておく
//    for(int i=0; i<block_num_max_; i++)
//    {
//      if(block_list_[i] != NULL)
//      {
//        free(block_list_[i]);
//      }
//      block_list_[i] = NULL;
//    }
    data_size_ = 0;
    block_num_ = 0;
    last_pos_ = 0;
  }
  // データ格納
  bool  put(unsigned long pos, unsigned char data)
  {
    bool  result = false;
    
    if(pos < data_size_)
    {
      // データブロックとデータブロック内の位置を算出
      int block = pos / block_size_;
      unsigned long i = pos % block_size_;
      // データを格納
      block_list_[block][i] = data;
      result = true;
    }
    return result;
  }
  // データ取り出し
  int   get(unsigned long pos)
  {
    int result = -1;
    if(pos < data_size_)
    {
      int block = pos / block_size_;
      unsigned long i = pos % block_size_;
      result = (int)block_list_[block][i];

      last_pos_ = pos + 1;
    }
    return result;    
  }
  // シーケンシャルデータ取り出し
  int   getNext()
  {
    return get(last_pos_);    
  }
  // 読出し位置変更
  void seek(unsigned long pos)
  {
    last_pos_ = pos;
  }
  // データサイズ取得
  unsigned long size()
  {
    return  data_size_;
  }
};

// ヒープに大サイズの領域を分割して保持するための管理クラスのインスタンス生成
MemoryManager s_MemoryManager;

//File  s_FileHd;
bool SmfFileAccessOpen( UCHAR * Filename )
{
  bool  result = false;

  SmfFileAccessClose();
  
  if( Filename!=NULL ){
    IPRINT(F("filename:"));
    IPRINTLN((const char *)Filename);
    File  fileHd = SD.open( (const char *)Filename );

    result = fileHd.available();

    if( result == true )
    {
      result = s_MemoryManager.alloc(fileHd.size());
      IPRINT(F("size:"));
      IPRINTLN(s_MemoryManager.size());

      if( result == true )
      {
        for( unsigned long pos=0; pos<s_MemoryManager.size(); pos++ )
        {
          int data = fileHd.read();
          s_MemoryManager.put(pos, data);
        }
        IPRINTLN(F("finish:"));
      }
      else
      {
        SmfFileAccessClose();
        result = false;
        IPRINTLN(F("failed:"));
      }
    }
    else
    {
      SmfFileAccessClose();
      result = false;
      IPRINTLN(F("failed:"));
    }
    fileHd.close();
  }
  return( result );
}
void SmfFileAccessClose()
{
  s_MemoryManager.release();
}
bool SmfFileAccessRead( UCHAR * Buf, unsigned long Ptr )
{
  bool  result = false;

  int data = s_MemoryManager.get(Ptr);
  if(data >= 0)
  {
    *Buf = (UCHAR)data;
    result = true;
  }
  return( result );
}
bool SmfFileAccessReadNext( UCHAR * Buf )
{
  bool  result = true;

  int data = s_MemoryManager.getNext();
  if( data>=0 ){
    *Buf = (UCHAR)data;
  }else{
    result = false;
  }
  return( result );
}
int SmfFileAccessReadBuf( UCHAR * Buf, unsigned long Ptr, int Lng )
{
  int result = 0;
  if( Buf!=NULL )
  {
    s_MemoryManager.seek(Ptr);
    for( int i=0; i<Lng; i++ )
    {
      int data = s_MemoryManager.getNext();
      if(data >= 0)
      {
        Buf[i] = (UCHAR)data;
        result++;
      }
      else
      {
        result = -1;
        break;
      }
    }
  }
  return( result );
}
unsigned int SmfFileAccessSize()
{
  return( s_MemoryManager.size() );
}

