#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "secrets.h"

#define SEND_FREQUENCY 500

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const int buttonPin = D3;

void setup()
{
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  Serial.println("Wi-Fi connected, IP: " + WiFi.localIP().toString());
}

void loop()
{
  if (digitalRead(buttonPin) == HIGH)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClient client;
      HTTPClient http;
      http.begin(client, "http://192.168.55.103:5000/signal");
      http.addHeader("Content-Type", "application/json");
      int code = http.POST("{\"btn\":\"pressed\"}");
      Serial.println("HTTP code: " + String(code));
      http.end();
    }
    delay(SEND_FREQUENCY);
  }
}
