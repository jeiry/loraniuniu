#include <ESP8266WiFi.h>

#include <SoftwareSerial.h>
SoftwareSerial uartGPS(14, 12); //gps
//RX=d5,TX=d6 13/d7rx 15/d8/tx
SoftwareSerial uartLora(13, 15); //lora
//ADC_MODE(ADC_VCC);
//int adcValue = A0;
const int MD0_pin = 16;
const int MD1_pin = 4;
const int led_pin = 2;
const int gps_pin = 5;
int interval = 0;
String unionid;
const int adc_len = 10;
int adcarr[adc_len];
int checkADC = 0;
struct
{
  char GPS_Buffer[512];
  bool isGetData;   //是否获取到GPS数据
  bool isParseData; //是否解析完成
  char UTCTime[11];   //UTC时间
  char latitude[11];    //纬度
  char N_S[2];    //N/S
  char longitude[12];   //经度
  char E_W[2];    //E/W
  bool isUsefull;   //定位信息是否有效
} Save_Data;

const unsigned int gpsRxBufferLength = 600;
char gpsRxBuffer[gpsRxBufferLength];
unsigned int ii = 0;

void setup()  {
  uint64_t chipid = ESP.getChipId();
  //      unionid = String((uint32_t)chipid);

  Serial.begin(9600);
  //  uartGPS.listen();
  uartGPS.begin(9600);
  uartLora.begin(9600);
  //关闭wifi
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);
  pinMode(MD1_pin, OUTPUT);
  pinMode(MD0_pin, OUTPUT);

  //    GPS
  pinMode(gps_pin, OUTPUT);

  Save_Data.isGetData = false;
  Save_Data.isParseData = false;
  Save_Data.isUsefull = false;

  Serial.println("start");
  delay(1000);
  //初始化LittleFS文件系统
  initFileSystem();
  //  saveConfig("unionid", (char*)String(random(10, 99)).c_str(), "/unionid.config");
  if (getStorage("unionid", "/unionid.config") == "") {
    saveConfig("unionid", (char*)String((uint32_t)chipid).c_str(), "/unionid.config");
    delay(1000);
    ESP.restart();
  }
  Serial.print("unionid:");
  Serial.println(getStorage("unionid", "/unionid.config"));
  //  uartGPS.println("start");

}
void lora(bool status) {
  if (status == true) {
    Serial.println("trun on lora");
    //    digitalWrite(gps_pin, LOW);
    digitalWrite(led_pin, HIGH);
    digitalWrite(MD1_pin, LOW);
    digitalWrite(MD0_pin, LOW);
  } else {
    Serial.println("trun off lora");
    digitalWrite(MD1_pin, HIGH);
    digitalWrite(MD0_pin, HIGH);
    digitalWrite(led_pin, LOW);
    //    digitalWrite(gps_pin, HIGH);
  }

}

String cmd;
int adc ;
int beat = 0;
void loop()  {
  gpsRead();  //获取GPS数据

  if (interval == 0) {
    Serial.println("interval 0");
    int adc = analogRead(A0);
    checkADC = adc;
    if (beat == 0) {
      beat ++;
      String data = "lora:" + getStorage("unionid", "/unionid.config") + ":beat:" + String(checkADC);
      Serial.println(data);
      Serial.println("=======");
      lora(true);
      digitalWrite(gps_pin, LOW);
      delay(2000);
      uartLora.println(data);
      delay(500);
      lora(false);
      //      digitalWrite(gps_pin, HIGH);
    } else {
      beat ++;
      if (beat >= 60) {
        beat = 0;
      }
    }
    //    Serial.println("=======");
    delay(2000);
    parseGpgga();
    interval ++;
  } else {
    interval ++;
    //20s
    if (interval >= 10) {
      interval = 0;
    }
  }

  delay(100);

}
bool gpsStatus = false;
int readTime = 0;

void parseGpgga() {
  if (Save_Data.isGetData)
  {
    Save_Data.isGetData = false;
    Serial.println(Save_Data.GPS_Buffer);
    int gpsStatus = getValue(Save_Data.GPS_Buffer, ',', 6).toInt();
    if (countValue(Save_Data.GPS_Buffer, ',') > 11) {
      if (gpsStatus == 1 or gpsStatus == 2) {

        String datalat = "gt:" + getStorage("unionid", "/unionid.config") + ":" +
                         getValue(Save_Data.GPS_Buffer, ',', 2);

        String datalon = "gn:" + getStorage("unionid", "/unionid.config") + ":" +
                         getValue(Save_Data.GPS_Buffer, ',', 4);
        digitalWrite(gps_pin, LOW);
        lora(true);
        digitalWrite(led_pin, HIGH);
        delay(250);
        digitalWrite(led_pin, LOW);
        delay(250);
        digitalWrite(led_pin, HIGH);
        delay(250);
        digitalWrite(led_pin, LOW);
        delay(250);
        digitalWrite(led_pin, HIGH);
        delay(250);
        digitalWrite(led_pin, LOW);
        delay(250);
        digitalWrite(led_pin, HIGH);
        delay(random(100, 500));
        uartLora.println(datalat);
        delay(random(1000, 2000));
        uartLora.println(datalon);
        delay(random(1000, 2000)); //休眠吧
        String data = "lora:" + getStorage("unionid", "/unionid.config") + ":beat:" + String(checkADC);
        uartLora.println(data);
        delay(random(500, 1000));
        lora(false);
        digitalWrite(led_pin, LOW);
        delay(random(175000, 180000)); //睡眠3分钟
        beat = 1;

      }
    }

    memset(Save_Data.GPS_Buffer, 0, 512);

  }
}
void gpsRead() {
  digitalWrite(gps_pin, HIGH); //开 GPS
  while (uartGPS.available())
  {
    gpsRxBuffer[ii++] = uartGPS.read();
    if (ii == gpsRxBufferLength)clrGpsRxBuffer();
  }

  char* GPS_BufferHead;
  char* GPS_BufferTail;
  //  if ((GPS_BufferHead = strstr(gpsRxBuffer, "$GPRMC,")) != NULL || (GPS_BufferHead = strstr(gpsRxBuffer, "$GNRMC,")) != NULL )
  if ((GPS_BufferHead = strstr(gpsRxBuffer, "$GNGGA,")) != NULL  )
  {
    if (((GPS_BufferTail = strstr(GPS_BufferHead, "\r\n")) != NULL) && (GPS_BufferTail > GPS_BufferHead))
    {
      memcpy(Save_Data.GPS_Buffer, GPS_BufferHead, GPS_BufferTail - GPS_BufferHead);
      Save_Data.isGetData = true;

      clrGpsRxBuffer();
    }
  }
}

void clrGpsRxBuffer(void)
{
  memset(gpsRxBuffer, 0, gpsRxBufferLength);      //清空
  ii = 0;
}

double GpsDataDmToDd(double data)
{
  double result;
  double dd, mm, temp;

  dd = (int)data / 100;//得到度dd
  mm = data - dd * 100; //得到分mm.mmmm
  temp = mm / 60;
  result = dd + temp;
  return result;
}

//拆分字符
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int countValue(String data, char separator) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
    }
  }
  return found;
}
