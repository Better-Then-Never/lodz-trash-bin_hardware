#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secrets.h"

#define SOUND_VELOCITY 0.034
#define trigPin 12
#define echoPin 14
#define checkInterval 3000
#define timeBeforeSessionEnds 20000
#define sensorCheckInterval 500
#define distanceToBinWall 11

void processData(AsyncResult &aResult);
void noopCallback(AsyncResult &aResult);
float getDistance();

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

String uid;
String databasePathSession;

bool isSessionStarted = false;
unsigned long lastCheck = 0;
unsigned long lastSensorCheck = 0;
unsigned long sessionStartTime = 0;
int trashUnitsCounter = 0;
bool wasTrashDetected = false;

void initWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\nWiFi Connected!");
}

void setup()
{
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  initWiFi();

  ssl_client.setInsecure();
  ssl_client.setTimeout(1000);
  ssl_client.setBufferSizes(4096, 1024);

  initializeApp(aClient, app, getAuth(user_auth), processData);
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  // Small startup message
  Serial.println("Smart bin starting...");
}

void loop()
{
  app.loop();
  Database.loop();
  yield();

  if (app.ready() && uid.isEmpty())
  {
    uid = app.getUid().c_str();
    databasePathSession = "/BinsData/" + uid + "/isSessionStarted";
    Serial.println("Database path: " + databasePathSession);
  }

  if (millis() - lastCheck > checkInterval && app.ready())
  {
    lastCheck = millis();
    Database.get(aClient, databasePathSession, processData);
  }

  if (isSessionStarted && (millis() - lastSensorCheck > sensorCheckInterval))
  {
    lastSensorCheck = millis();

    float distanceCm = getDistance();

    Serial.print("Distance: ");
    Serial.println(distanceCm);

    // Detect trash drop
    if (distanceCm > 0 && distanceCm < distanceToBinWall && !wasTrashDetected)
    {
      wasTrashDetected = true;
      trashUnitsCounter++;

      Serial.print("Trash dropped! Count: ");
      Serial.println(trashUnitsCounter);

      // Write counter after increment, with noopCallback so it won't be treated as session update
      String countPath = "/BinsData/" + uid + "/trashUnitsCounter";
      Database.set<int>(aClient, countPath, trashUnitsCounter, noopCallback);
    }
    else if (distanceCm >= distanceToBinWall || distanceCm == 0)
    {
      wasTrashDetected = false;
    }

    // Check for session timeout
    if (millis() - sessionStartTime > timeBeforeSessionEnds)
    {
      Serial.print("Session ended. Total trash units: ");
      Serial.println(trashUnitsCounter);

      String databasePathSession = "/BinsData/" + uid + "/isSessionStarted";
      Database.set<String>(aClient, databasePathSession, "false", processData);

      isSessionStarted = false;
      wasTrashDetected = false;
    }
  }

  // DO NOT reset sessionStartTime here unconditionally.
  // sessionStartTime is set when processData receives a "true" session flag.

  delay(100);
}

float getDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0)
  {
    return 0;
  }

  float distanceCm = duration * SOUND_VELOCITY / 2;

  return distanceCm;
}

void processData(AsyncResult &aResult)
{
  if (aResult.isError())
  {
    Serial.printf("Error: %s\n", aResult.error().message().c_str());
    return;
  }

  if (!aResult.available())
    return;

  RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
  String value = RTDB.to<String>();
  value.trim();

  Serial.println("Firebase value: " + value);

  // Only accept explicit "true"/"false" for session flag (do not treat numeric counters as session updates).
  if (!(value == "true" || value == "false"))
  {
    Serial.println("Not a session-flag update — ignoring.");
    return;
  }

  bool newIsSessionStarted = (value == "true");

  if (newIsSessionStarted != isSessionStarted)
  {
    isSessionStarted = newIsSessionStarted;
    Serial.println(isSessionStarted ? "Session Started" : "Session Ended");
    if (isSessionStarted)
    {
      sessionStartTime = millis();
      trashUnitsCounter = 0;
      wasTrashDetected = false;
    }
    else
    {
      wasTrashDetected = false;
    }
  }
}

void noopCallback(AsyncResult &aResult)
{
  // Used for counter writes so processData isn't triggered by them.
  if (aResult.isError())
  {
    Serial.printf("Counter write error: %s\n", aResult.error().message().c_str());
  }
  else
  {
    // Optional: print confirmation of counter write (comment out if noisy)
    // RealtimeDatabaseResult &res = aResult.to<RealtimeDatabaseResult>();
    // Serial.println("Counter write OK: " + res.to<String>());
  }
}
