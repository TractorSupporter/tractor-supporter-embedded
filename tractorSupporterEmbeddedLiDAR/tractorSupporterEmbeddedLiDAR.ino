// In RPLidar.h and RPLidar.h begin() function has to be changed from bool to void type
// 
// they can be installed with modified ZIP file from https://dronebotworkshop.com/getting-started-with-lidar/
//
// Code based on https://github.com/robopeak/rplidar_arduino and https://lingshunlab.com/book/esp32/esp32-use-rplidar-a1
//
// The code was tested and found to be working when compiled using the ESP32 board package by Espressif Systems, version 2.0.11

#include <RPLidar.h>
#include <vector>
#include <algorithm>

#include <WiFi.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include <ArduinoJson.h>

RPLidar lidar;

#define RPLIDAR_MOTOR 14 

struct Measurement {
  float angle;
  float distance;

  Measurement(float a, float d) : angle(a), distance(d) {}
};

std::vector<Measurement> measurements;

bool lidarStopped = true;
bool lidarON = false;

#define nBuffer 50
WiFiUDP udp;
char bufferData[nBuffer];

unsigned long lastMessageTime = 0;
unsigned long connectionCheckInterval = 1000;

void serializeAndSend(const std::vector<Measurement> &measurements);
void receivePacketFromServer();
void connectionCheckSend();
void connectToWifi();
void startLidar();
void stopLidar();

void setup() {
  Serial.begin(115200);

  connectToWifi();
  udp.begin(udpPort);

  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  lidar.begin(Serial2);

  pinMode(RPLIDAR_MOTOR, OUTPUT);
  delay(1000);

  Serial.println("END OF SETUP");
  
}

void loop() {

  if(lidarON && lidarStopped)
  {
    startLidar();
  }

  if(!lidarON && !lidarStopped)
  {
    stopLidar();
  }

  if(lidarStopped == false)
  {
    if (IS_OK(lidar.waitPoint()))
    {
      float distance = lidar.getCurrentPoint().distance;

      float angle    = lidar.getCurrentPoint().angle; 

      if ((angle >= 0 && angle <= 30) || (angle >= 330 && angle <= 360))
      {
        // Normalize angle to [-30 : + 30]
        if (angle >= 330) angle -= 360; 
  
        measurements.emplace_back(angle, distance);
      }

      if (measurements.size() == 60) {
        Serial.println("Revolution complete!");

        std::sort(measurements.begin(), measurements.end(), [](const Measurement &a, const Measurement &b) {
          return a.angle < b.angle;
        });

        serializeAndSend(measurements);

        measurements.clear();
      }

    } else {
      Serial.println("ELSE");
      stopLidar();
      sleep(1000);
      startLidar();
    }
  }

  connectionCheckSend();
  receivePacketFromServer();
}


void serializeAndSend(const std::vector<Measurement> &measurements) {

  String measurementsString = "";
  for (const auto &m : measurements) {
    measurementsString += String(m.angle, 2) + ";" + String(m.distance, 2) + ";";
  }

  const int capacity = JSON_OBJECT_SIZE(2);
  StaticJsonDocument<capacity> doc;

  doc["sensor"] = "lidarSensor";
  doc["measurements"] = measurementsString;

  String serializedData;
  serializeJson(doc, serializedData);

  Serial.println("Serialized Data:");
  Serial.println(serializedData);

  udp.beginPacket(udpAddress, udpPort);
  udp.print(serializedData);
  udp.endPacket();
}

void connectionCheckSend() {
  unsigned long currentTime = millis();
  if (currentTime - lastMessageTime >= connectionCheckInterval) {

    StaticJsonDocument<50> doc;
    doc["message"] = "connection check";

    String serializedData;
    serializeJson(doc, serializedData);

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Attempting to reconnect...");
      stopLidar();
      connectToWifi();
    }

    udp.beginPacket(udpAddress, udpPort);
    udp.print(serializedData);
    udp.endPacket();

    Serial.println("Sent connection check message.");

    lastMessageTime = currentTime;
  }
}

void receivePacketFromServer() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {

    udp.read(bufferData, nBuffer);
    bufferData[packetSize] = '\0';

    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, bufferData);

    if (error) {
      Serial.print("Error parsing packet: ");
      Serial.println(error.c_str());
      return;
    }
    
    if (doc.containsKey("shouldRun")) {
      lidarON = doc["shouldRun"];
      Serial.print("Updated lidarON: ");
      Serial.println(lidarON);
    } else {
      Serial.println("Received packet does not contain lidarON key.");
    }
  }
}

void connectToWifi(){
  WiFi.persistent(false);
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(5000);
}

void startLidar()
{
  lidar.startScan();
  digitalWrite(RPLIDAR_MOTOR, HIGH);
  delay(1000);
  lidarStopped = false;
}

void stopLidar()
{
  lidar.stop();
  digitalWrite(RPLIDAR_MOTOR, LOW);
  delay(2000);
  measurements.clear();
  lidarStopped = true;
}
