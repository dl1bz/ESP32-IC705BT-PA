// Edit by PA5MR && PE1OFO
// z_userprog from VK3PE17

/* 
CIV_template - an easy to use example for handling a communication channel to ICOM radios

  Created by Wilfried Dilling, DK8RW, May 2022
  Released into the public domain


This Arduino sketch is based on and uses the library "CIVmasterLib" written and released to 
the public domain by Wilfried Dillig, DK8RW in May 2022 or newer.


It is fully functional, but carries in addition lots of comments in order to make it better
understandable.
The SW has also been structured in several tabs in order to make it easier for the user to
create own sketches based on CIVmasterLib.
Put your own code in the (almost) empty tab "z_userprog.ino" and put the calls to 
your procedures into one of the places marked with "//!//"

Don't worry - although this sketch looks big (because of lots of explaining comments), you 
don't have to work through and understand the whole code in order to create a running system
- just look for "//!//" to put your calls to your own code in file "z_userprog.ino"


Tasks: 
- this is a template, which shall show how the CIV class, i.e. a single point to point 
  connection between a CI-V master(Arduino or ESP32) and an ICOM radio is utilized best based
  on CIVmasterLib

- by default it processes the following infos:
  - is the radio ON or OFF ?
  - is the radio in TX or RX mode ?
  - which frequency is in use by the radio ?
  
- optional also the following info is processed:
  - which modulation mode and which RX-Filter is in use ? (enable with "modmode" in a_defines.h)

For a first impression, compile the unmodified sketch and let it run on an ESP32 or on one of the
supprted Arduino boards and have a look into the Serial Monitor (Baudrate 115200).

Of course, this Sketch can also be used to control the radios - just add your commands to the radio
in "CiV_sendCmds()", but this has not been covered by this example yet (another example to come?).


!!! New !!! 28.05.22   fastPTT !

Thanks to the info from PU5VEN via VK3PE about one specific ICOM command, I have been able to implement
a "fastPTT" mode !
By this, the PTT delay from 80ms(average)..130ms could be reduced down to 10ms(average)..30ms! 
This is much faster and in the range which the IC705 can compensate :-)

So, the risk of blowing something up has decreased significantly, hi.
However, this is all SW, so the note below still applies ;-)

Note: Please be careful if you want to use the RX/TX info as a PTT for PAs - the delay is
      pretty high, unfortunately - something between 12ms and 130ms!  (fastPTT: 30ms max !)
      The IC705 can only compensate 30ms.
      It is your own responsibility to ensure, that no damage occures to your radio or the PA 
      in the case, when the PTT change RX->TX comes later than the HF-power from the radio.


*/

// includes ---------------------------------------------------------------------


#include <CIVmaster.h>  // CIVcmds.h will be included automatically in addition

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// First, select the radio in use by commenting/uncommenting the matching lines ! 
//!//

//---------------------------------------
//IC705 (via Bluetooth, default):
  #define useBT
  #define fastPTT
  uint8_t civAddr = CIV_ADDR_705;
//---------------------------------------
//IC7300:
//IC7300 doesn't support fastPTT !
//  uint8_t civAddr = CIV_ADDR_7300;
//---------------------------------------
//IC9700:
//  #define fastPTT
//  uint8_t civAddr = CIV_ADDR_9700;


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Please don't forget to set debug mode ON or OFF according your wishes in the
// include file "a_defines.h" (default is debug mode ON) !
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include "a_defines.h"


//-------------------------------------------------------------------------------
// create the civ object in use

CIV     civ;    // create the CIV-Interface object; by this you can easily access
                // the interface to the radio later on

// module wide variables --------------------------------------------------------

// this is the structure, which carries the result of the civ-methods writeMsg and readMsg
CIVresult_t CIVresultL;
/*
parameters of the structure CIVresult_t:
.retVal         // the summary/result of the command processing like CIV_OK, CIV_NOK,  ...
.address        // the address of the radio which got/sent the message
.cmd[5]         // the command, which has been sent/got by the radio; cmd[0]= length of the command
.datafield[10]  // the data, which has been sent/got by the radio; datafield[0]= length of the data
.value          // if the datafield carries valid data, here is the binary number. Otherwise set to 0


possible return values (retVal_t):
  CIV_OK           =  0,  // valid command received by the radio and accepted
  CIV_OK_DAV       =  1,  // valid command received by the radio, accepted and data (e.g. frequency)
                          // are available

  // this is the border between good and bad and will be used as such -----------------

  CIV_NOK          =  2,  // command received by the radio, but NOT accepted; something is wrong
  CIV_HW_FAULT     =  3,  // onewire bus: dataline is shortcut to ground
  CIV_BUS_BUSY     =  4,  // onewire bus: attempt to send data failed, because of traffic on the bus 
  CIV_BUS_CONFLICT =  5,  // onewire bus: attempt to send data failed because of bus collision
  CIV_NO_MSG       =  6   // no answer from the radio !

 ... see also CIVmaster.h ...
*/

// some control variables/timers:

// with these timestamps time differences of up to 65535 ms == 1min 5,5sec max. can be handled

// uint16_t is used here in order to save RAM space (because of the small Atmel processors)

uint16_t    ts_CIVcmdSent = 0;   // time stamp[ms] of the last command sent 
                                 // this is common for all commands, the last wins !
                                 // used for determining the timeout
uint16_t    ts_RXTX_sent  = 0;   // time stamp[ms] of the last RXTX query sent to the radio
uint16_t    ts_f_sent     = 0;   // time stamp[ms] of the last frequency query sent to the radio
// add by DL1BZ
uint16_t    ts_TXPWR_sent = 0;   // time stamp[ms] of the last TXPWR query sent to the radio

#ifdef modmode
uint16_t    ts_Mod_sent   = 0;   // time stamp[ms] of the last ModMode query sent to the radio
#endif
//!// put here additional timers in case you are adding additional queries


bool        CIVwaitForAnswer = false; // if true, a command has been sent to the radio which
                                      // has not been answered yet


// Global variables of the radio data -------------------------------------------
// in general, the initial values are set to an undefined state in order 
// to force necessary actions initially after connecting to the radio

// "database" variables:

radioOnOff_t    G_radioOn   = RADIO_NDEF; // don't know initially, whether the radio 
                                          // is switched on and connected
bool            G_RXTX      = 0;          // 0 == RX; 1 == TX on
unsigned long   G_frequency = 0;          // operating frequency in [Hz]
uint8_t         G_Mod       = MOD_NDEF;   // Modulation mode (USB, LSB, etc...)
uint8_t         G_RXfilter  = FIL_NDEF;   // RXfilter in use (Fil1, Fil2, Fil3);
// add by DL1BZ
unsigned short  G_TXPWR     = 0;          // TX Power (0..255 = 0-10W)

//!//    put additional variables here, if necessary ... or put them into file z_userprog.ino


// timer  variables -------------------------------------------------------------
uint16_t t_curr_lp;       // temporary time of the current baseloop (at the beginning) for calculations
uint16_t ts_last_lp;      // time stamp - will be updated at the end of every baseloop run

//=============================================================================================

//==========  General initialization  of  the system  =========================================

void setup() {

  // initialize the serial interface (for debug messages)
  Serial.begin(debugBdRate);
  Serial.println("");
  delay(100);
  Serial.println (VERSION_STRING);
  

  // initialize the civ object/module (true means "use BT" in case of the ESP32)
  #ifdef useBT
    // mod by DL1BZ
    civ.setupp(true,"ESP32-XPA125B"); // BT identifier show at IC705
  #else
    civ.setupp();     // onewire (HW)
  #endif

  // tell civ, which address of the radio is to be used
  civ.registerAddr(civAddr);

// note: in fastPTT mode this may be significantly slower, since it doesn't influence the PTT delay in this case
  init_delayMeas; // debugging on ESP32: setup the PTT delay measurement

  //!// put your setup code (independent from civ) here ... or put it into file z_userprog.ino
  userSetup();

  // initialise the baseloop timers ...
  t_curr_lp = millis();
  ts_last_lp = t_curr_lp;
  
}

//============================  main  loop ====================================================

void loop() {

  t_curr_lp = millis();
  
  if (uint16_t(t_curr_lp - ts_last_lp) >= BASELOOP_TICK) {
    // payload ------------------------------------------------------

    set_delayMeas;  // debugging on ESP32: check the trigger input and store the time when input changes

    // first, read and process the answers from the radio (if available)
    CIV_getProcessAnswers();

    // second, write commands to the radio (if possible and necessary)
    CIV_sendCmds();

    // at this point, all avaliable data of the radio have been handled - CIV processing done !
    // the variables in the "database" variables have been updated, if necessary you can work with them

    //!// put your base loop code (independent from civ) here ... or put it into file z_userprog.ino
    userBaseLoop();

    // payload ------------------------------------------------------
    ts_last_lp = t_curr_lp;
	} // if BASELOOP_TICK
  
} // end loop
