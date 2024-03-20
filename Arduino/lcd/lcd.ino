#include <SPI.h>
#include <MFRC522.h>
#include <MsTimer2.h>
#include <LiquidCrystal_I2C.h>
#include <stdlib.h>

#define DEBUG

#define SS_PIN 10
#define RST_PIN 9

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);

bool timerIsrFlag = false;

unsigned int secCount;

char lcdLine1[17] = "Power On";
char lcdLine2[17] = "Ready";

void IRFIDRead();
void lcdDisplay(int _x, int _y, char* _msg);
void timerIsr();

void setup() 
{
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);

  SPI.begin();

  rfid.PCD_Init();

  MsTimer2::set(1000, timerIsr);
  MsTimer2::start();

#ifdef DEBUG
  Serial.begin(19200);
#endif

}

void loop() 
{
  if(timerIsrFlag)
  {
    timerIsrFlag = false;

    IRFIDRead();

    if(!(secCount % 10))
    {
      lcdDisplay(0, 0, lcdLine1);
      lcdDisplay(0, 1, lcdLine2);
    }
  }
}

void timerIsr()
{
  timerIsrFlag = true;
  secCount++;

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

void IRFIDRead()
{
  char ID[11] = "";
  char temp[3] = "";

  if(!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
    return;
  }

#ifdef DEBUG
  Serial.print("Card UID : ");
#endif

  for(byte i = 0; i < 4; i++)
  {
    temp[4] = "";

#ifdef DEBUG
    Serial.print(rfid.uid.uidByte[i]);
    Serial.print(' ');
#endif

    itoa(rfid.uid.uidByte[i], temp, 10);
    strcat(ID, temp); // RFID UID 문자열로 변경
  }
  
  lcdDisplay(0, 0, "  Welcome  ");
  lcdDisplay(0, 1, ID);

#ifdef DEBUG
  Serial.print(ID);
  Serial.println();
#endif

}
