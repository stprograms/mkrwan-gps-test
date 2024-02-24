#include "../include/LoraWanSender.hpp"

static void packData(byte *buffer, size_t bufSize, uint8_t satellites, float lat,
                     float lng, int16_t altitude, uint8_t availCount);

LoraWanSender::LoraWanSender(String app_eui, String app_key)
{
    // LoRa module initialization
    // Initialize the modem with your regional band (eg. US915, AS923,...)
    if (!modem.begin(EU868))
    {
        Serial.println("- Failed to start module");
        while (true)
            delay(100);
    };

    // With the module initialized, we can get its version and device EUI
    // Your network server provider requires device EUI information to connect to their network
    Serial.print("- Your module version is: ");
    Serial.println(modem.version());
    Serial.print("- Your device EUI is: ");
    Serial.println(modem.deviceEUI());

    this->app_eui = app_eui;
    this->app_key = app_key;
}

void LoraWanSender::login()
{
    if (!this->logged_in)
    {
        // Join procedure to the network server
        // OTAA method need appEUI and appKey, this information is provided by the network server
        int connected = this->modem.joinOTAA(this->app_eui, this->app_key);
        if (!connected)
        {
            Serial.println("- Something went wrong; are you indoor? Move near a window and retry...");
        }
        else
        {
            this->logged_in = true;
        }
    }
}

void LoraWanSender::send()
{
    // try to login into TTN
    this->login();

    // pack and send data
    {
        int err;
        byte buffer[15];
        packData(buffer,
                 sizeof(buffer),
                 this->satellites,
                 this->latitude,
                 this->longitude,
                 this->altitude,
                 0);

        this->modem.setPort(2);
        this->modem.beginPacket();
        this->modem.write(buffer, sizeof(buffer));
        err = this->modem.endPacket(false);
        if (err > 0)
        {
            Serial.println("Message sent successfully");
        }
        else
        {
            SERIAL_PORT_HARDWARE_OPEN.println("Sending failed: " + err);
        }
    }
}

void LoraWanSender::read()
{
    char rcv[64];
    unsigned int i = 0;

    if (!this->modem.available())
    {
        Serial.println("No downlink message received at this time.");
        return;
    }
    while (this->modem.available())
    {

        Serial.print("Received: ");
        for (unsigned int j = 0; j < i; j++)
        {
            Serial.print(rcv[j] >> 4, HEX);
            Serial.print(rcv[j] & 0xF, HEX);
            Serial.print(" ");
        }
        Serial.println();
        rcv[i++] = (char)this->modem.read();
    }
}

void LoraWanSender::setGps(float latitude, float longitude, uint8_t satellites, int16_t altitude)
{
    this->latitude = latitude;
    this->longitude = longitude;
    this->satellites = satellites;
    this->altitude = altitude;
}

static void packData(byte *buffer, size_t bufSize, uint8_t satellites, float lat,
                     float lng, int16_t altitude, uint8_t availCount)
{
    uint16_t chargingCycles = 106;

    if (bufSize < 15)
    {
        Serial.println("Invalid buffer size");
        return;
    }

    // Build package for sending. Currently dummy data
    // 0: Number of satellites
    // 1-4: Latitude (float32)
    // 5-8: Longitude (float32)
    // 9-10: altitude (int16)
    // 11: Sysstatus (uint8)
    // 12: SoC (uint8)
    // 13: temp (int8)
    // 14-15: Charging cycles (uint16)

    buffer[0] = satellites;

    memcpy(buffer + 1, &lat, sizeof(lat));
    memcpy(buffer + 5, &lng, sizeof(lng));
    memcpy(buffer + 9, &altitude, sizeof(altitude));
    buffer[11] = (availCount << 1) | 0x01; // Status build availCount + gpsinit
    buffer[12] = 97;                       // 97% Soc
    buffer[13] = 12;                       // 12 °C
    memcpy(buffer + 14, &chargingCycles, sizeof(chargingCycles));
}
