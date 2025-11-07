#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secrets.h"

#define SOUND_VELOCITY 0.034f
#define trigPin 12
#define echoPin 14
#define checkInterval 250
#define timeBeforeSessionEnds 20000UL
#define sensorCheckInterval 50
#define distanceToBinWall 12

void processData(AsyncResult &aResult);
void noopCallback(AsyncResult &aResult);
float getDistance();

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient *aClient = nullptr;
RealtimeDatabase Database;

String uid;

bool isSessionStarted = false;
unsigned long lastCheck = 0;
unsigned long lastSensorCheck = 0;
unsigned long sessionStartTime = 0;
int trashUnitsCounter = 0;
bool wasTrashDetected = false;

bool pendingInitialCounterWrite = false;

char databasePathSessionBuf[128];
char countPathBuf[128];

unsigned long lastDropTime = 0;
const unsigned long dropDebounceMs = 300UL;

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
  delay(10);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  initWiFi();

  ssl_client.setInsecure();
  ssl_client.setTimeout(1000);
  ssl_client.setBufferSizes(2048, 512);

  aClient = new AsyncClient(ssl_client);

  initializeApp(*aClient, app, getAuth(user_auth), processData);
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  Serial.println("Smart bin starting...");
  Serial.printf("Initial free heap: %u\n", ESP.getFreeHeap());
}

void loop()
{
  app.loop();
  Database.loop();
  yield();

  if (app.ready() && uid.isEmpty())
  {
    uid = app.getUid();
    uid.trim();

    snprintf(databasePathSessionBuf, sizeof(databasePathSessionBuf), "/BinsData/%s/isSessionStarted", uid.c_str());
    snprintf(countPathBuf, sizeof(countPathBuf), "/BinsData/%s/trashUnitsCounter", uid.c_str());

    Serial.print("Database path: ");
    Serial.println(databasePathSessionBuf);
    Serial.printf("Free heap after uid set: %u\n", ESP.getFreeHeap());
  }

  if (millis() - lastCheck > checkInterval && app.ready() && !uid.isEmpty())
  {
    lastCheck = millis();
    Serial.printf("Free heap before Database.get: %u\n", ESP.getFreeHeap());
    Database.get(*aClient, databasePathSessionBuf, processData);
  }

  if (pendingInitialCounterWrite && app.ready() && !uid.isEmpty())
  {
    pendingInitialCounterWrite = false;
    Serial.printf("Performing deferred initial counter write. Free heap: %u\n", ESP.getFreeHeap());
    Database.set<int>(*aClient, countPathBuf, trashUnitsCounter, noopCallback);
  }

  if (isSessionStarted && (millis() - lastSensorCheck > sensorCheckInterval))
  {
    lastSensorCheck = millis();

    float distanceCm = getDistance();
    Serial.print("Distance: ");
    Serial.println(distanceCm);

    if (distanceCm > 0 && distanceCm < distanceToBinWall && !wasTrashDetected)
    {
      wasTrashDetected = true;

      if (millis() - lastDropTime >= dropDebounceMs)
      {
        trashUnitsCounter++;
        lastDropTime = millis();

        sessionStartTime = millis();

        Serial.print("Trash dropped! Count: ");
        Serial.println(trashUnitsCounter);
        Serial.printf("Free heap before counter write: %u\n", ESP.getFreeHeap());

        if (!uid.isEmpty())
        {
          Database.set<int>(*aClient, countPathBuf, trashUnitsCounter, noopCallback);
        }
      }
      else
      {
        Serial.println("Drop ignored due to debounce.");
      }
    }
    else if (distanceCm >= distanceToBinWall || distanceCm == 0)
    {
      wasTrashDetected = false;
    }

    if (millis() - sessionStartTime > timeBeforeSessionEnds)
    {
      Serial.print("Session ended due to inactivity. Total trash units: ");
      Serial.println(trashUnitsCounter);
      Serial.printf("Free heap before session end write: %u\n", ESP.getFreeHeap());

      if (!uid.isEmpty())
      {
        Database.set<String>(*aClient, databasePathSessionBuf, "false", processData);
      }

      isSessionStarted = false;
      wasTrashDetected = false;
    }
  }

  delay(10);
}

float getDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);

  if (duration == 0)
  {
    return 0;
  }

  float distanceCm = duration * SOUND_VELOCITY / 2.0f;
  return distanceCm;
}

void processData(AsyncResult &aResult)
{
  Serial.printf("Entering processData, free heap: %u\n", ESP.getFreeHeap());

  if (aResult.isError())
  {
    Serial.printf("processData: Error: %s\n", aResult.error().message().c_str());
    return;
  }

  if (!aResult.available())
  {
    Serial.println("processData: no data available");
    return;
  }

  RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();

  String value = RTDB.to<String>();
  value.trim();

  Serial.print("Firebase value: ");
  Serial.println(value);

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

      pendingInitialCounterWrite = true;
      lastDropTime = 0;
    }
    else
    {
      wasTrashDetected = false;
    }
  }
}

void noopCallback(AsyncResult &aResult)
{
  if (aResult.isError())
  {
    Serial.printf("noopCallback: Counter write error: %s\n", aResult.error().message().c_str());
  }
}
