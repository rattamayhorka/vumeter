#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>
#include "oled.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)
#define USART_BUFFER_SIZE 21
#define NUM_BUFFERS 4 // Número de buffers (artist, title, album)

volatile char artist_buffer[USART_BUFFER_SIZE];
volatile char title_buffer[USART_BUFFER_SIZE];
volatile char album_buffer[USART_BUFFER_SIZE];
volatile char year_buffer[USART_BUFFER_SIZE];

volatile uint8_t usart_buffer_index = 0;

volatile char *buffers[NUM_BUFFERS] = {artist_buffer, title_buffer, album_buffer,year_buffer};
int current_buffer_index = 0;

volatile uint8_t timer_counter = 0;
const uint8_t ResetThreshold = 30; // Umbral de tiempo en decenas de milisegundos (3 segundos)

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
    // Configura el temporizador (ajusta según tu microcontrolador)
    // En el ejemplo, se usa el temporizador 1 (TIMER1)
    TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler de 1024
    TCNT1 = 0; // Inicializa el contador
    TIMSK |= (1 << TOIE1); // Habilita la interrupción por desbordamiento del temporizador
}

uint16_t adc_read(void){
    // Leer el valor del ADC en el pin 0 del puerto A (PA0)
    ADCSRA |= (1 << ADSC); // Iniciar la conversión
    while (ADCSRA & (1 << ADSC)); // Esperar a que la conversión termine
    return ADC; // Devolver el valor convertido
}

void usart_transmit(unsigned char data) {
    while (!(UCSRA & (1 << UDRE))); // Esperar a que el registro de transmisión esté vacío
    UDR = data; // Enviar el dato
}

void OLED_print(void){
    // Borra la pantalla LCD
    OLED_clrscr();
    
    // Muestra el contenido de artist_buffer en la primera línea
    OLED_gotoxy(0, 0);
    OLED_Puts((char *)artist_buffer);
    
    // Muestra el contenido de title_buffer en la segunda línea
    OLED_gotoxy(0, 1);
    OLED_Puts((char *)title_buffer);
    
    // Muestra el contenido de album_buffer en la tercera línea
    OLED_gotoxy(0, 2);
    OLED_Puts((char *)album_buffer);
    
    OLED_gotoxy(0, 3);
    OLED_Puts((char *)year_buffer);
    // Reinicia los buffers y el índice
    artist_buffer[0] = '\0';
    title_buffer[0] = '\0';
    album_buffer[0] = '\0';
    year_buffer[0] = '\0';
    usart_buffer_index = 0;
    current_buffer_index = 0;
}

ISR(USART_RXC_vect){
    char received_data = UDR; // Leer el carácter recibido

    if (received_data == '\a'){
        // Borrar la pantalla LCD cuando se recibe '\a'
        OLED_clrscr();
        usart_buffer_index = 0;
    } else if (received_data == '\n'){
        // Cambiar al siguiente buffer cuando se recibe '\n'
        current_buffer_index++;

        // Si current_buffer_index es igual al número de buffers, significa que se han completado todos
        if (current_buffer_index == NUM_BUFFERS){
            OLED_print();
        } else{
            usart_buffer_index = 0; // Reiniciar el índice del buffer actual
        }
    } else{
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
    if (timer_counter >= ResetThreshold){
        // Si han pasado 3 segundos sin recibir datos, realiza una acción (por ejemplo, ir al inicio del LCD)
    }
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
  OLED_clrscr();
  OLED_gotoxy(0,0); OLED_Puts("Bienvenido...");
  OLED_gotoxy(0,1); OLED_Puts("'vumeter' creado por:");
  OLED_gotoxy(0,2); OLED_Puts("rattamayhorka");
  OLED_gotoxy(0,3); OLED_Puts("OCT-16-2023 00:00-PM");
  _delay_ms(5000);
  OLED_gotoxy(0,0);
  OLED_clrscr();
  timer_init();
  sei();
}

int main(void){
  boot();
  uint16_t prevAdcValue = 255; //valor especifico para que no mande "ruido"
  while (1){
    uint16_t adcValue = adc_read();
    
    if (adcValue != prevAdcValue){  //transmite unicamente cuando cambia de valor el ADC
           set_volume(adcValue);
           usart_transmit('\n');
           prevAdcValue = adcValue; //mantiene el valor de ADC para no enviar ruido
    }
    _delay_ms(10);
  }
}
