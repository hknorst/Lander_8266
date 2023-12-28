#include <Arduino.h>

/**************************************************************************
 *
 * Interfacing ESP8266 NodeMCU with ST7789 TFT display (240x240 pixel).
 * Graphics test example.
 * This is a free software with NO WARRANTY.
 * https://simple-circuit.com/
 *
 *************************************************************************/
/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit 1.8" TFT Breakout w/SD card
    ----> http://www.adafruit.com/products/358
  The 1.8" TFT shield
    ----> https://www.adafruit.com/product/802
  The 1.44" TFT breakout
    ----> https://www.adafruit.com/product/2088
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams.
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional).

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 *************************************************************************/

#include <Adafruit_ST7789.h>

#include <Orbitron_Medium_40.h>
#include <Orbitron_Bold_20.h>
#include <lato_Bold_Italic_20.h>
#include <SHT31.h>
#include <weather_icons.h>

#include <NTPClient.h>
// change next line to use with another board/shield
#include <WiFi.h>
// #include <WiFi.h> // for WiFi shield
// #include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// SYSTEM_MODE(MANUAL);
// SYSTEM_THREAD(ENABLED);
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// RGB565 custom colors
#define COLOR_PINK 0xf816
#define COLOR_LIME 0x87e0
#define COLOR_PEACH 0xfd89

/****************
*****************
Pin assignment
*****************
Display   Boron
SDA       MOSI
SCL       SCK
RES       A5
DC        A4
*****************/

#define LED_BLUE -1
#define LED_RED -1
#define BACKLIGHT 17

// ST7789 TFT module connections
#define TFT_DC 16 // TFT DC  pin is connected to NodeMCU pin D1 (GPIO5)
#define TFT_RST 4 // TFT RST pin is connected to NodeMCU pin D2 (GPIO4)
#define TFT_CS 5  // TFT CS  pin is connected to NodeMCU pin D8 (GPIO15)
// initialize ST7789 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
// Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC /* DC */, TFT_CS /* CS */);
// Arduino_GFX *tft = new Arduino_ST7789(bus, TFT_RST, 2 /* rotation */);

// SHT31 sht31 = SHT31();
//  If you are using the Photon 2, please comment out the following since Photon 2 does not have a fuel gauge
//  FuelGauge fuel;

char num = 0;
char number[8];

const float Pi = 3.14159265359;

// Arduino_Canvas_Indexed *timeCanvas = new Arduino_Canvas_Indexed(240 /* width */, 110 /* height */, tft);
// Arduino_GFX *temperatureCanvas = new Arduino_Canvas(240 /* width */, 40 /* height */, tft);

GFXcanvas16 timeCanvas(240, 110);
GFXcanvas16 temperatureCanvas(240, 40);

String todayHigh, todayLow, dayoneHigh, dayoneLow, daytwoHigh, daytwoLow, daythreeHigh, daythreeLow;
String todayForecast, dayoneForecast, daytwoForecast, daythreeForecast;

unsigned long previousMillis = 0; // will store last time LED was updated
// constants won't change:
const long interval = 1000;   // milliiseconds
const long pubinterval = 600; // interval at which to publish in seconds. this is done by counting number of intervals
unsigned long intervalcount = 0;

String city = "";
float lati = 0;
float longi = 0;

void drawTimeConsole(void);
void drawTemperatureConsole(void);
void drawWeatherBitmap(int x, int y, String forecast);
void ledBlink(void);
void getCity(void);
int getTimezone(void);
void weatherApi(void);

uint32_t _epoch = 1703114897;

const char *ssid = "Casa do Tadeuzinho";
const char *password = "#1nt3rn3t!";

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionally you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

void setup(void)
{

  // set time zone to pacific standard time
  // Time.zone(-7);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BACKLIGHT, OUTPUT);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print(F("\nHello! ST77xx TFT Test"));

  // remember to modify SPI setting as necessary for your MCU board in Adafruit_ST77xx.cpp

  tft.init(240, 320, SPI_MODE2);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);

  digitalWrite(BACKLIGHT, HIGH);
  timeClient.begin();

  Serial.println(F("\nInitialized"));

  // draw the static icons
  tft.drawRGBBitmap(15, 135, sunrise, 40, 31);
  tft.drawRGBBitmap(85, 135, sunset, 40, 31);
  tft.drawRGBBitmap(160, 128, isro, 68, 66);

  // draw the horizontal console dividing lines
  tft.drawLine(0, 120, 240, 120, 0x8c71); // gray
  tft.drawLine(0, 199, 240, 199, 0x8c71); // gray

  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);

  getCity();
  timeClient.setTimeOffset(getTimezone());
  timeClient.update();
  // display the time and temperature and then update it regularly in the loop function

  delay(15000);

  drawTimeConsole();
  // drawTemperatureConsole();
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // timeClient.update();
    if (city.isEmpty())
    {
      getCity();
    }

    previousMillis = currentMillis;
    drawTimeConsole();
    // drawTemperatureConsole();

    // ledBlink();
    intervalcount++;
  }
  if (intervalcount > pubinterval)
  {
    intervalcount = 0;
    String data = String(10);
    // Particle.publish("get_day_weather", data, PRIVATE);
    // Particle.publish("get_night_weather", data, PRIVATE);
  }
}

void getCity(void)
{

  if (WiFi.status() == WL_CONNECTED)
  { // Check WiFi connection status

    WiFiClient client;
    HTTPClient http;
    String url = "http://ip-api.com/json/";
    city = "";

    if (http.begin(client, url))
    {
      int httpCode = http.GET();

      if (httpCode > 0)
      {
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = http.getString();
          // Serial.println("HTTP GET request successful");
          // Serial.println("Response: " + payload);

          // Parse the JSON string
          DynamicJsonDocument jsonDoc(1024);
          DeserializationError error = deserializeJson(jsonDoc, payload);

          serializeJsonPretty(jsonDoc, Serial);

          // Check for parsing errors
          const char *value = jsonDoc["status"];

          if (!error && String(value) == "success")
          {
            const char *value = jsonDoc["city"];
            city = String(value);
            const char *lat = jsonDoc["lat"];
            //lati = atof(lat);
            const char *lon = jsonDoc["lon"];
            //longi = atof(lon);
            Serial.println(city);
            Serial.println(value);
            Serial.println(lat);
            Serial.println(lon);
            return;
          }
          else
          {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
          }
        }
      }
      else
      {
        Serial.println("HTTP GET request failed");
      }

      http.end();
    }
    else
    {
      Serial.println("Unable to connect to server");
    }
  }
}

int getTimezone(void)
{

  if (WiFi.status() == WL_CONNECTED)
  { // Check WiFi connection status

    WiFiClient client;
    HTTPClient http;
    String url = "http://worldtimeapi.org/api/ip";

    if (http.begin(client, url))
    {
      int httpCode = http.GET();

      if (httpCode > 0)
      {
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = http.getString();
          // Serial.println("HTTP GET request successful");
          // Serial.println("Response: " + payload);

          // Parse the JSON string
          DynamicJsonDocument jsonDoc(1024);
          DeserializationError error = deserializeJson(jsonDoc, payload);

          

          const char *value = jsonDoc["abbreviation"];

          if (value != nullptr && value[0] != '\0')
          {
            // Check for parsing errors
            int offset = String(value).toInt();
            offset = offset * 60 * 60;
            // timeClient.setTimeOffset(offset);
            Serial.println(offset);
            return offset;
          }
          else
          {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return 0;
          }
        }
        else
        {
          Serial.println("HTTP GET request failed");
        }

        http.end();
        return 0;
      }
      else
      {
        Serial.println("Unable to connect to server");
        return 0;
      }
    }
  }
  return 0;
}

/*
// The webhook response data will contain the sunrise and sunset time together as a string
void getSunriseSunset(const char *event, const char *data)
{
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setFont();
  tft.setTextSize(2);
  tft.setCursor(5, 175);
  tft.print(data);
}

*/

String numtoWeekday(int num)
{
  switch (num)
  {
  case 1:
    return ("SUN");
  case 2:
    return ("MON");
  case 3:
    return ("TUE");
  case 4:
    return ("WED");
  case 5:
    return ("THU");
  case 6:
    return ("FRI");
  case 7:
    return ("SAT");
  }
  return "NULL";
}

void drawTimeConsole(void)
{

  u32_t epoch = timeClient.getEpochTime();
  Serial.println(epoch);

  // u32_t epoch = _epoch;
  uint8_t _hour = hour(epoch);
  uint8_t _minute = minute(epoch);
  uint8_t _second = second(epoch);
  uint8_t _weekday = weekday(epoch);
  uint8_t _day = day(epoch);
  uint16_t _year = year(epoch);
  uint8_t _month = month(epoch);
  char timestring[10];

  timeCanvas.fillScreen(ST77XX_BLACK);
  timeCanvas.setFont(&Lato_BoldItalic10pt7b);
  timeCanvas.setTextSize(1);
  timeCanvas.setTextColor(ST77XX_CYAN);
  timeCanvas.setCursor(0, 30);
  // timeCanvas.print("WED,19 APR,2023"); //sample string for testing
  // String data = numtoWeekday(_weekday) + ", ";
  timeCanvas.print(numtoWeekday(_weekday));
  timeCanvas.print(",");

  switch (_month)
  {
  case 1:
    timeCanvas.print("JAN ");
    // data += "JAN";
    break;
  case 2:
    timeCanvas.print("FEB ");
    // data += "FEV";
    break;
  case 3:
    timeCanvas.print("MAR ");
    // data += "MAR";
    break;
  case 4:
    // data += "ABR";
    timeCanvas.print("APR ");
    break;
  case 5:
    // data += "MAI";
    timeCanvas.print("MAY ");
    break;
  case 6:
    // data += "JUN";
    timeCanvas.print("JUN ");
    break;
  case 7:
    // data += "JUL";
    timeCanvas.print("JUL ");
    break;
  case 8:
    // data += "AGO";
    timeCanvas.print("AUG ");
    break;
  case 9:
    // data += "SET";
    timeCanvas.print("SEPT ");
    break;
  case 10:
    // data += "OUT";
    timeCanvas.print("OCT ");
    break;
  case 11:
    // data += "NOV";
    timeCanvas.print("NOV ");
    break;
  case 12:
    // data += "DEZ";
    timeCanvas.print("DEC ");
    break;
  }
  // data += " ";
  // data += _day;
  // data += ", ";
  // data += _year;
  timeCanvas.print(_day);
  timeCanvas.print(",");
  timeCanvas.print(_year);
  // tft.print(data);
  // Serial.print(data);

  timeCanvas.setFont();
  timeCanvas.setTextSize(1);
  timeCanvas.setCursor(205, 14);

  timeCanvas.fillRect(210, 0, 25, 10, ST77XX_BLUE);

  timeCanvas.fillRect(207, 3, 3, 4, ST77XX_BLUE);
  timeCanvas.setTextColor(ST77XX_WHITE);
  // If you are using the Photon 2, please comment out the following since Photon 2 does not have a fuel gauge
  // timeCanvas.print(fuel.getVCell());timeCanvas.print("V");

  timeCanvas.setFont(&Orbitron_Medium_40);

  timeCanvas.setTextSize(1);
  timeCanvas.setTextColor(ST77XX_GREEN);

  timeCanvas.setCursor(0, 75);
  sprintf(timestring, "%02d:%02d ", _hour, _minute);
  // sprintf(timestring, "%02d:%02d ", 2, 2); //test string that is the widest when displayed
  timeCanvas.print(timestring);
  // calculate the width of the time string so that you can appent the AM/PM at the end of it
  // this is required because the font width is not the same for all digits
  int16_t x1, y1;
  uint16_t w, h;
  timeCanvas.getTextBounds(timestring, 0, 75, &x1, &y1, &w, &h);

  timeCanvas.setFont(&Orbitron_Bold_20);
  timeCanvas.setTextSize(1);
  timeCanvas.setTextColor(0xe469); // orange
  timeCanvas.setCursor(w, 60);
  if (_hour > 12)
    timeCanvas.print(" PM");
  else
    timeCanvas.print(" AM");

  timeCanvas.setFont(&Lato_BoldItalic10pt7b);
  timeCanvas.setTextSize(1);
  timeCanvas.setTextColor(COLOR_PINK);
  timeCanvas.setCursor(0, 105);
  timeCanvas.print(city);

  // drawWeatherBitmap(192, 45, todayForecast);

  timeCanvas.setFont();
  timeCanvas.setTextSize(2);
  timeCanvas.setTextColor(ST77XX_WHITE);

  if (todayHigh == "  ")
  {
    timeCanvas.setCursor(205, 95);
    timeCanvas.print(todayLow);
  }
  else
  {
    timeCanvas.setCursor(180, 95);
    timeCanvas.print(todayHigh);
    timeCanvas.print("|");
    timeCanvas.print(todayLow);
  }
  // timeCanvas->flush();
  tft.drawRGBBitmap(0, 0, timeCanvas.getBuffer(), timeCanvas.width(), timeCanvas.height());
}

/*

void drawTemperatureConsole(void)
{
  float temp = ((sht31.getTemperature()) * 1.8) + 32;
  float hum = sht31.getHumidity();

  temperatureCanvas.fillScreen(ST77XX_BLACK);
  temperatureCanvas.drawRGBBitmap(5, 10, thermometer, 15, 30);

  temperatureCanvas.setTextSize(1);
  temperatureCanvas.setFont(&Roboto_Condensed_Light_Italic_24);
  temperatureCanvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  temperatureCanvas.setCursor(30, 30);
  temperatureCanvas.print(temp);
  temperatureCanvas.print("F");
  temperatureCanvas.setCursor(140, 30);
  temperatureCanvas.print(hum);
  temperatureCanvas.print("%rH");

  tft.drawRGBBitmap(0, 200, temperatureCanvas.getBuffer(), temperatureCanvas.width(), temperatureCanvas.height());
}

void getNightWeather(const char *event, const char *night_data)
{
  int index0, index1, index2, index3, index4, index6, index5, index7, index8, index9, index10, index11, index12;
  String name;
  String weatherString = String(night_data);
  Serial.println(night_data);



  index0 = weatherString.indexOf('~');
  name = weatherString.substring(0, index0);
  Serial.println(name);
  if (name == "Tonight")
  {
    Serial.println(night_data);
    todayHigh = "  ";
    index1 = weatherString.indexOf('~', index0 + 1);
    todayLow = weatherString.substring(index0 + 1, index1);

    index2 = weatherString.indexOf('~', index1 + 1);
    todayForecast = weatherString.substring(index1 + 1, index2);

    index3 = weatherString.indexOf('~', index2 + 1);
    dayoneHigh = weatherString.substring(index2 + 1, index3);

    index4 = weatherString.indexOf('~', index3 + 1);
    dayoneForecast = weatherString.substring(index3 + 1, index4);

    index5 = weatherString.indexOf('~', index4 + 1);
    dayoneLow = weatherString.substring(index4 + 1, index5);

    index6 = weatherString.indexOf('~', index5 + 1);
    daytwoHigh = weatherString.substring(index5 + 1, index6);

    index7 = weatherString.indexOf('~', index6 + 1);
    daytwoForecast = weatherString.substring(index6 + 1, index7);

    index8 = weatherString.indexOf('~', index7 + 1);
    daytwoLow = weatherString.substring(index7 + 1, index8);

    index9 = weatherString.indexOf('~', index8 + 1);
    daythreeHigh = weatherString.substring(index8 + 1, index9);

    index10 = weatherString.indexOf('~', index9 + 1);
    daythreeForecast = weatherString.substring(index9 + 1, index10);

    index11 = weatherString.indexOf('~', index10 + 1);
    daythreeLow = weatherString.substring(index10 + 1, index11);
  }
  else
    Serial.println("Not tonight");
}

void getDayWeather(const char *event, const char *day_data)
{
  int index0, index1, index2, index3, index4, index6, index5, index7, index8, index9, index10, index11, index12;
  String name;
  String weatherString = String(day_data);
  Serial.println(day_data);

  index0 = weatherString.indexOf('~');
  name = weatherString.substring(0, index0);
  Serial.println(name);
  if ((name == "Today") || (name == "This Afternoon"))
  {
    Serial.println(day_data);
    index1 = weatherString.indexOf('~', index0 + 1);
    todayHigh = weatherString.substring(index0 + 1, index1);

    index2 = weatherString.indexOf('~', index1 + 1);
    todayForecast = weatherString.substring(index1 + 1, index2);

    index3 = weatherString.indexOf('~', index2 + 1);
    todayLow = weatherString.substring(index2 + 1, index3);

    index4 = weatherString.indexOf('~', index3 + 1);
    dayoneHigh = weatherString.substring(index3 + 1, index4);

    index5 = weatherString.indexOf('~', index4 + 1);
    dayoneForecast = weatherString.substring(index4 + 1, index5);

    index6 = weatherString.indexOf('~', index5 + 1);
    dayoneLow = weatherString.substring(index5 + 1, index6);

    index7 = weatherString.indexOf('~', index6 + 1);
    daytwoHigh = weatherString.substring(index6 + 1, index7);

    index8 = weatherString.indexOf('~', index7 + 1);
    daytwoForecast = weatherString.substring(index7 + 1, index8);

    index9 = weatherString.indexOf('~', index8 + 1);
    daytwoLow = weatherString.substring(index8 + 1, index9);

    index10 = weatherString.indexOf('~', index9 + 1);
    daythreeHigh = weatherString.substring(index9 + 1, index10);

    index11 = weatherString.indexOf('~', index10 + 1);
    daythreeForecast = weatherString.substring(index10 + 1, index11);

    index12 = weatherString.indexOf('~', index11 + 1);
    daythreeLow = weatherString.substring(index11 + 1, index12);
  }
  else
    Serial.println("Not today");
}

void drawWeatherBitmap(int x, int y, String forecast)
{
  if (!strcmp(forecast.c_str(), "Sunny"))
    timeCanvas.drawRGBBitmap(x, y, sunny, 40, 40);

  if (!strcmp(forecast.c_str(), "Mostly Sunny"))
    timeCanvas.drawRGBBitmap(x, y, partial_cloudy, 40, 40);

  else if (!strcmp(forecast.c_str(), "Clear"))
    timeCanvas.drawRGBBitmap(x, y, moon, 40, 40);

  else if (!strcmp(forecast.c_str(), "Mostly Clear"))
    timeCanvas.drawRGBBitmap(x, y, moon, 40, 40);

  else if (!strcmp(forecast.c_str(), "Partly Cloudy"))
    timeCanvas.drawRGBBitmap(x, y, cloudy, 40, 40);

  else if (forecast.indexOf("Cloudy") > 0)
    timeCanvas.drawRGBBitmap(x, y, cloud, 40, 40);

  else if (!strcmp(forecast.c_str(), "Showers"))
    timeCanvas.drawRGBBitmap(x, y, rain, 40, 40);

  else if (!strcmp(forecast.c_str(), "Windy"))
    timeCanvas.drawRGBBitmap(x, y, windy, 40, 40);

  else if (forecast.indexOf("Rain") > 0)
    timeCanvas.drawRGBBitmap(x, y, rain, 40, 40);

  else if (forecast.indexOf("thunder") > 0)
    timeCanvas.drawRGBBitmap(x, y, rain_thunder, 40, 40);

  else if (forecast.indexOf("Snow") > 0)
    timeCanvas.drawRGBBitmap(x, y, snow, 40, 40);

  else if (forecast.indexOf("Overcast") > 0)
    timeCanvas.drawRGBBitmap(x, y, cloud, 40, 40);
}

void ledBlink(void)
{
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);

  for (int r = 0; r < 4; r++)
  {
    digitalWrite(LED_RED, HIGH);
    delay(30);
    digitalWrite(LED_RED, LOW);
    delay(30);
  }

  for (int b = 0; b < 4; b++)
  {
    digitalWrite(LED_BLUE, HIGH);
    delay(30);
    digitalWrite(LED_BLUE, LOW);
    delay(30);
  }
}

*/

void weatherApi(float lati, float longi)
{

  WiFiClient client;
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/dwd-icon?latitude=";
  url += lati;
  url += "&longitude=";
  url += longi;
  url += "&hourly=temperature_2m";
  Serial.println(url);

  if (http.begin(client, url))
  {
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
        // Serial.println("HTTP GET request successful");
        Serial.println("Response: " + payload);

        // Parse the JSON string
        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        // const char *value = jsonDoc["abbreviation"];

        // if (value != nullptr && value[0] != '\0')
        //{
        //  Check for parsing errors
        // int offset = String(value).toInt();
        // offset = offset * 60 * 60;
        //  timeClient.setTimeOffset(offset);
        // Serial.println(offset);
        // return offset;
        //}
        // else
        //{
        //  Serial.print("Failed to parse JSON: ");
        //  Serial.println(error.c_str());
        //  return 0;
        //}
      }
      else
      {
        Serial.println("HTTP GET request failed");
      }

      http.end();
      // return 0;
    }
    else
    {
      Serial.println("Unable to connect to server");
      // return 0;
    }
  }
}
// return 0;
