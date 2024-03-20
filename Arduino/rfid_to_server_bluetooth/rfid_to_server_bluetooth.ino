#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdlib.h>
#include <string.h>
#include <MFRC522.h>
#include <SPI.h>
#include <MsTimer2.h>

#define ARR_CNT   5
#define CMD_SIZE  60

#define SS_PIN    10
#define RST_PIN   9

#define LOGID   "PRJ_BT"

#define B   A0
#define G   A1
#define R   A2

// #define DEBUG

bool timerIsrFlag = false;
bool updateTimeFlag;
bool lcdShow = true;

int sensorTime;

char lcdLine1[17] = "SMART OFFICE!";
char lcdLine2[17] = "";
char sendBuf[CMD_SIZE];
char recvId[10];
char cardID[10];

unsigned int secCount;

typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
} DATETIME;

DATETIME dateTime = {0, 0, 0, 12, 0, 0}; // datetime 초기화

// 사용할 주변 장치 선언
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial BTSerial(6, 7); // RX ==>BT:TXD, TX ==> BT:RX

// 사용자 정의 함수 원형 선언
void IRfidRead();
void lcdDisplay(int _x, int _y, char* _msg);
void timerIsr();
void socketEvent();
void clock_calc(DATETIME* dateTime);
void wifi_Setup();
void wifi_Init();
int server_Connect();
void printWifiStatus();

void setup() 
{
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);

  SPI.begin();

  rfid.PCD_Init();

#ifdef DEBUG
  Serial.begin(9600);
#endif

  BTSerial.begin(9600); // set the data rate for the SoftwareSerial port

  MsTimer2::set(1000, timerIsr);
  MsTimer2::start();

  pinMode(B, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(R, OUTPUT);

  sprintf(sendBuf, "[GETTIME]\n"); // 현재 시간 요청 명령어
  BTSerial.write(sendBuf, strlen(sendBuf));
  BTSerial.flush();
}

void loop() 
{
  if (BTSerial.available() && !rfid.PICC_IsNewCardPresent()) // 블루투스를 통해 정보가 들어오면 실행
  {
    bluetoothEvent(); // 블루투스 이벤트 함수 실행
  }

  if(timerIsrFlag)
  {
    timerIsrFlag = false;

    IRfidRead();

#ifdef DEBUG
  Serial.print("Card UID : ");
  Serial.print(cardID);
  Serial.println();
#endif

    if(!(secCount % 7)) // 7초 마다 lcd 비움
    {
      lcdDisplay(0, 0, lcdLine1);
      // lcdDisplay(0, 1, lcdLine2);
      lcdShow = true;

      analogWrite(R, 0);
      analogWrite(G, 0);
      analogWrite(B, 0);
    }

    sprintf(lcdLine2, "%02d.%02d  %02d:%02d:%02d", dateTime.month, dateTime.day, dateTime.hour, dateTime.min, dateTime.sec);
    
    if(lcdShow == true)
    {
      lcdDisplay(0, 1, lcdLine2);
    }
    
    if(updateTimeFlag)
    {
      BTSerial.write("[GETTIME]\n");
      updateTimeFlag = false;
    }
  }
}

void IRfidRead()
{
  memset(cardID, 0x0, sizeof(cardID));

  char temp[4] = "";

  if(!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
    return;
  }

  for(byte i = 0; i < 4; i++)
  {
    temp[4] = "";
    itoa(rfid.uid.uidByte[i], temp, 10); // RFID UID 문자열로 변경
    strcat(cardID, temp); 
  }
  
#ifdef DEBUG
  lcdDisplay(0, 0, "  Welcome  ");
  lcdDisplay(0, 1, cardID);
#endif

  lcdShow = false;

  sprintf(sendBuf, "[PRJ_SQL]MYID@%s\n", cardID); // 서버에게 명령어 보냄, MYID : 대상 아두이노 요청 명령어
  BTSerial.write(sendBuf, strlen(sendBuf));
  BTSerial.flush();
}

void lcdDisplay(int _x, int _y, char* _msg)
{
  int len = 16 - strlen(_msg);

  lcd.setCursor(_x, _y);
  lcd.print(_msg);

  for(int i = len; i > 0; i--)
  {
    lcd.write(' ');
  }
}

void timerIsr()
{
  timerIsrFlag = true;
  secCount++;
  clock_calc(&dateTime);
}

void bluetoothEvent()
{
  int i = 0;
  char* pToken;
  char* pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  sendBuf[0] = '\0';
  len = BTSerial.readBytesUntil('\n', recvBuf, CMD_SIZE-1);
  BTSerial.flush();

#ifdef DEBUG
  Serial.print("recv : ");
  Serial.print(recvBuf);
#endif

  pToken = strtok(recvBuf, "[@]");

  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }

  //[KSH_ARD]LED@ON : pArray[0] = "KSH_ARD", pArray[1] = "LED", pArray[2] = "ON"
  if ((strlen(pArray[1]) + strlen(pArray[2])) < 16)
  {
    sprintf(lcdLine2, "%s %s", pArray[1], pArray[2]);
    lcdDisplay(0, 1, lcdLine2);
  }

  if(!strcmp(pArray[1], LOGID)) // 응답으로 들어온 경우 명령어로 인식 X
  {
    return;
  }
  else if (!strncmp(pArray[1], " New", 4)) // New Connected
  {

#ifdef DEBUG
    Serial.write('\n');
#endif

    lcdDisplay(0, 1, lcdLine2);
    updateTimeFlag = true;
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {

#ifdef DEBUG
    Serial.write('\n');
#endif

    return ;
  }
  else if(!strcmp(pArray[0], "GETTIME")) //GETTIME
  {  
    dateTime.year = (pArray[1][0]-0x30) * 10 + pArray[1][1]-0x30 ;
    dateTime.month =  (pArray[1][3]-0x30) * 10 + pArray[1][4]-0x30 ;
    dateTime.day =  (pArray[1][6]-0x30) * 10 + pArray[1][7]-0x30 ;
    dateTime.hour = (pArray[1][9]-0x30) * 10 + pArray[1][10]-0x30 ;
    dateTime.min =  (pArray[1][12]-0x30) * 10 + pArray[1][13]-0x30 ;
    dateTime.sec =  (pArray[1][15]-0x30) * 10 + pArray[1][16]-0x30 ;

#ifdef DEBUG
    sprintf(sendBuf, "\nTime %02d.%02d.%02d %02d:%02d:%02d\n\r", dateTime.year, dateTime.month, dateTime.day, dateTime.hour, dateTime.min, dateTime.sec);
    Serial.println(sendBuf);
#endif

    return;
  }
  else if(!strcmp(pArray[1], "HELLO"))
  {
    lcdShow = false;

    lcdDisplay(0, 0, pArray[1]);
    lcdDisplay(0, 1, pArray[2]);

    analogWrite(G, 255);
  }
  else if(!strcmp(pArray[1], "GOODBYE"))
  {
    lcdShow = false;

    lcdDisplay(0, 0, pArray[1]);
    lcdDisplay(0, 1, pArray[2]);

    analogWrite(G, 255);
  }
  else if(!strcmp(pArray[1], "ACCESS_DENIED"))
  {
    lcdShow = false;

    lcdDisplay(0, 0, cardID);
    lcdDisplay(0, 1, "ACCESS DENIED");

    analogWrite(R, 255);
  }
  else
  {
    return;
  }

  BTSerial.write(sendBuf, strlen(sendBuf));
  BTSerial.flush();

#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif

}

void clock_calc(DATETIME* dateTime)
{
  int ret = 0;
  dateTime->sec++;          // increment second

  if(dateTime->sec >= 60)                              // if second = 60, second = 0
  { 
      dateTime->sec = 0;
      dateTime->min++; 
             
      if(dateTime->min >= 60)                          // if minute = 60, minute = 0
      { 
          dateTime->min = 0;
          dateTime->hour++;                               // increment hour
          if(dateTime->hour == 24) 
          {
            dateTime->hour = 0;
            updateTimeFlag = true;
          }
       }
    }
}
