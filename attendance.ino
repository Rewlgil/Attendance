#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD.h"

#define SS_PIN  12
#define RST_PIN 13

#define CS_PIN 14

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x3f, 16, 2);
File dataFile;
WiFiManager wifiManager;
HTTPClient httpClient;

String url = "https://student.eng.chula.ac.th/app/checkrfid.php?rfid=";

String dirName = "/Attendance";
String fileName = "/ID";
String newFile = "";

String strID = "";
uint32_t lastread = 0;
bool cleared = true;
bool online = false;

String readtag(void);


void setup() {
  Serial.begin(115200);
//  wifiManager.resetSettings();
  lcd.begin();
  lcd.backlight(); 
  
  Serial.print("Initializing SD card...");
  lcd.home();
  lcd.print("checking SD card");
  lcd.home();
  delay(1000);
  
  if (!SD.begin(CS_PIN))
  {
    Serial.println("Card failed, or not present");
    lcd.print("card FAILED     ");
    while(1);
  }
  Serial.println("card initialized.");
  Serial.println("Creating Dir: /Attendance");
  if(!SD.mkdir(dirName))
  {
    Serial.println("mkdir failed");
    lcd.home();
    lcd.print("mkdir failed    ");
    while(1);
  }
  Serial.println("Dir created");
  lcd.home();
  lcd.print("Dir created     ");

  File dir = SD.open(dirName);
  if(!dir){
      Serial.println("Failed to open directory");
      while(1);
  }

  String number;
  uint8_t lastFile = 0;
  
  File file = dir.openNextFile();
  while(file)
  {
    String _fileName = file.name();
    Serial.println(_fileName);
    
    if (_fileName.substring(_fileName.lastIndexOf(".")) == ".TXT" &&
        _fileName.substring(_fileName.lastIndexOf("/"), 
                            _fileName.lastIndexOf("/") + fileName.length()) == fileName)
    {
      number = _fileName.substring(_fileName.lastIndexOf("/") + fileName.length(),
                                   _fileName.lastIndexOf("."));
                                   
      if (number.toInt() > lastFile)
        lastFile = number.toInt();
    }
    file = dir.openNextFile();
  }
  file.close();
  dir.close();

  fileName.concat((lastFile + 1) < 10 ? "0" : "");
  fileName.concat(lastFile + 1);
  fileName.concat(".TXT");
  newFile = dirName + fileName;

  file = SD.open(newFile, FILE_WRITE);
  if(!file)
  {
    Serial.println("Failed to open file for writing");
  }
  file.close();
  Serial.print("File created: ");   Serial.println(newFile);
  lcd.home();           lcd.print("File created!!! ");
  delay(2000);          lcd.clear();
  lcd.home();           lcd.print(dirName);
  lcd.setCursor(0, 1);  lcd.print(fileName);

  SPI.begin();
  rfid.PCD_Init();

  delay(5000);          lcd.clear();
  lcd.home();           lcd.print("connecting Wi-Fi");

  wifiManager.setTimeout(60);
  wifiManager.setAPCallback(configModeCallback);
  if(! wifiManager.autoConnect("card_reader")) 
  {
    Serial.println("failed to connect and hit timeout");
    online = false;
    lcd.clear();
    lcd.home();           lcd.print("offline mode    ");
  }
  else
  {
    online = true;
    lcd.clear();
    lcd.home();           lcd.print("connectted to : ");
    lcd.setCursor(0, 1);  lcd.print(WiFi.SSID());
  }
  Serial.print("\n\n");
  delay(5000);            lcd.clear();
  lcd.home();             lcd.print("tab to check-in");
}
void loop() 
{
  strID = readtag();
  if (strID != "")
  {
    Serial.println(strID);
    if (online)
    {
      url = "https://student.eng.chula.ac.th/app/checkrfid.php?rfid=";
      url.concat(strID);
      Serial.print("url = ");       Serial.println(url.c_str());
      httpClient.begin(url.c_str());
      int httpCode = httpClient.GET();
      
      if( httpCode == 200 ) 
      {
        String response = httpClient.getString();
        Serial.print("response: "); Serial.println(response);
        if (response == "0")
        {
          lcd.home();               lcd.print("  ---UNKNOW---  ");
          lcd.setCursor(0, 1);      lcd.print("                ");
          lcd.setCursor(0, 1);      lcd.print(strID);
          appendFile(strID + "\n");
        }
        else
        {
          lcd.home();               lcd.print("    WELCOME     ");
          lcd.setCursor(0, 1);      lcd.print("                ");
          lcd.setCursor(0, 1);      lcd.print(response);
          appendFile(response + "\n");
        }
      }
      else
      {
        Serial.printf("error HTTP code: %d\n", httpCode);
        lcd.clear();
        lcd.home();               lcd.print("error ");              lcd.print(httpCode);
        lcd.setCursor(0, 1);      lcd.print("                ");
        lcd.setCursor(0, 1);      lcd.print(strID);
        appendFile(strID + "\n");
      }
    }
    else
    {
      lcd.home();               lcd.print("  ---offline--- ");
      lcd.setCursor(0, 1);      lcd.print("                ");
      lcd.setCursor(0, 1);      lcd.print(strID);
      appendFile(strID + "\n");
    }
    
    lastread = millis();
    cleared = false;
  }
  if (millis() - lastread >= 4000 && cleared == false)
  {
    lcd.home();             lcd.print("tab to check-in ");
    lcd.setCursor(0, 1);    lcd.print("                ");
    cleared = true;
  }
}

void configModeCallback (WiFiManager *mywifiManager){
  lcd.home();           lcd.print("Starting AP...  ");
  lcd.setCursor(0, 1);  lcd.print("AP : card_reader");
}

String readtag(void){
  String strID = "";

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
//    Serial.print("PICC type: ");
//    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
//    Serial.println(rfid.PICC_GetTypeName(piccType));

    for (byte i = 0; i < 7; i++)
    {
      strID += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
         String(rfid.uid.uidByte[i], HEX);
    }
    strID.toUpperCase();
//    Serial.println(strID);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  return strID;
}

void appendFile(String message){
    Serial.println("Appending to file");

    File file = SD.open(newFile, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending\n");
        lcd.home();       lcd.print("SD card failed  ");
        return;
    }
    if(file.print(message))
        Serial.println("Message appended\n");
    else
    {
        Serial.println("Append failed\n");
        lcd.home();       lcd.print("Append failed   ");
        cleared = false;
        lastread = millis();
    }
    file.close();
}
