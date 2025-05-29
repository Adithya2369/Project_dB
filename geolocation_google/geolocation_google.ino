#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// WiFi credentials
char myssid[] = "adithya";         // your network SSID (name)
char mypass[] = "00000000";        // your network password

// Google Geolocation API credentials
const char* Host = "www.googleapis.com";
String thisPage = "/geolocation/v1/geolocate?key=";
// !! IMPORTANT: Replace with your own API key and keep it secret !!
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

  // Build JSON payload
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

  if (more_text) {
    Serial.println("JSON Payload:");
    Serial.println(jsonString);
  }

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

    // Wait for response
    String response;
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r" || line.length() == 0) {
        break; // Headers ended
      }
    }

    // Read the body (may be chunked)
    String body;
    while (client.available()) {
      body += client.readString();
    }

    // Remove chunked encoding if present
    int jsonStart = body.indexOf('{');
    if (jsonStart == -1) {
      Serial.println("JSON start marker not found!");
    } else {
      String jsonBody = body.substring(jsonStart);
      if (more_text) {
        Serial.println("Raw JSON response:");
        Serial.println(jsonBody);
      }

      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, jsonBody);

      if (!error) {
        latitude = doc["location"]["lat"].as<double>();
        longitude = doc["location"]["lng"].as<double>();
        accuracy = doc["accuracy"].as<double>();
      } else {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
      }
    }
  } else {
    Serial.println("Connection failed.");
  }

  client.stop();
  Serial.println("closing connection");

  Serial.print("Latitude = ");
  Serial.println(latitude, 7);
  Serial.print("Longitude = ");
  Serial.println(longitude, 7);
  Serial.print("Accuracy = ");
  Serial.println(accuracy, 2);

  delay(60000); // Run every 60 seconds
}
