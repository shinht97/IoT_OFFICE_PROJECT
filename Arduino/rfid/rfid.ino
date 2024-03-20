#include <SPI.h>
#include <MFRC522.h>
#include <MsTimer2.h>

#define SS_PIN 10
#define RST_PIN 9

// #define
// #define

MFRC522 rfid(SS_PIN, RST_PIN);

void InterRFRead();

void setup() 
{
  Serial.begin(9600);

  SPI.begin();

  rfid.PCD_Init();

  MsTimer2::set(1000, InterRFRead);
  MsTimer2::start();
}

void loop() 
{
  
}

void InterRFRead()
{
  if(!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
    return;
  }

  Serial.print("Card UID : ");

  for(byte i = 0; i < 4; i++)
  {
    Serial.print(rfid.uid.uidByte[i]);
  }

  Serial.println();
}

