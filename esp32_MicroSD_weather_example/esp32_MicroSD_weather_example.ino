#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <RTClib.h>
#include <FS.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define SEALEVELPRESSURE_HPA (1013.25)
#define SD_CS 5

Adafruit_BME280 bme;
float temp, alt, pres, hum;
Adafruit_SH1106 display(21, 22);
RTC_DS1307 rtc;
String hora, minutos, segundos;

String dataMessage;
String clock_display, clock_data;

static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
float latitude, longitude;
String lat_str, lng_str;
String position_gps;

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  bool status;
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //define the type of display used and the I2C address
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();

  // Initialize SD card
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    //    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    //    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    //    return;    // init failed
  }

  File file = SD.open("/datalog.txt");
  if (!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/datalog.txt", "Temp, Press, Alt, Hum, Clock \r\n");
  }
  else {
    Serial.println("File already exists");
  }
  file.close();

  ss.begin(GPSBaud);

  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  delay(2);
}

void loop() {
  DateTime now = rtc.now();
  hora = now.hour();
  minutos = now.minute();
  segundos = now.second();

  clock_display = "clock: " + hora + ":" + minutos + ":" + segundos;

  while (ss.available() > 0)
    if (gps.encode(ss.read()))
      displayInfo();
}

void displayInfo() {
  if (gps.location.isValid())
  {
    latitude = gps.location.lat();
    lat_str = String(latitude , 6);
    longitude = gps.location.lng();
    lng_str = String(longitude , 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  temp = bme.readTemperature();
  pres = (bme.readPressure() / 100.0F);
  alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
  hum = bme.readHumidity();

  String t_bme = "t: " + String(temp) + " C";
  String p_bme = "p: " + String(pres) + " hPa";
  String a_bme = "a: " + String(alt) + " m";
  String h_bme = "h: " + String(hum) + " %";

  clock_data = String(hora) + ":" + String(minutos) + ":" + String(segundos);

  position_gps = lat_str + "/" + lng_str;

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(t_bme);
  display.setCursor(0, 10);
  display.println(p_bme);
  display.setCursor(0, 20);
  display.println(a_bme);
  display.setCursor(0, 30);
  display.println(h_bme);
  display.setCursor(0, 40);
  display.println(clock_display);
  //  display.setCursor(0, 40);
  //  display.println(hora);
  //  display.setCursor(10, 40);
  //  display.println(":");
  //  display.setCursor(20, 40);
  //  display.println(minutos);
  //  display.setCursor(30, 40);
  //  display.println(":");
  //  display.setCursor(40, 40);
  //  display.println(segundos);
  display.setCursor(0, 50);
  display.println(position_gps);
  display.display();

  dataMessage = String(temp) + "," + String(pres) + "," + String(alt) + "," + String(hum) + "," + String(clock_data) + "," + String(lat_str) + "," + String(lng_str) + "\r\n";
  appendFile(SD, "/datalog.txt", dataMessage.c_str());
  Serial.println(dataMessage);
  //  Serial.println(lat_str);
  //  Serial.println(lng_str);
  delay(3000);
}
