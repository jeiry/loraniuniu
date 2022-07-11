#include <LittleFS.h>
#include <ArduinoJson.h>
/**
   初始化文件系统
*/
void initFileSystem() {
  //  LittleFS.format();
  Serial.println("Mount LittleFS");
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
}

/**
   写入配置文件
*/
bool saveConfig(char *key, char *value, char*filename) {
  StaticJsonDocument<200> doc;
  doc[key] = value;

  File configFile = LittleFS.open(filename, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    Serial.println(configFile);
    return false;
  }

  serializeJson(doc, configFile);
  delay(200);
  configFile.close();
  return true;
}

/**
   读取配置文件
*/
const char* loadConfig(char *key, char*file) {
  File configFile = LittleFS.open(file, "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return "";
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return "";
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<200> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return "";
  }
  configFile.close();
  return doc[key];
}

//获取flash中的数据
String getStorage(char* key,char* path) {
  const char* value = loadConfig(key, path);
  return String(value);
}
