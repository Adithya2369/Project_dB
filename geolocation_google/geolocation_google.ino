//this code is correct but will not give the desired output, because the billing is not enabled in the google cloud console

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

char myssid[] = "adithya";         // your network SSID (name)
char mypass[] = "00000000";         // your network password

//Credentials for Google GeoLocation API...
const char* Host = "www.googleapis.com";
String thisPage = "/geolocation/v1/geolocate?key=";
String key = "AIzaSyC8krAosapmRizFAvxiqjH5KfDYa8lDHz0";

String jsonString = "{\n";

double latitude = 0.0;
double longitude = 0.0;
double accuracy = 0.0;
int more_text = 1; // set to 1 for more debug output

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Start");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Connecting to ");
  Serial.println(myssid);
  WiFi.begin(myssid, mypass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");

  if (n == 0) {
    Serial.println("no networks found");
    return;
  }

  Serial.print(n);
  Serial.println(" networks found...");

  if (more_text) {
    Serial.println("{");
    Serial.println("\"homeMobileCountryCode\": 234,");
    Serial.println("\"homeMobileNetworkCode\": 27,");
    Serial.println("\"radioType\": \"gsm\",");
    Serial.println("\"carrier\": \"Vodafone\",");
    Serial.println("\"wifiAccessPoints\": [");

    for (int i = 0; i < n; ++i) {
      Serial.println("{");
      Serial.print("\"macAddress\" : \"");
      Serial.print(WiFi.BSSIDstr(i));
      Serial.println("\",");
      Serial.print("\"signalStrength\": ");
      Serial.println(WiFi.RSSI(i));
      Serial.println(i < n - 1 ? "}," : "}");
    }
    Serial.println("]");
    Serial.println("}");
  }

  jsonString = "{\n";
  jsonString += "\"homeMobileCountryCode\": 234,\n";
  jsonString += "\"homeMobileNetworkCode\": 27,\n";
  jsonString += "\"radioType\": \"gsm\",\n";
  jsonString += "\"carrier\": \"Vodafone\",\n";
  jsonString += "\"wifiAccessPoints\": [\n";

  for (int j = 0; j < n; ++j) {
    jsonString += "{\n";
    jsonString += "\"macAddress\" : \"" + WiFi.BSSIDstr(j) + "\",\n";
    jsonString += "\"signalStrength\": " + String(WiFi.RSSI(j)) + "\n";
    jsonString += (j < n - 1) ? "},\n" : "}\n";
  }

  jsonString += "]\n";
  jsonString += "}\n";

  WiFiClientSecure client;
  client.setInsecure(); // Bypass SSL certificate validation

  Serial.print("Requesting URL: ");
  Serial.println("https://" + String(Host) + thisPage + key);

  if (client.connect(Host, 443)) {
    Serial.println("Connected");

    client.println("POST " + thisPage + key + " HTTP/1.1");
    client.println("Host: " + String(Host));
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("User-Agent: ESP32");
    client.print("Content-Length: ");
    client.println(jsonString.length());
    client.println();
    client.print(jsonString);

    delay(500);

    String response;
    while (client.available()) {
      response += client.readStringUntil('\n');
    }

    if (more_text) {
      Serial.println(response);
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      latitude = doc["location"]["lat"] | 0.0;
      longitude = doc["location"]["lng"] | 0.0;
      accuracy = doc["accuracy"] | 0.0;
    } else {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.println("Connection failed.");
  }

  client.stop();
  Serial.println("closing connection");

  Serial.print("Latitude = ");
  Serial.println(latitude, 6);
  Serial.print("Longitude = ");
  Serial.println(longitude, 6);
  Serial.print("Accuracy = ");
  Serial.println(accuracy);

  delay(60000); // Run every 60 seconds
}
