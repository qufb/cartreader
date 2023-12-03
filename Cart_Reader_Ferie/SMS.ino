//***********************************************************
// SEGA MASTER SYSTEM / MARK III / GAME GEAR / SG-1000 MODULE
//***********************************************************

//******************************************
//   Menus
//******************************************
// Adapters menu
static const char SMSAdapterItem1[] PROGMEM = "SMS/MarkIII raphnet";
static const char SMSAdapterItem2[] PROGMEM = "SMS Retrode";
static const char SMSAdapterItem3[] PROGMEM = "SMS Retron3in1";
static const char SMSAdapterItem4[] PROGMEM = "GameGear Retrode";
static const char SMSAdapterItem5[] PROGMEM = "GameGear Retron3in1";
static const char SMSAdapterItem6[] PROGMEM = "SG-1000 raphnet";
static const char* const SMSAdapterMenu[] PROGMEM = { SMSAdapterItem1, SMSAdapterItem2, SMSAdapterItem3, SMSAdapterItem4, SMSAdapterItem5, SMSAdapterItem6 };

// Operations menu
static const char SMSOperationItem1[] PROGMEM = "Read Rom";
static const char SMSOperationItem2[] PROGMEM = "Read from SRAM";
static const char SMSOperationItem3[] PROGMEM = "Write to SRAM";
static const char* const SMSOperationMenu[] PROGMEM = { SMSOperationItem1, SMSOperationItem2, SMSOperationItem3 };

// Rom sizes menu
static const char SMSRomSizeItem1[] PROGMEM = "8 KB";
static const char SMSRomSizeItem2[] PROGMEM = "16 KB";
static const char SMSRomSizeItem3[] PROGMEM = "24 KB";
static const char SMSRomSizeItem4[] PROGMEM = "32 KB";
static const char SMSRomSizeItem5[] PROGMEM = "40 KB";  //SG-1000 40k mapping not yet supported
static const char SMSRomSizeItem6[] PROGMEM = "48 KB";  //SG-1000 40k mapping not yet supported
static const char SMSRomSizeItem7[] PROGMEM = "64 KB";
static const char SMSRomSizeItem8[] PROGMEM = "128 KB";
static const char SMSRomSizeItem9[] PROGMEM = "256 KB";
static const char SMSRomSizeItem10[] PROGMEM = "512 KB";
static const char SMSRomSizeItem11[] PROGMEM = "1024 KB";
static const char* const SG1RomSizeMenu[] PROGMEM = { SMSRomSizeItem1, SMSRomSizeItem2, SMSRomSizeItem3, SMSRomSizeItem4 };                                      // Rom sizes for SG-1000
static const char* const SMSRomSizeMenu[] PROGMEM = { SMSRomSizeItem4, SMSRomSizeItem7, SMSRomSizeItem8, SMSRomSizeItem9, SMSRomSizeItem10, SMSRomSizeItem11 };  // Rom sizes for SMS and GG

// Init systems
static bool system_sms = false;     // SMS or MarkIII
static bool system_gg = false;      // GameGear
static bool system_sg1000 = false;  // SG-1000

// Init adapters
static bool adapter_raphnet = false;  // raphet adapater (SMS-to-MD or MIII-to-MD)
static bool adapter_retrode = false;  // Retrode adapter (SMS-to-MD or GG-to-MD)
static bool adapter_retron = false;   // Retron 3in1 adapter (SMS-to-MD or GG-to-MD)

//*********************************************************
//  Main menu with systems/adapters setups to choose from
//*********************************************************
void smsMenu() {
  system_gg = true;
  adapter_retron = true;

  for (;;) smsOperations();
}

//****************************************************
//  Create custom menu depending on selected setup
//****************************************************
void smsOperations() {
  setup_SMS();
  readROM_SMS();

  display_Update();
  wait();
}

//********************************
//   Setup I/O
//********************************
void setup_SMS() {
  println_Msg(F("setup_SMS"));

  // Request 5V
  //setVoltage(VOLTS_SET_5V);

  // Set Address Pins to Output
  //A0-A7
  DDRF = 0xFF;
  //A8-A14
  DDRK = 0xFF;
  //A15
  DDRH |= (1 << 3);
  //A16-A19
  DDRB |= (1 << DDB4);
  DDRB |= (1 << DDB5);
  DDRB |= (1 << DDB6);
  DDRB |= (1 << DDB7);

  //For Retrode adapter
  if (adapter_retrode) {
    PORTH &= ~((1 << 0) | (1 << 3) | (1 << 5));
    PORTL &= ~(1 << 1);
    DDRH &= ~((1 << 0) | (1 << 5));
    DDRL &= ~((1 << 1));
    // Set Control Pins to Output OE(PH6)
    DDRH |= (1 << 6);
    // WR(PL5) and RD(PL6)
    DDRL |= (1 << 5) | (1 << 6);
    // Setting OE(PH6) HIGH
    PORTH |= (1 << 6);
    // Setting WR(PL5) and RD(PL6) HIGH
    PORTL |= (1 << 5) | (1 << 6);
  }

  // For Raphnet and Retron adapters
  else {
    // Set Control Pins to Output RST(PH0) WR(PH5) OE(PH6)
    DDRH |= (1 << 0) | (1 << 5) | (1 << 6);
    // CE(PL1)
    DDRL |= (1 << 1);
    // Setting RST(PH0) WR(PH5) OE(PH6) HIGH
    PORTH |= (1 << 0) | (1 << 5) | (1 << 6);
    // CE(PL1)
    PORTL |= (1 << 1);
  }

  delay(400);
}

byte readByte_SMS(unsigned long myAddress) {
  if (adapter_retrode && system_gg) {
    // Set Data Pins (D8-D15) to Input
    DDRA = 0x00;
  } else {
    // Set Data Pins (D0-D7) to Input
    DDRC = 0x00;
  }

  // Set Address
  PORTF = myAddress & 0xFF;
  PORTK = (myAddress >> 8) & 0xFF;
  if (!adapter_retrode) {
    // A15/M0-7(PH3) and OE(PH6) are connected
    PORTH = (PORTH & 0b11110111) | ((myAddress >> 12) & 0b00001000);
  }
  PORTB = (PORTB & 0b11101111) | ((myAddress >> 12) & 0b00010000);
  PORTB = (PORTB & 0b11011111) | ((myAddress >> 12) & 0b00100000);
  PORTB = (PORTB & 0b10111111) | ((myAddress >> 12) & 0b01000000);
  PORTB = (PORTB & 0b01111111) | ((myAddress >> 12) & 0b10000000);

  __asm__("nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t");

  if (adapter_retrode) {
    // Switch RD(PL6) and OE(PH6) to LOW
    PORTL &= ~(1 << 6);
    PORTH &= ~(1 << 6);
  } else {
    // Switch CE(PL1) and OE(PH6) to LOW
    PORTL &= ~(1 << 1);
    PORTH &= ~(1 << 6);
  }

  __asm__("nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t");

  // Read
  byte tempByte = (adapter_retrode && system_gg) ? PINA : PINC;

  if (adapter_retrode) {
    // Switch RD(PL6) and OE(PH6) to HIGH
    PORTH |= (1 << 6);
    PORTL |= (1 << 6);
  } else {
    // Switch CE(PL1) and OE(PH6) to HIGH
    PORTH |= (1 << 6);
    PORTL |= (1 << 1);
  }

  __asm__("nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t");

  //print_Msg(F("readByte_SMS "));
  //print_Msg_PaddedHex32(myAddress);
  //print_Msg_PaddedHexByte(tempByte);
  //println_Msg(F(""));

  return tempByte;
}

byte readNibble(byte data, byte number) {
  return ((data >> (number * 4)) & 0xF);
}

//******************************************
//  Read ROM and save it to the SD card
//******************************************
void readROM_SMS() {
  println_Msg(F("readROM_SMS"));

  // Set default bank size to 16KB
  word bankSize = 16 * 1024UL;

  long cartSize = 1024 * 1024UL;  // 1MB

  for (byte currBank = 0x0; currBank < (cartSize / bankSize); currBank++) {
    print_Msg(F("bank "));
    print_Msg_PaddedHex16(currBank);
    println_Msg(F(""));
    if (currBank % 4 == 0) {
      wait();
    }

    int addr_i = 0;
    // Read 16KB
    for (word currBuffer = 0; currBuffer < bankSize; currBuffer += 512) {
      // Fill SD buffer
      for (int currByte = 0; currByte < 512; currByte++) {
        sdBuffer[currByte] = readByte_SMS((0x4000UL * currBank) + (unsigned long)(currBuffer + currByte));
      }
      // hexdump for debugging:
      for (word xi = 0; xi < 512; xi++) {
        if (xi % 16 == 0) {
          unsigned long addr = (unsigned long)(xi + currBuffer) + (0x4000UL * currBank);
          print_Msg_PaddedHex32(addr);
          print_Msg(F(" "));
        }
        print_Msg_PaddedHexByte(sdBuffer[xi]);
        if (xi > 0 && ((xi + 1) % 16) == 0) {
          println_Msg(F(""));
        } else {
          print_Msg(F(" "));
        }
      }
    }
  }
}
