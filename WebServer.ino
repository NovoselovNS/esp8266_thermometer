

String getContentType(String filename) {
  if (server.hasArg("download"))    return "application/octet-stream";
  else if (filename.endsWith(".htm"))  return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css"))  return "text/css";
  else if (filename.endsWith(".js"))   return "application/javascript";
  else if (filename.endsWith(".json")) return "text/json";
  else if (filename.endsWith(".png"))  return "image/png";
  else if (filename.endsWith(".gif"))  return "image/gif";
  else if (filename.endsWith(".jpg"))  return "image/jpeg";
  else if (filename.endsWith(".ico"))  return "image/x-icon";
  else if (filename.endsWith(".xml"))  return "text/xml";
  else if (filename.endsWith(".pdf"))  return "application/x-pdf";
  else if (filename.endsWith(".zip"))  return "application/x-zip";
  else if (filename.endsWith(".gz"))   return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz)) path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  if (SD.exists(path)) {
    File file = SD.open(path, FILE_READ);
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void temp_js()
{
  if (server.arg("h") != "")
  {
    int h = server.arg("h").toInt();
    String data = "var humidity= [\n";
    if (h == 10)
    {
      for (int i = 0; i < points_10m; i++)
      {
        if ((h_10m[i] > 0.0) && (h_10m[i] < 110.0))
          data += String(h_10m[i]);
        else
          data += "null";
        data += String(",\n");
      }
    }
    else if (h == 120)
    {
      for (int i = 0; i < points_120m; i++)
      {
        if ((h_120m[i] > 0.0) && (h_120m[i] < 110.0))
          data += String(h_120m[i]);
        else
          data += "null";
        data += String(",\n");
      }
    }
    data += "]";
    server.send(200, "application/javascript", data);
    return;
  }

  if (server.arg("sr") != "")
  {
    byte nenorm = 0;
    float srednee;
    int sr = server.arg("sr").toInt();
    String data = "var sr" + String(sr) + "= [\n";
    for (int i = 0; i < points_10m; i++)
    {
      srednee = 0;
      for (int k = sr * 5; k < (sr * 5 + 5); k++)
      {
        srednee += _10m[k][i] + config.calib[k];
        nenorm += (filt(_10m[k][i] + config.calib[k]) == "null");
      }
      srednee /= 5.0;
      if (nenorm)
        data += "null";
      else
        data += String(srednee);
      data += String(",\n");
    }
    data += "]";
    server.send(200, "application/javascript", data);
    return;
  }

  if (server.arg("srd") != "")
  { byte nenorm = 0;
    float srednee;
    int sr = server.arg("srd").toInt();
    String data = "var srd" + String(sr) + "= [\n";
    for (int i = 0; i < points_120m; i++)
    {
      srednee = 0;
      for (int k = sr * 5; k < (sr * 5 + 5); k++)
      {
        srednee += _120m[k][i] + config.calib[k];
        nenorm += (filt(_120m[k][i] + config.calib[k]) == "null");
      }
      srednee /= 5.0;
      if (nenorm)
        data += "null";
      else
        data += String(srednee);
      data += String(",\n");
    }
    data += "]";
    server.send(200, "application/javascript", data);
    return;
  }

  if (server.arg("n") != "")
  {
    int k = server.arg("n").toInt();
    String data = "var dat" + String(k) + "= [\n";
    for (int i = 0; i < points_10m; i++)
    {
      data += filt(_10m[k][i] + config.calib[k]);
      data += String(",\n");
    }
    data += "]";
    server.send(200, "application/javascript", data);
    return;
  }

  if (server.arg("nd") != "")
  {
    int k = server.arg("nd").toInt();
    String data = "var dat" + String(k) + "= [\n";
    for (int i = 0; i < points_120m; i++)
    {
      data += filt(_10m[k][i] + config.calib[k]);
      data += String(",\n");
    }
    data += "]";
    server.send(200, "application/javascript", data);
    return;
  }

  if (server.arg("t") != "")
  {
    int k = server.arg("t").toInt();
    String data = "var time= [\n";
    if (k == 10)
    {
      for (int i = 0; i < points_10m; i++)
        data += String("\"" + time2str(t_10[i]) + "\",\n");
    }
    else if (k == 120)
    {
      for (int i = 0; i < points_120m; i++)
        data += String("\"" + time2str(t_120[i]) + "\",\n");
    }

    data += "]";
    server.send(200, "application/javascript", data);
    return;
  }
}

String saved_htm(String url)
{
  String message = "<html>\n\
        <head>\n\
        <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\
        <title>...</title>\n\
        <meta http-equiv=\"refresh\" content=\"1; url=" + url + "\">\n\
        </head>\n\
        <body>\n\
        <script src=\"save\" type=\"text/javascript\"></script>\n\
        <h2>Сохранено</h2>\n\
        </body>\n\
        </html>\n";
  return message;
}


void calibrate_htm()
{
  if (server.arg("ds1") != "")
  {
    for (int i = 0; i < N_DS18B20; i++)
      config.calib[i] = server.arg("ds" + String(i)).toFloat();
    server.send(200, "text/html", saved_htm("/calibrate.htm"));
    return;
  }

  String message = "<html>\n\
  <head>\n\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\
  <title>Калибровка</title>\n\
  <style>table, th, td {border: 1px solid black;}</style>\n\
  </head>\n\
  <body>\n\
  <h2>Калибровка</h2>\n\
  <form action=\"/calibrate.htm\" method=\"GET\">\n\
  <table>\n\
  <tbody>\n";

  for (int i = 0; i < N_DS18B20; i++)
  {
    message += "<tr><td>" + String(i / 5 + 1) + "-" + String(i % 5 + 1) + + "</td><td><input type=\"number\" min=\"any\" max=\"any\" step=\"0.01\" name=\"ds" + String(i) + "\" value=" + String(config.calib[i]) + "></td></tr>\n";
  }
  message += "\
  <tr><td></td><td><input type=\"submit\" value=\"Сохранить\"></td></tr>\n\
  </tbody>\n\
  </table>\n\
  </form>\n\
  </body>\n\
  </html>\n";

  server.send(200, "text/html", message);
}

void table_htm()
{
  /* 0.2
     .1.
     4.3
  */
  if (server.arg("n") != "")
  {
    int k = server.arg("n").toInt();  //k - лоток 0..3
    String message = "<html>\
  <head><title></title><meta charset=utf-8><meta http-equiv=\"refresh\" content=\"" + String(config.ds18b20_delay / 1000) + "; url=/table?n=" + String(k) + "\"></head>\
  <body><table style=\"width:30%\" height=\"100% \">\n\
  <tr><td title=\"" + String(k + 1) + "-1 min: " + String(min_t[k * 5 + 0] + config.calib[k * 5 + 0]) + " max: " + String(max_t[k * 5  + 0] + config.calib[k * 5 + 0]) + "\">" + String(last[k * 5 + 0] + config.calib[k * 5 + 0]) + "</td><td></td><td title=\"" + String(k + 1) + "-3 min: " + String(min_t[k * 5 + 1] + config.calib[k * 5 + 1]) + " max: " + String(max_t[k * 5 + 1] + config.calib[k * 5 + 1]) + "\">" + String(last[k * 5 + 1] + config.calib[k * 5 + 1]) + "</td></tr>\n\
  <tr><td></td><td title=\"" + String(k + 1) + "-2 min: " + String(min_t[k * 5 + 2] + config.calib[k * 5 + 2]) + " max: " + String(max_t[k * 5 + 2] + config.calib[k * 5 + 2]) + "\">" + String(last[k * 5 + 2] + config.calib[k * 5 + 2]) + "</td><td></td></tr>\n\
  <tr><td title=\"" + String(k + 1) + "-5 min: " + String(min_t[k * 5 + 4] + config.calib[k * 5 + 4]) + " max: " + String(max_t[k * 5 + 4] + config.calib[k * 5 + 5]) + "\">" + String(last[k * 5 + 4] + config.calib[k * 5 + 4]) + "</td><td></td><td title=\"" + String(k + 1) + "-4 min: " + String(min_t[k * 5 + 3] + config.calib[k * 5 + 3]) + " max: " + String(max_t[k * 5 + 3] + config.calib[k * 5 + 3]) + "\">" + String(last[k * 5 + 3] + config.calib[k * 5 + 3]) + "</td></tr>\n\
</table> </body</html>";
    server.send(200, "text/html", message);
  }
}

void status_htm()
{
  int time = millis() / 1000;

  char time_str[100];
  sprintf(time_str, "%02d:%02d:%02d", (time / 3600), ((time % 3600) / 60), (time % 60));
  String message = "<html>\n\
  <head>\n\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\
  </head><body>\n";
  
  
                   message+= "<br>Время работы: " + String(time_str)\
                   +"<br>Измерения"+(config.active?" идут ":" не идут ")+"<a href=/toggle>Переключить</a>"\
                   +"<br>Время на часах: " + rtc.getDateStr() + String(" ") + rtc.getTimeStr()\
                   +"<br>SD карта " + (sdcard ? String() : String("не ")) + String("работает")\
                   +"<br>Свободная память: " + String(ESP.getFreeHeap())\
                   +"<br>DHT22: " +  dht.readHumidity() + String("% ") + dht.readTemperature() + String("C")\
                   +"<br>Подключено датчиков DS18B20: " + String(sensors.getDeviceCount()) + "<br>";


  for (int k = 0; k < sensors.getDeviceCount(); k++)
    message += printAddress(config.addresses[0][k]) + "<br>";
message+="</body><html>";
  server.send(200, "text/html", message);
}

void network_htm()
{
  if (server.arg("sta_ssid") != "")
  {
    strcpy(config.sta_ssid, server.arg("sta_ssid").c_str());
    strcpy(config.sta_password, server.arg("sta_password").c_str());
    strcpy(config.hostname, server.arg("hostname").c_str());
    strcpy(config.ap_ssid, server.arg("ap_ssid").c_str());
    server.send(200, "text/html", saved_htm("/network.htm"));
    return;
  }
  String message = "<html>\
  <head><title></title><meta charset=utf-8><style>table, th, td {border: 1px solid black;}</style></head>\
  <body>\n\
  <form action=\"/network.htm\" method=\"GET\">\n\<table>\n\
  <tr><td>STA_SSID:</td><td><input type=\"text\" name=\"sta_ssid\" value=\"" + String(config.sta_ssid) + "\"></td></tr>\n\
  <tr><td>STA_password:</td><td><input type=\"password\" name=\"sta_password\" value=\"" + String(config.sta_password) + "\"></td></tr>\n\
  <tr><td>hostname:</td><td><input type=\"text\" name=\"hostname\" value=\"" + String(config.hostname) + "\"></td></tr>\n\
  <tr><td>AP_SSID:</td><td><input type=\"text\" name=\"ap_ssid\" value=\"" + String(config.ap_ssid) + "\"></td></tr>\n\
  <tr><td></td><td><input type=\"submit\" value=\"Сохранить\"></td></tr>\n\
  </table></form></body</html>";
  server.send(200, "text/html", message);
}

void save_js()
{
  save();
  server.send(200, "application/javascript", "");
}

void toggle_htm()
{
  config.active^=true;
  server.send(200, "text/html", saved_htm("/status.htm"));
}


void nomera_htm()
{
  if (server.arg("nom1") != "")
  {
    config.table_n = server.arg("table").toInt();
    for (int k = 0; k < N_DS18B20; k++) //k=0..19
    {
      char code[24];
      strcpy(code, server.arg("nom" + String(k)).c_str());
      int values[8];
      if ( 8 == sscanf( code, "%x:%x:%x:%x:%x:%x:%x:%x%*c",
                        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6], &values[7] ) )
      {
        for (int i = 0; i < 8; ++i )
        {
          config.addresses[1][k][i] = (uint8_t) values[i];
        }
      }
    }
    server.send(200, "text/html", saved_htm("/nomera.htm"));
    return;
  }

  String message = "<html>\n\
  <head>\n\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\
  <title>Нумерация</title>\n\
  <style>table, th, td {border: 1px solid black;}</style>\n\
  </head>\n\
  <body>\n\
  <h2>Нумерация</h2>\n\
  <form action=\"/nomera.htm\" method=\"GET\">\n\
  <table>\n\
  <tbody>\n";

  message += "<tr><th>подключенные датчики</th><th>t(без калибровки)</th><th>.....</th><th>№</th><th>код датчика для номера</th></tr>";
  for (int i = 0; i < N_DS18B20; i++)
  {
    message += "<tr><td>" + printAddress(config.addresses[0][i]) + "</td>\
    <td>" + String(last[i]) + "</td>\
    <td>.....</td>\
    <td>" + String(i / 5 + 1) + "-" + String(i % 5 + 1) + "</td>\
    <td><input type=\"text\" name=\"nom" + String(i) + "\" value=" + printAddress(config.addresses[1][i]) + "></td>\
    </tr>\n";
  }
  message += "\
  <tr><td><input name=\"table\" type=\"radio\" value=\"0\" " + (config.table_n == 0 ? String("checked") : String("")) + " >использовать эту таблицу</td>\
  <td><input type=\"submit\" value=\"Сохранить\"></td>\
  <td></td>\
  <td></td>\
  <td><input name=\"table\" type=\"radio\" value=\"1\" " + (config.table_n == 1 ? String("checked") : String("")) + " >использовать эту таблицу</td></tr>\n\
  </tbody>\n\
  </table>\n\
  </form>\n\
  </body>\n\
  </html>\n";


  server.send(200, "text/html", message);
}

void grafiki_htm()
{

  if (server.arg("_10m_delay") != "")
  {
    config._10m_delay = server.arg("_10m_delay").toInt();
    config._120m_delay = server.arg("_120m_delay").toInt();
    config._10m_max = server.arg("_10m_max").toInt();
    config._120m_max = server.arg("_120m_max").toInt();
    config.ds18b20_delay = server.arg("ds18b20_delay").toInt();
    config.sdcard_delay = server.arg("sdcard_delay").toInt();
    server.send(200, "text/html", saved_htm("/grafiki.htm"));
    return;
  }

  String message = "<html>\
  <head><title></title><meta charset=utf-8><style>table, th, td {border: 1px solid black;}</style></head>\
  <body>\n\
  <form action=\"/grafiki.htm\" method=\"GET\">\n\<table>\n\
  <tr><td>Период измерений для графика 10мин, сек</td><td><input type=\"number\" min=\"5\" max=\"any\" step=\"1\" name=\"_10m_delay\" value=\"" + String(config._10m_delay) + "\"></td></tr>\n\
  <tr><td>Период измерений для графика 120мин, сек</td><td><input type=\"number\" min=\"5\" max=\"any\" step=\"1\" name=\"_120m_delay\" value=\"" + String(config._120m_delay) + "\"></td></tr>\n\
  <tr><td>число точек для 10</td><td><input type=\"number\" min=\"10\" max=\"150\" step=\"1\" name=\"_10m_max\" value=\"" + String(config._10m_max) + "\"></td></tr>\n\
  <tr><td>число точек для 120</td><td><input type=\"number\" min=\"10\" max=\"150\" step=\"1\" name=\"_120m_max\" value=\"" + String(config._120m_max) + "\"></td></tr>\n\
  <tr><td>Частота измерений, мс</td><td><input type=\"number\" min=\"1000\" max=\"any\" step=\"1\" name=\"ds18b20_delay\" value=\"" + String(config.ds18b20_delay) + "\"></td></tr>\n\
  <tr><td>Частота записи на карту, мс</td><td><input type=\"number\" min=\"1000\" max=\"any\" step=\"1\" name=\"sdcard_delay\" value=\"" + String(config.sdcard_delay) + "\"></td></tr>\n\
  <tr><td></td><td><input type=\"submit\" value=\"Сохранить\"></td></tr>\n\
  </table></form></body></html>";
  server.send(200, "text/html", message);
}

void set_rtc()
{
  if (server.arg("d") != "")
  {
    rtc.halt(true);
    rtc.writeProtect(false);
    rtc.setTime(server.arg("hours").toInt(), server.arg("min").toInt(), server.arg("sec").toInt());
    rtc.setDate(server.arg("d").toInt(), server.arg("m").toInt(), server.arg("y").toInt());
    rtc.halt(false);
    rtc.writeProtect(true);
    server.send(200, "text/html", saved_htm("/clock.htm"));
  }
  else
    server.send(452, "text/plain", "452");

}

void get_rtc()
{
  Time now = rtc.getTime();
  String message = "var date = [" + String(now.date) + ", " + String(now.mon) + ", " + String(now.year) + "]\n";
  message += "var time = [" + String(now.hour) + ", " + String(now.min) + ", " + String(now.sec) + "]\n";
  server.send(200, "application/javascript", message);
}

void timedRefresh_js()
{
  if (server.arg("t") != "")
  {
    int t = server.arg("t").toInt();
    if (t == 10)
      t = config._10m_delay;
    else if (t == 120)
      t = config._120m_delay;

    t *= 1000;//ms
    String message = "\
    function timedRefresh(timeoutPeriod) {\n\
    setTimeout(\"location.reload(true); \",timeoutPeriod);\n\
    }\n\
    window.onload = timedRefresh(" + String(t) + ");";
    server.send(200, "application/javascript", message);//?
  }
}


void reset_minmax_js()
{
  reset_min_max();
  server.send(200, "text/html", saved_htm("/"));
}

void all_htm()
{
  String message = "\
  <html>\n\
<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'></head>\n\
<body>\n\
  Даты измерений<br>\n";
  File root = SD.open("/");
  File entry;
  while (true)
  {
    entry = root.openNextFile();
    if (!entry)
      break;
    if ((entry.isDirectory()) && (SD.exists(String("/") + entry.name() + String("/time.js"))))
    {
      message += "<a href= \"" + String(entry.name()) + "/\">" + String(entry.name()) + "</a><br>\n";
    }
    entry.close();
  }
  root.close();

  message += "\
    <a href=/>На главную</a>\n\
  </body>\n\
</html>";
  server.send(200, "text/html", message);
}
