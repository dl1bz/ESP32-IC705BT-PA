/*
CIV_template - z_userprog - adapted for the requirements of Glenn, VK3PE by DK8RW, May 16, 22
    vk3pe is using a "TTGO" ESP32 module.
    Band select BCD outputs set to be active Hi.NOTE: a BCD to Decimal chip will be used also
     to provide 10 band outputs.
    PTT output is active LOW

This is the part, where the user can put his own procedures in

The calls to this user programs shall be inserted wherever it suits - search for //!//
in all files

*/

#include <TFT_eSPI.h>       //using this LIB now.  https://github.com/Bodmer/TFT_eSPI    
// IMPORTANT!  
//      In the "User_Setup_Select.h" file, enable "#include <User_Setups/Setup25_TTGO_T_Display.h>"

// #define WIFI

#ifdef WIFI
// activate Wifi
#include <WiFi.h>
#include <PubSubClient.h>

// Replace the next variables with your SSID/Password combination
const char* ssid = "ABCNET";
const char* password = "passw0rd";
#define CONNECTION_TIMEOUT 10
#endif


//=========================================================================================
// user part of the defines

// if defined, the bit pattern of the output pins is inverted in order to compensate
// the effect of inverting HW drivers (active, i.e.uncommented by default)
#define invDriver         //if active, inverts band BCD out
//#define Inv_PTT           //if active, PTT out is Low going.




#define VERSION_USER "usrprg VK3PE V0_3 May 31st, 2022 with mods by DL1BZ, 2023"

#define NUM_BANDS 13   /* Number of Bands (depending on the radio) */

//-----------------------------------------------------------------------------------------
//for TFT
TFT_eSPI tft = TFT_eSPI();

#define screen_width  240       //placement of text etc must fit withing these boundaries.
#define screen_heigth 135

// all my known colors for ST7789 TFT (but not all used in program)
#define B_DD6USB 0x0004    //   0,   0,   4  my preferred background color !!!   now vk3pe ?
#define BLACK 0x0000       //   0,   0,   0
#define NAVY 0x000F        //   0,   0, 123
#define DARKGREEN 0x03E0   //   0, 125,   0
#define DARKCYAN 0x03EF    //   0, 125, 123
#define MAROON 0x7800      // 123,   0,   0
#define PURPLE 0x780F      // 123,   0, 123
#define OLIVE 0x7BE0       // 123, 125,   0
#define LIGHTGREY 0xC618   // 198, 195, 198
#define DARKGREY 0x7BEF    // 123, 125, 123
#define BLUE 0x001F        //   0,   0, 255
#define GREEN 0x07E0       //   0, 255,   0
#define CYAN 0x07FF        //   0, 255, 255
#define RED 0xF800         // 255,   0,   0
#define MAGENTA 0xF81F     // 255,   0, 255
#define YELLOW 0xFFE0      // 255, 255,   0
#define WHITE 0xFFFF       // 255, 255, 255
#define ORANGE 0xFD20      // 255, 165,   0
#define GREENYELLOW 0xAFE5 // 173, 255,  41
#define PINK 0xFC18        // 255, 130, 198
//*************************************************************

//=================================================
// Mapping of port-pins to functions on ESP32 TTGO
//=================================================

// the Pins for SPI
#define TFT_CS    5
#define TFT_DC   16
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_RST  23
#define TFT_BL    4

#define PTTpin    17      //PTT out pin
#define PTTpinHF  26      //PTT out HF
#define PTTpinVHF 33      //PTT out VHF
#define PTTpinUHF 32      //PTT out UHF

boolean HF_ptt_Enable;
boolean VHF_ptt_Enable;
boolean UHF_ptt_Enable;

int bandvoltage;
#define LED 27       //Band voltage
#define C_RELAIS  25 //Coax relais HF / VHF-UHF

// For analogue PWM output
int brightness = 0;
const int freq = 5000;     //PWM Freq
const int ledChannel = 0;  //
const int resolution = 10; // possible resolution 8, 10, 12, 15 bit

//=========================================================================================
// user part of the database
// e.g. :
uint8_t         G_currentBand = NUM_BANDS;  // Band in use (default: not defined)

//=====================================================
// this is called, when the RX/TX state changes ...
//=====================================================
void  userPTT(uint8_t newState) {

#ifdef debug
    Serial.println(newState);                     //prints '1' for Tx, '0' for Rx
#endif
    tft.setFreeFont(&FreeSansBold9pt7b);        //previous setup text was smaller.
    //tft.setTextColor(WHITE) ;


    if (newState) {                                    // '1' = Tx mode
       Draw_TX();
    }   //Tx mode
    else {
       Draw_RX();
    } //Rx mode

#ifdef Inv_PTT 
    digitalWrite(PTTpin, !newState);    //--inverted-- output version:  Clr =Tx, Hi =Rx  
    if (HF_ptt_Enable) {
        digitalWrite(PTTpinHF, !newState);
    }
    if (VHF_ptt_Enable) {
        digitalWrite(PTTpinVHF, !newState);
    }
    if (UHF_ptt_Enable) {
        digitalWrite(PTTpinUHF, !newState);
    }
#else
    digitalWrite(PTTpin, newState);    // Clr =Rx, Hi =Tx 
    if (HF_ptt_Enable) {
        digitalWrite(PTTpinHF, newState);
    }
    if (VHF_ptt_Enable) {
        digitalWrite(PTTpinVHF, newState);
    }
    if (UHF_ptt_Enable) {
        digitalWrite(PTTpinUHF, newState);
    }
#endif 
}

//=========================================================================================
// creating bandinfo based on the frequency info

//-----------------------------------------------------------------------------------------
// tables for band selection and bittpattern calculation

//for IC-705 which has no 60M band:
//---------------------------------
// !!! pls adapt "NUM_BANDS" if changing the number of entries in the tables below !!!

// lower limits[kHz] of the bands: NOTE, these limits may not accord with band  edges in your country.
constexpr unsigned long lowlimits[NUM_BANDS] = {
  1000, 2751, 4501,  6001,  8501, 13001, 16001, 19501, 23001, 26001, 35001, 144000 , 430000
};
// upper limits[kHz] of the bands:  //see NOTE above.
constexpr unsigned long uplimits[NUM_BANDS] = {
  2750, 4500, 6000,  8500, 13000, 16000, 19500, 23000, 26000, 35000, 60000 ,148000 , 470000
};


// "xxM" display for the TFT display. ie show what band the unit is current on in "meters"
const String (band2string[NUM_BANDS + 1]) = {
    // 160     80   60    40     30      20     17      15     12     10      6     NDEF
      "160m"," 80m", " 60m",  " 40m"," 30m", " 20m"," 17m", " 15m"," 12m"," 10m","  6m", "  2m","70cm" ," Out"

};

//------------------------------------------------------------
// set the bitpattern in the HW

void set_HW(uint8_t BCDsetting) {

   

#ifdef debug
    // Test output to control the proper functioning:
    Serial.print(" Pins ");
    #endif

}

//-----------------------------------------------------------------------------------------
// get the bandnumber matching to the frequency (in kHz)

byte get_Band(unsigned long frq) {
    byte i;
    for (i = 0; i < NUM_BANDS; i++) {
        //for (i=1; i<NUM_BANDS; i++) {   
        if ((frq >= lowlimits[i]) && (frq <= uplimits[i])) {
            return i;
        }
    }
    return NUM_BANDS; // no valid band found -> return not defined
}

//------------------------------------------------------------------
//    Show frequency in 'kHz' and band in 'Meters' text on TFT vk3pe
//------------------------------------------------------------------
void show_Meters(void)
{

    // Show Freq[KHz]
    tft.setCursor(5, 120);                //- 
    tft.fillRect(0, 80, 105, 55, BLACK);   //-erase   x,y,width, height 
    tft.drawRoundRect(0, 80, 105, 55, 5, WHITE);
    tft.setTextColor(YELLOW);               //-
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextSize(2);
    tft.print(band2string[G_currentBand]); //-

}

void show_Mode(uint8_t newModMode, uint8_t newRXfilter)
{
       tft.setFreeFont(NULL);         // Set font to GLCD
       // tft.setFreeFont(&FreeSans9pt7b);
       // tft.setFreeFont(&Tiny3x3a2pt7b);
       tft.fillRect(105, 80, 85, 55, BLACK);   //erase previous freq   vk3pe x,y,width,height,colour 10,40,137,40
       tft.drawRoundRect(105, 80, 85, 55, 5, WHITE);
       tft.setTextSize(2);
       tft.setCursor(115, 90);
       tft.setTextColor(YELLOW);
       tft.print(modModeStr[newModMode]);
       tft.setCursor(115, 115);
       tft.print(FilStr[newRXfilter]);

}

void user_TXPWR(unsigned short getTXPWR) {
       unsigned long TXPWR_W;
       TXPWR_W=((getTXPWR*100)/255); // calculate in procent like IC705

       tft.setFreeFont(NULL);         // Set default font
       tft.setTextSize(2);
       tft.fillRect(170, 1, 65, 28, MAROON); // productive setting background
       // tft.fillRect(170, 1, 65, 28, BLACK); //debug setting background
       tft.setTextColor(WHITE);  // print TXPWR white if output power < 4W 
       if (getTXPWR > 101) {
         tft.setTextColor(YELLOW); // print TXPWR yellow if out > 4W for attention of PA max. input power
       }
       if (getTXPWR > 128) {
         tft.setTextColor(ORANGE); // print TXPWR orange if out > 5W for attention of PA max. input power
       }
       tft.setCursor(170,7);
       tft.print(0.1 * TXPWR_W, 1); //output TXPWR as Watt with one decimal digit
       tft.println("W"); // add W for display TXPWR value
}

#ifdef WIFI
void setup_wifi() {
     delay(1000);

    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting");
    int timeout_counter = 0;

    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(200);
        timeout_counter++;
        if(timeout_counter >= CONNECTION_TIMEOUT*5){
        // ESP.restart();
        Serial.print("no WLAN connect...exit WiFi");
        return;
        }
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}
#endif

void init_DAC() {
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(LED, ledChannel);
}

void Draw_TX() {
        tft.fillRoundRect(190, 80, 50, 55, 10, RED);
        tft.drawLine(215, 88, 215, 128, WHITE);
        tft.drawLine(194, 88, 215, 103, WHITE);
        tft.drawLine(235, 88, 215, 103, WHITE);
        // tft.print("Tx");
}

void Draw_RX() {
        tft.fillRoundRect(190, 80, 50, 55, 10, GREEN);
        tft.fillRect(197, 90, 15, 30, BLACK);
        tft.drawLine(232, 85, 232, 125, BLACK);
        tft.drawLine(232, 85, 212, 90, BLACK);
        tft.drawLine(232, 125, 212, 120, BLACK);
        // tft.print("Rx");
}

void Clear_Scr() {
        tft.fillRect(0, 31, 240, 104, BLACK);
}

void BT_Conn_Status(const char* read_Conn_Status) {
    const char* Conn_Yes = strstr(read_Conn_Status, "R_ON");

    tft.setFreeFont(NULL);         // Set default font
    tft.setTextSize(2);
   
    if (read_Conn_Status == Conn_Yes) {
      tft.fillRect(80, 1, 85, 28, MAROON); // productive setting background
      tft.setTextColor(GREEN);
      tft.setCursor(80,7);
      tft.print("BT CAT");
      Clear_Scr();
      Draw_RX();
    } else {
      tft.fillRect(80, 1, 155, 28, MAROON); // productive setting background
      tft.setTextColor(WHITE);
      tft.setCursor(80,7);
      tft.print("OFFLINE");
      Clear_Scr(); // clear screen
      tft.setCursor(0,40);
      tft.print("Please pair\n\nyour ICOM IC-705\n\nvia Bluetooth !");
    }
}

//------------------------------------------------------------
// process the frequency received from the radio
//------------------------------------------------------------

void set_PAbands(unsigned long frequency) {
    unsigned long freq_kHz;
    
    freq_kHz = G_frequency / 1000;            // frequency is now in kHz
    G_currentBand = get_Band(freq_kHz);     // get band according the current frequency

    // tft.setFreeFont(&FreeSansBold9pt7b);   //bigger numbers etc from now on. <<<<<<<<-------------------
    // tft.setFreeFont(&FreeMonoBold18pt7b);
    tft.setFreeFont(&Orbitron_Light_32);
    tft.setTextSize(1);
    tft.setCursor(5, 67);
    
    if (freq_kHz < 100000) {
      tft.setCursor(25, 67);                 // for bigger print size
    } 
    if (freq_kHz < 10000) {
       tft.setCursor(40, 67);
    }
    
    //already use white from previous :-
    tft.fillRoundRect(0, 35, tft.width(), 40, 5, BLUE);   //erase previous freq   vk3pe x,y,width,height,colour 10,40,137,40
    // tft.drawRoundRect(0, 35, tft.width(), 40, 5, BLACK);    //with white border.
    tft.setTextColor(WHITE);               // at power up not set!
    // mod by DL1BZ
    tft.print(0.000001 * frequency, 5);    //show Frequency in MHz with 5 decimal digits
    // tft.println("M"); // add W for display TXPWR value

#ifdef debug
    // Test-output to serial monitor:
    Serial.print("Frequency: ");  Serial.println(freq_kHz);
    Serial.print("Band: ");     Serial.println(G_currentBand);
    Serial.println(band2string[G_currentBand]);
 #endif

    // MR Hier stellen we het voltage in
        int bandcode;
    bandcode = G_currentBand;
        int sendDAC;  // calculated value we will send to DAC at the end
        int corrFact; // because if a load exist like PA we will have a lower, non-linear voltage output as calculated
        corrFact = 0; // add a little bit more mV, here with R=470Ohm and C=22uF at PIN 27

    switch (bandcode) {
    case 0:  // 160M
        // Manual XPA125B 230mV
        bandvoltage=230; // in mV
        // 3
        corrFact = 6;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 1:  // 80M
        // Manual XPA125B 460mV
        bandvoltage = 460;
        // 4
        corrFact = 14;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 2:  // 60M
        // Manual XPA125B 690mV
        bandvoltage = 690;
        // 5
        corrFact = 19;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 3:  // 40M
        // Manual XPA125B 920mV
        bandvoltage = 920;
        // 8
        corrFact = 26;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 4:  // 30M
        // Manual XPA125B 1150mV
        bandvoltage = 1150;
        // 8
        corrFact = 30;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 5:  // 20M
        // Manual XPA125B 1380mV
        bandvoltage = 1380;
        // 9
        corrFact = 35;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 6:  // 17M
        // Manual XPA125B 1610mV
        bandvoltage = 1610;
        // 9
        corrFact = 44;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 7:  // 15M
        // Manual XPA125B 1840mV
        bandvoltage = 1840;
        // 10
        corrFact = 58;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 8:  // 12M
        // Manual XPA125B 2070mV
        bandvoltage = 2070;
        // 12
        corrFact = 55;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 9:  // 10M
        // Manual XPA125B 2300mV
        bandvoltage = 2300;
        // 15
        corrFact = 63;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 10:  // 6M
        // Manual XPA125B 2530mV
        bandvoltage = 2530;
        // 15
        corrFact = 71;
        bandvoltage = bandvoltage + corrFact; // add corrFact for new bandvoltage for calculate
        break;
    case 11:  // 2M
        bandvoltage = 0;
        break;
    case 12:  // 70CM
        bandvoltage = 0;
        break;
    case 13:  // NDEF
        bandvoltage = 0;
        break;

    }

    
    if (freq_kHz > 0 && freq_kHz < 60000) {
        digitalWrite(C_RELAIS, LOW);
        HF_ptt_Enable = 1;
        VHF_ptt_Enable = 0;
        UHF_ptt_Enable = 0;

        
    }

    else if
        (freq_kHz > 144000 && freq_kHz < 148000) {
        digitalWrite(C_RELAIS, HIGH);
        HF_ptt_Enable = 0;
        VHF_ptt_Enable = 1;
        UHF_ptt_Enable = 0;

    }
    else if
        (freq_kHz > 430000 && freq_kHz < 470000) {
        digitalWrite(C_RELAIS, HIGH);
        HF_ptt_Enable = 0;
        VHF_ptt_Enable = 0;
        UHF_ptt_Enable = 1;

    }

    Serial.print("Bandvoltage in mV for XIEGU-PA with LOAD: "); Serial.println(bandvoltage);
    Serial.print("Correct factor in mV                    : "); Serial.println(corrFact);

    sendDAC = bandvoltage * 1024 / 3300; // a value at 1024 is 3V3 output without load
    
    // check if the value not greater as 1024 because we use a resolution of 10bit = 2^10 = 1024 = 3V3
    if (sendDAC > 1024) {
      sendDAC = 1024;
    }
    
    Serial.print("Send value to DAC: ");     Serial.println(sendDAC);
    
    // set analog voltage for bandswitching on PA for XIEGU protocol, e.g. XPA125B or Micro PA50 at PIN 27
    ledcWrite(ledChannel, sendDAC); // send to ESP32-DAC

    show_Meters();            //Show frequency in kHz and band in Meters (80m etc) on TFT
}

//=========================================================================================
// this is called, whenever there is new frequency information ...
// this is available in the global variable "G_frequency" ...
void userFrequency(unsigned long newFrequency) {

    set_PAbands(G_frequency);

}

//-----------------------------------------------------------------------------------------
// initialise the TFT display
//-----------------------------------------------------------------------------------------

void init_TFT(void)
{
    //tft.init(screen_heigth, screen_width) ;  //not used

    tft.init();
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);              // switch backlight on

    tft.fillScreen(BLACK);
    tft.setRotation(1);
    tft.fillRoundRect(0, 0, tft.width(), 30, 5, MAROON);   // background for screen title
    tft.drawRoundRect(0, 0, tft.width(), 30, 5, WHITE);    //with white border.

    tft.setTextSize(2);                  //for default Font only.Font is later changed.
    tft.setTextColor(YELLOW);
    tft.setCursor(10, 7);                //top line
    tft.print("IC705");

    tft.setTextColor(WHITE);            //white from now on

    tft.setCursor(185, 45);               //
    // tft.setTextSize(3);
    // tft.print("MHz");
    tft.setTextSize(1);
    // tft.setCursor(150, 95) ; 
    // tft.setCursor(135, 107);             //
    // tft.print("band");                   //"160m" etc   or Out if invalid Freq. for Ham bands.
}

//=========================================================================================
// this will be called in the setup after startup
void  userSetup() {

    Serial.println(VERSION_USER);

    // set the used HW pins (see defines.h!) as output and set it to 0V (at the Input of the PA!!) initially

    pinMode(PTTpin, OUTPUT);     //PTT out pin
    pinMode(PTTpinHF, OUTPUT);   //PTTHF out pin
    pinMode(PTTpinVHF, OUTPUT);  //PTTVHF out pin
    pinMode(PTTpinUHF, OUTPUT);  //PTTUHF out pin

    pinMode(C_RELAIS, OUTPUT);   // Coax Relais HF / VHF-UHF
    digitalWrite(PTTpin, LOW);       //set 'Rx mode' > high
    digitalWrite(PTTpinHF, LOW);
    digitalWrite(PTTpinVHF, LOW);
    digitalWrite(PTTpinUHF, LOW);
 


    init_TFT();  // initialize T-DISPLAY LILYGO TTGO v1.1
    
    init_DAC(); // initialize analog output

    userPTT(0);  // initialize the "RX" symbol in the screen



    #ifdef WIFI
    setup_wifi();
    #endif
}

//-------------------------------------------------------------------------------------
// this will be called in the baseloop every BASELOOP_TICK[ms]
void  userBaseLoop() {

}
