#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <MFRC522.h>

//Configure your wifi connection
const char* ssid = "your_network_ssid";
const char* password = "your_network_password";
String base_url = "http://your_ip_address:your_port"; //ex: 192.168.1.5:12345

//Customize how quickly to send the next read, 3 seconds (in milliseconds) seems like a good sweet spot.
unsigned long timeout = 3000;

unsigned long now = 0;
unsigned long last = 0;

ESP8266WebServer server(80);
HTTPClient client;
String url_send;

#define RST_PIN         0          // Configurable, see typical pin layout above
#define SS_PIN          15         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

String getMac() {
  unsigned char macarr[6];
  WiFi.macAddress(macarr);
  
  String mac = String(macarr[0], HEX); mac += ":";
  mac += String(macarr[1], HEX); mac += ":";
  mac += String(macarr[2], HEX); mac += ":";
  mac += String(macarr[3], HEX); mac += ":";
  mac += String(macarr[4], HEX); mac += ":";
  mac += String(macarr[5], HEX);
  return mac;
}

String getIP() {
  IPAddress ip = WiFi.localIP();
  String ret = (String)ip[0] + "." + (String)ip[1] + "." + (String)ip[2] + "." + (String)ip[3];
  return ret;
}

void handleRoot() {
  String to_send = "Mac: " + getMac() + "<br />\
  IP: " + getIP();

  server.send(200, "text/html", to_send);
}

void handleNotFound(){
  server.send(404, "text/html", "Method not found.");
}

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	delay(4);				// Optional delay. Some board do need more time after init to be ready
  Serial.println();
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  //Show connection info in the serial console
  Serial.println();
  Serial.println("Wifi connected.");
  Serial.println(getIP());
  Serial.println(getMac());

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);
  server.begin();
}

bool sendAction(String url) {
  now = millis();
  if((now - last) > timeout) {
    last = millis();
    Serial.println("Sending " + url);
    client.begin(url);
    client.GET();
    client.end();
    return true;
  }
  return false;
}

String getCardID(){
  String uid= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
     uid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uid.toUpperCase();
  return uid;
}

void loop() {
  
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

    String struid = getCardID();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    sendAction(base_url + "/?accessoryId=" + struid + "&buttonName=" + struid + "&event=0");
    Serial.println(struid);
	}
}