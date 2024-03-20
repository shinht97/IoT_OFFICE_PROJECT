#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <stdlib.h>

// define pin Num of RFID sensor
#define SS_PIN  10
#define RST_PIN 9

// WIFI를 통한 서버 접속을 위한 define
#define AP_SSID "SEMICON_2.4G"
#define AP_PASS  "a1234567890"
#define SERVER_NAME "10.10.52.193" // 라즈베리파이 서버 주소
#define SERVER_PORT 5000 // 포트
#define LOGID   "PRJ_WIFI2" // 접속 아이디 변경 필요
#define PASSWD  "PASSWD"

// WIFI pin 정의
#define WIFIRX    6  //6:RX --> ESP8266 TX
#define WIFITX    7  //7:TX --> ESP8266 RX
#define LED_BUILTIN_PIN 13

#define R   A0
#define G   A1
#define B   A2

#define Relay   2
#define FAN     3

#define CMD_SIZE  50
#define ARR_CNT   7

#define DEBUG
// #define DEBUG_WIFI

bool timerIsrFlag = false;
bool updateTimeFlag;

int sensorTime;

char sendBuf[CMD_SIZE];
char lcdLine1[17] = ""; // 뭔가 까리한 말 
char lcdLine2[17] = "";
// char getSensorId[10];
char cardID[11] = "";

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

// 사용할 주변 장치 초기화
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

// 사용자 정의 함수 원형 선언
// void IRfidRead();
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
  Serial.begin(19200);
#endif

  wifi_Setup();

  MsTimer2::set(1000, timerIsr);
  MsTimer2::start();
}

void loop() 
{
  if(client.available()) // 와이파이를 통해 데이터가 들어올 경우
  {
    socketEvent();
  }

  if(timerIsrFlag)
  {
    timerIsrFlag = false;

    // IRfidRead();

#ifdef DEBUG
  Serial.print("Card UID : ");
  Serial.print(cardID);
  Serial.println();
#endif

    if(!(secCount % 10)) // 10초 마다 lcd 비움
    {
      lcdDisplay(0, 0, lcdLine1);
      lcdDisplay(0, 1, lcdLine2);
    }

    // sprintf(lcdLine2, "%02d.%02d  %02d:%02d:%02d", dateTime.month, dateTime.day, dateTime.hour, dateTime.min, dateTime.sec);
    // lcdDisplay(0, 1, lcdLine2);

    if(updateTimeFlag)
    {
      client.print("GETTIME\n");
      updateTimeFlag = false;
    }
  }
}

// void IRfidRead()
// {
//   char temp[4] = "";

//   if(!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
//   {
//     return;
//   }

//   for(byte i = 0; i < 4; i++)
//   {
//     temp[4] = "";
//     itoa(rfid.uid.uidByte[i], temp, 10); // RFID UID 문자열로 변경
//     strcat(cardID, temp); 
//   }
  
//   // lcdDisplay(0, 0, "  Welcome  ");
//   // lcdDisplay(0, 1, ID);

//   sprintf(sendBuf, "[%s]MYID@%s\n", LOGID, cardID); // 서버에게 명령어 보냄, MYID : 대상 아두이노 요청 명령어
//   client.write(sendBuf, strlen(sendBuf));
//   client.flush();
// }

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
}

void socketEvent() // wifi를 통해 정보가 들어왔을때 실행되는 함수
{
  int i = 0;
  char* pToken;
  char* pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  sendBuf[0] = '\0';
  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE-1);
  client.flush();

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

    strcpy(lcdLine2, "Server Connected");
    lcdDisplay(0, 1, lcdLine2);
    updateTimeFlag = true;
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {

#ifdef DEBUG
    Serial.write('\n');
#endif

    client.stop(); // 보드가 꺼졌다 켜졌을 경우 서버에 다시 접속
    server_Connect();

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
    sprintf(sendBuf,"\nTime %02d.%02d.%02d %02d:%02d:%02d\n\r",dateTime.year,dateTime.month,dateTime.day,dateTime.hour,dateTime.min,dateTime.sec );
    Serial.println(sendBuf);
#endif

    return;
  }
  else if(!strcmp(pArray[1], "DEVICE"))
  {
    analogWrite(R, atoi(pArray[2]));
    analogWrite(G, atoi(pArray[3]));
    analogWrite(B, atoi(pArray[4]));

    digitalWrite(Relay, atoi(pArray[5]));

    analogWrite(FAN, atoi(pArray[6]));

    return;
  }
  else
  {
    return;
  }

  client.write(sendBuf, strlen(sendBuf));
  client.flush();

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

void wifi_Setup() 
{
  wifiSerial.begin(19200);
  wifi_Init();
  server_Connect();
}

void wifi_Init()
{
  do 
  {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) 
    {

#ifdef DEBUG_WIFI
      Serial.println("WiFi shield not present");
#endif
    
    }
    else
      break;
  } while (1);

#ifdef DEBUG_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif

  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) 
  {

#ifdef DEBUG_WIFI
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
#endif
  
  }
  
  sprintf(lcdLine1, "ID:%s", LOGID);
  lcdDisplay(0, 0, lcdLine1);
  sprintf(lcdLine2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0, 1, lcdLine2);

#ifdef DEBUG_WIFI
  Serial.println("You're connected to the network");
  printWifiStatus();
#endif

}

int server_Connect()
{

#ifdef DEBUG_WIFI
  Serial.println("Starting connection to server...");
#endif

  if (client.connect(SERVER_NAME, SERVER_PORT)) 
  {

#ifdef DEBUG_WIFI
    Serial.println("Connect to server");
#endif
    client.print("["LOGID":"PASSWD"]");   

  }
  else
  {

#ifdef DEBUG_WIFI
    Serial.println("server connection failure");
#endif
  
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}