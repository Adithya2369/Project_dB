#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// WiFi credentials
char myssid[] = "adithya";
char mypass[] = "00000000";

// Google Geolocation API credentials
const char* Host = "www.googleapis.com";
String thisPage = "/geolocation/v1/geolocate?key=";
// !! Replace this with your own API key !!
String key = "AIzaSyC8krAosapmRizFAvxiqjH5KfDYa8lDHz0";

String jsonString = "{\n";
double latitude = 0.0;
double longitude = 0.0;
double accuracy = 0.0;
int more_text = 1;

String serverIP = "192.168.223.245"; // Flask server IP

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
  get_db();
  delay(60000); // Run every 60 seconds
}

// ---------------------------------------------------------------------------------

void get_db() {
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

  // ---- HTTPS Client (Google API) ----
  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // Disable SSL verification

  Serial.print("Requesting URL: ");
  Serial.println("https://" + String(Host) + thisPage + key);

  if (secureClient.connect(Host, 443)) {
    Serial.println("Connected");

    secureClient.println("POST " + thisPage + key + " HTTP/1.1");
    secureClient.println("Host: " + String(Host));
    secureClient.println("Connection: close");
    secureClient.println("Content-Type: application/json");
    secureClient.println("User-Agent: ESP32");
    secureClient.print("Content-Length: ");
    secureClient.println(jsonString.length());
    secureClient.println();
    secureClient.print(jsonString);

    while (secureClient.connected()) {
      String line = secureClient.readStringUntil('\n');
      if (line == "\r" || line.length() == 0) break;
    }

    String body;
    while (secureClient.available()) {
      body += secureClient.readString();
    }

    int jsonStart = body.indexOf('{');
    if (jsonStart != -1) {
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
    } else {
      Serial.println("JSON start marker not found!");
    }
  } else {
    Serial.println("Connection to Google failed.");
  }

  secureClient.stop();
  Serial.println("closing connection");

  Serial.print("Latitude = ");
  Serial.println(latitude, 7);
  Serial.print("Longitude = ");
  Serial.println(longitude, 7);
  Serial.print("Accuracy = ");
  Serial.println(accuracy, 2);

  // ---- HTTPS Client (Nominatim API) ----
  HTTPClient http_nominatim;
  String url_location = "https://nominatim.openstreetmap.org/reverse?format=json&lat=" 
                      + String(latitude, 7) + "&lon=" + String(longitude, 7)
                      + "&zoom=10&addressdetails=1";

  http_nominatim.begin(secureClient, url_location);
  http_nominatim.setUserAgent("ESP32"); // Required by Nominatim
  int httpCode = http_nominatim.GET();

  String state_district = "Unknown";
  if (httpCode > 0) {
    String payload = http_nominatim.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      state_district = doc["address"]["state_district"] | "Unknown";
      Serial.print("state_district: ");
      Serial.println(state_district);
    } else {
      Serial.println("Nominatim JSON parse error");
    }
  } else {
    Serial.println("Nominatim request failed");
  }
  http_nominatim.end();

  // ---- HTTP Client (Flask Server) ----
  WiFiClient plainClient;
  HTTPClient http_flask;

  String locationQuery = state_district;
  locationQuery.toLowerCase();
  locationQuery.replace(" ", "%20");
  String url_flask = "http://" + serverIP + ":5000/info?location=" + locationQuery;

  http_flask.begin(plainClient, url_flask);
  httpCode = http_flask.GET();

  if (httpCode > 0) {
    String payload = http_flask.getString();
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      int allowedDB = doc["allowed_dB"];
      Serial.print("Allowed dB at ");
      Serial.print(state_district);
      Serial.print(": ");
      Serial.println(allowedDB);
    } else {
      Serial.println("Failed to parse JSON from Flask");
    }
  } else {
    Serial.println("Failed to contact Flask API");
  }
  http_flask.end();
}
