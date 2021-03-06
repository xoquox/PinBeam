#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <Bounce2.h>


#include "settings.h"

#define BUTTON_DEBOUNCE_MS 50


struct config_frame 
{
  int dir;
  int state;
  Bounce debouncer = Bounce();
  bool init;
};


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

//Bounce debouncer[pin_lookup_size] = Bounce();
config_frame frame[pin_lookup_size];



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
      if(strstr(pch, PINBEAM_ID))  //Config INPUTs
      {
        Serial.println("Own ID found");
        pch = strtok(NULL, " ");
        strncpy(temp_ID, pch, 2);
        temp_ID[2] = '\0';
        id = getID(temp_ID);
        if(strstr(pch, "UP"))
        {
          //pinMode(id,INPUT_PULLUP);
          frame[id].dir = INPUT_PULLUP;
          Serial.print(temp_ID);
          Serial.println(" PIN set to PULLUP");
        }
        else
        {
          //pinMode(id,INPUT);
          frame[id].dir = INPUT;
          Serial.print(temp_ID);
          Serial.println(" PIN set to INPUT Only");          
        }
        //debouncer[id].attach(id);
        //debouncer[id].interval(BUTTON_DEBOUNCE_MS);
        pinMode(id,frame[id].dir);
        frame[id].debouncer.attach(id);
        frame[id].debouncer.interval(BUTTON_DEBOUNCE_MS);
        frame[id].debouncer.update();
        frame[id].state = frame[id].debouncer.read();
      }
      pch = strtok(buffer, " "); //dreckig
      pch = strtok(NULL, " "); //dreckig
      pch = strtok(NULL, " "); //dreckig
      if(strstr(pch, PINBEAM_ID))  //conifg OUTPUTs
      {
        Serial.println("Own ID found");
        pch = strtok(NULL, " ");
        strncpy(temp_ID, pch, 2);
        temp_ID[2] = '\0';
        id = getID(temp_ID);
        //pinMode(id,OUTPUT);
        frame[id].dir = OUTPUT;
        pinMode(id,frame[id].dir);
        frame[id].state = digitalRead(id);
        frame[id].init = true;
        Serial.print(temp_ID);
        Serial.println(" PIN set to OUTPUT");
      }
    }
    else if(strstr(buffer, PINBEAM_ID))
    {
      pch = strtok(buffer, " ");
      strncpy(temp_ID, pch, 2);
      temp_ID[2] = '\0';
      id = getID(temp_ID);
      digitalWrite(id, HIGH);
      
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
    if((frame[i].dir == OUTPUT) && (frame[i].debouncer.update() || frame[i].init))
    {
      char buf[256];
      frame[i].init = false;
      strcpy(buf,PINBEAM_ID);
      strcat(buf, " ");
      strcat(buf, pin_lookup[i]);
      if(frame[i].debouncer.read())
      {
        strcat(buf, " LOW");
        frame[i].state = LOW;
      }
      else
      {
        strcat(buf, " HIGH");
        frame[i].state = HIGH;
      }
      mqttClient.publish(PIN_NETWORK,buf);
      Serial.print("Button changed:");
      Serial.println(buf);      
    }
  }
  //delay(100);
  connectMqtt();
  mqttClient.loop();
}

