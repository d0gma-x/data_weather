#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <DHT.h>
#include <SDS011.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#define OLED_SDA 21
#define OLED_SCL 22
#define SD_CS 13

int UVOUT = 15; //Output from the sensor
int REF_3V3 = 4; //3.3V power on the ESP32 board

//BME280
Adafruit_BME280 bme;
float temp, pres, hum;
int status;
//Pantalla OLED
Adafruit_SH1106 display(21, 22);
//DHT22
#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht (DHTPIN, DHTTYPE);
float dht_temp_cel, dht_hum, dht_temp_fa;
//SDS198
float p10 = 0;
float p25 = 0;
SDS011 sensor_pm10;
#ifdef ESP32
HardwareSerial puerto_10(2);
#endif
//MicroSD
String dataMessage;

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

void setup()
{
  Serial.begin(115200);
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);

  dht.begin();

  bool status;
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("ERROR BME280");
    while (1);
  }

  sensor_pm10.begin(&puerto_10);

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();

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
}

int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for (int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return (runningValue);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void loop()
{
  int uvLevel = averageAnalogRead(UVOUT);
  int refLevel = averageAnalogRead(REF_3V3);
  //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
  float outputVoltage = 3.3 / refLevel * uvLevel;
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level

  temp = bme.readTemperature();
  pres = (bme.readPressure() / 100.0F);
  hum = bme.readHumidity();

  dht_temp_cel = dht.readTemperature();
  dht_hum = dht.readHumidity();
  dht_temp_fa = dht.readTemperature(true);

  sensor_pm10.read(&p10, &p25);

  String bme280_data = String(temp) + "/" + String(hum) + "/" + String(pres);
  String dht_data = String(dht_temp_cel) + "/" + String(dht_hum) + "/" + String(dht_temp_fa);
  String sds011_data = String(p10) + "/" + String(p25);

  Serial.println("---------------------------------");
  Serial.print("bme_data: ");
  Serial.println(bme280_data);
  Serial.print("dht_data: ");
  Serial.println(dht_data);
  Serial.print("UV(mW/cm^2): ");
  Serial.println(uvIntensity);
  Serial.print("pm_100(ug/m^3: ");
  Serial.println(sds011_data);
  Serial.println("---------------------------------");

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(bme280_data);
  display.setCursor(0, 10);
  display.println(dht_data);
  display.setCursor(0, 20);
  display.println(uvIntensity);
  display.setCursor(0, 30);
  display.println(sds011_data);
  display.display();

  dataMessage = String(temp) + "," + String(pres) + "," + String(alt) + "," + String(hum) + "," + String(clock_data) + "," + String(lat_str) + "," + String(lng_str) + "\r\n";
  appendFile(SD, "/datalog.txt", dataMessage.c_str());

  delay(1000);
}
