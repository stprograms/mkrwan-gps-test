#include <Arduino.h>
#include <Arduino_MKRGPS.h>
#include <Arduino_PMIC.h>
#include <ArduinoLowPower.h>

#include "../lib/LoraWanSender/include/LoraWanSender.hpp"
#include "secrets.h"

LoRaModem modem;
LoraWanSender *sender;

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
        Serial.println("Error in set charge voltage");
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

    sender = new LoraWanSender(SECRET_APP_EUI, SECRET_APP_KEY);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
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

        sender->setGps(GPS.latitude(), GPS.longitude(), GPS.satellites(), GPS.altitude());

        sender->send();
        sender->read();

        // wait for 1 hour
        digitalWrite(LED_BUILTIN, LOW);
        LowPower.deepSleep(/*UPDATE_INTERVAL*/ 5 * 
        60 * 1000);
        digitalWrite(LED_BUILTIN, HIGH);
    }
}
