//thingspeak設定變數
String writeAPIKey =    "2MM69QFI2X4UDXXX";  //填入thingspeak write API key ***必填
//CC3000無線網路設定變數
#define WLAN_SSID       "3203BIGDATA"     //填入無線網路名稱  ***必填
#define WLAN_PASS       "3203BIGDATA"     //填入無線網路密碼  ***必填
//GP2Y1010校正公式變數
#define slope_factor            "1109"     //填入斜率參數  ***必填
#define interception_factor     "586"      //填入截距參數  ***必填

//版本號
#define VERSION 0.98
//cc3000設定
#define WLAN_SECURITY   WLAN_SEC_WPA2     //無線網路加密方式
#include <Wire.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <avr/wdt.h>
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);
//指定指示燈腳位
#define red_led     7 
#define yellow_led  8
#define green_led   9
//thingspeak上傳設定
String thingSpeakAddress = "http://api.thingspeak.com/update?";
#define Thingspeak_INTERVAL 60000
unsigned long currentMillis = 0;
unsigned long previousUpdateMillis = 0;
boolean firstRun = true;
boolean needUpdate;

//SENSOR設定                                                        
//溫溼度  
#include <DHT.h>
#define DHTPIN 4 //指定要量測的腳位為D4
#define DHTTYPE DHT22 //指定溫溼度感測器種類為DHT22
DHT dht(DHTPIN, DHTTYPE);
//溫濕度校正變數設定
#define Compensator_tem -7
#define Compensator_hum 17
//===========================
//土壤濕度
#define soilSensor 1 //指定要量測的腳位為A1
//有機汙染
#define gasSensor 2 //指定要量測的腳位為A2
//懸浮微粒
#define measurePin  0 //指定要量測的腳位為A0
#define dustledPower  2   //指定led控制來源腳位為D2
#define samplingTime  300
#define deltaTime  40
#define sleepTime  48000
float voMeasured = 0;
float refMeasured = 0;
float calcVoltage = 0;

//****************************************************************************
void setup(void)
{ 
  //指定通訊視窗9600
  Serial.begin(9600);   

  Serial.println("ProbeCube is starting");
  Serial.print("code version: ");
  Serial.println(VERSION);

  //LED們初始化
  pinMode(red_led,OUTPUT);
  pinMode(yellow_led,OUTPUT);
  pinMode(green_led,OUTPUT);

  do{
    LED_start();
    LED_check();
    Serial.print(F("Initializing WiFi chip..."));  //CC3000初始化
    if (!cc3000.begin())
    {
      Serial.println(F("Couldn't begin()! Check your wiring?"));
      LED_error();
      while(1);
    }

    Serial.println(F("\nDeleting old connection profiles"));
    if (!cc3000.deleteProfiles()) {
      Serial.println(F("Failed!"));
      LED_error();
      while(1);
    }
    // Connect to WiFi network
    Serial.print(F("Connecting to WiFi network ..."));
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
    Serial.println(F("done!"));

    // Wait for DHCP to complete
    Serial.println(F("Request DHCP ..."));
    LED_error();
    delay(5000);
  }
  while (!cc3000.checkDHCP());

  Serial.println(F("Connection is established!"));
  
  //懸浮微粒感測器初始化
  pinMode(dustledPower,OUTPUT);

  LED_ok();

  delay(500);       //延遲500ms 

}

void loop(void)     //循環函數區域
{ 
  currentMillis=millis();   //記錄當下時間
  if(currentMillis - previousUpdateMillis >= Thingspeak_INTERVAL){ //判斷是否需要上傳
    needUpdate = true;
  }
  else{ 
    needUpdate =false;
  }
  if(firstRun){   //若為第一次執行則強迫判斷為需要更新
    needUpdate = true;
    firstRun = false;
  }
  if(needUpdate){ //若判斷為需要更新則開始測量&上傳
    Serial.println(F("Starting to monitor the air....."));

    previousUpdateMillis=currentMillis;

    //讀取溫溼度
    float sensor_tem = measure_tem();
    float sensor_hum = measure_hum();

    //讀取土壤濕度
    int sensor_soil = measure_soil();

    //讀取有機氣體汙染
    float sensor_voc = measure_voc(); 

    //讀取灰塵感測
    float sensor_dust = measure_dust();

    //產生亂數
    int ran = random(100,999);
    Serial.println(F("Detection is finished"));
    Serial.println(F("====================="));
    Serial.print(F("Tm:"));
    Serial.print(sensor_tem);
    Serial.println(F("C"));
    Serial.print(F("Hm:"));
    Serial.print(sensor_hum);
    Serial.println(F("%"));
    Serial.print(F("Soil:"));
    Serial.print(sensor_soil);
    Serial.println(F("ohm"));
    Serial.print(F("VOCs:"));
    Serial.print(sensor_voc);
    Serial.println(F("ppm"));
    Serial.print(F("PM:"));
    Serial.print(sensor_dust); 
    Serial.print(F("ug/m3"));
    Serial.print(F(" , S-"));
    Serial.print(slope_factor);
    Serial.print(F(" , I-"));
    Serial.println(interception_factor);
    
    Serial.println(F("====================="));



    //啟動看門狗
    wdt_enable (WDTO_8S);
    // 取得 thingspeak IP
    uint32_t ip = 0;
    Serial.print(F("Finding api.thingspeak.com -> "));
    while  (ip  ==  0)  {
      if  (!  cc3000.getHostByName("api.thingspeak.com", &ip))  {
        Serial.println(F("Couldn't resolve!"));
        while(1){
        }
      }
      delay(500);
    }  
    cc3000.printIPdotsRev(ip);
    Serial.println(F(""));
    Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80); //定義client

    wdt_reset ();

    //使用http get送出數據 
    client.fastrprint(F("GET "));
    client.print(thingSpeakAddress);
    client.fastrprint(F("api_key="));
    client.print(writeAPIKey);
    Serial.println ("Sending data:"); //顯示上傳的數據
    Serial.print(F("GET "));
    Serial.print(thingSpeakAddress);
    Serial.println(writeAPIKey);
    LED_send();
    wdt_reset ();
    client.fastrprint(F("&field1="));
    client.print(sensor_tem);
    LED_send();
    client.fastrprint(F("&field2="));
    client.print(sensor_hum);
    LED_send();
    client.fastrprint(F("&field3="));
    client.print(sensor_soil);
    LED_send();
    client.fastrprint(F("&field4="));
    client.print(sensor_voc);
    LED_send();
    client.fastrprint(F("&field5="));
    client.print(sensor_dust);
    LED_send();
    client.fastrprint(F("\r\n"));
    wdt_reset ();
    wdt_disable();

    Serial.print(F("&field1="));
    Serial.print(sensor_tem);
    Serial.print(F("&field2="));
    Serial.print(sensor_hum);
    Serial.print("&field3=");
    Serial.print(sensor_soil);
    Serial.print(F("&field4="));
    Serial.print(sensor_voc);
    Serial.print(F("&field5="));
    Serial.print(sensor_dust);
    Serial.print(F("&field8="));
    Serial.print(ran);
    Serial.print(F("\r\n"));

    Serial.println(F("Data has been sent successfully, waiting for next update..."));
    Serial.println();    
  }
}
//方法呼叫區******************************************************************
float measure_tem(){
  return (dht.readTemperature() + Compensator_tem);
}

float measure_hum(){
  return (dht.readHumidity() + Compensator_hum);
}

float measure_soil(){
  return analogRead(soilSensor);
}

float measure_voc(){
  float tgsADVcalib_tm = analogRead(gasSensor) * (-0.0256 * dht.readTemperature() + 1.535); //將ADV做溫度補償
  float tgsADVcalib_tm_hm = tgsADVcalib_tm * (-0.0029 * dht.readHumidity() + 1.1938); //將ADV做濕度補償
  float voc = (tgsADVcalib_tm_hm - 113.87) / 10.497; //將ADV換算成ppm濃度
  if (voc < 0){   
    voc = 0;   //將負數的讀值歸零
  }   
  return voc;
}

float measure_dust(){
  float voMeasured = 0;
  float dustDensity = 0;
  int k = 0;
  while ( dustDensity < 1 ){
    for(int i = 1; i <= 200; i ++){
      digitalWrite(dustledPower,LOW); // power on the LED
      delayMicroseconds(samplingTime);
      voMeasured += analogRead(measurePin); // read the dust value
      delayMicroseconds(deltaTime);
      digitalWrite(dustledPower,HIGH); // turn the LED off
      delayMicroseconds(sleepTime);

    }
    voMeasured /= 200;

    float slope = atoi(slope_factor);
    float interception = atoi(interception_factor);
    dustDensity = slope /1000 * voMeasured - interception;

    if( dustDensity < 1){
      Serial.print(F("Dust value error, will check again"));
      Serial.print(F("("));
      Serial.print(voMeasured);
      Serial.print(F(" , "));
      Serial.print(slope);
      Serial.print(F(" , "));
      Serial.print(interception);
      Serial.print(F(" , "));
      Serial.print(dustDensity);
      Serial.println(F(")"));
      k++;
    }
    if ( k==1 ) {
      Serial.println(F("Can't detect dust"));
      dustDensity = 0;
      break;
    }
  }
  return dustDensity;
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
}

void LED_warning_yellow()
{
  for (int i=0; i <= 270; i++){
    digitalWrite(yellow_led, HIGH);
    delay(100); 
    digitalWrite(yellow_led, LOW);
    delay(100);
  }
}

void LED_warning_red()
{
  for (int i=0; i <= 270; i++){
    digitalWrite(red_led, HIGH);
    digitalWrite(yellow_led, HIGH);
    delay(100); 
    digitalWrite(red_led, LOW);
    digitalWrite(yellow_led, LOW);
    delay(100);
  }
}
