#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>

extern uint8_t _rowOffsets[4] = {0x00,0x20,0x40,0x60};

#define SCL PB6
#define SDI PB5
#define CS PB1

#define FUNCTION_SET 0x20
#define LINES_2_4 0x08
#define LINES_1_3 0x00
#define DOUBLEHEIGHT_ON 0x04
#define DOUBLEHEIGHT_OFF 0x00
#define SET_EXREG_RE 0x02 // Extended function registers enable
#define CLEAR_EXREG_RE 0x00
#define SET_EXREG_IS 0x01 // Special registers enable
#define CLEAR_EXREG_IS 0x00

#define SET_DISPLAY 0x08
#define DISPLAY_ON 0x04
#define DISPLAY_OFF 0x00
#define CURSOR_ON 0x02
#define CURSOR_OFF 0x00
#define CURSOR_BLINK_ON 0x01
#define CURSOR_BLINK_OFF 0x00

#define SET_DDRAM_ADDR 0x80

void putData (uint8_t data){
  for (int m = 0; m < 8; m++){
    if ((data & 0x01) == 0x01){ // Comapring the LSB i.e. (0000 0001)
      PORTB |= (1 << SDI);
    }
    else{
      PORTB &= ~(1 << SDI);
    }
    //while (0);
    data = (data >> 1); // Left Shift 1 bit to be compared
    PORTB &= ~(1 << SCL);  // Beginning of CLOCK Cycle
    _delay_us(1);
    PORTB |= (1 << SCL);
    _delay_us(1);
    PORTB &= ~(1 << SCL); // Ending of CLOCK Cycle
    _delay_us(1);
  }
}

void OLED_Command(unsigned char data){ // Command Writing Function
  uint8_t firstByte = data & 0x0F; // Clear upper nibble.
  uint8_t secondByte = (data >> 4) & 0x0F; // Right shift by 4 and clear upper nibble.

  PORTB &= ~(1 << CS); // clearSC() CS MUST be low for that transfer of data
  _delay_us(1);

  putData(0b00011111);

  putData(firstByte);
  putData(secondByte);

  PORTB |= (1 << CS); // antes setCS()END of data Line

}

void OLED_Data(unsigned char data){
  uint8_t firstByte = data & 0x0F; // Clear upper nibble.
  uint8_t secondByte = (data >> 4) & 0x0F; // Right shift by 4 and clear upper nibble.

  PORTB &= ~(1 << CS); // clearSC() CS MUST be low for that transfer of data
  _delay_us(1);

  putData(0b01011111);

  putData(firstByte);
  putData(secondByte);

  PORTB |= (1 << CS);; // antes setCS()END of data Line

}

void functionSet(uint8_t lines, uint8_t doubleHeight, uint8_t extensionRegIS){
  OLED_Command(FUNCTION_SET | lines | doubleHeight | CLEAR_EXREG_RE | extensionRegIS);
}

void setDisplay(uint8_t display, uint8_t cursor, uint8_t cursorBlink){
  OLED_Command(SET_DISPLAY | display | cursor | cursorBlink);
}

void OLED_gotoxy(int x, int y){
  //uint8_t DDRAM_addr = SET_DDRAM_ADDR | (_rowOffsets[y] + x);
  OLED_Command(SET_DDRAM_ADDR | (_rowOffsets[y] + x));
  _delay_ms(1);
}

void OLED_Init(void){
  OLED_Command(0x2A); // Function Set (extended command set)
  OLED_Command(0x71); // Function Selection A
  OLED_Data(0x5C);    // Enable 5V Regulator
  OLED_Command(0x28); // Function Set (fundamental command set)
  OLED_Command(0x08); // Display off, Cursor off, Blink off
  OLED_Command(0x2A); // Function Set (extended command set)
  OLED_Command(0x79); // OLED command set enabled
  OLED_Command(0xD5); // Set displayed clock divide ratio/oscillator frequency
  OLED_Command(0x70); // Set displayed clock divide ratio/oscillator frequency
  OLED_Command(0x78); // OLED command set disabled
  OLED_Command(0x09); // Extended function set (4-Lines)
  OLED_Command(0x06); // COM SEG direction
  OLED_Command(0x72); // Function selection B
  OLED_Data(0x00);    // ROM CGRAM
  OLED_Command(0x28); // Function Set (fundamental command set)
  OLED_Command(0x79); // OLED command set enabled
  OLED_Command(0xDA); // Set SEG pins hardware configuration
  OLED_Command(0x00); // Set SEG pins hardware configuration
  OLED_Command(0xDC); // Function selection C
  OLED_Command(0x10); // Function selection C
  OLED_Command(0x81); // Contrast Control
  OLED_Command(0xFF); // Contrast value of 7F = 127/256
  OLED_Command(0xD9); // Set phase length
  OLED_Command(0xF1); // Set phase length
  OLED_Command(0xDB); // Set VCOMH deselect level
  OLED_Command(0x40); // Set VCOMH deselect level
  OLED_Command(0x78); // OLED command set disabled
  functionSet(LINES_2_4, DOUBLEHEIGHT_OFF, CLEAR_EXREG_IS);
  OLED_Command(0x01); // Clear display
  OLED_gotoxy(0, 0);
  setDisplay(DISPLAY_ON, CURSOR_OFF, CURSOR_BLINK_OFF);
  _delay_ms(10);
}

void OLED_clrscr(void){
  OLED_Command(0x01); // Clear display
}

void OLED_Home(void){
  OLED_Command(0x02); // Return Home (0,0)
}

void OLED_Shift_R(void){
  OLED_Command(0b00011100); //shift Right
}

void OLED_Shift_L(void){
  OLED_Command(0b00011000); // shift Left
}

void OLED_Puts(const char *s){ // print string on lcd (no auto linefeed)
  register char c;
  while ((c = *s++)){
    OLED_Data(c);
  }

}
