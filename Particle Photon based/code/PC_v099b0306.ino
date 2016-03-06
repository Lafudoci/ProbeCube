// This #include statement was automatically added by the Particle IDE.
#include "ProbeCube_DHT/ProbeCube_DHT.h"

// This #include statement was automatically added by the Particle IDE.
#include "ProbeCube_blynk/ProbeCube_blynk.h"

// This #include statement was automatically added by the Particle IDE.
#include "ProbeCube_ThingSpeak/ProbeCube_ThingSpeak.h"

//thingspeak設定

unsigned long myChannelNumber = 26XXX; //填入thingspeak channel ID ***必填
const char * myWriteAPIKey = "Z9LVXARGKI6KJXXX"; //填入thingspeak API key ***必填

//Blynk設定

char auth[] = "146f692bc59f48c8ada05568bbXXXXXXXX"; //填入blynk auth key ***必填

//版本號
#define VERSION "v099b0306"

//計時上傳
int LastTimeCheck = 0;
int ThingspeakUpdateTime = 0;
int BlynkUpdateTime = 0;
#define Thingspeak_INTERVAL 60000 //指定每隔60秒上傳資料至thingspeak
#define Blynk_INTERVAL 5000 //指定每隔5秒更新blink

//無線網路設定

TCPClient client;
char WiFi_RSSI[5];
int wifisig_percent;
char tsmsg[60];
char StartMsg[60];

//SENSOR 設定
//溫溼度
#define DHTPIN 2 //指定要量測的腳位為D2
#define DHTTYPE DHT22 //指定溫溼度感測器種類為DHT22
float dht_tem = 0;
float dht_hum = 0;
float measure_tem = 0;
float measure_hum = 0;
#define MIN_TEMPERATURE -30
#define MAX_TEMPERATURE 99
#define MIN_HUMIDITY 0
#define MAX_HUMIDITY 100
char sensor_tem[5];
char sensor_hum[5];
DHT dht(DHTPIN, DHTTYPE);
//=======溫濕度補償==========
#define Compensator_tem 0
#define Compensator_hum 0
//===========================
//有機汙染
#define gasSensor 0 //指定要量測的腳位為A0
char sensor_voc[5];
//懸浮微粒
long  pmcf10 = 0;
long  pmcf25 = 0;
long  pmcf100 = 0;
long  pmat10 = 0;
long  pmat25 = 0;
long  pmat100 = 0;
char buf[50];

//***************************************************

STARTUP(WiFi.selectAntenna(ANT_AUTO));

void setup(void)
{
  
  sprintf(StartMsg, "Start ver. %s , wifi signal: %d %%", VERSION, wifi_signal());
  Particle.publish("Starting", StartMsg);

  Serial.begin(115200);
  
  dht.begin(); //dht感測器初始化

  Serial1.begin(9600); //懸浮微粒感測器初始化

  Blynk.begin(auth); //blynk初始化

  ThingSpeak.begin(client);  //ThingSpeak方法初始化

  delay(500); //延遲500ms

}

void loop(void)  //循環函數區域
{

  Blynk.run();
  
  

  Serial.println(F("--------------------------------"));
  
  //讀取溫濕度
  measure_dht();
  sprintf(sensor_tem, "%.1f", measure_tem); //將浮點數格式化
  sprintf(sensor_hum, "%.1f", measure_hum);

  //讀取有機氣體汙染
  sprintf(sensor_voc, "%.1f", measure_voc());

  //讀取灰塵感測
  delay (500);
  measure_dust();
  int sensor_dust25 = pmat25;
  int sensor_dust100 = pmat100;
  

  //將數據上傳

  //使用thingspeak方法送出數據
  if (millis() > ThingspeakUpdateTime) {
    ThingSpeak.setField(1, sensor_tem);
    ThingSpeak.setField(2, sensor_hum);
    ThingSpeak.setField(4, sensor_voc);
    ThingSpeak.setField(5, sensor_dust25);
    ThingSpeak.setField(6, sensor_dust100);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    Serial.println(F("Data has been sent to Thingspeak !!")); //serial & particle回報狀態訊息
    sprintf(tsmsg, "ProbeCube to Thingspeak OK, wifi signal: %d %%", wifi_signal());
    Particle.publish("Update", tsmsg);
    
    ThingspeakUpdateTime = millis() + Thingspeak_INTERVAL;
  }

  //用Blynk方法送出數據
  if (millis() > BlynkUpdateTime) {
    Blynk.virtualWrite(V1, sensor_tem);
    Blynk.virtualWrite(V2, sensor_hum);
    Blynk.virtualWrite(V4, sensor_voc);
    Blynk.virtualWrite(V5, sensor_dust25);
    Blynk.virtualWrite(V6, sensor_dust100);
    
//    Blynk.virtualWrite(V10, wifi_signal());
    
    Serial.println(F("Data has been sent to Blynk !!"));
    BlynkUpdateTime = millis() + Blynk_INTERVAL;
  }
  
  //若millis溢位則重新計時
  if ( millis() < LastTimeCheck ){
      ThingspeakUpdateTime = Thingspeak_INTERVAL;
      BlynkUpdateTime = Blynk_INTERVAL;
  }
  LastTimeCheck = millis();

}
//方法呼叫區*********************************************


void measure_dht() {
    
    int dhttry;

    do{
		dhttry = 0;
        dht_tem = dht.getTempCelcius();
        dht_hum = dht.getHumidity();
        
        if (dhttry > 0){
        Particle.publish("Error", "DHT error");
        delay (1000);
        }
        
        dhttry++;
    } while(isnan(dht_tem) != 0
        || isnan(dht_hum) != 0
        || dht_tem > MAX_TEMPERATURE
        || dht_tem < MIN_TEMPERATURE
        || dht_hum > MAX_HUMIDITY
        || dht_hum < MIN_HUMIDITY);
    
    measure_tem = Compensator_tem + dht_tem;
    measure_hum = Compensator_hum + dht_hum;

}

float measure_voc() {
  float voc = 0;
  for (int i = 1; i <= 5; i ++) {
  float tgsADVcalib_tm = analogRead(gasSensor) * (-0.0256 * dht_tem + 1.535); //將ADV做溫度補償
  float tgsADVcalib_tm_hm = tgsADVcalib_tm * (-0.0029 * dht_hum + 1.1938); //將ADV做濕度補償
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

  unsigned char c;
  unsigned char high;
  while (Serial1.available()) {
    c = Serial1.read();
    if ((count == 0 && c != 0x42) || (count == 1 && c != 0x4d)) {
      Serial.println("check failed");
      Particle.publish("Error", "G3 check failed");
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

int wifi_signal(){
    int i = 0;
    int wifisig = WiFi.RSSI();
    wifisig_percent = wifisig * 0.7937 + 100.79;
    
    while (wifisig > 0){
        wifisig = WiFi.RSSI();
        delay(1000);
        if ( i > 3){
            Particle.publish("Error", "RSSI error");
            wifisig_percent = 0;
            break;
        }
        i++;
    }
     
    return wifisig_percent;

}
