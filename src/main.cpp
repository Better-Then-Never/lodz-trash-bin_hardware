#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secrets.h"

void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

String uid;
String databasePath;

bool isSessionStarted = false;
unsigned long lastCheck = 0;
const unsigned long checkInterval = 3000;

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

  initWiFi();

  ssl_client.setInsecure();
  ssl_client.setTimeout(1000);
  ssl_client.setBufferSizes(4096, 1024);

  initializeApp(aClient, app, getAuth(user_auth), processData);
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop()
{
  app.loop();
  Database.loop();

  if (app.ready() && uid.isEmpty())
  {
    uid = app.getUid().c_str();
    databasePath = "/BinsData/" + uid + "/isSessionStarted";
    Serial.println("Database path: " + databasePath);
  }

  if (millis() - lastCheck > checkInterval && app.ready())
  {
    lastCheck = millis();
    Database.get(aClient, databasePath, processData);
  }
}

void processData(AsyncResult &aResult)
{
  if (aResult.isError())
  {
    Serial.printf("Error: %s\n", aResult.error().message().c_str());
    return;
  }

  if (aResult.available())
  {
    RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
    String value = RTDB.to<String>();
    value.trim();
    Serial.println("Firebase value: " + value);

    bool newIsSessionStarted = (value == "true" || value == "1");

    if (newIsSessionStarted != isSessionStarted)
    {
      isSessionStarted = newIsSessionStarted;
      Serial.println("Session State Changed");
    }
  }
}
