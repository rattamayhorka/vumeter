// oled.h

#ifndef OLED_H
#define OLED_H


#define SCL PB6
#define SDI PB5
#define CS PB1


extern uint8_t _rowOffsets[4];

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


void putData(uint8_t data);
void OLED_Command(unsigned char data);
void OLED_Data(unsigned char data);
void functionSet(uint8_t lines, uint8_t doubleHeight, uint8_t extensionRegIS);
void setDisplay(uint8_t display, uint8_t cursor, uint8_t cursorBlink);
void OLED_gotoxy(int x, int y);
void OLED_Init(void);
void OLED_Puts(const char *s);

void OLED_clrscr(void);
void OLED_Home(void);

#endif // OLED_H
