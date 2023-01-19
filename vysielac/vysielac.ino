// ----------------------------------------------------------------------------------//
// Project           - Snímanie teploty, tlaku a vlhkosti pôdy a odosielanie údajov pomocou LoRA na prijímač
// Vypracovala       - Daniela Tomková
// Software Platform - C/C++,HTML,CSS, Arduino IDE, Libraries
// Hardware Platform - TTGo LoRa ESP32 OLED v1, 0.96" OLED, Arduino MKRWAN 1300, BMP280 Sensor, DHT22 Sensor
// Sensors Used      - BMP280 Senzor teploty a tlaku, Kapacitný senzor vlhkosti pôdy
// Last Modified     - 15. 01. 2023
// -------------------------------------------------------------------------------------

//Libraries for LoRa
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>

//Libraries for BMP280
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

//Libraries for OLED Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//Unicode support
#include <U8g2_for_Adafruit_GFX.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 866E6

//OLED pins
#define OLED_RST 23
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define CSMS 36

//global variables for temperature and Humidity
float temperature = 0;
float pressure = 0;
float humidity = 0;

int readDelay = 5000;
int sendDelay = 10000;

//initilize packet counter
int readingID = 0;
String LoRaMessage = "";

//define BMP instance
Adafruit_BMP280 bme; // I2C

////define OLED instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

uint32_t delayMS;

const int dry = 2672; // value for dry sensor
const int wet = 912; // value for wet sensor

//------------------Initialize OLED Module-------------------------------------------//
void initOLED() {
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);                 // connect u8g2 procedures to Adafruit GFX
  u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR08_te);
  u8g2_for_adafruit_gfx.setForegroundColor(WHITE);      // apply Adafruit GFX color
  u8g2_for_adafruit_gfx.setCursor(0, 15);               // start writing at this position
  u8g2_for_adafruit_gfx.print("LoRa vysielač");
  display.display();
}

//----------------------Initialize LoRa Module-----------------------------------------//
void initLoRa() {
  int counter;
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  
  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Increment readingID on every new reading
    Serial.println("Starting LoRa failed!");
    u8g2_for_adafruit_gfx.print("LoRa modul KO!"); 
  } else {
    Serial.println("LoRa Initialization OK!");
    u8g2_for_adafruit_gfx.print("LoRa modul OK!");
  }
  display.display();
  delay(2000);
}

//-------------------Initialize BMP280 Sensor-------------------------------------------//
void initBMP(){
  Serial.println(F("Initializing BMP280 Sensor"));
  if (!bme.begin(0x76)) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  Serial.println(F("BMP280 is OK!"));
}

//-------------------Initialize CSMS-------------------------------------------//
void initCSMS(){}

//-----------------function for fetching BMP280 readings--------------------------------//
void getBMPreadings()
{
 pressure=abs(bme.readPressure()/10);
 Serial.print(F("Pressure: "));
 Serial.print(pressure);
 Serial.println(F(" hPa"));

 temperature=bme.readTemperature();
 Serial.print(F("Temperature: "));
 Serial.print(temperature);
 Serial.println(F(" °C"));
}

//-----------------function for fetching CSMS readings--------------------------------//
void getCSMSreadings()
{
 int sensorVal=analogRead(CSMS);
 humidity = map(sensorVal, wet, dry, 100, 0); 

 if(humidity<0) humidity=0;
 if(humidity>100) humidity=100;
 
 Serial.print(F("Humidity: "));
 Serial.print(humidity);
 Serial.println(F(" %"));
}

//-----------------function for fetching All  readings at once--------------------------//
void getReadings()
{
  getCSMSreadings();
  getBMPreadings();
  delay(readDelay);
}

//-------------------------Display Readings on OLED-------------------------------------//
void displayReadings()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 15);
  u8g2_for_adafruit_gfx.println("LoRa vysielač");
  u8g2_for_adafruit_gfx.setCursor(0, 31);
  u8g2_for_adafruit_gfx.print("TEPLOTA");
  u8g2_for_adafruit_gfx.setCursor(50, 31);
  u8g2_for_adafruit_gfx.print(":");
  u8g2_for_adafruit_gfx.setCursor(60, 31);
  u8g2_for_adafruit_gfx.print(temperature);
  u8g2_for_adafruit_gfx.setCursor(110, 31);
  u8g2_for_adafruit_gfx.print("°C");
  u8g2_for_adafruit_gfx.setCursor(0, 47);
  u8g2_for_adafruit_gfx.print("VLHKOSŤ");
  u8g2_for_adafruit_gfx.setCursor(50, 47);
  u8g2_for_adafruit_gfx.print(":");
  u8g2_for_adafruit_gfx.setCursor(60, 47);
  u8g2_for_adafruit_gfx.print(humidity);
  u8g2_for_adafruit_gfx.setCursor(110, 47);
  u8g2_for_adafruit_gfx.print("%");
  u8g2_for_adafruit_gfx.setCursor(0, 63);
  u8g2_for_adafruit_gfx.print("TLAK");
  u8g2_for_adafruit_gfx.setCursor(50, 63);
  u8g2_for_adafruit_gfx.print(":");
  u8g2_for_adafruit_gfx.setCursor(60, 63);
  u8g2_for_adafruit_gfx.print(pressure);
  u8g2_for_adafruit_gfx.setCursor(110, 63);
  u8g2_for_adafruit_gfx.print("hPa");
  display.display();
  Serial.print("Sending packet: ");
  Serial.println(readingID);
  readingID++;
}

//------------------Send data to receiver node using LoRa-------------------------//
void sendReadings() {
  if (LoRa.begin(BAND)) {
    LoRaMessage = String(readingID) + "/" + String(temperature) + "&" + String(humidity) + "#" + String(pressure);
    //Send LoRa packet to receiver
    LoRa.beginPacket();
    LoRa.print(LoRaMessage);
    LoRa.endPacket();
    Serial.println("Packet Sent!");
    displayReadings();
    delay(sendDelay);
  }
}

void setup() 
{
  //initialize Serial Monitor
  Serial.begin(115200);
  //
  initCSMS();
  initBMP();
  initOLED();
  initLoRa();
}

void loop() {
  getReadings();
  sendReadings();
}
