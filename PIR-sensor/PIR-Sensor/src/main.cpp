#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>


#define LED_PIN         2
#define ADCRefVoltage   1.126
#define R1              3000000
#define R2              887000
#define R1_Cell         3000000
#define R2_Cell         681000

uint8_t broadcastAddress[] = {0x98, 0x3D, 0xAE, 0x49, 0x47, 0xC0};

float readBattVoltage();
float readCellVoltage();
bool SendMessage();

RTC_DATA_ATTR bool isDark = 0;
RTC_DATA_ATTR bool isDarkOLD = 0;
RTC_DATA_ATTR int DarkCNTR = 0;
RTC_DATA_ATTR int SendCNTR = 0;

void setup() 
{
    // delay(250);

    // Serial1.begin(115200,SERIAL_8N1,20,21);

    analogSetPinAttenuation(0,ADC_2_5db);
    analogSetPinAttenuation(1,ADC_2_5db);
    pinMode(LED_PIN, OUTPUT);

    pinMode(3, OUTPUT);
    pinMode(4, INPUT);
    pinMode(5, OUTPUT);
    gpio_hold_dis(GPIO_NUM_5);
    digitalWrite(5,LOW);
    gpio_hold_en(GPIO_NUM_5);

    gpio_deep_sleep_hold_en();
    
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

    if (wakeupCause == ESP_SLEEP_WAKEUP_TIMER) 
    {
        // Serial1.print("Entering ESP_SLEEP_WAKUP_TIMER\n");
        if(digitalRead(LED_PIN))
        {
            gpio_hold_dis(GPIO_NUM_2);
            digitalWrite(LED_PIN, LOW);
            gpio_hold_en(GPIO_NUM_2);
            esp_sleep_enable_timer_wakeup(9950000); // 9.95 Sekunden in Mikrosekunden
            if(DarkCNTR)
                DarkCNTR--;
            if(SendCNTR)
            {
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
                SendCNTR--;
            }
            else if(isDark)
                esp_deep_sleep_enable_gpio_wakeup(1 << 4,ESP_GPIO_WAKEUP_GPIO_HIGH);

                
            // Serial1.printf("LED off, SendCNTR = %u\n",SendCNTR);
        }
        else
        {
            gpio_hold_dis(GPIO_NUM_2);
            digitalWrite(LED_PIN, HIGH);
            gpio_hold_en(GPIO_NUM_2);
            esp_sleep_enable_timer_wakeup(50000); // 50 mSekunden in Mikrosekunden
            // Serial1.print("LED on\n");
        }
        if(DarkCNTR == 0)
        {
            float CellVoltage = readCellVoltage();

            // Serial1.printf("Check if dark, CellVoltage = %.2f\n",CellVoltage);

            if(CellVoltage < 0.1)
                isDark = true;
            else 
                isDark = false;

            DarkCNTR = 60;
    
            if(isDark != isDarkOLD)
            {    
                isDarkOLD = isDark;
                if(isDark)
                {
                    // Serial1.print("its dark\n");
                    gpio_deep_sleep_hold_dis();
                    gpio_hold_dis(GPIO_NUM_3);
                    digitalWrite(3,HIGH);
                    gpio_hold_en(GPIO_NUM_3);
                    gpio_deep_sleep_hold_en();
                }
                else
                {
                    // Serial1.print("its bright\n");
                    gpio_deep_sleep_hold_dis();
                    gpio_hold_dis(GPIO_NUM_3);
                    digitalWrite(3,LOW);
                    gpio_hold_en(GPIO_NUM_3);
                    gpio_deep_sleep_hold_en();
                }
            }
        } 
        if(isDark && digitalRead(4) && !SendCNTR)
        {
            if(!SendMessage())
                DarkCNTR = 0;
        }
        esp_deep_sleep_start();
    }
    else if(wakeupCause == ESP_SLEEP_WAKEUP_GPIO)
    {
        // Serial1.print("Ext-Wakueup\n");
        SendMessage();
        esp_sleep_enable_timer_wakeup(50000); // 50 mSekunden in Mikrosekunden
        esp_deep_sleep_start();
    }
    else
    {
        esp_sleep_enable_timer_wakeup(1);
        esp_deep_sleep_start();
    }
}

void loop() 
{
}

bool SendMessage()
{
    if(isDark)
    {
        if(!SendCNTR)
        {
            WiFi.mode(WIFI_STA);
            if (esp_now_init() != ESP_OK) 
            {
                // Serial1.print("error init ESPNow\n");
                SendCNTR = 1;
                WiFi.mode(WIFI_OFF);
                return 1;
            }

            esp_now_peer_info_t peerInfo;
            memcpy(peerInfo.peer_addr, broadcastAddress, 6);
            peerInfo.channel = 0; 
            peerInfo.encrypt = false;
            if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                // Serial1.print("error adding peer\n");
                SendCNTR = 1;
                WiFi.mode(WIFI_OFF);
                return 1;
            }
            char data[50];
            sprintf(data, "Hoflicht an, BattVoltage: %.2f", readBattVoltage());

            esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)data, strlen(data));
            if (result == ESP_OK) {
                // Serial1.print("message sent\n");
                SendCNTR = 5;
            } else {
                // Serial1.print("error sending\n");
                SendCNTR = 1;
            }
            WiFi.mode(WIFI_OFF);
            return 1;
        }
        else
            return 1;
    }
    else
        return 1;
}
float readBattVoltage()
{
    uint32_t buf = 0;

    for (uint8_t i = 0; i < 100; i++)
    {
            buf += analogRead(0);
    }
    buf /=100;

    float Volatgeraw = buf * ADCRefVoltage / 4095;
    float BattVoltage = (Volatgeraw * (R1+R2))/ R2;
    
    // Serial.printf("Voltage: %.2f V\n",BattVoltage);
    return BattVoltage;
}
float readCellVoltage()
{
    uint32_t buf = 0;

    for (uint8_t i = 0; i < 100; i++)
    {
            buf += analogRead(1);
    }
    buf /=100;

    float Volatgeraw = buf * ADCRefVoltage / 4095;
    float Voltage = (Volatgeraw * (R1_Cell+R2_Cell))/ R2_Cell;
    
    // Serial.printf("Voltage: %.2f V\n",BattVoltage);
    return Voltage;
}
