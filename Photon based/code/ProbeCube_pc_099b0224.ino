// This #include statement was automatically added by the Particle IDE.
#include "blynk/blynk.h"
#include "ThingSpeak/ThingSpeak.h"
#include "PietteTech_DHT/PietteTech_DHT.h"


//thingspeak設定============================================================

unsigned long myChannelNumber = 26769;
const char * myWriteAPIKey = "Z9LVXARGKI6KJJ1H"; //填入thingspeak API key ***必填

//Blynk設定===================================================================

char auth[] = "07dfd97a0c8c4927ba6a67126fa7231c";


//版本號
#define VERSION "v1b0223"

//計時上傳
unsigned long currentMillis = 0;
unsigned long previousUpdateMillis = 0;
int ThingspeakUpdateTime = 0;
int BlynkUpdateTime = 0;
#define Thingspeak_INTERVAL 60000
#define Blynk_INTERVAL 5000

//無線網路設定========================================================

TCPClient client;

//SENSOR 設定==============================================================
//溫溼度
#define DHTPIN 3 //指定要量測的腳位為D3
#define DHTTYPE DHT22 //指定溫溼度感測器種類為DHT22
char sensor_tem[5];
char sensor_hum[5];
//declaration
void dht_wrapper(); // must be declared before the lib initialization
// Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
//=======溫濕度補償==========
#define Compensator_tem 0
#define Compensator_hum 0
//===========================
//土壤濕度
#define soilSensor 2 //指定要量測的腳位為A2
//有機汙染
#define gasSensor 1 //指定要量測的腳位為A1
char sensor_voc[5];
//懸浮微粒
long  pmcf10 = 0;
long  pmcf25 = 0;
long  pmcf100 = 0;
long  pmat10 = 0;
long  pmat25 = 0;
long  pmat100 = 0;
char buf[50];
//指定指示燈腳位
#define red_led     7
#define yellow_led  8
#define green_led   9
//****************************************************************************

void setup(void)     //初始化函數
{
  Particle.publish("Start version" , VERSION);

  Serial.begin(115200);

  //LED們初始化
  pinMode(red_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);
  pinMode(green_led, OUTPUT);

  LED_start();
  LED_check();

  //懸浮微粒感測器初始化
  Serial1.begin(9600);

  //blynk
  Blynk.begin(auth);

  //ThingSpeak方法初始化================================================================

  ThingSpeak.begin(client);

  LED_ok();

  delay(500);       //延遲500ms

}


void dht_wrapper() {
  DHT.isrCallback();
}


void loop(void)     //循環函數區域
{

  Blynk.run();

  Serial.println(F("--------------------------------"));

  int DHTresult = -1;

  while (DHTresult < 0) {
    DHTresult = DHT.acquireAndWait();

    switch (DHTresult) {
      case DHTLIB_OK:
        break;
      case DHTLIB_ERROR_CHECKSUM:
        Particle.publish("Error Checksum error");
        break;
      case DHTLIB_ERROR_ISR_TIMEOUT:
        Particle.publish("Error tISR time out error");
        break;
      case DHTLIB_ERROR_RESPONSE_TIMEOUT:
        Particle.publish("Error Response time out error");
        break;
      case DHTLIB_ERROR_DATA_TIMEOUT:
        Particle.publish("Error Data time out error");
        break;
      case DHTLIB_ERROR_ACQUIRING:
        Particle.publish("Error Acquiring");
        break;
      case DHTLIB_ERROR_DELTA:
        Particle.publish("Error Delta time to small");
        break;
      case DHTLIB_ERROR_NOTSTARTED:
        Particle.publish("Error Not started");
        break;
      default:
        Particle.publish("Unknown error");
        break;
    }
  }

  sprintf(sensor_tem, "%.1f", measure_tem());
  sprintf(sensor_hum, "%.1f", measure_hum());

  //讀取土壤濕度
  int sensor_soil = measure_soil();

  //讀取有機氣體汙染
  sprintf(sensor_voc, "%.1f", measure_voc());

  //讀取灰塵感測
  delay (500);
  measure_dust();
  int sensor_dust25 = pmat25;
  int sensor_dust100 = pmat100;
  //產生亂數
  int ran = random(100, 999);



  //將數據上傳 ======================================================


  //使用thingspeak方法送出數據
  if (millis() > ThingspeakUpdateTime) {
    ThingSpeak.setField(1, sensor_tem);
    ThingSpeak.setField(2, sensor_hum);
    ThingSpeak.setField(3, sensor_soil);
    ThingSpeak.setField(4, sensor_voc);
    ThingSpeak.setField(5, sensor_dust25);
    ThingSpeak.setField(6, sensor_dust100);
    ThingSpeak.setField(8, ran);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    Serial.println(F("Data has been sent to Thingspeak !!")); //serial & particle回報狀態訊息
    Particle.publish("Report" , "ProbeCube to Thingspeak OK");
    ThingspeakUpdateTime = millis() + Thingspeak_INTERVAL;
  }

  //用Blynk方法送出數據
  if (millis() > BlynkUpdateTime) {
    Blynk.virtualWrite(V1, sensor_tem);
    Blynk.virtualWrite(V2, sensor_hum);
    Blynk.virtualWrite(V3, sensor_soil);
    Blynk.virtualWrite(V4, sensor_voc);
    Blynk.virtualWrite(V5, sensor_dust25);
    Blynk.virtualWrite(V6, sensor_dust100);

    Serial.println(F("Data has been sent to Blynk !!"));
    BlynkUpdateTime = millis() + Blynk_INTERVAL;
  }


}
//方法呼叫區******************************************************************
float measure_tem() {
  return (DHT.getCelsius() + Compensator_tem);
}

float measure_hum() {
  return (DHT.getHumidity() + Compensator_hum);
}

float measure_soil() {
  return analogRead(soilSensor);
}

float measure_voc() {
  float voc = 0;
  for (int i = 1; i <= 5; i ++) {
  float tgsADVcalib_tm = analogRead(gasSensor) * (-0.0256 * DHT.getCelsius() + 1.535); //將ADV做溫度補償
  float tgsADVcalib_tm_hm = tgsADVcalib_tm * (-0.0029 * DHT.getHumidity() + 1.1938); //將ADV做濕度補償
  float voc_raw = (tgsADVcalib_tm_hm - 113.87) / 10.497; //將ADV換算成ppm濃度
  voc += 0.1732 * voc_raw - 5.0602;
  delay(1000);

}
  voc /= 5;
  
  if (voc < 0) {
    voc = 0;   //將負數的讀值歸零
  }
  return voc;
}

void measure_dust() {
  int count = 0;
  Serial.println("entered");

  unsigned char c;
  unsigned char high;
  while (Serial1.available()) {
    c = Serial1.read();
    if ((count == 0 && c != 0x42) || (count == 1 && c != 0x4d)) {
      Serial.println("check failed");
      break;
    }
    if (count > 15) {
      Serial.println("complete");
      break;
    }
    else if (count == 4 || count == 6 || count == 8 || count == 10 || count == 12 || count == 14) high = c;
    else if (count == 5) {
      pmcf10 = 256 * high + c;
      Serial.print("CF=1, PM1.0=");
      Serial.print(pmcf10);
      Serial.println(" ug/m3");
    }
    else if (count == 7) {
      pmcf25 = 256 * high + c;
      Serial.print("CF=1, PM2.5=");
      Serial.print(pmcf25);
      Serial.println(" ug/m3");
    }
    else if (count == 9) {
      pmcf100 = 256 * high + c;
      Serial.print("CF=1, PM10=");
      Serial.print(pmcf100);
      Serial.println(" ug/m3");
    }
    else if (count == 11) {
      pmat10 = 256 * high + c;
      Serial.print("atmosphere, PM1.0=");
      Serial.print(pmat10);
      Serial.println(" ug/m3");
    }
    else if (count == 13) {
      pmat25 = 256 * high + c;
      Serial.print("atmosphere, PM2.5=");
      Serial.print(pmat25);
      Serial.println(" ug/m3");
    }
    else if (count == 15) {
      pmat100 = 256 * high + c;
      Serial.print("atmosphere, PM10=");
      Serial.print(pmat100);
      Serial.println(" ug/m3");
    }
    count++;
  }
  while (Serial1.available()) Serial1.read();
  return;
}

void LED_start()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(yellow_led, HIGH);
  digitalWrite(green_led, HIGH);
  delay(1000);
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, LOW);
  delay(500);
  digitalWrite(red_led, HIGH);
  digitalWrite(yellow_led, HIGH);
  digitalWrite(green_led, HIGH);
  delay(1000);
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, LOW);
}

void LED_check()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(yellow_led, HIGH);
  digitalWrite(green_led, LOW);
}

void LED_wait()
{
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, HIGH);
  digitalWrite(green_led, LOW);
}

void LED_error()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, LOW);
}

void LED_ok()
{
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, HIGH);
}

void LED_send()
{
  digitalWrite(green_led, LOW);
  delay(100);
  digitalWrite(green_led, HIGH);
  delay(100);
  digitalWrite(green_led, LOW);
  delay(100);
  digitalWrite(green_led, HIGH);
  delay(100);
  digitalWrite(green_led, LOW);
  delay(100);
  digitalWrite(green_led, HIGH);
}

void LED_warning_yellow()
{
  for (int i = 0; i <= 270; i++) {
    digitalWrite(yellow_led, HIGH);
    delay(100);
    digitalWrite(yellow_led, LOW);
    delay(100);
  }
}

void LED_warning_red()
{
  for (int i = 0; i <= 270; i++) {
    digitalWrite(red_led, HIGH);
    digitalWrite(yellow_led, HIGH);
    delay(100);
    digitalWrite(red_led, LOW);
    digitalWrite(yellow_led, LOW);
    delay(100);
  }
}
