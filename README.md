<b>How to run a program: </b>
- Open Arduino IDE
- Perpare IDE for ESP32 development (set Board and Port)
- Create secrets.ino file
- Paste
  ```//----------------------------Configuration
  /* WiFi network name and password */
  const char* ssid = ""; 
  const char* pwd = "";
  
  // IP address to send UDP data to (address of the server).
  const char* udpAddress = ""; // your server (PC) IPv4
  const int udpPort = 8080; // server port```
- Click upload button
