#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"

const uint8_t customPatterns[8][8] = {
    // Carácter 1
    { 0b00000, 0b00000, 0b00000, 0b00000,
      0b00000, 0b00000, 0b00000, 0b11111 },
    // Carácter 2
    { 0b00000, 0b00000, 0b00000, 0b00000,
      0b00000, 0b00000, 0b11111, 0b11111 },
    // Carácter 3
    { 0b00000, 0b00000, 0b00000, 0b00000,
      0b00000, 0b11111, 0b11111, 0b11111 },
    // Carácter 4
    { 0b00000, 0b00000, 0b00000, 0b00000,
      0b11111, 0b11111, 0b11111, 0b111111 },
    // Carácter 5
    { 0b00000, 0b00000, 0b00000, 0b11111,
      0b11111, 0b11111, 0b11111, 0b11111 },
    // Carácter 6
    { 0b00000, 0b00000, 0b11111, 0b11111,
      0b11111, 0b11111, 0b11111, 0b11111 },
    // Carácter 7
    { 0b00000, 0b11111, 0b11111, 0b11111,
      0b11111, 0b11111, 0b11111, 0b11111 },
    // Carácter 8
    { 0b11111, 0b11111, 0b11111, 0b11111,
      0b11111, 0b11111, 0b11111, 0b11111 },
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
    for (int i = 0; i < 8; i++) {
       lcd_command(0x40 + i * 8); // Moverse a la dirección de escritura correcta para el carácter i
       for (int j = 0; j < 8; j++) {
          lcd_data(customPatterns[i][j]); // Cargar el patrón en la memoria de caracteres
       }
    }
    while (1) {
        uint16_t adcValue = adc_read(); // Leer el valor del ADC
        uint8_t charIndex = ((adcValue * 7) / 1023); // Calcular el índice para el carácter especial
        for (int col = 0; col < 20; col++) {
           lcd_gotoxy(col, 0); // Fila 1
           lcd_data(charIndex); // Mostrar el carácter especial en la posición actual
           lcd_gotoxy(col, 1); // Fila 2
           lcd_data(charIndex); // Mostrar el carácter especial en la posición actual
           lcd_gotoxy(col, 2); // Fila 3
           lcd_data(charIndex); // Mostrar el carácter especial en la posición actual
           lcd_gotoxy(col, 3); // Fila 4
           lcd_data(charIndex); // Mostrar el carácter especial en la posición actual
        }
        if (charIndex > 7){
           charIndex = 7;
        }

    _delay_ms(100); // Esperar 100 ms antes de actualizar la pantalla
    }
}
