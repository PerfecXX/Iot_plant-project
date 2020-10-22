
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <TridentTD_LineNotify.h>

#define DHTPIN 17
#define DHTTYPE DHT11
#define MOISTURE A0 //pin soil
#define RELAY 2 //set pin relay

const char* ssid ="TNI_ROBOT"; ; //***change
const char* password ="tnieng406"; //***change
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "3e2529d4-40ed-4322-9833-8e6b0dccbb3d"; //***change
const char* mqtt_username = "pdNw9nVS2M12qmPm5K2xkoPLGvMAmWsH";  //***change
const char* mqtt_password = "tSO56gFCsHIrp1u5!lpzamnTTTGM!ZXG";  //***change
const String LINE_TOKEN = "UXoQuM29uL2sKu7Afd45mtdgpe4EuR5Le7zS3GVmSku"; //***change

String motorState ; //show motor state 0=found 1 = not found

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long last = 0;
const unsigned long tdelay = 1000;
short trigger =0; //check state moi in soil 0 = dry 

char msg[100];

void reconnect();
void onoff(int ); //sent LED status to netpie
void callback(char* ,byte* , unsigned int ); //get data from netpie
void line_noti(String,float ); //sent message to line

void setup()
{
  
  Serial.begin(115200);

  pinMode(RELAY,OUTPUT); //set relay spent power
 
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // access wifi
  while(WiFi.status() !=WL_CONNECTED) //check disconnect
  {
    delay(500);
    Serial.print(".");  
  }
  Serial.println(" WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)");
  LINE.setToken(LINE_TOKEN);

  dht.begin();
}

void loop()
{
   
  float humidityAir = dht.readHumidity(); //get air humidity
  float temperature = dht.readTemperature(); // get air temp
  int moisture = map(analogRead(MOISTURE), 0, 4095, 0, 100); //get soil humidity

  if(digitalRead(RELAY) == 0)
    motorState ="relay is off";
   else if(digitalRead(RELAY) == 1)
    motorState ="relay is on";
    
  if(!client.connected())
      reconnect();
  
   client.loop();
   if(millis() - last > tdelay) { //check delay
     if(isnan(humidityAir) || isnan(temperature) || isnan(moisture)) {
          String data = "{\"data\": {\"humidityair\":" + String(humidityAir) + 
                                 ",\"temperature\":" + String(temperature) +   
                                 ",\"moisture\":" + String(moisture) + 
                                 "}}";
                                 
        Serial.println(motorState);
        Serial.println(data);
      //in case of nan (Not a number) is read from DHT
     }
     else {
        String info = "{\"data\": {\"humidityair\":" + String(humidityAir) + 
                                 ",\"temperature\":" + String(temperature) +   
                                 ",\"moisture\":" + String(moisture) + 
                                 "}}";
        Serial.println(motorState);
        Serial.println(info);
        info.toCharArray(msg, (info.length() + 1));
        client.publish("@shadow/data/update", msg); // sent data to shadow netpie
        last = millis();
      }
    }
    if(trigger ==0)
      if(moisture <= 20){
          trigger =1;   
          line_noti("test: moisture dry",moisture);
        }
    if(trigger ==1)
      if(moisture >= 90){
          trigger =0;   
          line_noti("test: moisture very wet",moisture);
        }
}

void reconnect()
{
  while (!client.connected()){
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)){
      Serial.println("connected");
      client.subscribe("@msg/relayState");}
    else
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.print("Try again in 5 sec");
        delay(5000);
      }
    }
}

void onoff(int motor) //sent LED status to netpie
{
  if(motor == 1)
  {
  Serial.println("Turn on relay");
  digitalWrite(RELAY,HIGH);

  }
   else if (motor == 0) 
   {
    Serial.println("Turn off relay");
    digitalWrite(RELAY,LOW);

  }
}

void callback(char* topic,byte* payload, unsigned int length) //get data from netpie
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String msg;
  for (int i = 0; i < length; i++) {
    msg = msg + (char)payload[i];
  }
   Serial.println(msg);
  if( String(topic) == "@msg/relayState") 
    if (msg == "on"){
      onoff(1); 
     client.publish("@shadow/data/update","{\"data\" : {\"relayState\" : \"on\"}}");
    } 
    else if (msg == "off") {
      onoff(0);
      client.publish("@shadow/data/update", "{\"data\" : {\"relayState\" : \"off\"}}");
    } 
}
void line_noti(String msg,float value)
{
  LINE.notify( msg + String(value));
}
