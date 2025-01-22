---
## Important notification, please read this first

From time to time I receive emails about this project. However, since I no longer own an IC-705, I have left this project accessible **for archival purposes only**. You can use it or modify it how you want, feel free with it. But I can't help anymore, if there any questions about.<br>
It was one of my first and oldest project when I started with developments around the ESP32 and hamradio.<br>
Today I don't use it anymore, because I now use a SDR-system with a Hermes Lite 2 and that is it, what I'm currently developing things for.<br>
In the meantime many things around the ESP32 platform were changed and updated, I can no longer say for sure whether this project needs a code revision. When I started and developed this project, I was working with the Arduino IDE 2.x and the ESP32 platform 2.x. Today we have now platform 3.x, which needs a code revise and new code adjustments. However, I will not revise this project any more, neither now nor later. You need to know, that you need an ESP32, which supports classical Bluetooth and serial BT profiles. Newer ESP32 like the ESPS3 support only Bluetooth LE, such ESP32 don't work with this project. The 705 support Bluetooth LE too, but for using this the most code of this project need a complete code  rewrite for this. In other words, you have to rewrite all from scratch.

# Control a PA with an ESP32 LILYGO TTGO T-DISPLAY v1.1 and an ICOM IC-705 via Bluetooth ###

This project is based of following existing projects:\
http://www.carnut.info/IC-705/ICOM_IC-705.html#BT "BLUETOOTH SWITCHING"\
https://www.qrz.com/db/pe1ofo "IC-705 BLUETOOTH CONTROLLER"\
https://github.com/WillyIoBrok/CIVmasterLib Wilfried(DK8RW) who has made an excellent library for control ICOM transceivers with CI-V and ESP32

and an article from DD6USB "Steuerung einer KW-Endstufe am Icom IC-705 via Bluetooth " in a German hamradio magazine FUNKAMATEUR published in edition at May 2022.  

Thanks and a lot of credits to these guys for their excellent work.

## Preamble ##
This project is only for use in hamradio. All things you do with it you do it at **your own risk**. I take no responsibility for any damage of devices. I use it by my own too, it works. But if you make any mistakes - it's yours and not my problem.
Beware, this all is a **work-in-progress** and I will fix things in the code.
Please check for regularly updates from time to time of this dashboard, so please keep it up-to-date.

## Requirements ##
- an ICOM IC-705 tranceiver
- a PA with PTT-switch input (all models/manufacturer), automatic band-switching with a PA which supports XIEGUs band-switching protocol like XPA125B or Micro PA-50
- an ESP32 LILYGO TTGO T-DISPLAY
- some hardware components depend what you want to do with it
- the source code is designed using Arduino IDE with ESP32 extensions, so please set-up your computing environment for using it, Google is your friend.

## My test environment ##
- development platform running Arduino IDE: PC WINDOWS 10 and Macbook Air M1 running macOS 12
- ESP32: LILYGO TTGO T-DISPLAY v1.1 (WiFi/BT/Display 1,14" LCD)
- PTT-switch: using an 4N25 optocoupler
- PA: Micro PA50 with XIEGU protocol (press 5x FN button and select XIEGU), if you connect the ESP32 at this PA with 3.5mm phone plug, it's VOX will be automatic disabled

## Project description ##
This project demonstrates one possibility to control a power amplifier switch ON/OFF and -if required- an automatic band-switching based of CAT data from an ICOM IC-705 paired with bluetooth. Look at the manual of IC-705 how it works with bluetooth.
You will need some additional electronic components between ESP32 and the IC-705. The ESP32 is using GPIO for PTT switching (H or L active, can be set) and DAC analog output for generating the voltage for band-switching.
The generated voltage is programmed for PA which support XIEGUs band-switching protocol (can be adapted easy for other protocols, look and understand the program code).

## Things I add in code by myself for more functionality ##
- re-design display form of representation
- show RF power settings, Mode and Filter selection
- check if the BT connection is active or not
- re-calibrate DAC settings/values for generate more correct voltages for band-switching
- change RX/TX signaling as a small symbol
- do some code cleanup for better readability
- transfer repetitive things to functions

## Schematic ##

...coming soon, please be patient. I will be testing some additional things, until I get all the electronic parts.

```
PTT:

|ESP32|             4N25      |PA|
                  +------+
PIN_26 >--|220R|--| 1  6 |--
                  |      |
GND |-------------| 2  5 |---> PTT PA  
                  |      |
                --| 3  4 |---| GND PA
                  +------+

Be aware: PIN_26 ESP32 output is H active, PTT PA is L active with this example.
If using band-switching you need short both GND, because we need a GND for band voltage signal !
Analog output band voltage signal is PIN_27 of ESP32
```

**Remarks only for Micro PA50:**
Micro PA50 3.5mm headphone jack backside:
```
Tip    (1) - PTT (L active, if tip and GND short the PA will switch ON)
Ring   (2) - band-data voltage input
             select XIEGU protocol: press 5x FN button multiple until XIEGU is shown,
             for band-voltages look in the manual of XIEGU XPA125B
Sleeve (3) - Ground (GND)
```
The Micro PA50 will be bypass 160m and 6m by itself, because operating range is only between 80m-10m. Normally the Micro PA50 has a RF-VOX and can operate without external PTT switch. It has a frequency counter and can select the band by itself if using RF-VOX.
But I had have sometimes a few issues if I using RF-VOX at 17m and 21m with a "fluttered" PTT. If you plug-in the 3.5mm plug in the Micro PA50, then the RF-VOX will be deactivated and the PA is controlled by external PTT AND band-switching.
The manually band-switch will be deactivated too if the 3.5mm plug put in.

**Remarks only for XIEGU XPA125B:**
For wiring the Mini-DIN (6 pole) jack of XPA125B look in the manual. Same functionality like Micro PA50. PTT switch also L-active, short the PTT-PIN to ground/GND will the XPA125B switch ON too.

**Do not overdrive the PA, the RF input for both PA is 5W as maximum !**

## Conclusion ##
For me this project is very helpful for better control my PAs Micro PA50 and my XIEGU XPA125B. Especially with the XPA125 I forgot sometimes to manually switch the right band if I change it.
The PTT line is now galvanically isolated from the transceiver too for more protection. It works great via BT, we will have a delay for about 24ms with the PTT at PA side.

## Final words for note ##
This program is free software; you can redistribute it and/or modify it how you want.
This codebase is a "work in progress","as is" without any kind of support.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

73 Heiko, DL1BZ

May, 19th 2023
