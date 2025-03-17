#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define LED_BLUE      4
#define LED_RED       5
#define LED_GREEN     6
#define TRIAC         7
#define LEDonTime     50
#define LEDoffTime    4950
#define TouchIn       19
#define TouchInDelay  2000
#define TouchonTime   1800000

#define TRIACIntervall  60000

float RemoteBattVoltage;
bool LightOnRecieved = false;

uint8_t broadcastAddress[] = {0x98, 0x3D, 0xAE, 0x49, 0x47, 0xC0};


void processMessage(char * data) {
  char *voltageStr = strstr(data, "BattVoltage:");
  if (voltageStr != NULL) {
    float RemoteBattVoltage = atof(voltageStr + 12);  
    if (strstr(data, "Hoflicht an")) {
      LightOnRecieved = true;
    } else 
    {
    }
  } else 
  {
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  char * data = (char *)incomingData;
  processMessage(data);
}

void BlinkLED(bool on)
{
  if(RemoteBattVoltage < 1.0)
  {
    if(on || digitalRead(TRIAC))
      digitalWrite(LED_GREEN,HIGH);
    else
      digitalWrite(LED_GREEN,LOW);
  }
  else if(RemoteBattVoltage < 3.5)
  {
    if(on || digitalRead(TRIAC))
      digitalWrite(LED_RED,HIGH);
    else
      digitalWrite(LED_RED,LOW);
  }
  else if(RemoteBattVoltage < 3.9)
    {
    if(on || digitalRead(TRIAC))
      digitalWrite(LED_GREEN,HIGH);
    else
      digitalWrite(LED_GREEN,LOW);
  }
  else 
  {
    if(on || digitalRead(TRIAC))
      digitalWrite(LED_BLUE,HIGH);
    else
      digitalWrite(LED_BLUE,LOW);
  }
}

void setup() 
{
    // delay(250);

    Serial1.begin(115200,SERIAL_8N1,20,21);


    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(TRIAC, OUTPUT);
    pinMode(TouchIn,INPUT_PULLDOWN);
   
    digitalWrite(LED_BLUE,LOW);
    digitalWrite(LED_RED,LOW);
    digitalWrite(LED_GREEN,LOW);
    digitalWrite(TRIAC,LOW);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial1.println("Error initializing ESP-NOW");
        return;
    }

     // Callback-Funktion registrieren
    esp_now_register_recv_cb(OnDataRecv);
    
}

void loop() 
{
  static int triacState = 0;
  static uint32_t TriacMillis;
  static int LEDState = 0;
  static uint32_t LEDMillis;
  static int touchState = 0;
  static uint32_t touchMillis;
  static bool Dauerlicht = 0;

  switch (triacState)
  {
    case 0:
      if(LightOnRecieved)
      {
        LightOnRecieved = 0;
        triacState ++;
        TriacMillis = millis();
      }
      break;

    case 1:
      digitalWrite(TRIAC,HIGH);
      triacState++;
      break;

    case 2:
      if(millis() - TriacMillis >= TRIACIntervall)
      {
        if(!LightOnRecieved && !Dauerlicht)
          digitalWrite(TRIAC,LOW);
        triacState = 0;
      }
      break;
    
    default:
      triacState = 0;
      break;
  }
  switch (LEDState)
  {
  case 0:
    BlinkLED(1);
    LEDMillis = millis();
    LEDState ++;
    break;

  case 1:
    if(millis() - LEDMillis >= LEDonTime)
      LEDState ++;
    break;

  case 2:
    BlinkLED(0);
    LEDMillis = millis();
    LEDState ++;
    break;

  case 3:
    if(millis() - LEDMillis >= LEDoffTime)
      LEDState= 0;
    break;

  default:
    LEDState= 0;
    break;
  }
  switch (touchState)
  {
  case 0:
    if(digitalRead(TouchIn))
    {
      touchState ++;
      touchMillis = millis();
    }
    break;

  case 1:
    if(millis() - touchMillis >= TouchInDelay)
    {
      if(digitalRead(TouchIn))
        touchState ++;
      else
        touchState = 0;
    }
    break;

  case 2:
    Dauerlicht = 1;
    digitalWrite(TRIAC,HIGH);
    touchState ++;
    touchMillis = millis();
    while (digitalRead(TouchIn))
    {
      
    }
    
    break;

  case 3:
    if(digitalRead(TouchIn))
      touchState++;
    if(millis() - touchMillis >= TouchonTime)
      touchState++;
    break;

  case 4:
    Dauerlicht = 0;
    digitalWrite(TRIAC,LOW);
    touchState = 0;
    break;
  
  default:
    touchState = 0;
    break;
  }
}

