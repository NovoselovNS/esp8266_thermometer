
String printAddress(DeviceAddress deviceAddress)
{
  char addr[16];
  sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
          deviceAddress[0],
          deviceAddress[1],
          deviceAddress[2],
          deviceAddress[3],
          deviceAddress[4],
          deviceAddress[5],
          deviceAddress[6],
          deviceAddress[7]);
  return String(addr);
}

void getTemp()
{
  if (!config.active) return;

  sensors.requestTemperatures();

  last_humidity = dht.readHumidity();
  for (int i = 0; i < N_DS18B20; i++)
  {
    last[i] = sensors.getTempC(config.addresses[config.table_n][i]);
    //last[i] = 30 + (float)((rand() % 100) / 100.0) + rand() % 9;
    //last_humidity = rand() % 70 + 20;
    if (last[i] > max_t[i]) max_t[i] = last[i];
    if (last[i] < min_t[i]) min_t[i] = last[i];
  }

  _write();
}

void shift_10()
{
  for (int k = 0; k < N_DS18B20; k++)
    for (int i = 0; i < max_array - 1; i++)
      _10m[k][i] = _10m[k][i + 1];

  for (int i = 0; i < max_array - 1; i++)
  {
    h_10m[i] = h_10m[i + 1];
    t_10[i] = t_10[i + 1];
  }
}

void shift_120()
{
  for (int k = 0; k < N_DS18B20; k++)
    for (int i = 0; i < max_array - 1; i++)
      _120m[k][i] = _120m[k][i + 1];

  for (int i = 0; i < max_array - 1; i++)
  {
    h_120m[i] = h_120m[i + 1];
    t_120[i] = t_120[i + 1];
  }
}

void _write()
{
  if ( (!config.active) || (!sdcard) || ((millis() - last_sd_write) < config.sdcard_delay) )
    return;

  byte nenorm = 0;
  float srednee;

  String date = String("/") + rtc.getDateStr();
  if (!SD.exists(date)) SD.mkdir(date);
  if (SD.exists(date + "/time.js"))
  {
    File file = SD.open(date + "/time.js", FILE_WRITE);
    file.seek(file.size() - 2);
    file.print(String("\"") + time2str(unix_time()) + String("\",\n]\n"));
    file.close();


    file = SD.open(date + "/humidity.js", FILE_WRITE);
    file.seek(file.size() - 2);
    file.print((((last_humidity<100.1)&&(last_humidity>0.1))?String(last_humidity):String("null")) + String(",\n]\n"));
    file.close();

    for (int i = 0; i < N_DS18B20; i++)
    {
      file = SD.open(date + "/temp" + String(i) + ".js", FILE_WRITE);
      file.seek(file.size() - 2);
      file.print(String(filt(last[i] + config.calib[i])) + String(",\n]\n"));
      file.close();
    }

    for (int sr = 0; sr < 4; sr++)
    {
      srednee = 0;
      nenorm = 0;
      for (int k = sr * 5; k < (sr * 5 + 5); k++)
      {
        srednee += last[k] + config.calib[k];
        nenorm += (filt(last[k] + config.calib[k]) == "null");//среднее не надо рассчитывать, если хотя бы одно слагаемое неадекватно
      }
      srednee /= 5.0;
      file = SD.open(date + "/sr" + String(sr) + ".js", FILE_WRITE);
      file.seek(file.size() - 2);
      if (nenorm)
      {
        file.print(String("null") + String(",\n]\n"));
      }
      else
      {
        file.print(String(srednee) + String(",\n]\n"));
      }
      file.close();
    }
  }
  else
  {
    copy(date, "index.htm");
    copy(date, "boost.js");
    copy(date, "export-data.js");
    copy(date, "exporting.js");
    copy(date, "highcharts.js");


    File file = SD.open(date + "/time.js", FILE_WRITE);
    file.print(String("var time=[\n\"") + time2str(unix_time()) + String("\",\n]\n"));
    file.close();

    file = SD.open(date + "/humidity.js", FILE_WRITE);
    file.print(String("var humidity=[\n") + (((last_humidity<100.1)&&(last_humidity>0.1))?String(last_humidity):String("null")) + String(",\n]\n"));
    file.close();

    for (int i = 0; i < N_DS18B20; i++)
    {
      file = SD.open(date + "/temp" + String(i) + ".js", FILE_WRITE);
      file.print(String("var temp" + String(i) + "=[\n") + String(filt(last[i] + config.calib[i])) + String(",\n]\n"));
      file.close();
    }

    for (int sr = 0; sr < 4; sr++)
    {
      file = SD.open(date + "/sr" + String(sr) + ".js", FILE_WRITE);
      srednee = 0;
      nenorm = 0;
      for (int k = sr * 5; k < (sr * 5 + 5); k++)
      {
        srednee += last[k] + config.calib[k];
        nenorm += (filt(last[k] + config.calib[k]) == "null");
      }
      srednee /= 5.0;
      file = SD.open(date + "/sr" + String(sr) + ".js", FILE_WRITE);
      if (nenorm)
      {
        file.print("var sr" + String(sr) + "=[\nnull,\n]\n");
      }
      else
      {
        file.print("var sr" + String(sr) + "=[\n" + String(srednee) + ",\n]\n");
      }
      file.close();
    }
  }
  last_sd_write = millis();
}
