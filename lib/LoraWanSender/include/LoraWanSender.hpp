#ifndef _LORA_WAN_SENDER_HPP
#define _LORA_WAN_SENDER_HPP

#include <MKRWAN.h>
#include <String.h>

class LoraWanSender
{
public:
    LoraWanSender(String app_eui, String app_key);

    void send();
    void read();

    void setGps(float latitude, float longitude, uint8_t satellites, int16_t altitude);


private:
    bool logged_in = false;
    LoRaModem modem;

    float latitude;
    float longitude;
    uint8_t satellites;
    int16_t altitude;

    String app_eui;
    String app_key;

    void login();
};

#endif
