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

#define PD2_MASK (1 << PD2)
#define PD3_MASK (1 << PD3)

char artist_buffer[USART_BUFFER_SIZE];
char title_buffer[USART_BUFFER_SIZE];
char album_buffer[USART_BUFFER_SIZE];
char year_buffer[USART_BUFFER_SIZE];
char message[10];


char *buffers[NUM_BUFFERS] = {artist_buffer, title_buffer, album_buffer,year_buffer};

volatile uint8_t usart_buffer_index = 0;

int current_buffer_index = 0;
int j = 0;
int i = 0;

volatile uint8_t interrupt_timer_counter = 0;
//const uint8_t ResetThreshold = 30; // Umbral de tiempo en decenas de milisegundos (1 segundos)

uint32_t prevVolume = 0;

const char buffer_saludo[10] = { 'H','E','C','H','O',' ','P','O','R',':'};
const char buffer_nombre[14] = { 'R','A','T','T','A','M','A','Y','H','O','R','K','A','.'};

int counter_1 = 0;
int aState_1;
int aLastState_1;

void init_encoder(void){
    DDRD &= ~PD2_MASK;  // PD2 como entrada (CLK)
    DDRD &= ~PD3_MASK;  // PD3 como entrada (DT)
    PORTD |= PD2_MASK | PD3_MASK;  // Habilitar resistencias de pull-up
}

int read_encoder(void){
    return ((PIND & PD2_MASK) >> PD2) | (((PIND & PD3_MASK) >> PD3) << 1);
}

void usart_init(void){
    UBRRH = (BAUDRATE>>8); // Configurar la velocidad de comunicación en 9600 bps
    UBRRL = BAUDRATE;
    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);    // Habilitar la transmisión y la recepción UART, así como la interrupción de recepción
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // Configurar el formato de 
}


void timer_init(void){
    TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler de 64
    TCNT1 = 65472; // (65536 - 64) para 1 Hz
    TIMSK |= (1 << TOIE1); // Habilita la interrupción por desbordamiento del temporizador
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
        } else if (buffer[i] == 0xC2) {
            buffer[i] = 0x80;
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
        interrupt_timer_counter = 0; // Reiniciar el contador de tiempo
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
        interrupt_timer_counter = 0; // Reiniciar el contador de tiempo cuando se recibe un carácter
    }
}

//Rutina de interrupción para el desbordamiento del temporizador
ISR(TIMER1_OVF_vect){
    interrupt_timer_counter++;
    PORTD ^= (1<<PD6);
    
}

void boot_splash(void){
    //bloques
    OLED_gotoxy(0,0); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x1F);
        _delay_ms(50);    
    }//BLOQUES ANTES DE HECHO POR
    OLED_gotoxy(0,1); 
    for(int i = 0; i < 5; ++i){
        OLED_Data(0x1F);
        _delay_ms(50);    
    }//HECHO POR
    for (int i = 0; i < 10; ++i){
        OLED_Data(buffer_saludo[i]);
        _delay_ms(50);
    }//BLOQUES DESPUES DE HECHO POR
    for(int i = 0; i < 6; ++i){
        OLED_Data(0x1F);
        _delay_ms(50);    
    }//BLOQUES ANTES DE RATTAMAYHORKA
    OLED_gotoxy(0,2);
    for(int i = 0; i < 3; ++i){
        OLED_Data(0x1F);
        _delay_ms(50);    
    }//RATTAMAYHORKA
    for (int i = 0; i < 14; ++i){
        OLED_Data(buffer_nombre[i]);
        _delay_ms(50);
    }//BLOQUES DESPUES DE RATTAMAYHORKA
    for(int i = 0; i < 4; ++i){
        OLED_Data(0x1F);
        _delay_ms(50);    
    }//BLOQUE DE TODA LA LINEA 4
    OLED_gotoxy(0,3); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x1F);
        _delay_ms(50);    
    }///BORRADO DE PRIMERA LINEA
    OLED_gotoxy(0,0); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }//BORRADO ANTES DE HECHO POR
    OLED_gotoxy(0,1); 
    for(int i = 0; i < 5; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }//HECHO POR
    for (int i = 0; i < 10; ++i){
        OLED_Data(buffer_saludo[i]);
        _delay_ms(50);
    }//BORRADO DESPUES DE HEHOC POR
    for(int i = 0; i < 6; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }//BORRADO ANTES DE RATTAMAYHORKA
    OLED_gotoxy(0,2);
    for(int i = 0; i < 3; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }

    for (int i = 0; i < 14; ++i){
        OLED_Data(buffer_nombre[i]);
        _delay_ms(50);
    }

    for(int i = 0; i < 4; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }

    OLED_gotoxy(0,3); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }

    OLED_gotoxy(0,0); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }

    OLED_gotoxy(0,1); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }
    OLED_gotoxy(0,2); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }
    OLED_gotoxy(0,3); 
    for(int i = 0; i < 20; ++i){
        OLED_Data(0x20);
        _delay_ms(50);    
    }
    OLED_gotoxy(0,0);
}    

void boot(void){ //funcion de inicio
    DDRB |= (1 << PB6) | (1 << PB5) | (1 << PB1); //configurar pb0,pb1,pb5 como salidas para el OLED
    DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD6); //configurar PD6 como salida LED de status
    
    DDRD &= ~(1 << PC0); //entradas
    DDRD &= ~(1 << PC1); //entradas
    DDRD &= ~(1 << PC2); //entradas

    OLED_Init(); // Inicializar OLED
    timer_init();
    //boot_splash();
    usart_init(); // Inicializar USART    
    sei();
    PORTD |= (1 << PD6);    
}
int main(void){
    boot();
    OLED_gotoxy(0,0); OLED_Puts("Esperando");
    OLED_gotoxy(0,1); OLED_Puts("conexion a PC...");
 
    while (1) {
        aState_1 = read_encoder();

        if (aState_1 != aLastState_1) {
            if ((aLastState_1 == 0b00 && aState_1 == 0b01) || (aLastState_1 == 0b01 && aState_1 == 0b11) ||
    (aLastState_1 == 0b11 && aState_1 == 0b10) || (aLastState_1 == 0b10 && aState_1 == 0b00)) {
                //counter_1++;
                char debug_message[20];
                sprintf(debug_message, "-i 1\n");
                for (int i = 0; debug_message[i] != '\0'; i++) {
                    usart_transmit(debug_message[i]);
                }

            } 
        }
        if ((aLastState_1 == 0b00 && aState_1 == 0b10) || (aLastState_1 == 0b01 && aState_1 == 0b00) ||
    (aLastState_1 == 0b11 && aState_1 == 0b01) || (aLastState_1 == 0b10 && aState_1 == 0b11)) {
                char debug_message[20];
                sprintf(debug_message, "-d 1\n");
                for (int i = 0; debug_message[i] != '\0'; i++) {
                    usart_transmit(debug_message[i]);
                }
                //usart_transmit("-d 1");
        }

        aLastState_1 = aState_1;
        _delay_ms(5);  // Pequeña pausa para evitar lecturas rápidas del encoder
    }
}
