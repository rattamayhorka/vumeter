#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "lcd.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*8UL)-1)
#define BUFFER_SIZE 64










































volatile char buffer[BUFFER_SIZE];
volatile uint8_t buffer_index = 0;

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

unsigned char usart_receive(void) {
    // Esperar a recibir datos
    while (!(UCSRA & (1 << RXC)));
    // Devolver el dato recibido
    return UDR;
}















int main(void) {
   
    lcd_init(LCD_DISP_ON); // Inicializa el LCD HD44780

    usart_init();
    lcd_clrscr();
   

    sei(); // Habilita las interrupciones globales






    lcd_puts("USART to LCD");
    //lcd_gotoxy(1, 0);

    while (1) {
        const char received_data = usart_receive();
        lcd_puts(&received_data); // Mostrar el dato en el LCD
    }

}
