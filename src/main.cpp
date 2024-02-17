#include <Arduino.h>
#include <Arduino_MKRGPS.h>
#include <Arduino_PMIC.h>
#include <MKRWAN.h>

#include "secrets.h"

LoRaModem modem;

static void sendLora();
static void readLora();
static void login();
static void packData(byte *buffer, size_t bufSize, uint8_t satellites, float lat,
                     float lng, int16_t altitude, uint8_t availCount);

/// @brief update interval in seconds
const int UPDATE_INTERVAL = (60 * 60);

void setup()
{
    SERIAL_PORT_HARDWARE_OPEN.begin(9600);

    // SERIAL_PORT_HARDWARE_OPEN.write("\r\nPress key to continue\r\n");
    // while (!SERIAL_PORT_HARDWARE_OPEN.available())
    // {
    //     delay(100);
    // }

    if (!PMIC.begin())
    {
        Serial.println("Failed to initialize PMIC!");
        while (1)
            ;
    }

    // Set the input current limit to 2 A and the overload input voltage to 3.88 V
    if (!PMIC.setInputCurrentLimit(2.0))
    {
        Serial.println("Error in set input current limit");
    }

    if (!PMIC.setInputVoltageLimit(3.88))
    {
        Serial.println("Error in set input voltage limit");
    }

    // set the minimum voltage used to feeding the module embed on Board
    if (!PMIC.setMinimumSystemVoltage(3.5))
    {
        Serial.println("Error in set minimum system volage");
    }

    // Set the desired charge voltage to 4.11 V
    if (!PMIC.setChargeVoltage(4.2))
    {
        Serial.println("Error in set charge volage");
    }

    // Set the charge current to 375 mA
    // the charge current should be defined as maximum at (C for hour)/2h
    // to avoid battery explosion (for example for a 750 mAh battery set to 0.375 A)
    if (!PMIC.setChargeCurrent(0.375))
    {
        Serial.println("Error in set charge current");
    }

    SERIAL_PORT_HARDWARE_OPEN.print("Charge status: ");
    SERIAL_PORT_HARDWARE_OPEN.println(PMIC.chargeStatus());

    if (!PMIC.enableCharge())
    {
        Serial.println(F("Failed to enable charging"));
    }

    if (!PMIC.canRunOnBattery())
    {
        SERIAL_PORT_HARDWARE_OPEN.println("Can run on battery");
    }
    PMIC.end();
    SERIAL_PORT_HARDWARE_OPEN.println("Initialization done!");

    SERIAL_PORT_HARDWARE_OPEN.write("Starting echo:\r\n");

    // If you are using the MKR GPS as shield, change the next line to pass
    // the GPS_MODE_SHIELD parameter to the GPS.begin(...)
    if (!GPS.begin())
    {
        SERIAL_PORT_HARDWARE_OPEN.println("Failed to initialize GPS!");
        while (1)
            ;
    }

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
}

void loop()
{
    // put your main code here, to run repeatedly:

    if (SERIAL_PORT_HARDWARE_OPEN.available())
    {
        String s = SERIAL_PORT_HARDWARE_OPEN.readString();

        SERIAL_PORT_HARDWARE_OPEN.write(s.c_str());
        SERIAL_PORT_HARDWARE_OPEN.write("\r\n");
    }

    if (GPS.available())
    {
        // read GPS values
        float latitude = GPS.latitude();
        float longitude = GPS.longitude();
        float altitude = GPS.altitude();
        int satellites = GPS.satellites();

        // print GPS values
        Serial.println();
        Serial.print("Location: ");
        Serial.print(latitude, 7);
        Serial.print(", ");
        Serial.println(longitude, 7);

        Serial.print("Altitude: ");
        Serial.print(altitude);
        Serial.println("m");

        Serial.print("Number of satellites: ");
        Serial.println(satellites);

        Serial.println();

        sendLora();

        readLora();
    }
}

static void login()
{
    static bool login = true;

    if (login)
    {
        // Join procedure to the network server
        // OTAA method need appEUI and appKey, this information is provided by the network server
        int connected = modem.joinOTAA(SECRET_APP_EUI, SECRET_APP_KEY);
        if (!connected)
        {
            Serial.println("- Something went wrong; are you indoor? Move near a window and retry...");
        }
        login = false;
    }
}

static void sendLora()
{
    static uint32_t last_sent = 0;
    uint32_t now = millis();

    if ((now - last_sent) > (UPDATE_INTERVAL * 1000))
    {
        // Every hour

        // try to login into TTN
        login();

        // pack and send data
        {
            int err;
            byte buffer[15];
            packData(buffer, sizeof(buffer), GPS.satellites(), GPS.latitude(),
                     GPS.longitude(), GPS.altitude(), 0);

            modem.setPort(2);
            modem.beginPacket();
            modem.write(buffer, sizeof(buffer));
            err = modem.endPacket(false);
            if (err > 0)
            {
                Serial.println("Message sent successfully");
            }
            else
            {
                SERIAL_PORT_HARDWARE_OPEN.println("Sending failed: " + err);
            }
        }

        last_sent = now;
    }
}

static void readLora()
{
    char rcv[64];
    unsigned int i = 0;

    if (!modem.available())
    {
        Serial.println("No downlink message received at this time.");
        return;
    }
    while (modem.available())
    {

        Serial.print("Received: ");
        for (unsigned int j = 0; j < i; j++)
        {
            Serial.print(rcv[j] >> 4, HEX);
            Serial.print(rcv[j] & 0xF, HEX);
            Serial.print(" ");
        }
        Serial.println();
        rcv[i++] = (char)modem.read();
    }
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
