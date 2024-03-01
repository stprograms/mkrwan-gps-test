#include <Arduino.h>
#include <Arduino_MKRGPS.h>
#include <Arduino_PMIC.h>
#include <ArduinoLowPower.h>
#include <RTCZero.h>

#include "../lib/LoraWanSender/include/LoraWanSender.hpp"
#include "secrets.h"

enum SystemState
{
    FETCH_DATA,
    UPLOAD_DATA,
    SLEEP
};

SystemState current_state;
LoraWanSender *sender;
uint32_t counter;
uint8_t satellites;

/// @brief update interval in seconds
const int UPDATE_INTERVAL = (60 * 60);

static void configurePMIC();
static void fetchData();
static void uploading();
static void sleeping();
static void gotoFetchData();
static void gotoSleep();
static void gotoUploading();

void setup()
{
    SERIAL_PORT_HARDWARE_OPEN.begin(9600);

    // SERIAL_PORT_HARDWARE_OPEN.write("\r\nPress key to continue\r\n");
    // while (!SERIAL_PORT_HARDWARE_OPEN.available())
    // {
    //     delay(100);
    // }

    configurePMIC();

    SERIAL_PORT_HARDWARE_OPEN.println("Initialization done!");

    // If you are using the MKR GPS as shield, change the next line to pass
    // the GPS_MODE_SHIELD parameter to the GPS.begin(...)
    if (!GPS.begin())
    {
        SERIAL_PORT_HARDWARE_OPEN.println("Failed to initialize GPS!");
        while (1)
            ;
    }
    // configure LED
    pinMode(LED_BUILTIN, OUTPUT);

    sender = new LoraWanSender(SECRET_APP_EUI, SECRET_APP_KEY);
    gotoFetchData();
}

void loop()
{
    switch (current_state)
    {
    case FETCH_DATA:
        fetchData();
        break;

    case UPLOAD_DATA:
        uploading();
        break;

    case SLEEP:
        sleeping();
        break;
    }
}

static void fetchData()
{
    uint32_t now = millis();

    if (now - counter > (5 * 60 * 1000))
    {
        if (satellites != 0)
        {
            gotoUploading();
        }
        else
        {
            // fetch data timeout -> go to sleep
            gotoSleep();
        }
        return;
    }

    if (GPS.available())
    {
        // read GPS values
        float latitude = GPS.latitude();
        float longitude = GPS.longitude();
        float altitude = GPS.altitude();
        satellites = (uint8_t)GPS.satellites();

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
    }

    if (satellites >= 10)
    {
        gotoUploading();
    }
}

static void uploading()
{
    sender->send();
    sender->read();

    gotoSleep();
}

static void sleeping()
{

    // Send GPS to sleep
    GPS.standby();

    // wait for UPDATE_INTERVAL
    digitalWrite(LED_BUILTIN, LOW);
    LowPower.deepSleep(UPDATE_INTERVAL * 1000);

    // woke up again
    digitalWrite(LED_BUILTIN, HIGH);

    GPS.wakeup();

    // Continue with fetching data
    gotoFetchData();
}

static void gotoSleep()
{
    Serial.println(F("Goto Sleep"));
    current_state = SLEEP;

    digitalWrite(LED_BUILTIN, LOW);
}

static void gotoFetchData()
{
    Serial.println(F("Goto Fetch Data"));
    current_state = FETCH_DATA;
    counter = millis();
    satellites = 0;

    digitalWrite(LED_BUILTIN, HIGH);
}

static void gotoUploading()
{
    Serial.println(F("Goto Uploading"));
    current_state = UPLOAD_DATA;
}

static void configurePMIC()
{
    if (PMIC.begin())
    {
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
    }
    else
    {
        Serial.println("Failed to initialize PMIC!");
    }
}
