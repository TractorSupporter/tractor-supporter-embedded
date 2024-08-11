#include <WiFi.h>
#include <WiFiUdp.h>

//----------------------------ProgramLogic
#define nBuffer 50
extern const char * ssid; 
extern const char * pwd;
extern const char *udpAddress;
extern const int udpPort;
WiFiUDP udp;
long i=1;
char iString[nBuffer] = "hello world";
char bufferData[nBuffer] = "hello world";

void setup(){
  Serial.begin(115200);

  connectToWifi();

  udp.begin(udpPort);
}

void loop(){
  preparePacketForServer();

  receivePacketFromServer();
  
  delay(900);
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

void preparePacketForServer(){
  sprintf(bufferData, "[%s]    idx: %d", iString, i++);
  Serial.print("Sending to server: ");
  Serial.println(bufferData);

  udp.beginPacket(udpAddress, udpPort);
  udp.print(bufferData);
  udp.endPacket();
  
  memset(bufferData, 0, nBuffer);
}

void receivePacketFromServer(){
  udp.parsePacket();
  if(udp.read(bufferData, 50) > 0){
    Serial.print("Setting new message from server: ");
    Serial.println((char *)bufferData);
    sprintf(iString, "%s", bufferData);
  }
}
