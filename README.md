House alarm system based on ESP8266 or ESP32

Copyright (c) 2017, 2018 by Danny Backx

This alarm system is designed to work with a bunch of similar controllers. You can choose which hardware and
capabilities go in each individual controller, and configure them accordingly.

The software needs a small amount of configuration to talk to the hardware. Current plan is to hardcode
configuration (small JSON texts) because it would take a large effort to build a GUI to configure modules
via the touch screen. Also no (network) remote configuration functionality is built-in, to prevent making
the system hackable.

Hardware platform
 - ESP8266 and/or ESP32
   Choose what you like, have, or need. Most importantly : the ESP32 has more pins.
   An ESP8266 with an OLED and a radio is out of pins, if you need to put more hardware in one box, you need an ESP32.
   An ESP8266 with OLED and PN532 card reader also has just enough pins.
 - Some sensors with wireless RF communication
   * https://www.aliexpress.com/item/Kerui-433MHz-Wireless-Intelligent-PIR-Sensor-Motion-Detector-For-GSM-PSTN-Security-Alarm-System-Auto-Dial/32566190623.html?spm=a2g0s.9042311.0.0.04PnSB
   * https://www.aliexpress.com/item/433MHz-Portable-Alarm-Sensors-Wireless-Fire-Smoke-Detector/32593947430.html?spm=a2g0s.9042311.0.0.04PnSB
 - Keypads with touch displays
   * https://www.aliexpress.com/item/1pcs-J34-F85-240x320-2-8-SPI-TFT-LCD-Touch-Panel-Serial-Port-Module-with-PCB/32795636902.html?spm=a2g0s.9042311.0.0.04PnSB
 - RF receivers
   * https://www.aliexpress.com/item/1set-2pcs-RF-wireless-receiver-module-transmitter-module-board-Ordinary-super-regeneration-433MHZ-DC5V-ASK-OOK/32606396563.html?spm=a2g0s.9042311.0.0.04PnSB
 - RFID card readers
   * https://www.aliexpress.com/item/2pcs-lot-MFRC-522-RC522-RFID-Kits-S50-13-56-Mhz-With-Tags-SPI-Write-Read/32620671237.html?spm=a2g0s.9042311.0.0.tm7J7e
   * https://www.aliexpress.com/item/PN532-NFC-RFID-Module-V3-Kits-Reader-Writer/32452824672.html?spm=a2g0s.9042311.0.0.XugjzW

Libraries used :
- https://github.com/Bodmer/TFT_eSPI : a library to work with touch displays based on
  e.g. the ILI9341

Wiring diagram

Wiring is currently based on the wiring diagram in http://nailbuster.com/?page_id=341 .
A copy is wiring/MyTouchSPIShield.png but this is created by david@nailbuster.com .

Several hardware configurations

1. Secure environment control panel
  This is for "safe" places, places that you can't get to without passing detection.
  When you're here, you can turn off the alarm without authentication.

2. Full fledged control panel
  This is a perimeter control panel : you can turn the alarm off only after authentication.
  Both a keyboard/pin based and a RFID tag based authentication are possible.

3. Remote sensors
  Several remote sensors can be hooked up via either RF transmitter/receiver pairs,
  or modules based on this framework which communicate via WiFi.

  Some sensors can feed information into the system, others just trigger the alarm (if it's on).

Overview of directories

Keypad
  Main development directory
  This contains code for a Oled + esp8266 + Radio combination

AlarmController
  This contains a stripped down version of Keypad, with the Oled related modules removed.

Controller32
  This is a port of AlarmController to the ESP32 platform
