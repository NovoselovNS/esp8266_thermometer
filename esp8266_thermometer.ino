/*
 * Инструкции
 * 
 * Arduino IDE 1.8.9
 * https://arduinomaster.ru/platy-arduino/proshivka-esp8266-cherez-arduino-ide/
 * esp8266 by esp8266community версии 2.5.2
 * 
 * библиотеки в libraries.zip скопировать в папку ~/Arduino/libraries/
 * 
 * http://wikihandbk.com/wiki/ESP8266:%D0%9F%D1%80%D0%BE%D1%88%D0%B8%D0%B2%D0%BA%D0%B8/Arduino/%D0%A0%D0%B0%D0%B1%D0%BE%D1%82%D0%B0_%D1%81_%D1%84%D0%B0%D0%B9%D0%BB%D0%BE%D0%B2%D0%BE%D0%B9_%D1%81%D0%B8%D1%81%D1%82%D0%B5%D0%BC%D0%BE%D0%B9_%D0%B2_%D0%B0%D0%B4%D0%B4%D0%BE%D0%BD%D0%B5_ESP8266_%D0%B4%D0%BB%D1%8F_IDE_Arduino
 * https://github.com/esp8266/arduino-esp8266fs-plugin/releases
 * 
 * в Arduino IDE -> Инструменты
 * Плата: NodeMCU ESP12-E
 * Flash Size: 4M (2M SPIFFS)
 * все остальное не важно
 * 
 * после прошивки кода Инструменты -> ESP8266 Sketch Data Upload
 * 
 * 
 * Подключение настроено следующим образом:
  D2    : DHT22 DATA
  D3    : DS18B20 DATA
  D4    : DS1302 RST
  RX    : DS1302 CLK
  TX    : DS1302 DAT
  D5    : SD SCK
  D6    : SD MISO
  D7    : SD MOSI
  D8    : SD CS
*/

#include <ESP8266WiFi.h>
#include "ESP8266WebServer.h"
#include <FS.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <SD.h>
#include <DS1302.h>
#include <time.h>

#define LED 2
#define RX_PIN 3   // GPIO3
#define TX_PIN 1   // GPIO1
#define ONE_WIRE_BUS D3
#define TEMPERATURE_PRECISION 12
#define DHTPIN D2
#define DHTTYPE DHT22
#define CS_PIN  D8
#define RST_DS1302  D4
#define DAT_DS1302  TX_PIN
#define CLK_DS1302  RX_PIN

#define max_array 130
#define N_DS18B20 20

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
DS1302 rtc(RST_DS1302, DAT_DS1302, CLK_DS1302);

int last_ds18b20,last_sd_write,  points_10m = 0, points_120m = 0;
float _10m[N_DS18B20][max_array],  _120m[N_DS18B20][max_array], last[N_DS18B20];//температуры
float h_10m[max_array],  h_120m[max_array], last_humidity;//влажности
float max_t[N_DS18B20], min_t[N_DS18B20];//макс мин

time_t t_10[max_array], t_120[max_array];

bool sdcard;

Ticker ticker_10m, ticker_120m;

struct config_struct {
  char magic[32] = "ptffdsjbnk";//для того чтобы различать разные версии настроек, сохраненные в EEPROM, и не загружать неподходящую структуру
  DeviceAddress addresses[2][N_DS18B20] = {};// 2 таблицы адресов
  byte table_n = 0;//индекс таблицы адресов, 0 - автоматическая, 1 - заданная вручную
  bool active = true;//считывать информацию с датчков?
  char sta_ssid[32] = "*******";
  char sta_password[64] = "**********";
  char hostname[32] = "esp8266_thermometer";
  char ap_ssid[32] = "esp";
  int ds18b20_delay = 5000; //ms, перодичность измерений, само измерение занимает >750мс
  int sdcard_delay = 5000; //ms, периодичность записи на карту памяти
  int conn_timeout = 10000; //ms, время ожидания подключения к сети, если не успели, то создается своя точка доступа
  int _10m_delay = 10; //s
  int _120m_delay = 120; //s
  int _10m_max = max_array - 5;
  int _120m_max = max_array - 5;
  float calib[N_DS18B20]={};//калибровки, привязаны к номеру в таблице, а не к серийнику, поэтому сначала пронумеровать надо
} config;

config_struct load() {
  config_struct tmp;
  EEPROM.begin(1024);
  EEPROM.get(0, tmp);
  EEPROM.end();
  if (strcmp(config.magic, tmp.magic) == 0)
    return tmp;
  else
    return config;
}

void save() {
  EEPROM.begin(1024);
  EEPROM.put(0, config);
  EEPROM.commit();
  EEPROM.end();
}

void _connect()
{
  WiFi.hostname(config.hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.sta_ssid, config.sta_password);

  last_ds18b20 = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (millis() - last_ds18b20 > config.conn_timeout)
    {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(config.ap_ssid);
      return;
    }
  }
}

ESP8266WebServer server(80);

void setup()
{
  config = load();
  _connect();

  SPIFFS.begin();
  sdcard = SD.begin(CS_PIN);

  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) server.send(404, "text/plain", "FileNotFound");
  });

  server.on("/timedRefresh", timedRefresh_js);
  server.on("/save", save_js);
  server.on("/toggle", toggle_htm);
  server.on("/temp_js", temp_js);
  server.on("/reset_minmax", reset_minmax_js);
  server.on("/table", table_htm);
  server.on("/set_rtc", set_rtc);
  server.on("/get_rtc", get_rtc);
  server.on("/status.htm", status_htm);
  server.on("/nomera.htm", nomera_htm);
  server.on("/grafiki.htm", grafiki_htm);
  server.on("/calibrate.htm", calibrate_htm);
  server.on("/network.htm", network_htm);
  server.on("/all", all_htm);
  server.begin();

  sensors.begin();
  for (int k = 0; k < sensors.getDeviceCount(); k++)
    if (sensors.getAddress(config.addresses[0][k], k))
      sensors.setResolution(config.addresses[0][k], TEMPERATURE_PRECISION);

  dht.begin();
  rtc.halt(false);
  rtc.writeProtect(true);

  ticker_10m.attach(config._10m_delay, add_10m);
  ticker_120m.attach(config._120m_delay, add_120m);
  pinMode(LED, OUTPUT);
  reset_min_max();
  last_sd_write=millis();
}

void loop()
{
  digitalWrite(LED, LOW);//на самом деле встроенный светодиод вкл

  if (millis() - last_ds18b20 > config.ds18b20_delay)
  {
    getTemp();
    last_ds18b20 = millis();
  }

  delay(5);
  server.handleClient();
  digitalWrite(LED, HIGH);//на самом деле светодиод выкл
  delay(50);
}

void add_10m()
{
  if (!config.active) return;
  if (points_10m < config._10m_max)
  {
    h_10m[points_10m] = last_humidity;
    t_10[points_10m] = unix_time();
    for (int i = 0; i < N_DS18B20; i++)
      _10m[i][points_10m] = last[i];
    points_10m++;
  }
  else
  {
    shift_10();
    h_10m[points_10m - 1] = last_humidity;
    t_10[points_10m - 1] = unix_time();
    for (int i = 0; i < N_DS18B20; i++)
      _10m[i][points_10m - 1] = last[i];
  }
}

void add_120m()
{
  if (!config.active) return;
  if (points_120m < config._120m_max)
  {
    h_120m[points_120m] = last_humidity;
    t_120[points_120m] = unix_time();
    for (int i = 0; i < N_DS18B20; i++)
      _120m[i][points_120m] = last[i];
    points_120m++;
  }
  else
  {
    shift_120();
    h_120m[points_120m - 1] = last_humidity;
    t_120[points_120m - 1] = unix_time();
    for (int i = 0; i < N_DS18B20; i++)
      _120m[i][points_120m - 1] = last[i];
  }
}

String filt(float a)//отфильтровает явный неадекват на графиках, возникающий из-за плохого соединения, например
{
  if ((a < 80.0) && (a > 10.0))
    return String(a);
  else
    return  "null";
}

void reset_min_max()
{
  for (int i = 0; i < N_DS18B20; i++)
  {
    max_t[i] = -55.0;
    min_t[i] = 125.0;
  }
}

time_t unix_time()//аналог time() из <ctime>
{
  Time now = rtc.getTime();
  tm tm_;
  tm_.tm_sec = now.sec;
  tm_.tm_min = now.min;
  tm_.tm_hour = now.hour;
  tm_.tm_mday = now.date;
  tm_.tm_mon = now.mon - 1; //месяцы с нуля!
  tm_.tm_year = now.year - 1900;
  tm_.tm_isdst = 0;
  return mktime(&tm_);
}

String time2str(time_t a)
{
  tm tm_;
  char buf[80];
  memcpy(&tm_, localtime(&a), sizeof (struct tm));
  strftime (buf, 200, "%d.%m.%Y %X", &tm_);
  return String(buf);
}

void copy(String date,String name)
{
  File file_a = SPIFFS.open("/sd/"+name, "r");
  File file_b = SD.open(date+"/"+name, FILE_WRITE);
  file_a.seek(0);
  file_b.seek(0);
  
  size_t n;  
  uint8_t buf[2048];
  while ((n = file_a.read(buf, sizeof(buf))) > 0) {
    file_b.write(buf, n);
  }
  
  file_b.close();
  file_a.close();
}
