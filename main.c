#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>
#include "oled.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)
#define USART_BUFFER_SIZE 100
#define NUM_BUFFERS 4 //Número de buffers (artist, title, album)
#define HISTERESIS 1 //Número que se maneja para eliminar el error en la entrada del ADC 
char artist_buffer[USART_BUFFER_SIZE];
char title_buffer[USART_BUFFER_SIZE];
char album_buffer[USART_BUFFER_SIZE];
char year_buffer[USART_BUFFER_SIZE];

volatile uint8_t usart_buffer_index = 0;

char *buffers[NUM_BUFFERS] = {artist_buffer, title_buffer, album_buffer,year_buffer};
int current_buffer_index = 0;
int j = 0;
volatile uint8_t timer_counter = 0;
const uint8_t ResetThreshold = 30; // Umbral de tiempo en decenas de milisegundos (1 segundos)

uint32_t prevVolume = 0;

void usart_init(void){
    UBRRH = (BAUDRATE>>8); // Configurar la velocidad de comunicación en 9600 bps
    UBRRL = BAUDRATE;
    // Habilitar la transmisión y la recepción UART, así como la interrupción de recepción
    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);    //se agrego rxcie para usar como interrupciones
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // Configurar el formato de 
}

void adc_init(void){
    // Configurar el ADC para el pin 0 del puerto A (PA0)
    ADMUX = (1 << REFS0); // Usar AVCC como referencia y configurar el canal a PA0
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Habilitar ADC y configurar el prescaler
}

void timer_init(void){
    TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler de 64
    TCNT1 = 65472; // (65536 - 64) para 1 Hz
    TIMSK |= (1 << TOIE1); // Habilita la interrupción por desbordamiento del temporizador
}

uint16_t adc_read(void){
    // Leer el valor del ADC en el pin 0 del puerto A (PA0)
    ADCSRA |= (1 << ADSC); // Iniciar la conversión
    while (ADCSRA & (1 << ADSC)); // Esperar a que la conversión termine
    return ADC; // Devolver el valor convertido
}

void usart_transmit(unsigned char data){
    while (!(UCSRA & (1 << UDRE))); // Esperar a que el registro de transmisión esté vacío
    UDR = data; // Enviar el dato
}

void scrollBuffer(char *buffer, int bufferSize, int j) {
    char first_buffer[21];
    strncpy(first_buffer, buffer, 20);  // Copia los primeros 20 caracteres de buffer a first_buffer
    first_buffer[20] = '\0';  // Asegura que first_buffer sea una cadena válida de C
    OLED_gotoxy(0, 0);

    // Itera sobre la cadena y reemplaza los caracteres específicos
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '{') {
            buffer[i] = 0xFD;
        } else if (buffer[i] == '}') {
            buffer[i] = 0xFF;
        } else if (buffer[i] == '[') {
            buffer[i] = 0xFA;
        } else if (buffer[i] == ']') {
            buffer[i] = 0xFC;
        }
    }

    if (strlen(buffer) <= bufferSize) {
        OLED_gotoxy(0, j);
        OLED_Puts(buffer);
    } else {
        int len = strlen(buffer);
        char temp[21];  // Buffer temporal para el desplazamiento
        for (int i = 0; i <= len - bufferSize; i++) {
            strncpy(temp, buffer + i, bufferSize);  // Copia el siguiente segmento de 20 caracteres
            temp[bufferSize] = '\0';  // Asegura que sea una cadena válida de C
            OLED_gotoxy(0, j);
            OLED_Puts(temp);
            _delay_ms(300);  // Pausa antes de desplazar
        }
    }

    OLED_gotoxy(0, j);
    OLED_Puts(first_buffer);
}

void OLED_print(void){
    // Borra la pantalla LCD
    OLED_clrscr();
    OLED_gotoxy(0,0);
    // Desplazar los buffers si su longitud es mayor a 20 caracteres
    
    scrollBuffer(artist_buffer, 20, 0);
    scrollBuffer(title_buffer, 20, 1);
    scrollBuffer(album_buffer, 20, 2);
    scrollBuffer(year_buffer, 20, 3);

    // Reinicia los buffers y el índice
    artist_buffer[0] = '\0';
    title_buffer[0] = '\0';
    album_buffer[0] = '\0';
    year_buffer[0] = '\0';
    usart_buffer_index = 0;
    current_buffer_index = 0;
    OLED_gotoxy(0,0);
}

ISR(USART_RXC_vect) {
    char received_data = UDR; // Leer el carácter recibido

    if (received_data == '\a'){
        // Borrar la pantalla LCD y reiniciar todos los buffers
        for (int i = 0; i < NUM_BUFFERS; i++){
            buffers[i][0] = '\0'; // Reiniciar el buffer
        }
        usart_buffer_index = 0; // Reiniciar el índice del buffer actual
        current_buffer_index = 0; // Reiniciar el índice del buffer actual
        timer_counter = 0; // Reiniciar el contador de tiempo
    } else if (received_data == '\n') {
        // Cambiar al siguiente buffer cuando se recibe '\n'
        current_buffer_index++;

        // Si current_buffer_index es igual al número de buffers, significa que se han completado todos
        if (current_buffer_index == NUM_BUFFERS){
            OLED_print();
        } else {
            usart_buffer_index = 0; // Reiniciar el índice del buffer actual
        }
    } else {
        // Almacenar el carácter en el buffer actual
        char *current_buffer = buffers[current_buffer_index];

        current_buffer[usart_buffer_index] = received_data;
        usart_buffer_index++;

        // Asegurarse de que el buffer no exceda su tamaño máximo
        if (usart_buffer_index >= USART_BUFFER_SIZE){
            usart_buffer_index = USART_BUFFER_SIZE - 1;
        }

        // Terminar la cadena en el buffer para que sea una cadena válida de C
        current_buffer[usart_buffer_index] = '\0';
        timer_counter = 0; // Reiniciar el contador de tiempo cuando se recibe un carácter
    }
}

//Rutina de interrupción para el desbordamiento del temporizador
ISR(TIMER1_OVF_vect){
    timer_counter++;
    PORTB ^= (1<<PB0);    
}

//funcion de manejo de volumen
void set_volume(uint32_t adcValue){
    char message[20];
    uint32_t volume = (uint32_t)((adcValue * 100) / 1024); //convierte adcValue a un valor de volumen en el rango de 0 a 100
    volume = (uint32_t)volume; // Convierte a uint32_t, para eliminar los decimales
    if (volume != prevVolume) { //si el valor de volumen no ha cambiado, no lo manda por USART
        sprintf(message, "%ld", volume); // Construir el mensaje con el valor de volumen de numeros a caracteres.
        for (int i = 0; message[i] != '\0'; i++) {
            usart_transmit(message[i]); // Enviar el mensaje por USART
        }
        prevVolume = volume;
    }
}

void boot(void){ //funcion de inicio
    DDRB |= (1 << PB6) | (1 << PB5) | (1 << PB1) | (1 << PB0); //configurar pb0,pb1,pb5 como salidas
    DDRC = 0xFF; // Inicializar puerto C como salida USART
    OLED_Init(); // Inicializar OLED
    adc_init(); // Inicializar el ADC
    usart_init(); // Inicializar USART
    //OLED_clrscr();
    
    OLED_Data(0b11111010);
    OLED_gotoxy(0,0); OLED_Puts("Bienvenido...");
    OLED_gotoxy(0,1); OLED_Puts("'vumeter' creado por:");
    OLED_gotoxy(0,2); OLED_Puts("@rattamayhorka");
    OLED_gotoxy(0,3); OLED_Puts("NOV-02-2023 22:00 PM");

    _delay_ms(5000);
    OLED_gotoxy(0,0);
    OLED_clrscr();
    timer_init();
    sei();
    PORTB |= (1 << PB0);
}

int main(void){
    boot();
    OLED_gotoxy(0,0); OLED_Puts("Esperando");
    OLED_gotoxy(0,1); OLED_Puts("conexion a PC...");

    uint16_t prevAdcValue = 255; //valor especifico para que no mande "ruido"

    while (1){
        uint16_t adcValue = adc_read();
    
        if (adcValue + HISTERESIS <= prevAdcValue || adcValue - HISTERESIS >= prevAdcValue ){
            set_volume(adcValue);
            usart_transmit('\n');
        }
        prevAdcValue = adcValue +2 ; //mantiene el valor de ADC para no enviar ruido
        _delay_ms(100);
    }
}