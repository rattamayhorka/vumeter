#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"

const uint8_t customPatterns[9][8] = {


    // Carácter 8
    { 0b11111111, 0b11111111, 0b11111111, 0b11111111,
      0b11111111, 0b11111111, 0b11111111, 0b11111111 },
    
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
    { 0b11111111, 0b11111111, 0b11111111, 0b11111111,
      0b11111111, 0b11111111, 0b11111111, 0b11111111 },
};

void adc_init() {
    // Configurar el ADC para el pin 0 del puerto A (PA0)
    ADMUX = (1 << REFS0); // Usar AVCC como referencia y configurar el canal a PA0
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Habilitar ADC y configurar el prescaler
}

uint16_t adc_read() {
    // Leer el valor del ADC en el pin 0 del puerto A (PA0)
    ADCSRA |= (1 << ADSC); // Iniciar la conversión
    while (ADCSRA & (1 << ADSC)); // Esperar a que la conversión termine
    return ADC; // Devolver el valor convertido
}

int main(void) {
    lcd_init(LCD_DISP_ON);
    adc_init(); // Inicializar el ADC
    lcd_clrscr();
    // Cargar los patrones en la memoria de caracteres
    lcd_command(0x40); // Iniciar en la dirección 0x40 (ubicación de caracteres especiales)
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 8; j++) {
            lcd_data(customPatterns[i][j]);
        }
    }

    while (1) {
        uint16_t adcValue = adc_read(); // Leer el valor del ADC en el pin 0 del puerto A (PA0)
        //uint8_t charIndex = adcValue >> 5; // Calcular el índice para el carácter especial
        if (adcValue < 512){
           uint8_t charIndex = adcValue * 9 / 512; // Calcular el índice para el carácter especial
              for (int col = 0; col < 16; col++) {
                 lcd_gotoxy(col, 0); // Fila de arriba
                 lcd_data(charIndex); // Mostrar el carácter especial en la posición actual
              }
        }
        if (adcValue >= 512){
           uint8_t charIndex = adcValue * 9 / 512; // Calcular el índice para el carácter especial
              for (int col = 0; col < 16; col++) {
                 lcd_gotoxy(col, 0); // Fila de arriba
                 lcd_data(9); // Mostrar el carácter especial en la posición actual
                 lcd_gotoxy(col, 1); // Fila de arriba
                 lcd_data(charIndex); // Mostrar el carácter especial en la posición actual
              }
        }
        _delay_ms(100); // Esperar 200 ms antes de actualizar la pantalla
    }
}
