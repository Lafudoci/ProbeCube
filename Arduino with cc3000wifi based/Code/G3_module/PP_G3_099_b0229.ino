//thingspeak設定變數
String writeAPIKey =    "K8F66UNFID63SXXX";  //填入thingspeak write API key ***必填
//CC3000無線網路設定變數
#define WLAN_SSID       "3203BIGDATA"     //填入無線網路名稱  ***必填
#define WLAN_PASS       "3203BIGDATA"     //填入無線網路密碼  ***必填

//版本號
#define VERSION "G3099b0229"
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
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
uint32_t ip = 0;
uint32_t DHCPwait;
uint32_t checkDHCP;
uint32_t IPwait;
//指定指示燈腳位
#define green_led     9
//thingspeak上傳設定
String thingSpeakAddress = "http://api.thingspeak.com/update?";
#define Thingspeak_INTERVAL 300000
unsigned long currentMillis = 0;
unsigned long previousUpdateMillis = 0;
boolean firstRun = true;
boolean RunOnce = false;
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
#include <SoftwareSerial.h>
SoftwareSerial Serial_g3(2 , 6); // RX, TX
long  pmcf10 = 0;
long  pmcf25 = 0;
long  pmcf100 = 0;
long  pmat10 = 0;
long  pmat25 = 0;
long  pmat100 = 0;
//char buf[50];
//****************************************************************************
void setup(void)
{
  //指定通訊視窗115200
  Serial.begin(115200);

  //g3
  Serial_g3.begin(9600);

  Serial.println("ProbeCube is starting");
  Serial.print("code version: ");
  Serial.println(VERSION);

  //LED們初始化
  pinMode(green_led, OUTPUT);

  do {
    DHCPwait = 0;
    checkDHCP = 1;
    IPwait = 0;

    LED_start();
    LED_check();
    //    displayDriverMode();

    Serial.println(F("Initialising the CC3000 wifi ..."));
    if (!cc3000.begin()) {
      Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
      for (;;);
    }

    uint16_t firmware = checkFirmwareVersion();
    if (firmware < 0x113) {
      Serial.println(F("Wrong firmware version!"));
      for (;;);
    }

    displayMACAddress();

    Serial.println(F("Deleting old connection profiles"));
    if (!cc3000.deleteProfiles()) {
      Serial.println(F("Failed!"));
      while (1);
    }

    /* Attempt to connect to an access point */
    char *ssid = WLAN_SSID;             /* Max 32 chars */
    Serial.print(F("Attempting to connect to ")); Serial.println(ssid);

    /* NOTE: Secure connections are not available in 'Tiny' mode! */
    if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
      Serial.println(F("Failed!"));
      while (1);
    }

    Serial.println(F("Connected!"));

    /* Wait for DHCP to complete */
    Serial.print(F("Request DHCP...."));
    while (!cc3000.checkDHCP()) {
      if ( DHCPwait > 600) {
        checkDHCP = 0;
        Serial.println(F("failed..retry after 3 mins"));
        LED_error();
        delay(30000);
        break;
      }
      LED_wait();
      delay(100);
      DHCPwait++;
    }


    while (!displayConnectionDetails()) {
      if ( IPwait > 30) {
        checkDHCP = 0;
        Serial.println(F("Connection failed.."));
        LED_error();
        break;
      }
      delay(1000);
      IPwait++;
    }
  } while (checkDHCP != 1);
  Serial.println(F("Connection is established!"));

  wdt_enable (WDTO_8S);
  // 取得 thingspeak IP
  Serial.print(F("Finding api.thingspeak.com -> "));
  while  (ip  ==  0)  {
    if  (!  cc3000.getHostByName("api.thingspeak.com", &ip))  {
      Serial.println(F("Couldn't resolve!"));
      while (1) {
      }
    }
    delay(500);
  }
  wdt_disable();
  Serial.print(F("("));
  cc3000.printIPdotsRev(ip);
  Serial.println(F(")"));

  LED_ok();

  delay(500);       //延遲500ms

}

void loop(void)     //循環函數區域
{
  currentMillis = millis(); //記錄當下時間
  if (currentMillis - previousUpdateMillis >= Thingspeak_INTERVAL) { //判斷是否需要上傳
    needUpdate = true;
  }
  else {
    needUpdate = false;
  }
  if (firstRun) { //若為第一次執行則強迫判斷為需要更新
    needUpdate = true;
    firstRun = false;
  }
  if (needUpdate) { //若判斷為需要更新則開始測量&上傳


    //==========PC_G3 ONLY============
    if (RunOnce) {
      Serial.println(F("Bye!"));
      wdt_enable (WDTO_15MS);
      while (1);
    }
    RunOnce = true;
    //================================

    Serial.println(F("Starting to monitor the air....."));
    previousUpdateMillis = currentMillis;

    //讀取溫溼度
    float sensor_tem = measure_tem();
    Serial.println(F("Tem OK!"));
    float sensor_hum = measure_hum();
    Serial.println(F("Hum OK!"));

    //讀取土壤濕度
    int sensor_soil = measure_soil();
    Serial.println(F("soil OK!"));

    //讀取有機氣體汙染
    float sensor_voc = measure_voc();
    Serial.println(F("voc OK!"));
    //讀取灰塵感測
    delay (2000);
    measure_dust();
    int sensor_dust25 = pmat25;
    int sensor_dust100 = pmat100;


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
    Serial.print(sensor_dust25);
    Serial.println(F("ug/m3"));

    Serial.println(F("====================="));

    //啟動看門狗
    wdt_enable (WDTO_8S);
    Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
    if (client.connected()) {
      Serial.println(F("Connected to Thingspeak server."));
    } else {
      while (1) {}
    }
    wdt_reset ();
    Serial.println ("Sending data:"); //顯示上傳的數據
    //使用http get送出數據
    client.fastrprint(F("GET "));
    client.print(thingSpeakAddress);
    client.fastrprint(F("api_key="));
    client.print(writeAPIKey);
    wdt_reset ();
    client.fastrprint(F("&field1="));
    client.print(sensor_tem);
    client.fastrprint(F("&field2="));
    client.print(sensor_hum);
    wdt_reset ();
    client.fastrprint(F("&field3="));
    client.print(sensor_soil);
    client.fastrprint(F("&field4="));
    client.print(sensor_voc);
    wdt_reset ();
    client.fastrprint(F("&field5="));
    client.print(sensor_dust25);
    client.fastrprint(F("\r\n"));
    wdt_reset ();

    Serial.print(F("GET "));
    Serial.print(thingSpeakAddress);
    Serial.println(writeAPIKey);
    Serial.print(F("&field1="));
    Serial.print(sensor_tem);
    Serial.print(F("&field2="));
    Serial.print(sensor_hum);
    Serial.print("&field3=");
    Serial.print(sensor_soil);
    Serial.print(F("&field4="));
    Serial.print(sensor_voc);
    Serial.print(F("&field5="));
    Serial.println(sensor_dust25);

    wdt_reset ();

    Serial.println(F("Data has been sent successfully!!!"));
    Serial.print(F("Thingspeak response: "));
    while (client.connected()) {
      while (client.available()) {
        char resp = client.read();
        Serial.print(resp);
      }
    }
    Serial.println(F("\nwaiting for next update..."));
    Serial.println("");
    client.close();
    wdt_disable();
  }
}

//方法呼叫區******************************************************************
/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;

#ifndef CC3000_TINY_DRIVER
  if (!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];

  if (!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.println(F("finished!"));
    Serial.print(F("IP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
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

float measure_voc() {
  float voc = 0;
  Serial.print(F("VoC Sampling 5 times..."));
  for (int i = 1; i <= 5; i ++) {
    float tgsADVcalib_tm = analogRead(gasSensor) * (-0.0256 * dht.readTemperature() + 1.535); //將ADV做溫度補償
    float tgsADVcalib_tm_hm = tgsADVcalib_tm * (-0.0029 * dht.readHumidity() + 1.1938); //將ADV做濕度補償
    voc += (tgsADVcalib_tm_hm - 113.87) / 10.497; //將ADV換算成ppm濃度
    delay(1000);
  }
  voc /= 5;
  if (voc < 0) {
    voc = 0;   //將負數的讀值歸零
  }
  return voc;
}

void measure_dust() {
  int count_dust = 0;
  Serial.print(F("PM25 OK!"));
  unsigned char c_dust = 0;
  unsigned char high_dust = 0;
  while (Serial_g3.available()) {
    c_dust = Serial_g3.read();
    if ((count_dust == 0 && c_dust != 0x42) || (count_dust == 1 && c_dust != 0x4d)) {
      Serial.println("check failed");
      break;
    }
    if (count_dust > 15) {
      Serial.println("complete");
      break;
    }
    else if (count_dust == 4 || count_dust == 6 || count_dust == 8 || count_dust == 10 || count_dust == 12 || count_dust == 14) {
      high_dust = c_dust;
    }
    else if (count_dust == 5) {
      pmcf10 = 256 * high_dust + c_dust;
      Serial.print("CF=1, PM1.0=");
      Serial.print(pmcf10);
      Serial.println(" ug/m3");
    }
    else if (count_dust == 7) {
      pmcf25 = 256 * high_dust + c_dust;
      Serial.print("CF=1, PM2.5=");
      Serial.print(pmcf25);
      Serial.println(" ug/m3");
    }
    else if (count_dust == 9) {
      pmcf100 = 256 * high_dust + c_dust;
      Serial.print("CF=1, PM10=");
      Serial.print(pmcf100);
      Serial.println(" ug/m3");
    }
    else if (count_dust == 11) {
      pmat10 = 256 * high_dust + c_dust;
      Serial.print("atmosphere, PM1.0=");
      Serial.print(pmat10);
      Serial.println(" ug/m3");
    }
    else if (count_dust == 13) {
      pmat25 = 256 * high_dust + c_dust;
      Serial.print("atmosphere, PM2.5=");
      Serial.print(pmat25);
      Serial.println(" ug/m3");
    }
    else if (count_dust == 15) {
      pmat100 = 256 * high_dust + c_dust;
      Serial.print("atmosphere, PM10=");
      Serial.print(pmat100);
      Serial.println(" ug/m3");
    }
    count_dust++;
  }
  while (Serial_g3.available()) Serial_g3.read();
  return;
}


  void LED_start()
  {
    digitalWrite(green_led, HIGH);
    delay(1000);
    digitalWrite(green_led, LOW);
    delay(500);
    digitalWrite(green_led, HIGH);
    delay(1000);
    digitalWrite(green_led, LOW);
  }

  void LED_check()
  {
    digitalWrite(green_led, LOW);
  }

  void LED_wait()
  {
    digitalWrite(green_led, LOW);
  }

  void LED_error()
  {
    digitalWrite(green_led, LOW);
  }

  void LED_ok()
  {
    digitalWrite(green_led, HIGH);
  }

  void LED_send()
  {
    digitalWrite(green_led, LOW);
    delay(100);
    digitalWrite(green_led, HIGH);
  }
