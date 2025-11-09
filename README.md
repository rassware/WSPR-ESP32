# WSPR-ESP32 Beacon

## description
This project creates a simple WSPR beacon with only a few components.

## components
1. An ESP32
2. A Si5351 clock generator breakout board
3. A low pass filter
4. An Antenna

## arduino ide libs
 * Etherkit JTEncode by Jason Mildrum
 * Etherkit Si5351 by Jason Mildrum
 * Async TCP by ESP32Async
 * ESP Async Webserver by ESP32Async

## how it works
The ESP32 makes the heavy lifting of the WSPR encoding and the clock generator generates the correct frequency.
The output power is around 10 mW.

## web ui included
There is a status page for the beacon, where all important parameter are visible and changeable over web socket.

## versions
### version 1.0
 - added more parameter to the config file
 - added validation to trigger value
 - added band parameter for switch bands

### Have fun and good DX, 73 DL2RN
