#include <WiFi.h>
#include <WiFiUdp.h>

//----------------------------ProgramLogic
#define nBuffer 50
#define SOUND_SPEED 0.0343 // cm/s
#define TRIG_PIN = 5;
#define ECHO_PIN = 18;
extern const char * ssid; 
extern const char * pwd;
extern const char *udpAddress;
extern const int udpPort;
WiFiUDP udp;
long i = 1;
char iString[nBuffer] = "hello world";
char bufferData[nBuffer] = "hello world";
double distanceMeasured = 0;
SemaphoreHandle_t semaphore;
TaskHandle_t distanceMeasureTaskHandle = NULL;

void setup(){
  Serial.begin(115200);

  semaphore = xSemaphoreCreateMutex();
  if (!semaphore){
    Serial.println("Mutex creation failed");
    while(1);
  }

  setUpPins();

  xTaskCreate(distanceMeasureTask, "Distance Measure Task", 10000, NULL, 1, &distanceMeasureTaskHandle);

  connectToWifi();

  udp.begin(udpPort);
}

void loop(){
  preparePacketForServer();

  receivePacketFromServer();
  
  delay(900);
}

void setUpPins(){
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void distanceMeasureTask(void *params){
  while(true){
    double signalSendTime = 0;
    double signalReceivedTime = 0;

    if (xSemaphoreTake(semaphore, portMAX_DELAY)){
      distanceMeasured = (signalReceivedTime - signalSendTime) * SOUND_SPEED / 2.0;
      Serial.print("Distance Measure Task: distance: ");
      Serial.println(distanceMeasured);
    }
    xSemaphoreGive(semaphore);


    delay(501);
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
