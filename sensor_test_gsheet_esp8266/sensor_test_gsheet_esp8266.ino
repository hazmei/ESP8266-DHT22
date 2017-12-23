/*
 * A simple code to push data to google sheet 
 * Based on article at http://embedded-lab.com/blog/post-data-google-sheets-using-esp8266/
 * 
 * Dependencies:
 *    - HTTPSRedirect library from http://embedded-lab.com/blog/post-data-google-sheets-using-esp8266/
 *    - DHT library (if you're using DHTxx temperature sensor)
 *    - ESP8266 library
 *  
 *  Improvements:
 *    - Better power management (deep sleep after data is streamed)
 *    - Physical indicator that sensor is sending back data
 *  
 *  To do:
 *    - Code cleanup
 *    - Further improve power efficiency
 */

#include <ESP8266WiFi.h>
#include <DHT.h>
#include "HTTPSRedirect.h"

#define DHTPIN          0         // D3
#define WARN_LED        13        // D7
#define DHTTYPE         DHT22     // DHT 22  (AM2302), AM2321

const long sleepduration_sec = 300; // 5 mins duration
const char* ssid = "...";
const char* password = "...";

const char *GScriptId = "...";
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort = 443;

bool isWarning = false;

// Prepare the url (without the varying data)
String url = String("/macros/s/") + GScriptId + "/exec?";

const char* fingerprint = "55 CC D9 B4 66 6B C6 F4 64 AA 18 46 92 E0 39 BE 77 EC 58 E7";

DHT dht(DHTPIN, DHTTYPE);
HTTPSRedirect client(httpsPort);

void setup() {
  dht.begin();
  Serial.begin(115200);
  Serial.println();

  pinMode(WARN_LED, OUTPUT);
  digitalWrite(WARN_LED, LOW);
  
  blinkstatus();
  retrieveDHT22();
  Serial.println("Sensor going to deep sleep");
  ESP.deepSleep(sleepduration_sec*1e6); // change sec to microsec
}

void blinkstatus(){
  for (int i=0;i<2;i++){
    delay(500);    
    digitalWrite(WARN_LED,HIGH);
    delay(500);
    digitalWrite(WARN_LED,LOW);
  }
}

void loop() {
}

void retrieveDHT22(){
  String buf;
  
  float humid = dht.readHumidity();
  float temp = dht.readTemperature();
  
  if (isnan(humid) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    if (!isWarning)
      digitalWrite(WARN_LED, HIGH);
    return;
  }

  float heatindex = dht.computeHeatIndex(temp, humid, false);
  
  wificonnect();

  buf += F("humidity=");
  buf += String(humid,2);
  buf += F("&temperature=");
  buf += String(temp,2);
  buf += F("&heatindex=");
  buf += String(heatindex,2);

  postdata(buf);
  WiFi.disconnect();
}

void wificonnect(){
  Serial.print("connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void postdata(String data){
  bool flag = false;
  for (int i=0;i<5;i++){
    int retVal = client.connect(host,httpsPort);
    if (retVal == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag){
    Serial.println("Unable to connect to server");
    if (!isWarning)
      digitalWrite(WARN_LED, HIGH);
  }

  if (!client.verify(fingerprint, host))
    Serial.println("Certificate mismatched!");

  String urlFinal = url + data;
  client.GET(urlFinal, host, googleRedirHost);
}
