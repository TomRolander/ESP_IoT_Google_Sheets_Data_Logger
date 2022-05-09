/**************************************************************************
  ESP IoT Google Sheets Data Logger

  Original Code:  2022-05-06

  Tom Rolander, MSEE
  Mentor, Circuit Design & Software
  Miller Library, Fabrication Lab
  Hopkins Marine Station, Stanford University,
  120 Ocean View Blvd, Pacific Grove, CA 93950
  +1 831.915.9526 | rolander@stanford.edu

 **************************************************************************/

/**************************************************************************

  To Do:
  - Adjust time for DST

 **************************************************************************/

#define PROGRAM "ESP8266 IoT Data Logger to Google Sheets"
#define VERSION "Ver 0.2 2022-05-09"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#endif

#include <UrlEncode.h>
#include "time.h"

#include <HTTPSRedirect.h>

#include <DHT.h>

#ifdef  ESP8266
#define DHT_SENSOR_PIN  5 // ESP8266 pin GPIO5 connected to DHT11 sensor
#endif
#ifdef  ESP32
#define DHT_SENSOR_PIN  5 // ESP8266 pin GPIO5 connected to DHT11 sensor
//#define DHT_SENSOR_PIN  21 // ESP32 pin GIOP21 connected to DHT11 sensor
#endif

#define DHT_SENSOR_TYPE DHT11

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// WiFi credentials
char* ssid = "Stanford";        
char* password = "";      

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -(8*3600);
const int   daylightOffset_sec = 3600;

// Gscript ID and required credentials
const char* host = "script.google.com";      
const char* GScriptId = "AKfycbwtsgdoqdvuy_B8lP3UovhuDfOdVBavK0VKvdhcqVdmpaV41CqRdq5_i1Tli7Wf3eliUg";
const int httpsPort = 443;      

String url = String("/macros/s/") + GScriptId + "/exec";

HTTPSRedirect* client = nullptr;


// --------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);  
  delay(1000); 

  Serial.println(PROGRAM);
  Serial.println(VERSION);

  dht_sensor.begin(); // initialize the DHT sensor
                           
  setupWiFi();                                      
  setupData(); 

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime(); 
}

void loop()
{
  float humi  = dht_sensor.readHumidity();
  float tempC = dht_sensor.readTemperature();

  // check whether the reading is successful or not
  if ( isnan(tempC) || isnan(humi)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print("%");
    Serial.print("  |  ");
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println("°C");
  }

  // Post the data to google spreadsheet
  postData(humi, tempC);
  getData();
  
  delay(30000);                                     // Time delay of 30 sec 
}
// --------------------------------------------------------------------------------------------------------


// Function to setup the WiFi connection 
void setupWiFi(){
  
  WiFi.begin(ssid, password);                           // Connect to the network 

  // Wait for the WiFi to be connected 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print("Connecting to WiFi...  ");
    Serial.println(ssid);
  }
  
  Serial.println("Connected to the WiFi network");   
  Serial.print(" IP address: ");
  Serial.println(WiFi.localIP());                      
}

void setupData(){
  client = new HTTPSRedirect(httpsPort); 
  client->setInsecure(); 
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  // Connect to host - "script.google.com"
  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times 
  bool WiFiFlag = false;
  for (int i=0; i<5; i++)
  {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) 
    {
       WiFiFlag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  // Connection Status, 1 = Connected, 0 is not.
  Serial.println("Connection Status: " + String(client->connected()));
  Serial.flush();

  // Exit if the connection is failed 
  if (!WiFiFlag)
  {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    Serial.flush();
    return;
  }
}

void postData(float humi, float tempC)
{
  if (!client->connected())             
  {
#if DEBUG  
    Serial.println("Connecting to client again..."); 
#endif
    client->connect(host, httpsPort);
  }

  char dateTimeStringBuff[50]; 
  getDateTimeString(dateTimeStringBuff, sizeof(dateTimeStringBuff));
  
  Serial.print(dateTimeStringBuff);
  Serial.print(" ");
  Serial.print(humi);
  Serial.print(" ");
  Serial.println(tempC);
     
  String urlFinal = url + "?datetime=" + urlEncode(dateTimeStringBuff) + "&humidity=" + String(humi) + "&tempC=" + String(tempC);
#if DEBUG  
  Serial.println("URL created:");
  Serial.println(urlFinal);
#endif

  if (client != nullptr)                           
  {
    if (!client->connected())                         
    {
      client->connect(host, httpsPort);              
      String payload = "?datetime=" + urlEncode(dateTimeStringBuff) + "&humidity=" + String(humi) + "&tempC=" + String(tempC);
      client->POST(url, host, payload);
      client->GET(url, host);
      Serial.println(">> Succeed to POST data"); 
    }
  }
  else 
  {
    Serial.println(">> Failed to POST data");
  }

  client->GET(urlFinal, host);
  String payload = client->getResponseBody(); 
#if DEBUG  
  Serial.println("Payload:");
  Serial.println(payload); 
#endif
  client->stop();    
}

void getData()
{
  if (!client->connected())             
  {
#if DEBUG  
    Serial.println("Connecting to client again..."); 
#endif
    client->connect(host, httpsPort);
  }

  String urlFinal = url + "?read";
#if DEBUG  
  Serial.println("URL created:");
  Serial.println(urlFinal);
#endif

  if (client != nullptr)                           
  {
    if (!client->connected())                         
    {
      client->connect(host, httpsPort);              
      String payload = "?read";
      client->POST(url, host, payload);
      client->GET(url, host);
      Serial.println(">> Succeed to GET data"); 
    }
  }
  else 
  {
    Serial.println(">> Failed to GET data");
  }

  client->GET(urlFinal, host);
  String payload = client->getResponseBody(); 
//#if DEBUG  
  Serial.println("Payload:");
  Serial.println(payload); 
//#endif
  client->stop();   
}


void printLocalTime()
  {
    char dateTimeStringBuff[50]; //50 chars should be enough
    
    getDateTimeString(dateTimeStringBuff, sizeof(dateTimeStringBuff));
    Serial.println(dateTimeStringBuff);
    Serial.println();
}

void getDateTimeString(char *dateTimeStringBuff, int nmbdateTimeStringBuff)
{
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(dateTimeStringBuff, nmbdateTimeStringBuff, "%Y-%m-%d %H:%M:%S", &timeinfo);
}