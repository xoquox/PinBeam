#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <Bounce2.h>


#include "settings.h"

#define BUTTON_DEBOUNCE_MS 50




static const int pin_lookup_size = 17;

static const char* pin_lookup[pin_lookup_size] =
{
  "NULL",
  "TX",
  "D4",
  "RX",
  "D2",
  "D1",
  "GPIO6",
  "GPIO7",
  "GPIO8",
  "SD2",
  "SD3",
  "GPIO11",
  "D6",
  "D7",
  "D5",
  "D8",
  "D0",
};

Bounce debouncer[pin_lookup_size] = Bounce();




WiFiClient wifiClient;
PubSubClient mqttClient;

int getID(char* ID) //returns real Port ID of Pin Name, 0 if not found
{
  //Serial.print(ID);
  for(int i = 0; i < pin_lookup_size ; i++)
  {
    //Serial.println(pin_lookup[i]);
    if(!strcmp(pin_lookup[i], ID))
    {
      Serial.print(pin_lookup[i]);
      Serial.println(" PIN FOUND");
      return i;
    }
  }
  Serial.println("ERROR, NO PIN FOUND");
  return 0;
}

void setup()
{
  Serial.begin(115200);
  delay(10);


  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname(HOSTNAME_ESP);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }


    
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
   char buffer[length + 1];
   char *pch;
   char temp_ID[3];
   int id;
   for (int i = 0; i < length; i++) 
   {
    buffer[i]=((char)payload[i]);
   }
   buffer[length] = '\0';
   Serial.println(buffer);
   Serial.println(topic);
   if(strstr(topic, "config"))
   {
    if(strstr(buffer, "SET"))
    {
      pch = strtok(buffer, " ");
      Serial.println("Found Config");
      Serial.println(pch);
      pch = strtok(NULL, " ");
      Serial.println(pch);
      if(strstr(pch, PINBEAM_ID))  
      {
        Serial.println("Own ID found");
        pch = strtok(NULL, " ");
        strncpy(temp_ID, pch, 2);
        temp_ID[2] = '\0';
        id = getID(temp_ID);
        if(strstr(pch, "UP"))
        {
          pinMode(id,INPUT_PULLUP);
          Serial.print(temp_ID);
          Serial.println(" PIN set to PULLUP");
        }
        else
        {
          pinMode(id,INPUT);
          Serial.print(temp_ID);
          Serial.println(" PIN set to INPUT Only");          
        }
        debouncer[id].attach(id);
        debouncer[id].interval(BUTTON_DEBOUNCE_MS);
      }
    }
   }
   //pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void connectMqtt() 
{
  char buffer[strlen(PIN_NETWORK) + strlen(PINBEAM_ID) + 10];
  strcpy(buffer, PIN_NETWORK);
  while (!mqttClient.connected()) 
  {
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(mqttHost, 1883);
    mqttClient.connect(HOSTNAME_ESP);
   
    mqttClient.subscribe(buffer);
    Serial.println(buffer);
    //strcat(buffer, "/config");
    //mqttClient.subscribe(buffer);
    mqttClient.subscribe("BEAM/config");
    Serial.println(buffer);
    
    
    mqttClient.setCallback (callback);
    delay(1000);
  }
}


void loop()
{
  for(int i = 0; i < pin_lookup_size; i++)
  {
    debouncer[i].update();
    if(debouncer[i].fell())
    {
      mqttClient.publish(PIN_NETWORK,PINBEAM_ID );
      Serial.print("Button presset:");
      Serial.println(pin_lookup[i]);
      
    }
  }
  //delay(100);
  connectMqtt();
  mqttClient.loop();
}

