---
# Control a PA with an ESP32 LILYGO TTGO T-DISPLAY v1.1 and an ICOM IC-705 via Bluetooth ###

This project is based of following existing projects:
http://www.carnut.info/IC-705/ICOM_IC-705.html#BT "BLUETOOTH SWITCHING"\
https://www.qrz.com/db/pe1ofo "IC-705 BLUETOOTH CONTROLLER"\
https://github.com/WillyIoBrok/CIVmasterLib Wilfried(DK8RW) who has made an excellent library for control ICOM transceivers with CI-V and ESP32\

and an article from DD6USB "Steuerung einer KW-Endstufe am Icom IC-705 via Bluetooth " in a German hamradio magazine FUNKAMATEUR published in edition at May 2022.  

Thanks and a lot of credits to these guys for their excellent work.

## Preamble ##
This project is only for use in hamradio. All things you do with it you do it at **your own risk**. I take no responsibility for any damage of devices. I use it by my own too, it works. But if you make any mistakes - it's yours and not my problem.
Beware, this all is a **work-in-progress** and I will fix things in the code.
Please check for regularly updates from time to time of this dashboard, so please keep it up-to-date.

## Requirements ##
- an ICOM IC-705 tranceiver
- an PA with PTT-switch input (all models/manufacturer), automatic band-switching with a PA which supports XIEGUs band-switching protocol like XPA125B or Micro PA-50
- an ESP32 LILYGO TTGO T-DISPLAY
- some hardware components depend what you want to do with it
- the source code is designed using Arduino IDE with ESP32 extensions, so please set-up your computing environment for using it, Google is your friend.

## My test environnment ##
- development platform running Arduino IDE: PC WINDOWS 10 and Macbook Air M1 running macOS 12
- ESP32: LILYGO TTGO T-DISPLAY v1.1 (WiFi/BT/Display 1,14" LCD)
- PTT-switch: using an 4N25 optocoupler
- PA: Micro PA50 with XIEGU protocol (press 5x FN button and select XIEGU), if you connect the ESP32 at this PA with 3.5mm phone plug, it's VOX will be automatic disabled

## Project description ##
This project demonstrates one possibility to control a power amplifier switch ON/OFF and -if required- an automatic band-switching based of CAT data from an ICOM IC-705 paired with bluetooth. Look at the manual of IC-705 how it works with bluetooth.
You will need some additional electronic components between ESP32 and the IC-705. The ESP32 is using GPIO for PTT switching (H or L active, can be set) and DAC analog output for generating the voltage for band-switching.
The generated voltage is programmed for PA which support XIEGUs band-switching protocol (can be adapted easy for other protocols, look and understand the program code).

## Final words for note ##
This program is free software; you can redistribute it and/or modify it how you want.
This codebase is a "work in progress","as is" without any kind of support.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

73 Heiko, DL1BZ