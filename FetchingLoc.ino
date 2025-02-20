#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
// Your Wi-Fi credentials
#define WIFI_SSID "UbasCabigon"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Use for Wi-Fi password if required

#define LED_PIN D2
// Your Firebase project credentials
#define FIREBASE_HOST "https://geofencing-2fcd0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "OvB1mf3RlE18X1q4yehMLjbopMQjWgHuv1Cxa6JM"

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Add these global variables at the top of your file, after the Firebase objects
const int MAX_POINTS = 50;  // Maximum number of geofence points
float geofenceLats[MAX_POINTS];
float geofenceLongs[MAX_POINTS];
int totalPoints = 0;

// Setup function
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Turn LED off initially
  Serial.begin(115200);
  delay(1000);

  // Start Wi-Fi connection
  Serial.println("Starting Wi-Fi connection...");
  WiFi.begin(WIFI_SSID);

  // Wi-Fi connection timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 15000) {
      Serial.println("Failed to connect to Wi-Fi.");
      Serial.println("Wi-Fi Status: " + String(WiFi.status()));
      return;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.ready()) {
    Serial.println("Failed to connect to Firebase.");
    Serial.println(firebaseData.errorReason());
    return;
  }
  Serial.println("Connected to Firebase");
}

// Add this function before the loop() function
bool isPointInPolygon(float lat, float lon, float* polyLats, float* polyLongs, int polyPoints) {
  bool inside = false;
  for (int i = 0, j = polyPoints - 1; i < polyPoints; j = i++) {
    if (((polyLats[i] > lat) != (polyLats[j] > lat)) &&
        (lon < (polyLongs[j] - polyLongs[i]) * (lat - polyLats[i]) / 
        (polyLats[j] - polyLats[i]) + polyLongs[i])) {
      inside = !inside;
    }
  }
  return inside;
}

// Modify your loop() function to include the geofence check
void loop() {
  if (Firebase.ready()) {
    float currentLat = 0;
    float currentLong = 0;
    bool locationValid = false;

    // Get current location
    if (Firebase.getFloat(firebaseData, "/UsersCurrentLocation/Latitude")) {
      currentLat = firebaseData.floatData();
      Serial.print("Current Latitude: ");
      Serial.println(currentLat, 7);
      
      if (Firebase.getFloat(firebaseData, "/UsersCurrentLocation/Longitude")) {
        currentLong = firebaseData.floatData();
        Serial.print("Current Longitude: ");
        Serial.println(currentLong, 7);
        locationValid = true;
      }
    }

    // Get geofence points
    Serial.println("\n--- Geofence Coordinates ---");
    int pointIndex = 0;
    bool morePoints = true;

    while (morePoints && pointIndex < MAX_POINTS) {
      String basePath = "/geofence/coordinates/" + String(pointIndex);
      
      if (Firebase.getFloat(firebaseData, basePath + "/latitude")) {
        geofenceLats[pointIndex] = firebaseData.floatData();
        
        if (Firebase.getFloat(firebaseData, basePath + "/longitude")) {
          geofenceLongs[pointIndex] = firebaseData.floatData();
          
          Serial.print("Point ");
          Serial.print(pointIndex);
          Serial.print(": Lat = ");
          Serial.print(geofenceLats[pointIndex], 7);
          Serial.print(", Long = ");
          Serial.println(geofenceLongs[pointIndex], 7);
          
          pointIndex++;
        } else {
          morePoints = false;
        }
      } else {
        morePoints = false;
      }
    }

    totalPoints = pointIndex;
    Serial.print("Total Geofence Points: ");
    Serial.println(totalPoints);

    // Check if user is inside geofence
    if (locationValid && totalPoints >= 3) {  // Need at least 3 points to form a polygon
      bool inside = isPointInPolygon(currentLat, currentLong, geofenceLats, geofenceLongs, totalPoints);
      
      Serial.println("\n=== GEOFENCE STATUS ===");
      if (inside) {
        Serial.println("USER IS INSIDE THE GEOFENCE AREA");
        digitalWrite(LED_PIN, HIGH);
      } else {
        Serial.println("USER IS OUTSIDE THE GEOFENCE AREA");
        digitalWrite(LED_PIN, LOW);
      }
      Serial.println("=====================\n");
    } else {
      Serial.println("\nNot enough points for geofence check or invalid location");
    }

    Serial.println("------------------------\n");
    delay(5000);
  }
}

