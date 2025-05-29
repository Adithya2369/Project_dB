#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// WiFi credentials
char myssid[] = "adithya";
char mypass[] = "00000000";

// Google Geolocation API
const char* googleHost = "www.googleapis.com";
String googleAPIKey = "AIzaSyC8krAosapmRizFAvxiqjH5KfDYa8lDHz0";
String googleEndpoint = "/geolocation/v1/geolocate?key=" + googleAPIKey;

// Flask server IP
String serverIP = "192.168.223.245";

double latitude = 0.0;
double longitude = 0.0;
double accuracy = 99999.0;
String state_district = "Unknown";

// ---------------------------------------------------------------------------------
// Setup and loop

void setup() {
  Serial.begin(115200);
  delay(1000);

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
  if (get_coordinates()) {
    get_location();
    get_db();
  }
  delay(60000);  // Run every 60 seconds
}

// ---------------------------------------------------------------------------------
// Function to get coordinates using Google Geolocation API

bool get_coordinates() {
  int attempts = 0;
  while (attempts < 3 && accuracy > 5000.0) {
    attempts++;
    Serial.printf("Attempt %d to get coordinates...\n", attempts);

    // WiFi scan
    int n = WiFi.scanNetworks();
    if (n == 0) {
      Serial.println("No networks found");
      return false;
    }

    // Build JSON payload
    String jsonString = "{\n";
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

    // Send POST request
    WiFiClientSecure googleClient;
    googleClient.setInsecure(); // No cert validation

    if (!googleClient.connect(googleHost, 443)) {
      Serial.println("Connection to Google API failed.");
      continue;
    }

    googleClient.println("POST " + googleEndpoint + " HTTP/1.1");
    googleClient.println("Host: " + String(googleHost));
    googleClient.println("Connection: close");
    googleClient.println("Content-Type: application/json");
    googleClient.println("User-Agent: ESP32");
    googleClient.print("Content-Length: ");
    googleClient.println(jsonString.length());
    googleClient.println();
    googleClient.print(jsonString);

    // Read response
    while (googleClient.connected()) {
      String line = googleClient.readStringUntil('\n');
      if (line == "\r") break;
    }

    String body;
    while (googleClient.available()) {
      body += googleClient.readString();
    }
    googleClient.stop();

    int jsonStart = body.indexOf('{');
    if (jsonStart == -1) {
      Serial.println("No JSON in response.");
      continue;
    }

    String jsonBody = body.substring(jsonStart);
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, jsonBody);
    if (error) {
      Serial.print("Parse error: ");
      Serial.println(error.c_str());
      continue;
    }

    latitude = doc["location"]["lat"].as<double>();
    longitude = doc["location"]["lng"].as<double>();
    accuracy = doc["accuracy"].as<double>();

    Serial.printf("Lat: %.7f, Lon: %.7f, Accuracy: %.2f\n", latitude, longitude, accuracy);
  }

  return accuracy < 5000.0;
}

// ---------------------------------------------------------------------------------
// Function to get location (state_district) using Nominatim

void get_location() {
  HTTPClient http;
  String url = "https://nominatim.openstreetmap.org/reverse?format=json&lat=" +
               String(latitude, 7) + "&lon=" + String(longitude, 7) +
               "&zoom=10&addressdetails=1";

  http.begin(url);
  http.setUserAgent("ESP32"); // Nominatim requires User-Agent

  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      state_district = doc["address"]["state_district"] | "Unknown";
    } else {
      Serial.println("Error parsing Nominatim JSON.");
    }
  } else {
    Serial.println("Failed to fetch location info.");
  }

  http.end();

  Serial.print("state_district: ");
  Serial.println(state_district);
}

// ---------------------------------------------------------------------------------
// Function to get allowed dB from Flask server

void get_db() {
  HTTPClient http;

  String query = state_district;
  query.toLowerCase();
  query.replace(" ", "%20");

  String url = "http://" + serverIP + ":5000/info?location=" + query;

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      int allowedDB = doc["allowed_dB"];
      Serial.printf("Allowed dB at %s: %d\n", state_district.c_str(), allowedDB);
    } else {
      Serial.println("Error parsing Flask response.");
    }
  } else {
    Serial.println("Failed to contact Flask server.");
  }

  http.end();
}
