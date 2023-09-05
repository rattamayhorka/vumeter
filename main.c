#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*8UL)-1)


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

void adc_init(void) {
    // Configurar el ADC para el pin 0 del puerto A (PA0)
    ADMUX = (1 << REFS0); // Usar AVCC como referencia y configurar el canal a PA0
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Habilitar ADC y configurar el prescaler
}

uint16_t adc_read(void) {
    // Leer el valor del ADC en el pin 0 del puerto A (PA0)
    ADCSRA |= (1 << ADSC); // Iniciar la conversión
    while (ADCSRA & (1 << ADSC)); // Esperar a que la conversión termine
    return ADC; // Devolver el valor convertido
}

// Funciones para USART
void usart_init(void) {
    // Configurar la velocidad de comunicación en 9600 bps
    UBRRH = (BAUDRATE>>8);
    UBRRL = BAUDRATE;

    // Habilitar la transmisión y la recepción UART
    UCSRB = (1 << TXEN) | (1 << RXEN);

    // Configurar el formato de trama: 8 bits de datos, 1 bit de parada
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

void usart_transmit(unsigned char data) {
    // Esperar a que el registro de transmisión esté vacío
    while (!(UCSRA & (1 << UDRE)));

    // Enviar el dato
    UDR = data;
}

void send_volume_message(uint8_t row) {
    char message[20];

    // Construir el mensaje basado en el valor de 'row'
    if (row == 3) {
        sprintf(message, "volumen de nivel 3");
    } else if (row == 2) {
        sprintf(message, "volumen de nivel 2");
    } else if (row == 1) {
        sprintf(message, "volumen de nivel 1");
    } else if (row == 0) {
        sprintf(message, "volumen de nivel 0");
    } else {
        sprintf(message, "Valor no válido");
    }

    // Enviar el mensaje por USART
    for (int i = 0; message[i] != '\0'; i++) {
        usart_transmit(message[i]);
    }
}

int main(void) {
    DDRC = 0xFF;
    lcd_init(LCD_DISP_ON);
    adc_init(); // Inicializar el ADC
    usart_init(); // Inicializar USART
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
        uint8_t charIndex;
        int row;

        if (adcValue >= 0 && adcValue <= 255) {
            charIndex = adcValue / 32; // Rango 0-255 dividido en 8 segmentos
            row = 3; // Asignar fila 1
        } else if (adcValue >= 256 && adcValue <= 511) {
            charIndex = (adcValue - 256) / 32; // Rango 256-511 dividido en 8 segmentos
            row = 2; // Asignar fila 2
        } else if (adcValue >= 512 && adcValue <= 767) {
            charIndex = (adcValue - 512) / 32; // Rango 512-767 dividido en 8 segmentos
            row = 1; // Asignar fila 3
        } else if (adcValue >= 768 && adcValue <= 1023) {
            charIndex = (adcValue - 768) / 32; // Rango 768-1023 dividido en 8 segmentos
            row = 0; // Asignar fila 4
        } else {
            charIndex = 8; // Valor predeterminado para valores fuera de los rangos
            row = 3; // Valor predeterminado para valores fuera de los rangos
        }
        
        for (int col = 0; col < 20; col++) {
            lcd_gotoxy(col, row); // Mover a la fila correspondiente
            lcd_data(charIndex); // Mostrar el carácter especial correspondiente en la posición actual
            // Enviar el valor leído por USART
            //usart_transmit(charIndex);
        }

        // Enviar el mensaje de volumen por USART
        send_volume_message(row);

        _delay_ms(10); // Esperar 10 ms antes de actualizar la pantalla
    }
}
