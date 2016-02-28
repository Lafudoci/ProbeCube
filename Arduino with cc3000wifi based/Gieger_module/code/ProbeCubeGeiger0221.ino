//////////////////////////////////////////////////
// Modified by Lafudoci based on Radiation-Watch.org
// Released under MIT License.
//////////////////////////////////////////////////
///　Digital I/O PIN Settings　///
int signPin = 2; //Radiation Pulse (Yellow)
int noisePin = 6; //Vibration Noise Pulse (White)
//VCC 5V (Red)
//GND (Blue)
////////////////////////////////

//thingspeak設定變數
String writeAPIKey =    "ZMQP9SDP20PJF1A6";  //填入thingspeak write API key ***必填
//CC3000無線網路設定變數
#define WLAN_SSID       "3203BIGDATA"     //填入無線網路名稱  ***必填
#define WLAN_PASS       "3203BIGDATA"     //填入無線網路密碼  ***必填

//版本號
#define VERSION "0.99_Geiger"
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
int DHCPwait = 0;
int checkDHCP = 0;
//指定指示燈腳位
#define red_led     7
#define yellow_led  8
#define green_led   9
//thingspeak上傳設定
String thingSpeakAddress = "http://api.thingspeak.com/update?";
#define Thingspeak_INTERVAL 60000

//SENSOR設定
//溫溼度
#include <DHT.h>
#define DHTPIN 4 //指定要量測的腳位為D4
#define DHTTYPE DHT22 //指定溫溼度感測器種類為DHT22
DHT dht(DHTPIN, DHTTYPE);
//溫濕度校正變數設定
#define Compensator_tem 0
#define Compensator_hum 0
//===========================
//土壤濕度
#define soilSensor 1 //指定要量測的腳位為A1
//有機汙染
#define gasSensor 2 //指定要量測的腳位為A2

const double alpha = 53.032; // cpm = uSv x alpha
float usv = 0;
//char sensor_usv[6];

void setup(void)
{
  //Serial setup
  //9600bps
  Serial.begin(9600);

  //PIN setting for Radiation Pulse
  pinMode(signPin, INPUT);
  digitalWrite(signPin, HIGH);

  //PIN setting for Noise Pulse
  pinMode(noisePin, INPUT);
  digitalWrite(noisePin, HIGH);

  Serial.println(F("ProbeCube is starting"));
  Serial.print(F("code version: "));
  Serial.println(VERSION);

  //LED們初始化
  pinMode(red_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);
  pinMode(green_led, OUTPUT);

  do {
    LED_start();
    LED_check();
    Serial.print(F("Initializing WiFi chip..."));  //CC3000初始化
    if (!cc3000.begin())
    {
      Serial.println(F("Couldn't begin()! Check your wiring?"));
      LED_error();
      while (1);
    }

    Serial.println(F("\nDeleting old connection profiles"));
    if (!cc3000.deleteProfiles()) {
      Serial.println(F("Failed!"));
      LED_error();
      while (1);
    }
    // Connect to WiFi network
    Serial.print(F("Connecting to WiFi network ..."));
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
    Serial.println(F("done!"));


    // Wait for DHCP to complete
    Serial.println(F("Request DHCP ..."));
    while (DHCPwait < 30) {
      if (cc3000.checkDHCP()) {
        checkDHCP = 1;
        Serial.println(F("Finished"));
        break;
      }
      DHCPwait++;
      delay(2000);
      Serial.print(F("Number of tried:"));
      Serial.println(DHCPwait);
    }
  }
  while (checkDHCP != 1);

  Serial.println(F("Connection is established!"));

  LED_ok();

  delay(500);       //延遲500ms


}

void loop(void)     //循環函數區域
{
  //計算輻射量
  String sensor_usv =  String(measure_radi(), 3);
  Serial.println("sensor_usv:");
  Serial.println(sensor_usv);
  //讀取溫溼度
  float sensor_tem = measure_tem();
  float sensor_hum = measure_hum();

  //讀取土壤濕度
  int sensor_soil = measure_soil();

  //讀取有機氣體汙染
  float sensor_voc = measure_voc();

  //啟動看門狗
  wdt_enable (WDTO_8S);
  // 取得 thingspeak IP
  uint32_t ip = 0;
  Serial.print(F("Finding api.thingspeak.com -> "));
  while  (ip  ==  0)  {
    if  (!  cc3000.getHostByName("api.thingspeak.com", &ip))  {
      Serial.println(F("Couldn't resolve!"));
      while (1) {
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
  Serial.println (F("Sending data:")); //顯示上傳的數據
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
  client.print(sensor_usv);
  LED_send();
  client.fastrprint(F("&field4="));
  client.print(sensor_voc);
  LED_send();
  //  client.fastrprint(F("&field5="));
  //  client.print(sensor_usv);
  LED_send();
  client.fastrprint(F("\r\n"));
  wdt_reset ();
  wdt_disable();

  Serial.print(F("&field1="));
  Serial.print(sensor_tem);
  Serial.print(F("&field2="));
  Serial.print(sensor_hum);
  Serial.print(F("&field3="));
  Serial.print(sensor_usv);
  Serial.print(F("&field4="));
  Serial.print(sensor_voc);
  //  Serial.print(F("&field5="));
  //  Serial.print(sensor_usv);
  Serial.print(F("\r\n"));

  Serial.println(F("Data has been sent successfully, waiting for next update..."));
  Serial.println();
}


float measure_tem() {
  return (dht.readTemperature() + Compensator_tem);
}

float measure_hum() {
  return (dht.readHumidity() + Compensator_hum);
}

float measure_soil() {
  return analogRead(soilSensor);
}

float measure_radi() {

  //CSV-formatting for serial output (substitute , for _)
  Serial.println(F("hour[h]_sec[s]_count_cpm_uSv/h_uSv/hError"));

  int index = 0; //Number of loops
  char msg[256] = ""; //Message buffer for serial output
  int signCount = 0; //Counter for Radiation Pulse
  int noiseCount = 0; //Counter for Noise Pulse

  int sON = 0; //Lock flag for Radiation Pulse
  int nON = 0; //Lock flag for Noise Puls

  double cpm = 0; //Count rate [cpm] of current
  double cpmHistory[50]; //History of count rates
  int cpmIndex = 0; //Position of current count rate on cpmHistory[]
  int cpmIndexPrev = 0; //Flag to prevent duplicative counting

  //Timing Settings for Loop Interval
  int prevTime = 0;
  int currTime = 0;

  int totalSec = 0; //Elapsed time of measurement [sec]
  int totalHour = 0; //Elapsed time of measurement [hour]

  //Time settings for CPM calcuaration
  int cpmTimeMSec = 0;
  int cpmTimeSec = 0;
  int cpmTimeMin = 0;

  //String buffers of float values for serial output
  char cpmBuff[20];
  char uSvBuff[20];
  char uSvdBuff[20];

  //Initialize cpmHistory[]
  for (int i = 0; i < 50; i++ )
  {
    cpmHistory[i] = 0;
  }

  //Get start time of a loop
  prevTime = millis();

  while ( totalSec < 60) {
    // Raw data of Radiation Pulse: Not-detected -> High, Detected -> Low
    // 輻射測量: 無檢測到->high, 檢測到->low
    int sign = digitalRead(signPin);

    // Raw data of Noise Pulse: Not-detected -> Low, Detected -> High
    // 雜訊測量: 無檢測到->low, 檢測到->high
    int noise = digitalRead(noisePin);

    //Radiation Pulse normally keeps low for about 100[usec]
    //輻射波訊號通常持續100usec
    //Deactivate Radiation Pulse counting for a while
    //若此迴圈偵測到(sign==0)且上一個迴圈也偵測到(sON==1)則不計數
    if (sign == 0 && sON == 0)
    {
      sON = 1;
      signCount++;
    } else if (sign == 1 && sON == 1) { //直到此次測量無偵測則開啟下一次計數
      sON = 0;
    }

    //Noise Pulse normally keeps high for about 100[usec]
    //背景雜訊通常持續100usec
    //Deactivate Noise Pulse counting for a while
    //若此迴圈偵測到(noise==1)且上一個迴圈也偵測到(nON==1)則不計數
    //Noise Pulse normally keeps high for about 100[usec]
    if (noise == 1 && nON == 0)
    { //Deactivate Noise Pulse counting for a while
      nON = 1;
      noiseCount++;
    } else if (noise == 0 && nON == 1) { //直到此次測量無偵測則開啟下一次計數
      nON = 0;
    }

    //Output readings to serial port, after 10000 loops
    //測量10000次後輸出到serial顯示，循環週期在arduino Nano(ATmega328)上大約為160-170毫秒
    if (index == 10000) //About 160-170 msec in Arduino Nano(ATmega328)
    {
      //      Serial.println("noiseCount:");
      //      Serial.println(noiseCount);
      //Get current time
      currTime = millis();

      //No noise detected in 10000 loops
      //若10000循環無雜訊則進行計算
      if (noiseCount == 0)
      {
        //Shift an array for counting log for each 6 sec.
        if ( totalSec % 6 == 0 && cpmIndexPrev != totalSec)
        {
          cpmIndexPrev = totalSec;
          cpmIndex++;

          if (cpmIndex >= 200)
          {
            cpmIndex = 0;
          }

          if (cpmHistory[cpmIndex] > 0)
          {
            cpm -= cpmHistory[cpmIndex];
          }
          cpmHistory[cpmIndex] = 0;
        }

        //Store count log
        cpmHistory[cpmIndex] += signCount;
        //Add number of counts
        cpm += signCount;

        //Get ready time for 10000 loops
        cpmTimeMSec += abs(currTime - prevTime);
        //Transform from msec. to sec. (to prevent overflow)
        if (cpmTimeMSec >= 1000)
        {
          cpmTimeMSec -= 1000;
          //Add measurement time to calcurate cpm readings (max=20min.)
          if ( cpmTimeSec >= 20 * 60 )
          {
            cpmTimeSec = 20 * 60;
          } else {
            cpmTimeSec++;
          }

          //Total measurement time
          totalSec++;
          //Transform from sec. to hour. (to prevent overflow)
          if (totalSec >= 3600)
          {
            totalSec -= 3600;
            totalHour++;
          }
        }

        //Elapsed time of measurement (max=20min.)
        double min = cpmTimeSec / 60.0;
        if (min != 0)
        {
          //Calculate cpm, uSv/h and error of uSv/h
          dtostrf(cpm / min, -1, 3, cpmBuff);
          dtostrf(cpm / min / alpha, -1, 3, uSvBuff);
          dtostrf(sqrt(cpm) / min / alpha, -1, 3, uSvdBuff);
        } else {
          //Devision by zero
          dtostrf(0, -1, 3, cpmBuff);
          dtostrf(0, -1, 3, uSvBuff);
          dtostrf(0, -1, 3, uSvdBuff);
        }

        //Create message for serial port
        sprintf(msg, "%d,%d.%03d,%d,%s,%s,%s",
                totalHour, totalSec,
                cpmTimeMSec,
                signCount,
                cpmBuff,
                uSvBuff,
                uSvdBuff
               );
        usv = cpm / min / alpha;

        //Send message to serial port
        Serial.print(msg);
        Serial.print(F(" -> usv: "));
        Serial.println(usv, 3);
      } else {
        Serial.println(F("high noise level"));
      }

      //Initialization for next 10000 loops
      prevTime = currTime;
      signCount = 0;
      noiseCount = 0;
      index = 0;
    }
    index++;
  }
  return usv;
}

float measure_voc() {
  float tgsADVcalib_tm = analogRead(gasSensor) * (-0.0256 * dht.readTemperature() + 1.535); //將ADV做溫度補償
  float tgsADVcalib_tm_hm = tgsADVcalib_tm * (-0.0029 * dht.readHumidity() + 1.1938); //將ADV做濕度補償
  float voc = (tgsADVcalib_tm_hm - 113.87) / 10.497; //將ADV換算成ppm濃度
  if (voc < 0) {
    voc = 0;   //將負數的讀值歸零
  }
  return voc;
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


