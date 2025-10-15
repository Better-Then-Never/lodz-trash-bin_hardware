#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secrets.h"

#define LED_PIN D1

void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

String uid;
String databasePath;

bool ledState = false;                    // текущее состояние LED
unsigned long lastCheck = 0;              // время последней проверки
const unsigned long checkInterval = 3000; // 3 секунды

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
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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
    databasePath = "/BinsData/" + uid + "/led_status";
    Serial.println("Database path: " + databasePath);
  }

  // Проверяем каждые 3 секунды
  if (millis() - lastCheck > checkInterval && app.ready())
  {
    lastCheck = millis();
    Database.get(aClient, databasePath, processData);
  }

  // Управление через Serial (как раньше)
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();

    if (command == "LED:ON" || command == "ON")
    {
      digitalWrite(LED_PIN, HIGH);
      ledState = true;
      Database.set<String>(aClient, databasePath, "true", processData);
      Serial.println("LED turned ON");
    }
    else if (command == "LED:OFF" || command == "OFF")
    {
      digitalWrite(LED_PIN, LOW);
      ledState = false;
      Database.set<String>(aClient, databasePath, "false", processData);
      Serial.println("LED turned OFF");
    }
    else
    {
      Serial.println("Commands: LED:ON | LED:OFF | ON | OFF");
    }
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

    bool newLedState = (value == "true" || value == "1");

    if (newLedState != ledState)
    {
      ledState = newLedState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.println(ledState ? "LED turned ON (from Firebase)" : "LED turned OFF (from Firebase)");
    }
  }
}
