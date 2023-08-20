#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"

const uint8_t customPatterns[8][8] = {
    // Carácter 1
    { 0b00000000, 0b11111111, 0b11111111, 0b11111111,
      0b11111111, 0b11111111, 0b11111111, 0b11111111 },

    // Carácter 2
    { 0b00000000, 0b00000000, 0b11111111, 0b11111111,
      0b11111111, 0b11111111, 0b11111111, 0b11111111 },

    // Carácter 3
    { 0b00000000, 0b00000000, 0b00000000, 0b11111111,
      0b11111111, 0b11111111, 0b11111111, 0b11111111 },

    // Carácter 4
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000,
      0b11111111, 0b11111111, 0b11111111, 0b11111111 },

    // Carácter 5
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000,
      0b00000000, 0b11111111, 0b11111111, 0b11111111 },

    // Carácter 6
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000,
      0b00000000, 0b000000000, 0b11111111, 0b11111111 },

    // Carácter 7
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000,
      0b00000000, 0b000000000, 0b00000000, 0b11111111 },

    // Carácter 8
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000,
      0b00000000, 0b000000000, 0b0000000, 0b00000000 },


};


int main(void) {
    lcd_init(LCD_DISP_ON);
    
    // Cargar los patrones en la memoria de caracteres
    lcd_command(0x40); // Iniciar en la dirección 0x40 (ubicación de caracteres especiales)
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            lcd_data(customPatterns[i][j]);
        }
    }

    while (1) {
        lcd_clrscr();
        
        for (int charIndex = 0; charIndex < 8; charIndex++) {
            for (int col = 0; col < 16; col++) {
                // Mostrar el carácter especial en la fila de arriba
                lcd_command(0x80 | col); // Mover el cursor a la fila de arriba y la columna actual
                lcd_data(charIndex); // Escribir el carácter especial
                
                // Mostrar el carácter especial en la fila de abajo
                lcd_command(0xC0 | col); // Mover el cursor a la fila de abajo y la columna actual
                lcd_data(charIndex); // Escribir el carácter especial
            }
            _delay_ms(500); // Esperar medio segundo antes de mostrar el siguiente carácter
        }
    }
}
