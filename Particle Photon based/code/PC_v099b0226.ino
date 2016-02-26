//thingspeak設定

unsigned long myChannelNumber = 23139; //填入thingspeak channel ID ***必填
const char * myWriteAPIKey = "FKQQX8C5IEADQY0H"; //填入thingspeak API key ***必填

//Blynk設定

char auth[] = "871d182377fa4ef0995ccc76038e3ecc"; //填入blynk auth key ***必填

//版本號
#define VERSION "v099b0226"

//計時上傳
int ThingspeakUpdateTime = 0;
int BlynkUpdateTime = 0;
#define Thingspeak_INTERVAL 60000
#define Blynk_INTERVAL 5000

//無線網路設定

TCPClient client;

//SENSOR 設定
//溫溼度
#define DHTPIN 2 //指定要量測的腳位為D2
#define DHTTYPE DHT22 //指定溫溼度感測器種類為DHT22
float dht_tem = 0;
float dht_hum = 0;
float measure_tem = 0;
float measure_hum = 0;
char sensor_tem[5];
char sensor_hum[5];
//declaration
void dht_wrapper(); // must be declared before the lib initialization
// Lib instantiate
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

void setup(void)     //初始化函數
{
  Particle.publish("Start ver." , VERSION);

  Serial.begin(115200);
  
  dht.begin();

  //懸浮微粒感測器初始化
  Serial1.begin(9600);

  //blynk
  Blynk.begin(auth);

  //ThingSpeak方法初始化
  ThingSpeak.begin(client);

  delay(500);       //延遲500ms

}

void loop(void)     //循環函數區域
{

  Blynk.run();

  Serial.println(F("--------------------------------"));

  measure_dht();
  sprintf(sensor_tem, "%.1f", measure_tem);
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
    Particle.publish("Report" , "ProbeCube to Thingspeak OK");
    ThingspeakUpdateTime = millis() + Thingspeak_INTERVAL;
  }


  //用Blynk方法送出數據
  if (millis() > BlynkUpdateTime) {
    Blynk.virtualWrite(V1, sensor_tem);
    Blynk.virtualWrite(V2, sensor_hum);
    Blynk.virtualWrite(V4, sensor_voc);
    Blynk.virtualWrite(V5, sensor_dust25);
    Blynk.virtualWrite(V6, sensor_dust100);

    Serial.println(F("Data has been sent to Blynk !!"));
    BlynkUpdateTime = millis() + Blynk_INTERVAL;
  }


}
//方法呼叫區*********************************************


void measure_dht() {

    do{
		int dhttry = 0;
        dht_tem = dht.getTempCelcius();
        dht_hum = dht.getHumidity();
        
        if (dhttry > 0){
        Particle.publish("DHT error");
        delay (1000);
        }
        
        dhttry++;
    } while(isnan(dht_tem) || isnan(dht_hum));
    
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
