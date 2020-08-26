#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SS_PIN  12
#define RST_PIN 13

#define chipSelect 14

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x3f, 16, 2);
File dataFile;

String readtag(void);

void setup() {
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight(); 
  
  Serial.print("Initializing SD card...");
  lcd.home();
  lcd.print("checking SD card");
  lcd.home();
  delay(1000);
  
  if (SD.begin(chipSelect))
  {
    Serial.println("card initialized.");
    createDir(SD, "/Attendance");
    listDir(SD, "/", 0);
    Serial.println("done!");
    lcd.print("card OK         ");
  }
  else
  {
    Serial.println("Card failed, or not present");
    lcd.print("card FAILED     ");
  }
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  SPI.begin();
  rfid.PCD_Init();
  delay(5000);
  lcd.clear();
  lcd.home();
  lcd.print("tab to check-in");
}

String strID = "";
uint32_t count = 0;
uint32_t lastread = 0;
bool cleared = true;

void loop()
{
  strID = readtag();
  if (strID != "")
  {
    Serial.println(strID);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(strID);
    cleared = false;
    lastread = millis();
    appendFile(SD, "/Attendance/ID.txt", (strID + "\n").c_str());
    count++;
  }
  if (millis() - lastread >= 2000 && cleared == false)
  {
    lcd.home();
    lcd.print("tab to check-in");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    cleared = true;
  }
}

String readtag(void)
{
  String strID = "";

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    //    Serial.print("PICC type: ");
    //    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    //    Serial.println(rfid.PICC_GetTypeName(piccType));

    for (byte i = 0; i < 4; i++)
    {
      strID += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
                String(rfid.uid.uidByte[i], HEX) + (i != 3 ? ":" : "");
    }
    strID.toUpperCase();
//    Serial.println(strID);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  return strID;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
        lcd.home();
        lcd.print("Dir created     ");
    } else {
        Serial.println("mkdir failed");
        lcd.home();
        lcd.print("mkdir failed    ");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        lcd.home();
        lcd.print("SD card failed  ");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
        lcd.home();
        lcd.print("Append failed   ");
        cleared = false;
        lastread = millis();
    }
    file.close();
}
