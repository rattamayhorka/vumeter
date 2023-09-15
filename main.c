#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "lcd.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU) / (BAUD * 8UL) - 1)
#define BUFFER_SIZE 64

volatile char buffer[BUFFER_SIZE];
volatile uint8_t buffer_index = 0;
volatile uint16_t no_data_timer = 0; // Variable para rastrear el tiempo sin datos

// Funciones para USART
void usart_init(void) {
    // Configurar la velocidad de comunicación en 9600 bps
    UBRRH = (BAUDRATE >> 8);
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

unsigned char usart_receive(void) {
    // Esperar a recibir datos
    while (!(UCSRA & (1 << RXC)));
    // Reiniciar el temporizador
    no_data_timer = 0;
    // Devolver el dato recibido
    return UDR;
}

int main(void) {
    lcd_init(LCD_DISP_ON); // Inicializa el LCD HD44780
    usart_init();
    lcd_clrscr();
    lcd_gotoxy(0, 0); // Coloca el cursor en la posición (0,0)
    lcd_puts("USART to LCD");

    while (1) {
        if (UCSRA & (1 << RXC)) {
            lcd_clrscr(); // Borrar la pantalla LCD
            lcd_gotoxy(0, 0); // Coloca el cursor en la posición (0,0)
            const char received_data = usart_receive();
            lcd_puts(&received_data); // Mostrar el dato en el LCD
        }
        else {
            // Incrementar el temporizador si no se ha recibido ningún dato
            _delay_ms(100); // Esperar 100 ms
            no_data_timer += 100; // Incrementar el temporizador en 100 ms
            if (no_data_timer >= 3000) { // Si han pasado 3 segundos sin datos
                lcd_puts("USART to LCD"); // Vuelve a mostrar el mensaje inicial
                no_data_timer = 0; // Reiniciar el temporizador
            }
        }
    }
}
