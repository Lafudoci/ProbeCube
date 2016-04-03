//thingspeak surroundings

unsigned long myChannelNumber = 103375; //Filling in thingspeak channel ID ***Required
const char * myWriteAPIKey = "GUIYRLFRLGQSXXXX"; //Filling in thingspeak API key ***Required

//Blynk surroundings

char auth[] = "07dfd97a0c8c4927ba6a67126fXXXXXXX"; //Filling in blynk auth key ***Required


//Versions
#define VERSION "v099b0325"

//Timer for updating
int LastTimeCheck = 0;
int ThingspeakUpdateTime = 0;
int BlynkUpdateTime = 0;
#define Thingspeak_INTERVAL 60000 //Upload data to thingspeak every 60 seconds
#define Blynk_INTERVAL 5000 //Update blink every 5 seconds

//Wifi surroundings

TCPClient client;
char WiFi_RSSI[5];
int wifisig_percent;
char tsmsg[60];
char StartMsg[60];

//Sensors settings
//Temperature and Humidity
#define DHTPIN 2 //Assign the desired pin "D2"
#define DHTTYPE DHT22 //Assign the type of dht sensor "DHT22"
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
//=======Calibration of temperature==========
#define Compensator_tem -4
#define Compensator_hum 15
//===========================
//Volatile liguids
#define gasSensor 0 //Assign the desired pin "A0"
char sensor_voc[5];
//Aerosols
long  pmcf10 = 0;
long  pmcf25 = 0;
long  pmcf100 = 0;
long  pmat10 = 0;
long  pmat25 = 0;
long  pmat100 = 0;
char buf[50];

//***************************************************

STARTUP(WiFi.selectAntenna(ANT_AUTO)); //Make the shifting of using build-in wifi chip or additional antenna

void setup(void)
{
  
  sprintf(StartMsg, "Start ver. %s , wifi signal: %d %%", VERSION, wifi_signal());
  Particle.publish("Starting", StartMsg);

  Serial.begin(115200); //initiates G3 sensor
  
  dht.begin(); //initiates dht sensor

  Serial1.begin(9600); //initiates aerosol sensor

  Blynk.begin(auth); //initiates blynk

  ThingSpeak.begin(client);  //initiates method ThingSpeak

  delay(500); //Delay for 500ms

}

void loop(void)  //Area for loop functions
{

  Blynk.run(); //Activates Blynk
  
  

  Serial.println(F("--------------------------------"));
  
  //read temperature and humidity
  measure_dht();
  sprintf(sensor_tem, "%.1f", measure_tem); //Formats floats to strings
  sprintf(sensor_hum, "%.1f", measure_hum);

  //read volatile liquids
  sprintf(sensor_voc, "%.1f", measure_voc());

  //read aerosol sensors
  delay (500);
  measure_dust();
  int sensor_dust25 = pmat25;
  int sensor_dust100 = pmat100;
  

  //Upload data

  //Send data through method thingspeak
  if (millis() > ThingspeakUpdateTime) {
    ThingSpeak.setField(1, sensor_tem);
    ThingSpeak.setField(2, sensor_hum);
    ThingSpeak.setField(4, sensor_voc);
    ThingSpeak.setField(5, sensor_dust25);
    ThingSpeak.setField(6, sensor_dust100);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    Serial.println(F("Data has been sent to Thingspeak !!")); //serial & particle report status messages
    sprintf(tsmsg, "Update Thingspeak ch %d OK, wifi signal: %d %%", myChannelNumber, wifi_signal());
    Particle.publish("Update", tsmsg);
    
    ThingspeakUpdateTime = millis() + Thingspeak_INTERVAL;
  }

  //send data through method Blynk 
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
  
  //reset timer if millis overflow
  if ( millis() < LastTimeCheck ){
      ThingspeakUpdateTime = Thingspeak_INTERVAL;
      BlynkUpdateTime = Blynk_INTERVAL;
  }
  LastTimeCheck = millis();

}
//Area of method calls*********************************************


void measure_dht() {
    
    int dhttry;

    do{
		dhttry = 0;
        dht_tem = dht.getTempCelcius();
        dht_hum = dht.getHumidity();
        
        if (dhttry > 0){
        Particle.publish("Error", "DHT error, will retry...");
        delay (1000);
        }
        
        dhttry++;
    } while(isnan(dht_tem) != 0
        || isnan(dht_hum) != 0
        || dht_tem > MAX_TEMPERATURE
        || dht_tem < MIN_TEMPERATURE
        || dht_hum > MAX_HUMIDITY
        || dht_hum < MIN_HUMIDITY);
    
    measure_tem = ( dht_tem -3.3221 ) / 0.988;
    measure_hum = ( dht_hum -0.8229 ) / 0.7886;

}

float measure_voc() {
  float voc = 0;
  for (int i = 1; i <= 5; i ++) {
  float tgsADVcalib_tm = analogRead(gasSensor) * (-0.0256 * dht_tem + 1.535); //formula for calibration, from ADV to temperature 
  float tgsADVcalib_tm_hm = tgsADVcalib_tm * (-0.0029 * dht_hum + 1.1938); //formula for calibration, from ADV to humidity
  float voc_raw = (tgsADVcalib_tm_hm - 113.87) / 10.497; //formula for ADV to ppm 
  voc += 0.1732 * voc_raw - 5.0602;
  delay(1000);
}
  voc /= 5;
  
  if (voc < 0) {
    voc = 0;   //Makes nagtive numbers zero instead
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
      Particle.publish("Error", "G3 check failed, will retry...");
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
        if ( i > 2){
            Particle.publish("Error", "RSSI error");
            wifisig_percent = 0;
            break;
        }
        i++;
    }
     
    return wifisig_percent;

}