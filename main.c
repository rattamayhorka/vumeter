#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "lcd.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*8UL)-1)
#define USART_BUFFER_SIZE 19

volatile char usart_buffer[USART_BUFFER_SIZE];
volatile char received_data;

volatile uint8_t usart_buffer_index = 0;
uint8_t lcd_column = 0;
uint32_t prevVolume = 0;
volatile uint8_t timer_counter = 0;
const uint8_t ResetThreshold = 30; // Umbral de tiempo en decenas de milisegundos (3 segundos)

void usart_init(void) {
    UBRRH = (BAUDRATE>>8); // Configurar la velocidad de comunicación en 9600 bps
    UBRRL = BAUDRATE;
    // Habilitar la transmisión y la recepción UART, así como la interrupción de recepción
    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);    //se agrego rxcie para usar como interrupciones
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // Configurar el formato de trama: 8 bits de datos, 1 bit de parada
}
void adc_init(void) {
    // Configurar el ADC para el pin 0 del puerto A (PA0)
    ADMUX = (1 << REFS0); // Usar AVCC como referencia y configurar el canal a PA0
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Habilitar ADC y configurar el prescaler
}

void timer_init(void) {
    // Configura el temporizador (ajusta según tu microcontrolador)
    // En el ejemplo, se usa el temporizador 1 (TIMER1)
    TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler de 1024
    TCNT1 = 0; // Inicializa el contador
    TIMSK |= (1 << TOIE1); // Habilita la interrupción por desbordamiento del temporizador
}

uint16_t adc_read(void) {
    // Leer el valor del ADC en el pin 0 del puerto A (PA0)
    ADCSRA |= (1 << ADSC); // Iniciar la conversión
    while (ADCSRA & (1 << ADSC)); // Esperar a que la conversión termine
    return ADC; // Devolver el valor convertido
}

void usart_transmit(unsigned char data) {
    while (!(UCSRA & (1 << UDRE))); // Esperar a que el registro de transmisión esté vacío
    UDR = data; // Enviar el dato
}

//Rutina de interrupción para USART (recepción completada)
ISR(USART_RXC_vect) {
    char received_data = UDR; // Leer el carácter recibido
    if (received_data == '\a'){
        lcd_clrscr();
    }
    else if (received_data == '\v'){
        lcd_home();
    }
    else if (received_data == '\n'){
        lcd_gotoxy(0,1);
    }
    else{
        usart_buffer[usart_buffer_index] = received_data; // Almacenar el carácter en el búfer
        usart_buffer_index++;
        lcd_putc(received_data); // Mostrar el carácter en el LCD
        timer_counter = 0; // Reiniciar el contador de tiempo cuando se recibe un carácter
    }
}

// Rutina de interrupción para el desbordamiento del temporizador
ISR(TIMER1_OVF_vect) {
    timer_counter++;
    if (timer_counter >= ResetThreshold) {
        // Si han pasado 3 segundos sin recibir datos, realiza una acción (por ejemplo, ir al inicio del LCD)
        lcd_clrscr();
    }
}

void set_volume(uint32_t adcValue) {
    char message[20];
    uint32_t volume = (uint32_t)((adcValue * 100) / 1024); //convierte adcValue a un valor de volumen en el rango de 0 a 100
    volume = (uint32_t)volume; // Convierte a uint32_t, para eliminar los decimales
    if (volume != prevVolume) { //si el valor de volumen no ha cambiado, no lo manda por USART
        sprintf(message, "%ld", volume); // Construir el mensaje con el valor de volumen
        for (int i = 0; message[i] != '\0'; i++) {
            usart_transmit(message[i]); // Enviar el mensaje por USART
        }
        prevVolume = volume;
    }
}

int main(void) {
    DDRC = 0xFF;
    lcd_init(LCD_DISP_ON);
    adc_init(); // Inicializar el ADC
    usart_init(); // Inicializar USART
    lcd_clrscr();
    lcd_gotoxy(0,0);lcd_puts("Bienvenido...");
    lcd_gotoxy(0,1);lcd_puts("'vumeter' creado por:");
    lcd_gotoxy(0,2);lcd_puts("rattamayhorka");
    lcd_gotoxy(0,3);lcd_puts("Septiembre - 2023");
    _delay_ms(5000);
    lcd_gotoxy(0,0);
    lcd_clrscr();
    timer_init();
    uint16_t prevAdcValue = 255;  // Un valor que no coincida con ninguna fila válida
    sei();
    while (1) {
        uint16_t adcValue = adc_read(); // Leer el valor del ADC

        if (adcValue != prevAdcValue) {
           set_volume(adcValue);
           usart_transmit('\n');
           prevAdcValue = adcValue;
        }
        _delay_ms(10); // Esperar 10 ms antes de actualizar la pantalla
    }
}
