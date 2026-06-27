#include "SdFat.h"
#include <SPI.h>
SdFat SD;
char m_name[40];
char fileName[40];
char compareName[40];
char buf1[10],buf2[10];
char sdir[10][40];
File file;
byte fileLength2,fileLength1;
unsigned int fileLength;
/* 
ATmega328  Connection
--------------------
PB0        PIA PA6 (PIA_PA6)
PB1        PIA PA7 (PIA_PA7)
PB2        SD CS (CABLESELECTPIN)
PB3        SD MOSI
PB4        SD MISO
PB5        SD SCK
PC0        PIA PB4 (FLAGPIN)
PC1        PIA PB5 (CHKPIN)
PC2        PIA PB0 (PIA_PB0)
PC3        PIA PB1 (PIA_PB1)
PC4        PIA PB2 (PIA_PB2)
PC5        PIA PB3 (PIA_PB3)
PD2        PIA PA0 (PIA_PA0)
PD3        PIA PA1 (PIA_PA1)
PD4        PIA PA2 (PIA_PA2)
PD5        PIA PA3 (PIA_PA3)
PD6        PIA PA4 (PIA_PA4)
PD7        PIA PA5 (PIA_PA5)
*/
#define CABLESELECTPIN  (10)
#define CHKPIN          (15)
#define PIA_PA0          (2)
#define PIA_PA1          (3)
#define PIA_PA2          (4)
#define PIA_PA3          (5)
#define PIA_PA4          (6)
#define PIA_PA5          (7)
#define PIA_PA6          (8)
#define PIA_PA7          (9)
#define FLAGPIN         (14)
#define PIA_PB0         (16)
#define PIA_PB1         (17)
#define PIA_PB2         (18)
#define PIA_PB3         (19)

#define SD_STATUS_OK             0x00
#define SD_STATUS_INIT_ERROR     0xF0
#define SD_STATUS_FILE_NOT_FOUND 0xF1
#define SD_STATUS_FILE_EXISTS    0xF3
#define SD_STATUS_CMD_ERROR      0xF4
#define STATUS_PROMPT_USER       0xFE
#define SD_STATUS_ERROR          0xFF
#define FILENAME_LENGTH            32


// ファイル名は、ロングファイルネーム形式対応
// The file name supports long filename format
boolean sdErrorFlag;

void sdInit(void)
{
// SD初期化
// SD card initialisation
  if( !SD.begin(CABLESELECTPIN,8) )
  {
//  Serial.println("Failed : SD.begin");
    sdErrorFlag = true;
  } 
  else 
  {
//  Serial.println("OK : SD.begin");
    sdErrorFlag = false;
  }
//Serial.println("START");
}

void setup()
{
// Serial.begin(9600);
// CS=pin10
// pin10 output
  pinMode(CABLESELECTPIN,OUTPUT);
  pinMode( CHKPIN,INPUT);  // CHK
  pinMode( PIA_PA0,OUTPUT); // 送信データ ... transmit data
  pinMode( PIA_PA1,OUTPUT); // 送信データ
  pinMode( PIA_PA2,OUTPUT); // 送信データ
  pinMode( PIA_PA3,OUTPUT); // 送信データ
  pinMode( PIA_PA4,OUTPUT); // 送信データ
  pinMode( PIA_PA5,OUTPUT); // 送信データ
  pinMode( PIA_PA6,OUTPUT); // 送信データ
  pinMode( PIA_PA7,OUTPUT); // 送信データ
  pinMode( FLAGPIN,OUTPUT);  // flag

  pinMode( PIA_PB0,INPUT_PULLUP); // 受信データ ... receive data
  pinMode( PIA_PB1,INPUT_PULLUP); // 受信データ
  pinMode( PIA_PB2,INPUT_PULLUP); // 受信データ
  pinMode( PIA_PB3,INPUT_PULLUP); // 受信データ

  digitalWrite(PIA_PA0,LOW);
  digitalWrite(PIA_PA1,LOW);
  digitalWrite(PIA_PA2,LOW);
  digitalWrite(PIA_PA3,LOW);
  digitalWrite(PIA_PA4,LOW);
  digitalWrite(PIA_PA5,LOW);
  digitalWrite(PIA_PA6,LOW);
  digitalWrite(PIA_PA7,LOW);
  digitalWrite(FLAGPIN,LOW);

  sdInit();
}

// 4BIT受信
// 4-bit receive
byte receiveNybble(void)
{
  // HIGHになるまでループ
  // Loop until CHKPIN = HIGH
  while(digitalRead(CHKPIN) != HIGH) { }
  // 受信
  // Receive
  byte j_data = digitalRead(PIA_PB0)+digitalRead(PIA_PB1)*2+digitalRead(PIA_PB2)*4+digitalRead(PIA_PB3)*8;
  // flagをセット
  // Set flag
  digitalWrite(FLAGPIN,HIGH);
  // LOWになるまでループ
  // Loop until CHKPIN = LOW
  while(digitalRead(CHKPIN) == HIGH) { }
  // flagをリセット
  // Reset flag
  digitalWrite(FLAGPIN,LOW);
  return(j_data);
}

// 1BYTE受信
// Receive 1 byte
byte receiveByte(void)
{
  byte i_data = 0;
  i_data=receiveNybble()*16;
  i_data=i_data+receiveNybble();
  return(i_data);
}

// 1BYTE送信
// Send 1 byte
void sendByte(byte i_data)
{
// 下位ビットから8ビット分をセット
// Set the lower 8 bits
  digitalWrite(PIA_PA0,(i_data)&0x01);
  digitalWrite(PIA_PA1,(i_data>>1)&0x01);
  digitalWrite(PIA_PA2,(i_data>>2)&0x01);
  digitalWrite(PIA_PA3,(i_data>>3)&0x01);
  digitalWrite(PIA_PA4,(i_data>>4)&0x01);
  digitalWrite(PIA_PA5,(i_data>>5)&0x01);
  digitalWrite(PIA_PA6,(i_data>>6)&0x01);
  digitalWrite(PIA_PA7,(i_data>>7)&0x01);
  digitalWrite(FLAGPIN,HIGH);
// HIGHになるまでループ
// Loop until CHKPIN = HIGH
  while(digitalRead(CHKPIN) != HIGH) { }
  digitalWrite(FLAGPIN,LOW);
// LOWになるまでループ
// Loop until CHKPIN = LOW
  while(digitalRead(CHKPIN) == HIGH) { }
}

// ファイル名の最後が「.cjr」でなければ付加
// Append '.cjr' if the filename does not end with '.cjr'
// Modified by Brett
void addCRJ(const char *src, char *dest) 
{
  size_t dest_size = sizeof(fileName);

  if (!src || !dest || dest_size == 0) return;

  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';

  size_t len = strlen(dest);

  if (len >= 4 &&
      dest[len - 4] == '.' &&
      tolower(dest[len - 3]) == 'c' &&
      tolower(dest[len - 2]) == 'j' &&
      tolower(dest[len - 1]) == 'r') 
  {
    return;
  }

  if (len + 4 < dest_size) 
  {
    strcat(dest, ".cjr");
  }
}

// SDカードから読込
// Read from SD card
void loadFromSD(void)
{
  int temp1 = 0;
  unsigned int loop1;
  // ファイルネーム取得
  // Get filename
  receiveName(m_name);
  addCRJ(m_name,fileName);
  // ファイルが存在しなければERROR
  // ERROR if file doesn't exist
  if (SD.exists(fileName))
  {
    // ファイルオープン
    // Open file
    file = SD.open( fileName, FILE_READ );
    if(file)
    {
      // 状態コード送信(OK)
      // Send OK status code
      sendByte(SD_STATUS_OK);
      // ファイルサイズ取得
      // Get file size
      fileLength = file.size();
      // データ送信
      // Send data
      if(fileLength>0)
      {
        for (loop1 = 1;loop1 <= fileLength;loop1++)
        {
          temp1 = file.read();
          sendByte(temp1);
        }
      }
      file.close();
      sdInit();
    }
    else
    {
      // 状態コード送信(ERROR)
      // Send (ERROR) status code
      sendByte(SD_STATUS_ERROR);
      sdInit();
    }
  }
  else
  {
  // 状態コード送信(FILE NOT FIND ERROR)
  // Send (FILE NOT FOUND) status code
  sendByte(SD_STATUS_FILE_NOT_FOUND);
  sdInit();
  }
}

// SDカードに書き込み
// Save to SD card
void saveToSD(void)
{
  unsigned int loop1;
  byte temp1,temp2,saveAddress1,saveAddress2,sum,blockNum;
  unsigned long saveAddress;
  boolean flag;
  // ファイルネーム取得
  // Get filename
  receiveName(m_name);
  addCRJ(m_name,fileName);
//Serial.println(fileName);

  // ファイルが存在すればdelete
  // Delete if file exists
  if (SD.exists(fileName))
  {
    SD.remove(fileName);
  }
  // ファイルオープン
  // Open file
  file = SD.open( fileName, FILE_WRITE );
  if(file )
  {
    // 状態コード送信(OK)
    // Send (OK) status code
    sendByte(SD_STATUS_OK);
    if(receiveByte() == SD_STATUS_OK)
    {
      // ヘッダブロック
      // Header block
      file.write(0x02);
      file.write(0x2A);
      temp1 = 0x00;
      file.write(temp1);
      file.write(0x1A);
      file.write(0xFF);
      file.write(0xFF);
      sum = 0x3C;
      // Copy 16 characters from m_name, pad with 0x00
      flag=false;
      for (loop1 = 0;loop1 <= 15;loop1++)
      {
        temp2 = m_name[loop1];
        if (temp2 == 0)
        {
          flag=true;
        }
        if (flag)
        {
          temp2=0;
        }
        sum = sum+temp2;
        file.write(temp2);
      }
      temp1 = receiveByte();
      file.write(temp1);
      temp1 = 0x00;
      file.write(temp1);
      file.write(0xFF);
      file.write(0xFF);
      file.write(0xFF);
      file.write(0xFF);
      file.write(0xFF);
      file.write(0xFF);
      file.write(0xFF);
      file.write(0xFF);
      file.write(sum);
      // SAVE開始アドレス受信
      // Receive SAVE start address
      saveAddress1 = receiveByte();
      saveAddress2 = receiveByte();
      saveAddress = saveAddress1*256+saveAddress2;
      // SAVEデータサイズ受信
      // Receive SAVE data size
      fileLength1 = receiveByte();
      fileLength2 = receiveByte();
      fileLength = fileLength1*256+fileLength2;
      // 256Byteデータブロック
      // 256 byte data block
      blockNum=1;
      while(fileLength1)
      {
        file.write(0x02);
        file.write(0x2A);
        sum = 0x2C; // 0x02+0x2A
        file.write(blockNum);
        sum = sum + blockNum;
        temp1 = 0x00;
        file.write(temp1);
        file.write(saveAddress1);
        file.write(saveAddress2);
        sum = sum + saveAddress1 + saveAddress2;
        for (loop1=1; loop1<=256; loop1++)
        {
          temp1 = receiveByte();
          file.write(temp1);
          sum = sum + temp1;
        }
        file.write(sum);
        saveAddress1++;
        fileLength1--;
        blockNum++;
      }
      // fileLength2 Byteデータブロック
      // fileLength2 byte data block
      if(fileLength2)
      {
        file.write(0x02);
        file.write(0x2A);
        sum = 0x2C; // 0x02+0x2a
        file.write(blockNum);
        sum = sum + blockNum;
        file.write(fileLength2);
        sum = sum + fileLength2;
        file.write(saveAddress1);
        file.write(saveAddress2);
        sum = sum + saveAddress1 + saveAddress2;
        for (loop1=1; loop1<=fileLength2; loop1++)
        {
          temp1 = receiveByte();
          file.write(temp1);
          sum = sum + temp1;
        }
        file.write(sum);
      }
//フッタブロック
      file.write(0x02);
      file.write(0x2A);
      file.write(0xFF);
      file.write(0xFF);
      saveAddress  = saveAddress + fileLength;
      saveAddress1 = saveAddress / 256;
      saveAddress2 = saveAddress % 256;
      file.write(saveAddress1);
      file.write(saveAddress2);
    }
    file.close();
  }
  else
  {
    // 状態コード送信(ERROR)
    // Send (ERROR) status code
    sendByte(SD_STATUS_FILE_NOT_FOUND));
    sdInit();
  }
}

// 比較文字列取得 32+1文字まで取得、ただしダブルコーテーションは無視する
// Get comparison string up to 33 characters, skip double quotation marks
void receiveName(char *fileName)
{
  char receiveData;
  unsigned int loop2 = 0;
  for (unsigned int loop1 = 0;loop1 <= FILENAME_LENGTH;loop1++)
  {
    receiveData = receiveByte();
    if (receiveData != 0x22)
    {
      fileName[loop2] = receiveData;
      loop2++;
    }
  }
}

// fileNameとcompareNameをcompareNameに0x00が出るまで比較
// Compare fileName with compareName until the null terminator (0x00) in compareName
// Brett: removed & replaced with strncasecmp() calls
// boolean f_match(char *fileName,char *compareName)
// {
//   boolean flag1 = true;
//   unsigned int loop1 = 0;
//   while (loop1 <=32 && compareName[0] != 0x00 && flag1 == true){
//     if (upper(fileName[loop1]) != upper(compareName[loop1])){
//       flag1 = false;
//     }
//     loop1++;
//     if (compareName[loop1]==0x00)
//     {
//       break;
//     }
//   }
//   return flag1;
// }

// 小文字->大文字
// Lower to upper
// Brett: no longer needed
// char upper(char c){
//   if('a' <= c && c <= 'z'){
//     c = c - ('a' - 'A');
//   }
//   return c;
// }


// SD-CARDのFILELIST
void sdDirList(void)
{
  // 比較文字列取得 32+1文字まで
  // Get comparison string (up to 33 characters)
  receiveName(compareName);
  File file2 = SD.open( "/" );
  if(file2)
  {
    // 状態コード送信(OK)
    // Send (OK) status code
    sendByte(SD_STATUS_OK);

    File entry =  file2.openNextFile();
    int cntl2 = 0;
    unsigned int br_chk =0;
    int page = 1;
    // 全件出力の場合には10件出力したところで一時停止、キー入力により継続、打ち切りを選択
    // When outputting all items, pause after every 10 items and let the 
    // user choose to continue or abort via key input
    while (!br_chk) 
    {
      if(entry)
      {
        entry.getName(fileName,36);
        unsigned int loop1=0;
        // 一件送信
        // Send one entry
        // 比較文字列でファイルネームを先頭から比較して一致するものだけを出力
        // Compare the filename from the beginning with the 
        // comparison string and output only the matching ones
        if (strncasecmp(fileName, compareName, FILENAME_LENGTH) == 0) 
        {
          // sdir[]にfileNameを保存
          // Save fileName to sdir[]
          strcpy(sdir[cntl2],fileName);
          sendByte(0x30+cntl2);
          sendByte(0x20);
          while (loop1<=36 && fileName[loop1]!=0x00)
          {
            sendByte(fileName[loop1]);
            loop1++;
          }
          sendByte(SD_STATUS_OK);
          cntl2++;
        }
      }
      // CNTL2 > 表示件数-1
      // CNTL2 > Items per page - 1
      if (!entry || cntl2 > 9)
      {
        // 継続・打ち切り選択指示要求
        // Prompt user to choose Continue or Cancel
        sendByte(STATUS_PROMPT_USER);

        // 選択指示受信(0:継続 B:前ページ 以外:打ち切り)
        // Receive selection
        // 0: Continue, B: Prev page, Other: Abort
        br_chk = receiveByte();
        // 前ページ処理
        // Handle previous page
        if (br_chk==0x42)
        {
          // 先頭ファイルへ
          // Go to the first file
          file2.rewindDirectory();
          // entry値更新
          // Update entry value
          entry =  file2.openNextFile();
          // もう一度先頭ファイルへ
          // Go to the first file again
          file2.rewindDirectory();
          if(page <= 2)
          {
            // 現在ページが1ページ又は2ページなら1(??)ページ目に戻る処理
            // Return to page 0 if currently on page 1 or 2
            page = 0;
          }
          else
          {
            // 現在ページが3ページ以降なら前々ページまでのファイルを読み飛ばす
            // If the current page is 3 or later, skip files up to 
            // the page before last (i.e. skip to current-2)
            page = page -2;
            cntl2=0;
            // page*表示件数
            // Number of results per page
            while(cntl2 < page*10)
            {
                entry =  file2.openNextFile();
                if (strncasecmp(fileName, compareName, FILENAME_LENGTH) == 0) 
                {
                  cntl2++;
                }
            }
         }
          br_chk=0;
        }
        // 1～0までの数字キーが押されたらsdir[]から該当するファイル名を送信
        // Send filename from sdir[] corresponding to the pressed 
        // number key (0-9)
        if(br_chk>=0x30 && br_chk<=0x39)
        {
          file = SD.open( sdir[br_chk-0x30], FILE_READ );
          if( file == true )
          {
            unsigned int loop2=0;
            sendByte(0xFD);
            while (loop2<=36 && sdir[br_chk-0x30][loop2]!=0x00)
            {
              sendByte(sdir[br_chk-0x30][loop2]);
              loop2++;
            }
            sendByte(SD_STATUS_OK);
            file.close();
          }
        }
        page++;
        cntl2 = 0;
      }
      // ファイルがまだあるなら次読み込み、なければ打ち切り指示
      // Continue reading files while they exist; 
      // send termination command when done
      if (entry)
      {
        entry =  file2.openNextFile();
      }
      else
      {
        br_chk=1;
      }
    }
    // 処理終了指示
    // Terminate processing
    sendByte(SD_STATUS_ERROR);
    sendByte(SD_STATUS_OK);
  }
  else
  {
    sendByte(SD_STATUS_FILE_NOT_FOUND));
  }
}

void loop()
{
  digitalWrite(PIA_PA2,LOW);
  digitalWrite(PIA_PA3,LOW);
  digitalWrite(FLAGPIN,LOW);
  // コマンド取得待ち
  // Waiting for command
  //Serial.print("cmd:");
  byte cmd = receiveByte();
  //Serial.println(cmd,HEX);
  if (!sdErrorFlag)
  {
    switch(cmd) 
    {
      // 83hでSDカードからLoad
      // Command code 0x83: load from SD card
      case 0x83:
        //状態コード送信(OK)
        // Send (OK) status code
        // Serial.println("LOAD START");
        sendByte(SD_STATUS_OK);
        loadFromSD();
        break;
      // 84hでSDカードにsave
      // Command code 0x84: save to SD card
      case 0x84:
        // 状態コード送信(OK)
        // Send (OK) status code
        // Serial.println("SAVE START");
        sendByte(SD_STATUS_OK);
        saveToSD();
        break;
      // 82hでSDカードファイル一覧
      // Command code 0x82: get SD card directory
      case 0x82:
        // 状態コード送信(OK)
        // Send (OK) status code
        //  Serial.println("SD DIR START");
        sendByte(SD_STATUS_OK);
        sdInit();
        sdDirList();
        break;
      default:
// 状態コード送信(CMD ERROR)
// Send (CMD ERROR) status code
        sendByte(SD_STATUS_CMD_ERROR);
    }
  } 
  else
  {
// 状態コード送信(ERROR)
// Send (ERROR) status code
    sendByte(SD_STATUS_INIT_ERROR);
    sdInit();
  }
}
