#include <ESP8266WiFi.h>
#include <Wire.h>
#include <BlynkSimpleEsp8266.h>
#include "Timer.h"

#include "PMS.h"
#include "DHT.h"
#include "SoftwareSerial.h"

#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>

// WiFi auth
#define ssid "P1213 b7"
#define pass "bimz2021"

// Thingspeak stuff
#define apiKeyChannel "MWTTN33G6A2F945G"  // write API key
#define channelID 1424514                 // channel ID
#define apiServer "api.thingspeak.com"    // Thingspeak server

// Blynk authentication key
#define auth "J7aWXrbQlyRkBCRtsAzQeok1qZ-HRYXW"

// DHTxx sensor type and digital pin
#define DHTTYPE DHT11
#define dht_dpin 4

LiquidCrystal_I2C lcd(0x27,16,2); 
DHT dht(dht_dpin, DHTTYPE);
WiFiClient client;
// reading PMS7003 sensor with software-defined serial
SoftwareSerial pmsSerial(3, 1); // RX: GPIO3, TX: GPIO1
PMS pms(pmsSerial);
PMS::DATA pmsData;
// initialize timers
Timer timer;
BlynkTimer blynkTimer;
// initialize global variables to store sensors readings
float humidity, temperature;
int PM_2_5, PM_10;
// indicates state of Blynk connection 
bool begun = false;
// indicates whether to display PMS reading or DHT11 reading on LCD
bool lcdPMS = false;

void setup(){ 
  // Begin the software-defined serial for debugging and reading PMS7003 sensor
  pmsSerial.begin(9600);
  ThingSpeak.begin(client);
  dht.begin();
  // every 5 seconds, check WiFi connection and try to connect if not connected 
  timer.every(5000, connectWiFi);
  // every 8 seconds, check Blynk connection and try to connect if not connected
  blynkTimer.setInterval(8000, setupBlynk);
  // every 5 seconds, display sensors readings on LCD
  timer.every(5000, writeToLCD);
  // every 5 seconds, send sensors readings to Blynk server 
  blynkTimer.setInterval(5000, writeToBlynk);
  // every 20 seconds, send sensors readings to Thingspeak server 
  // Thingspeak needs 15-second invervals between requests
  timer.every(20000, sendToThingspeak);
}


void loop() {
  timer.update();
  blynkTimer.run();
  // read the sensor every loop since pms.read(pmsData) is not guaranteed to yield a result
  readSensor();
  client.stop();
}

void connectWiFi(){
  pmsSerial.println("WIFI");
  WiFi.mode(WIFI_STA);
  // if NodeMCU is not connected to WiFi
  if (WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, pass);
    pmsSerial.print(".");
    pmsSerial.println("\nWiFi Connected.");
    pmsSerial.println(WiFi.localIP());
  }
}

void readSensor(){
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (pms.read(pmsData)){
    PM_2_5 = pmsData.PM_AE_UG_2_5;
    PM_10 = pmsData.PM_AE_UG_10_0;
    pmsSerial.println("Dust Concentration");
    pmsSerial.println("PM2.5 :" + String(PM_2_5) + "(ug/m3)");
    pmsSerial.println("PM10  :" + String(PM_10) + "(ug/m3)");
  }
}

void setupBlynk(){
  pmsSerial.println("SETUP BLYNK");
  // if NodeMCU is connected to WiFi and not connected to Blynk server
  if ((WiFi.status() == WL_CONNECTED) && (!begun)){
    Blynk.begin(auth, ssid, pass, IPAddress(192,168,0, 14), 8080);
    pmsSerial.println("BLYNK BEGUN");
    begun = true;
  }
}


void writeToLCD(){
  // LCD's SDA is connected to GPIO2 and SCL to GPIO0
  Wire.begin(2, 0) ; 
  lcd.init(); 
  lcd.backlight();
  if (!lcdPMS){
    lcd.setCursor(0, 0);
    lcd.print("Nhiet do: ");
    lcd.print(temperature);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Do am: ");
    lcd.print(humidity);
    lcd.print("%");
    lcdPMS = true;
  } else {
    lcd.setCursor(0, 0);
    lcd.print("PM2.5: ");
    lcd.print(PM_2_5);
    lcd.print("ug/m3");
    lcd.setCursor(0, 1);
    lcd.print("PM10: ");
    lcd.print(PM_10);
    lcd.print("ug/m3");
    lcdPMS = false;
  }
//  Serial.println("LCD");
}


void writeToBlynk(){
  pmsSerial.println("BLYNK WRITE");
  if (Blynk.connected()){
    Blynk.virtualWrite(V6, temperature);
    Blynk.virtualWrite(V5, humidity);
    Blynk.virtualWrite(V4, PM_10);
    Blynk.virtualWrite(V3, PM_2_5);
  }
//  Serial.println("Blynk");
}

void sendToThingspeak(){
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, PM_2_5);
  ThingSpeak.setField(4, PM_10);
  int writeSuccess = ThingSpeak.writeFields(channelID, apiKeyChannel);
  if (writeSuccess) pmsSerial.println("THINGSPEAK");
  
}
