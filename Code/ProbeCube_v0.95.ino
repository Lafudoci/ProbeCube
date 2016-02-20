//ProbeCube_v0.95
//Copyright (C) 2015 Lafudoci (https://github.com/Lafudoci)

//thingspeak設定============================================================
  String thingSpeakAddress = "http://api.thingspeak.com/update?";
  String writeAPIKey = "api_key_here";  //填入thingspeak API key ***必填

//CC3000無線網路設定========================================================
 
  #define WLAN_SSID       "SSID_HERE"     //填入無線網路名稱  ***必填
  #define WLAN_PASS       "PASSWORD_HERE"      //填入無線網路密碼  ***必填

  #define WLAN_SECURITY   WLAN_SEC_WPA2     //無線網路加密方式
  
  #include <Wire.h>
  #include <Adafruit_CC3000.h>
//  #include <ccspi.h>
  #include <SPI.h>
  #include <avr/wdt.h>
  #define ADAFRUIT_CC3000_IRQ   3
  #define ADAFRUIT_CC3000_VBAT  5
  #define ADAFRUIT_CC3000_CS    10
  Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2); 
//SENSOR 設定==============================================================                                                           
  //溫溼度  
  #include <DHT.h>
  #define DHTPIN 4 //指定要量測的腳位為D4
  #define DHTTYPE DHT22 //指定溫溼度感測器種類為DHT22
  DHT dht(DHTPIN, DHTTYPE);
  //=======溫濕度補償==========
  #define Compensator_tem -0
  #define Compensator_hum 0
  //===========================
  //土壤濕度
  #define soilSensor 1 //指定要量測的腳位為A1
  //有機汙染
  #define gasSensor 2 //指定要量測的腳位為A2
  //灰塵
  #define measurePin  0 //指定要量測的腳位為A0
  #define dustledPower  2   //指定led控制來源腳位為D2
  #define samplingTime  280
  #define deltaTime  40
  #define sleepTime  9680
  float voMeasured = 0;
  float calcVoltage = 0;
  //中位數filter
  #include <MedianFilter.h>
  MedianFilter medFilter;
  //指定指示燈腳位
  #define red_led     7 
  #define yellow_led  8
  #define green_led   9
//****************************************************************************

void setup(void)     //初始化函數 
{ 
  //指定通訊視窗9600
  Serial.begin(9600);   
 
  Serial.println("ProbeCube is starting v0.95");
  
  //LED們初始化
  pinMode(red_led,OUTPUT);
  pinMode(yellow_led,OUTPUT);
  pinMode(green_led,OUTPUT);
  
  LED_start();
  LED_check();
  
  //懸浮微粒感測器初始化
  pinMode(dustledPower,OUTPUT);

//CC3000初始化================================================================
    Serial.println(F("Initializing WiFi chip..."));
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
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()){
    LED_error();
    delay(100);
  }
//準備開始測量的訊息==========================================================
  Serial.println(F("Starting to monitor the air....."));
  

  LED_ok();
  
  delay(500);       //延遲500ms 
   
}

void loop(void)     //循環函數區域
{ 
  Serial.println(F("--------------------------------"));
    
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
     Serial.print(sensor_dust ); 
     Serial.println(F("ug/m3"));
     Serial.println();
  delay (500);


//將數據上傳thingspeak ======================================================
  
  //啟動看門狗
wdt_enable (WDTO_8S);
  
  // 取得 thingspeak IP
  uint32_t ip = 0;
  Serial.print(F("api.thingspeak.com -> "));
  while  (ip  ==  0)  {
    if  (!  cc3000.getHostByName("api.thingspeak.com", &ip))  {
      Serial.println(F("Couldn't resolve!"));
      while(1){}
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
      Serial.print(writeAPIKey);
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
      client.fastrprint(F("&field8="));
      client.print(ran);
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
      
      Serial.println(F("Data sended !!"));      

//判斷觀測數據是否有需要注意的狀況,從輕微判斷到嚴重
int ledstatus = 0;

if ( (sensor_voc > 20) && (sensor_voc < 30) ){
  ledstatus = 1;
  Serial.println(F("Voc status is not good!"));
}
if ( (sensor_dust > 50) && (sensor_dust < 100) ){
  ledstatus = 1;
  Serial.println(F("Dust status is not good!"));
}

if ( sensor_voc > 30 ){
  ledstatus = 2;
  Serial.println(F("Voc status is bad!"));
}
if ( sensor_dust > 100 ){
  ledstatus = 2;
  Serial.println(F("Dust status is bad!"));
}

//判斷ledstatus變數來決定要執行何種燈號
if (ledstatus == 1){
  LED_warning_yellow();
  Serial.println(F("Yellow alert! Air condition is not good!"));
}else if (ledstatus == 2){
  LED_warning_red();
  Serial.println(F("Red alert! Air condition is bad!"));
}else{
  Serial.println(F("Great! The air condition is good!"));
 delay(54000);
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
  digitalWrite(dustledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);
  voMeasured = analogRead(measurePin); // read the dust value
  delayMicroseconds(deltaTime);
  digitalWrite(dustledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);
  calcVoltage = voMeasured * (5.0 / 1024.0);
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
   float dustDensity = ( 0.17 * calcVoltage - 0.1 ) * 1000;  // 將電壓換算成微粒濃度並將數值呈現改成較常用的單位( ug/m3 )
   if ( dustDensity < 1 ){
    dustDensity = 0; //將負數的讀值歸零
  }
  return medFilter.run(dustDensity); //最後利用中位數filter去除極端值
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
