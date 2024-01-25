#include <Arduino.h>
#include <Adafruit_ST7789.h>

#include <Orbitron_Medium_40.h>
#include <Orbitron_Bold_20.h>
#include <lato_Bold_Italic_20.h>
#include <Roboto_Condensed_Light_Italic_24.h>
#include <Adafruit_BMP085.h>
#include <weather_icons.h>
#include <new_weather_icons.h>

#include <BluetoothSerial.h>
#include "driver/adc.h"
#include <esp_bt.h>

#include <NTPClient.h>
// change next line to use with another board/shield
#include <WiFi.h>
// #include <WiFi.h> // for WiFi shield
// #include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

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

Adafruit_BMP085 bmp;
// SHT31 sht31 = SHT31();

// Arduino_Canvas_Indexed *timeCanvas = new Arduino_Canvas_Indexed(240 /* width */, 110 /* height */, tft);
// Arduino_GFX *temperatureCanvas = new Arduino_Canvas(240 /* width */, 40 /* height */, tft);

GFXcanvas16 timeCanvas(240, 110);
GFXcanvas16 temperatureCanvas(240, 40);

bool bmpStatus = true;

RTC_DATA_ATTR int bootCount = 0; // data saved during sleep

typedef struct
{
  String city = "";
  String timezone = ""; // String timezone
  long utc_offset = 0;  // unix time
  String sunRise = "";
  String sunSet = "";
  float lat = 0;
  float lon = 0;
  float temp = 0;
  int humidity = 0;
  float forecastTemperature[12];
  uint32_t forecastTime[12];
  int forecastHumidity[12];
  String units = "";
  int weather_code = 0;
} Weather;

Weather weather;

void drawTimeConsole(void);
void drawTemperatureConsole(void);
void drawWeatherBitmap(int x, int y);
void ledBlink(void);
void getCity(void);
// int getTimezone(void);
String weatherApi(String url);
void getSunriseSunset();
void getWeather();
void getForecast();

// const char *ssid = "Casa do Tadeuzinho";
// const char *password = "#1nt3rn3t!";

// const char *ssid = "PU3D";
// const char *password = "#printup365";

const char *ssid = "knorst";
const char *password = "#1nt3rn3t!";

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionally you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60          /* Time ESP32 will go to sleep (in seconds) */

void setup(void)
{
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BACKLIGHT, OUTPUT);

  Serial.begin(115200);

  tft.init(240, 320, SPI_MODE2);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);

  digitalWrite(BACKLIGHT, HIGH);

  // WiFi.begin(ssid, password);
  // WiFi.begin(ssid, WPA2_AUTH_PEAP, username, password);

  // while (WiFi.status() != WL_CONNECTED)
  //{
  //   delay(500);
  //   Serial.print(".");
  // }

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setFont();
  tft.setTextSize(2);
  tft.print("WiFi connecting...");
  res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setFont();
    tft.setTextSize(2);
    tft.print("WiFi error");

    while (true)
    {
    }
    // ESP.restart();
  }

  tft.fillScreen(ST77XX_BLACK);
  // if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  Serial.print(F("\nHello! ST77xx TFT Test"));

  if (!bmp.begin())
  {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    bmpStatus = false;
  }

  // remember to modify SPI setting as necessary for your MCU board in Adafruit_ST77xx.cpp

  timeClient.begin();

  Serial.println(F("\nInitialized"));

  // draw the static icons
  tft.drawRGBBitmap(15, 135, sunrise, 40, 31);
  tft.drawRGBBitmap(85, 135, sunset, 40, 31);
  // tft.drawRGBBitmap(160, 128, isro, 68, 66);
  tft.drawRGBBitmap(160, 128, nasa, 80, 66);

  // draw the horizontal console dividing lines
  tft.drawLine(0, 120, 240, 120, 0x8c71); // gray
  tft.drawLine(0, 199, 240, 199, 0x8c71); // gray

  getCity();
  getWeather();
  getForecast();
  timeClient.setTimeOffset(weather.utc_offset);
  timeClient.update();

  // display the time and temperature and then update it regularly in the loop function

  getSunriseSunset();

  drawTimeConsole();
  drawTemperatureConsole();
}

void loop()
{
  if (weather.city == "")
  {
    getCity();
  }

  int i = 0;
  for (i; i < 12; i++)
  {
    if (hour(timeClient.getEpochTime()) == hour(weather.forecastTime[i]))
    {
      weather.temp = weather.forecastTemperature[i];
      weather.humidity = weather.forecastHumidity[i];
      break;
    }
  }
  Serial.println(i);

  if (i > 11)
  {
    Serial.println("Updating hourly forecast");
    getWeather(); // update hourly forecast temp
    timeClient.update();
  }

  if (bmpStatus)
  {
    weather.temp = bmp.readTemperature();
  }

  drawTimeConsole();
  drawTemperatureConsole();

  String timeNow = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
  Serial.println(timeNow + " - Going to light-sleep now");
  int updateSecs = 60 - timeClient.getSeconds();
  Serial.println("Update in: " + String(updateSecs));
  Serial.flush();
  esp_sleep_enable_timer_wakeup(updateSecs * uS_TO_S_FACTOR);
  esp_light_sleep_start();
}

void getCity(void)
{

  if (WiFi.status() == WL_CONNECTED)
  { // Check WiFi connection status

    WiFiClient client;
    HTTPClient http;
    String url = "http://ip-api.com/json/";

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

          // serializeJsonPretty(jsonDoc, Serial);

          // Check for parsing errors
          const char *value = jsonDoc["status"];

          if (!error && String(value) == "success")
          {
            const char *value = jsonDoc["city"];
            weather.city = String(value);
            weather.lat = jsonDoc["lat"];
            weather.lon = jsonDoc["lon"];
            const char *_timeZone = jsonDoc["timezone"];
            weather.timezone = String(_timeZone);
            // Serial.println(timeZone);
            //  Serial.println(value);
            //  Serial.println(lat, 4);
            //  Serial.println(lon, 4);
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
    }
    else
    {
      Serial.println("Unable to connect to server");
    }
    http.end();
    return;
  }
}

// The webhook response data will contain the sunrise and sunset time together as a string
void getSunriseSunset(void)
{
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setFont();
  tft.setTextSize(2);
  tft.getTextBounds(weather.sunRise, 5, 175, &x1, &y1, &w, &h); // get boundaries of drawn text
  tft.setCursor(int(15 - ((w - 40) / 2)), 175);                 // the icon offset minus the the half diference
  tft.print(weather.sunRise);
  tft.getTextBounds(weather.sunSet, 85, 175, &x1, &y1, &w, &h);
  tft.setCursor(int(85 - ((w - 40) / 2)), 175);
  tft.print(weather.sunSet);

  Serial.println(w);
  Serial.println(h);
}

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
  uint32_t epoch = timeClient.getEpochTime();
  uint8_t _hour = timeClient.getHours();
  uint8_t _minute = timeClient.getMinutes();
  uint8_t _second = timeClient.getSeconds();

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
  timeCanvas.setCursor(5, 105);
  timeCanvas.print(weather.city);

  drawWeatherBitmap(192, 45);

  timeCanvas.setFont();
  timeCanvas.setTextSize(2);
  timeCanvas.setTextColor(ST77XX_WHITE);

  timeCanvas.getTextBounds(String(weather.temp), 192, 95, &x1, &y1, &w, &h);
  timeCanvas.setCursor(int(192 - ((w - 192) / 2)), 95);
  timeCanvas.print(weather.temp);
  // if (todayHigh == "  ")
  //{
  //   timeCanvas.setCursor(205, 95);
  //   timeCanvas.print(todayLow);
  // }
  // else
  //{
  //   timeCanvas.setCursor(180, 95);
  //   timeCanvas.print(todayHigh);
  //   timeCanvas.print("|");
  //   timeCanvas.print(todayLow);
  // }
  //  timeCanvas->flush();
  tft.drawRGBBitmap(0, 0, timeCanvas.getBuffer(), timeCanvas.width(), timeCanvas.height());
}

void drawTemperatureConsole(void)
{
  // float temp = 30; //((sht31.getTemperature()) * 1.8) + 32;
  // float hum = 55;  // sht31.getHumidity();

  temperatureCanvas.fillScreen(ST77XX_BLACK);
  temperatureCanvas.drawRGBBitmap(5, 10, thermometer, 15, 30);

  temperatureCanvas.setTextSize(1);
  temperatureCanvas.setFont(&Roboto_Condensed_Light_Italic_24);
  temperatureCanvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  temperatureCanvas.setCursor(30, 30);
  temperatureCanvas.print(weather.temp);
  temperatureCanvas.print(weather.units);
  temperatureCanvas.setCursor(140, 30);
  temperatureCanvas.print(weather.humidity);
  temperatureCanvas.print("%rH");

  tft.drawRGBBitmap(0, 200, temperatureCanvas.getBuffer(), temperatureCanvas.width(), temperatureCanvas.height());
}

/*

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
*/
void drawWeatherBitmap(int x, int y)
{
  // timeCanvas.drawRGBBitmap(x, y, night, 40, 40);
  // return;

  if (weather.weather_code == 0)
    timeCanvas.drawRGBBitmap(x, y, sunny, 40, 40);

  if (weather.weather_code == 2)
    timeCanvas.drawRGBBitmap(x, y, partial_cloudy, 40, 40);

  else if (weather.weather_code == 1)
    timeCanvas.drawRGBBitmap(x, y, moon, 40, 40);

  else if (weather.weather_code == 1)
    timeCanvas.drawRGBBitmap(x, y, moon, 40, 40);

  else if (weather.weather_code == 2)
    timeCanvas.drawRGBBitmap(x, y, cloudy, 40, 40);

  else if (weather.weather_code == 3)
    timeCanvas.drawRGBBitmap(x, y, partial_cloudy, 40, 40);

  // else if (!strcmp(forecast.c_str(), "Windy"))
  //   timeCanvas.drawRGBBitmap(x, y, windy, 40, 40);
  else if (weather.weather_code == 61)
    timeCanvas.drawRGBBitmap(x, y, rain, 40, 40);
  else if (weather.weather_code == 63)
    timeCanvas.drawRGBBitmap(x, y, rain, 40, 40);
  else if (weather.weather_code == 65)
    timeCanvas.drawRGBBitmap(x, y, rain, 40, 40);

  else if (weather.weather_code == 95)
    timeCanvas.drawRGBBitmap(x, y, thunderStorm, 40, 40);

  else if (weather.weather_code == 71)
    timeCanvas.drawRGBBitmap(x, y, snow, 40, 40);
  else if (weather.weather_code == 73)
    timeCanvas.drawRGBBitmap(x, y, snow, 40, 40);
  else if (weather.weather_code == 75)
    timeCanvas.drawRGBBitmap(x, y, snow, 40, 40);

  else if (weather.weather_code == 3)
    timeCanvas.drawRGBBitmap(x, y, cloud, 40, 40);
}
/*
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

void getWeather()
{
  String url = "https://api.open-meteo.com/v1/dwd-icon?latitude=";
  url += String(weather.lat, 4);
  url += "&longitude=";
  url += String(weather.lon, 4);
  url += "&timezone=auto&timeformat=unixtime";
  url += "&&forecast_hours=12&timeformat=unixtime&hourly=temperature_2m,relative_humidity_2m";
  Serial.println(url);
  Serial.println("");

  String payload = weatherApi(url);

  // Parse the JSON string
  DynamicJsonDocument jsonDoc(4500);
  DeserializationError error = deserializeJson(jsonDoc, payload);

  serializeJsonPretty(jsonDoc, Serial);
  Serial.println("");

  const char *_units = jsonDoc["hourly_units"]["temperature_2m"];
  weather.units = String(_units);

  for (int i = 0; i < 12; i++)
  {
    weather.forecastTemperature[i] = jsonDoc["hourly"]["temperature_2m"][i];
    weather.forecastHumidity[i] = jsonDoc["hourly"]["relative_humidity_2m"][i];
    weather.forecastTime[i] = jsonDoc["hourly"]["time"][i];
    weather.forecastTime[i] += weather.utc_offset;
  }

  weather.temp = weather.forecastTemperature[0];
  weather.humidity = weather.forecastHumidity[0];
}

void getForecast()
{
  String url = "https://api.open-meteo.com/v1/dwd-icon?latitude=";
  url += String(weather.lat, 4);
  url += "&longitude=";
  url += String(weather.lon, 4);
  url += "&timezone=auto";
  // url += timeZone;
  url += "&timeformat=unixtime";
  url += "&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,weather_code&forecast_days=1";
  // url += "&hourly=temperature_2m&forecast_hours=1";
  // url = "https://api.open-meteo.com/v1/dwd-icon?latitude=-30.12&longitude=-51.27&hourly=temperature_2m";
  Serial.println(url);
  Serial.println("");

  String payload = weatherApi(url);

  // Serial.println("HTTP GET request successful");
  // Serial.println("Response: " + payload);

  // Parse the JSON string
  DynamicJsonDocument jsonDoc(4500);
  DeserializationError error = deserializeJson(jsonDoc, payload);

  serializeJsonPretty(jsonDoc, Serial);
  Serial.println("");

  weather.utc_offset = jsonDoc["utc_offset_seconds"];
  Serial.println(weather.utc_offset);

  u32_t _epoch = jsonDoc["daily"]["sunrise"][0];
  _epoch += weather.utc_offset;
  weather.sunRise = String(hour(_epoch));
  weather.sunRise += ":";
  weather.sunRise += String(minute(_epoch));

  _epoch = jsonDoc["daily"]["sunset"][0];
  _epoch += weather.utc_offset;
  Serial.println(_epoch);

  weather.sunSet = String(hour(_epoch));
  weather.sunSet += ":";
  weather.sunSet += String(minute(_epoch));

  weather.weather_code = jsonDoc["daily"]["weather_code"][0];
}

String weatherApi(String url)
{

  WiFiClientSecure client;
  client.setInsecure(); // the magic line, use with caution

  HTTPClient http;
  String payload = "";

  if (http.begin(client, url.c_str()))
  {
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK)
      {
        payload = http.getString();
      }
      else
      {
        Serial.println("HTTP GET request failed");
      }
    }
    else
    {
      Serial.println("Unable to connect to server");
    }
  }
  http.end();
  return payload;
}