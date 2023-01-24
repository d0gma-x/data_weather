#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <DHT.h>
#include <SDS198.h>
#define OLED_SDA 21
#define OLED_SCL 22

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
float p100 = 0;
SDS198 sensor_pm100;
#ifdef ESP32
HardwareSerial puerto_100(2);
#endif

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

  sensor_pm100.begin(&puerto_100);

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();
}
//Takes an average of readings on a given pin
//Returns the average
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

  sensor_pm100.read(&p100);

  String bme280_data = String(temp) + "/" + String(hum) + "/" + String(pres);
  String dht_data = String(dht_temp_cel) + "/" + String(dht_hum) + "/" + String(dht_temp_fa);
  String sds198_data = String(p100);

  Serial.println("---------------------------------");
  Serial.print("bme_data: ");
  Serial.println(bme280_data);
  Serial.print("dht_data: ");
  Serial.println(dht_data);
  Serial.print("UV(mW/cm^2): ");
  Serial.println(uvIntensity);
  Serial.print("pm_100(ug/m^3: ");
  Serial.println(sds198_data);
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
  display.println(sds198_data);
  display.display();

  delay(1000);
}
