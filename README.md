# MKRWAN GPS Test
Simple GPS Test application that fetches the current GPS location using MKRWAN 1310 and the
GPSShield. The received location will then be uploaded to
[TheThingsNetwork](https://www.thethingsnetwork.org/).

## Hardware Setup
The basic setup consists of a MKRWan 1310 with an attached battery. The original GPSShield is
attached via the JTAG cable. Since the MKRWan does not provide a 5V power when the board is powered
through battery, the GPS Shield would not be powered in such a setup. Therefore, an additional 
connection of VCC is established between the MKRWan and the GPSShield.

Of course, an fitting Lora antenna is also attached to the MKRWan board.
