/**********************************************************************************
                    Cartridge Reader for Arduino Mega2560

   This project represents a community-driven effort to provide
   an easy to build and easy to modify cartridge dumper.

   Date:             2023-10-17
   Version:          12.9

   SD lib: https://github.com/greiman/SdFat
   LCD lib: https://github.com/olikraus/u8g2
   Neopixel lib: https://github.com/adafruit/Adafruit_NeoPixel
   Rotary Enc lib: https://github.com/mathertel/RotaryEncoder
   SI5351 lib: https://github.com/etherkit/Si5351Arduino
   RTC lib: https://github.com/adafruit/RTClib
   Frequency lib: https://github.com/PaulStoffregen/FreqCount

   Compiled with Arduino IDE 2.2.1

   Thanks to:
   MichlK - ROM Reader for Super Nintendo
   Jeff Saltzman - 4-Way Button
   Wayne and Layne - Video Game Shield menu
   skaman - Cart ROM READER SNES ENHANCED, Famicom Cart Dumper, Coleco-, Intellivision, Virtual Boy, WSV, PCW, ARC, Atari 2600/5200/7800, ODY2, Fairchild, MSX, Pokemon Mini, C64, Vectrex modules
   Tamanegi_taro - PCE and Satellaview modules
   splash5 - GBSmart, Wonderswan, NGP and Super A'can modules
   partlyhuman - Casio Loopy module
   hkz & themanbehindthecurtain - N64 flashram commands
   Andrew Brown & Peter Den Hartog - N64 controller protocol
   libdragon - N64 controller checksum functions
   Angus Gratton - CRC32
   Snes9x - SuperFX sram fix
   insidegadgets - GBCartRead
   RobinTheHood - GameboyAdvanceRomDumper
   Gens-gs - Megadrive checksum
   fceux - iNes header

   And a special Thank You to all coders and contributors on Github and the Arduino forum:
   jiyunomegami, splash5, Kreeblah, ramapcsx2, PsyK0p4T, Dakkaron, majorpbx, Pickle, sdhizumi,
   Uzlopak, sakman55, Tombo89, scrap-a, borti4938, vogelfreiheit, CaitSith2, Modman,
   philenotfound, karimhadjsalem, nsx0r, ducky92, niklasweber, Lesserkuma, BacteriaMage,
   vpelletier, Ancyker, mattiacci, RWeick, joshman196, partlyhuman, ButThouMust, hxlnt,
   breyell

   And to nocash for figuring out the secrets of the SFC Nintendo Power cartridge.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

**********************************************************************************/

#include "OSCR.h"

void setup() {
  //enable_serial
  Serial.begin(9600);
  while (!Serial) {
		; // wait for serial port to connect. Needed for Native USB only
	}
  Serial.println(F("START"));
  wait();

  smsMenu();

  Serial.println(F("DONE"));
}

void loop() {
  delay(250);
}

// Common
// 21 chars for SNES ROM name, one char for termination
char romName[22];
unsigned long sramSize = 0;
int romType = 0;
byte saveType;
word romSize = 0;
word numBanks = 128;
char checksumStr[9];
bool errorLvl = 0;
byte romVersion = 0;
char cartID[5];
unsigned long cartSize;
unsigned int flashid;
char flashid_str[5];
char vendorID[5];
unsigned long fileSize;
unsigned long sramBase;
unsigned long flashBanks;
bool flashX16Mode;
bool flashSwitchLastBits;

// Variable to count errors
unsigned long writeErrors;

// Operation mode
byte mode = 0xFF;

//remember folder number to create a new folder for every game
int foldern;
// 4 chars for console type, 4 chars for SAVE/ROM, 21 chars for ROM name, 4 chars for folder number, 3 chars for slashes, one char for termination, one char savety
char folder[38];

// Array that holds the data
byte sdBuffer[512];

// soft reset Arduino: jumps to 0
// using the watchdog timer would be more elegant but some Mega2560 bootloaders are buggy with it
void (*resetArduino)(void) __attribute__((noreturn)) = 0;

void print_STR(byte string_number, boolean newline) {
  char string_buffer[22];
  strcpy_P(string_buffer, "MSG:");
  if (newline)
    println_Msg(string_buffer);
  else
    print_Msg(string_buffer);
}

/******************************************
   Common I/O Functions
 *****************************************/
// Switch data pins to write
void dataOut() {
  DDRC = 0xFF;
}

// Switch data pins to read
void dataIn() {
  // Set to Input and activate pull-up resistors
  DDRC = 0x00;
  // Pullups
  PORTC = 0xFF;
}

void _print_Error(void) {
  errorLvl = 1;
  //setColor_RGB(255, 0, 0);
  display_Update();
}

void print_Error(const __FlashStringHelper* errorMessage) {
  println_Msg(errorMessage);
  _print_Error();
}

void print_Error(byte errorMessage) {
  print_STR(errorMessage, 1);
  _print_Error();
}

void _print_FatalError(void) {
  println_Msg(F(""));
  //print_STR(press_button_STR, 1);
  display_Update();
  wait();
  resetArduino();
}

void print_FatalError(const __FlashStringHelper* errorMessage) {
  print_Error(errorMessage);
  _print_FatalError();
}

void print_FatalError(byte errorMessage) {
  print_Error(errorMessage);
  _print_FatalError();
}

void wait() {
  // Switch status LED off
  //statusLED(false);
#if defined(enable_LCD)
  wait_btn();
#elif defined(enable_OLED)
  wait_btn();
#elif defined(enable_serial)
  wait_serial();
#endif
}

void print_Msg(const __FlashStringHelper* string) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(string);
#endif
#ifdef enable_serial
  Serial.print(string);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(string);
#endif
}

void print_Msg(const char myString[]) {
#if (defined(enable_LCD) || defined(enable_OLED))
  // test for word wrap
  if ((display.tx + strlen(myString) * 6) > 128) {
    unsigned int strPos = 0;
    // Print until end of display
    while (display.tx < 122) {
      display.print(myString[strPos]);
      strPos++;
    }
    // Newline
    display.setCursor(0, display.ty + 8);
    // Print until end of display and ignore remaining characters
    while ((strPos < strlen(myString)) && (display.tx < 122)) {
      display.print(myString[strPos]);
      strPos++;
    }
  } else {
    display.print(myString);
  }
#endif
#ifdef enable_serial
  Serial.print(myString);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(myString);
#endif
}

void print_Msg(long unsigned int message) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message);
#endif
#ifdef enable_serial
  Serial.print(message);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(message);
#endif
}

void print_Msg(byte message, int outputFormat) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message, outputFormat);
#endif
#ifdef enable_serial
  Serial.print(message, outputFormat);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(message, outputFormat);
#endif
}

void print_Msg(word message, int outputFormat) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message, outputFormat);
#endif
#ifdef enable_serial
  Serial.print(message, outputFormat);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(message, outputFormat);
#endif
}

void print_Msg(int message, int outputFormat) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message, outputFormat);
#endif
#ifdef enable_serial
  Serial.print(message, outputFormat);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(message, outputFormat);
#endif
}

void print_Msg(long unsigned int message, int outputFormat) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message, outputFormat);
#endif
#ifdef enable_serial
  Serial.print(message, outputFormat);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(message, outputFormat);
#endif
}

void print_Msg(String string) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(string);
#endif
#ifdef enable_serial
  Serial.print(string);
#endif
#ifdef global_log
  if (!dont_log) myLog.print(string);
#endif
}

void print_Msg_PaddedHexByte(byte message) {
  if (message < 16) print_Msg(0, HEX);
  print_Msg(message, HEX);
}

void print_Msg_PaddedHex16(word message) {
  print_Msg_PaddedHexByte((message >> 8) & 0xFF);
  print_Msg_PaddedHexByte((message >> 0) & 0xFF);
}

void print_Msg_PaddedHex32(unsigned long message) {
  print_Msg_PaddedHexByte((message >> 24) & 0xFF);
  print_Msg_PaddedHexByte((message >> 16) & 0xFF);
  print_Msg_PaddedHexByte((message >> 8) & 0xFF);
  print_Msg_PaddedHexByte((message >> 0) & 0xFF);
}
void println_Msg(String string) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(string);
  display.setCursor(0, display.ty + 8);
#endif
#ifdef enable_serial
  Serial.println(string);
#endif
#ifdef global_log
  if (!dont_log) myLog.println(string);
#endif
}

void println_Msg(byte message, int outputFormat) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message, outputFormat);
  display.setCursor(0, display.ty + 8);
#endif
#ifdef enable_serial
  Serial.println(message, outputFormat);
#endif
#ifdef global_log
  if (!dont_log) myLog.println(message, outputFormat);
#endif
}

void println_Msg(const char myString[]) {
#if (defined(enable_LCD) || defined(enable_OLED))
  // test for word wrap
  if ((display.tx + strlen(myString) * 6) > 128) {
    unsigned int strPos = 0;
    // Print until end of display
    while ((display.tx < 122) && (myString[strPos] != '\0')) {
      display.print(myString[strPos]);
      strPos++;
    }
    // Newline
    display.setCursor(0, display.ty + 8);
    // Print until end of display and ignore remaining characters
    while ((strPos < strlen(myString)) && (display.tx < 122) && (myString[strPos] != '\0')) {
      display.print(myString[strPos]);
      strPos++;
    }
  } else {
    display.print(myString);
  }
  display.setCursor(0, display.ty + 8);
#endif
#ifdef enable_serial
  Serial.println(myString);
#endif
#ifdef global_log
  if (!dont_log) myLog.println(myString);
#endif
}

void println_Msg(const __FlashStringHelper* string) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(string);
  display.setCursor(0, display.ty + 8);
#endif
#ifdef enable_serial
  Serial.println(string);
#endif
#ifdef global_log
  char myBuffer[15];
  strlcpy_P(myBuffer, (char*)string, 15);
  if ((strncmp(myBuffer, "Press Button...", 14) != 0) && (strncmp(myBuffer, "Select file", 10) != 0)) {
    if (!dont_log) myLog.println(string);
  }
#endif
}

void println_Msg(long unsigned int message) {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.print(message);
  display.setCursor(0, display.ty + 8);
#endif
#ifdef enable_serial
  Serial.println(message);
#endif
#ifdef global_log
  if (!dont_log) myLog.println(message);
#endif
}

void display_Update() {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.updateDisplay();
#endif
#ifdef enable_serial
  delay(100);
#endif
#ifdef global_log
  if (!dont_log) myLog.flush();
#endif
}

void display_Clear() {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.clearDisplay();
  display.setCursor(0, 8);
#endif
#ifdef global_log
  if (!dont_log) myLog.println("");
#endif
}

void display_Clear_Slow() {
#if (defined(enable_LCD) || defined(enable_OLED))
  display.setDrawColor(0);
  for (byte y = 0; y < 64; y++) {
    display.drawLine(0, y, 128, y);
  }
  display.setDrawColor(1);
  display.setCursor(0, 8);
#endif
}

/******************************************
  User Control
*****************************************/

// For incoming serial data
int incomingByte;

// Using Serial Monitor
int checkButton() {
  while (Serial.available() == 0) {
  }
  incomingByte = Serial.read() - 48;

  //Next
  if (incomingByte == 52) {
    return 1;
  }

  //Previous
  else if (incomingByte == 69) {
    return 2;
  }

  //Selection
  else if (incomingByte == 240) {
    return 3;
  }

  return 0;
}

void wait_serial() {
  println_Msg(F("wait_serial"));
  if (errorLvl) {
    errorLvl = 0;
  }
  while (Serial.available() == 0) {
  }
  incomingByte = Serial.read() - 48;
  /* if ((incomingByte == 53) && (fileName[0] != '\0')) {
      // Open file on sd card
      sd.chdir(folder);
      if (myFile.open(fileName, O_READ)) {
        // Get rom size from file
        fileSize = myFile.fileSize();

        // Send filesize
        char tempStr[16];
        sprintf(tempStr, "%d", fileSize);
        Serial.write(tempStr);

        // Wait for ok
        while (Serial.available() == 0) {
        }

        // Send file
        for (unsigned long currByte = 0; currByte < fileSize; currByte++) {
          // Blink led
          if (currByte % 1024 == 0)
            blinkLED();
          Serial.write(myFile.read());
        }
        // Close the file:
        myFile.close();
      }
      else {
        print_FatalError(open_file_STR);
      }
    }*/
}
