#include "CTBot.h"
#include "DHT.h"
#include <DHT_U.h>
#include <ArduinoJson.h>

#define SERIAL_PORT         115200
#define DELAY               500
#define HUMIDITY_PIN        4
#define DHTTYPE             DHT11
#define RELAY1_PIN          16
#define RELAY2_PIN          5
#define ANALOG_PIN          A0
#define SSID_WIFI           ""  
#define PASSWORD_WIFI       ""  
#define TOKEN_BOT           ""
#define IOT_ID              "" //random generate
#define TIMEOUT_CHECK_POWER 10

CTBot myBot;
DHT_Unified dht(HUMIDITY_PIN, DHTTYPE);

void setup() {
	Serial.begin(SERIAL_PORT);
	Serial.println("Starting TelegramBot...");

	myBot.wifiConnect(SSID_WIFI, PASSWORD_WIFI);
	myBot.setTelegramToken(TOKEN_BOT);

  dht.begin();
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

	if (myBot.testConnection())
		Serial.println("\ntestConnection OK");
	else
		Serial.println("\ntestConnection NOK");
}

void loop() {
	TBMessage msg;
	if (CTBotMessageText == myBot.getNewMessage(msg)){
    handleMessage(msg);
  }

	delay(DELAY);
}

void handleMessage(TBMessage msg){
  DynamicJsonDocument doc(200);
  doc["iot_id"] = IOT_ID;
  
  if(msg.text == "/turnOnPC_" IOT_ID){
    myBot.sendMessage(msg.group.id, powerONPC(doc));
    checkPowerPC(msg.group.id);
  }else if(msg.text == "/hardTurnOffPC_" IOT_ID){
    myBot.sendMessage(msg.group.id, powerOffPC(doc));
    checkPowerPC(msg.group.id);
  }else if(msg.text == "/restartPC_" IOT_ID){
    myBot.sendMessage(msg.group.id, restartPC(doc));
  }else if(msg.text == "/getInfoPC_" IOT_ID){
    myBot.sendMessage(msg.group.id, getInfoPC());
  }else if(msg.text == "/getIdBot"){
    String iotIdString(IOT_ID); 
    myBot.sendMessage(msg.group.id, "ID: " + iotIdString);
  }
}
String restartPC(DynamicJsonDocument &doc){
  if(isPowerOn()){
    handlePowerPC(RELAY2_PIN, 500);
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["type"] = "power";
    data["message"] = "Sedang merestart PC";
  }else{
    doc["status"] = "error";
    JsonObject data = doc.createNestedObject("data");
    data["type"] = "power";
    data["message"] = "PC dalam keadaan mati";
  }
  return jsonToSTR(doc);
}
String powerOffPC(DynamicJsonDocument &doc){
  if(isPowerOn()){
    handlePowerPC(RELAY1_PIN, 5000);
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["type"] = "power";
    data["message"] = "Sedang mematikan PC secara paksa";
  }else{
    doc["status"] = "error";
    JsonObject data = doc.createNestedObject("data");
    data["type"] = "power";
    data["message"] = "PC sudah mati";
  }

  return jsonToSTR(doc);
}

String powerONPC(DynamicJsonDocument &doc){
  if(isPowerOn()){
    doc["status"] = "error";
    JsonObject data = doc.createNestedObject("data");
    data["type"] = "power";
    data["message"] = "PC sudah menyala";
  }else{
    handlePowerPC(RELAY1_PIN, 500);
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["type"] = "power";
    data["message"] = "Sedang menyalakan PC";
  }

  return jsonToSTR(doc);
}

void checkPowerPC(uint32_t id_user){
  DynamicJsonDocument doc(200);
  doc["iot_id"] = IOT_ID;
  JsonObject data = doc.createNestedObject("data");
  
  int count = 0;
  while(count < TIMEOUT_CHECK_POWER){
    if(isPowerOn()){
      Serial.println(id_user);
      Serial.println(count);
      doc["status"] = "success";
      data["type"] = "power";
      data["message"] = "PC Menyala";
      myBot.sendMessage(id_user, jsonToSTR(doc));
    }
    count++;
    delay(500);
  }

  if(!isPowerOn()){
    doc["status"] = "success";
    data["type"] = "power";
    data["message"] = "PC Mati";
    myBot.sendMessage(id_user, jsonToSTR(doc));
  }
}

bool isPowerOn(){
  int sensorValue = 0; 
  float sensorVoltage = 0; 
  sensorValue = analogRead(ANALOG_PIN);
  sensorVoltage = ((float)(sensorValue * 5.0)/1023.0)/ (7500.0/(15000.0+7500.0));
  return sensorVoltage > 1;
}

void handlePowerPC(int pin, int sleep){
  digitalWrite(pin, LOW);
	delay(sleep);
  
  digitalWrite(pin, HIGH);
	delay(sleep);
}

String getInfoPC(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    return tempResDHTP("error", "Error reading temperature!", 0, 0);
  } 
  float temp = event.temperature;

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)){
    return tempResDHTP("error", "Error reading humidity!", 0, 0);
  }
  float humidity = event.relative_humidity;

  char result[100];
  sprintf(result, "Temperature: %.2f°C\nHumidity: %.2f%%", temp, humidity);
  return tempResDHTP("success", result, temp, humidity);
}

String tempResDHTP(String status, String msg, float temp, float humidity){
  DynamicJsonDocument doc(200);
  doc["iot_id"] = IOT_ID;
  doc["status"] = status;
  JsonObject data = doc.createNestedObject("data");
  data["type"] = "status";
  data["message"] = msg;
  data["temperature"] = temp;
  data["humidity"] = humidity;
  data["power_on"] = isPowerOn();
  return jsonToSTR(doc);
}

String jsonToSTR(DynamicJsonDocument doc){
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
  return jsonString;
}

