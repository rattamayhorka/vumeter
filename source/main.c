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

#define DISPLAY_WIDTH 20
#define DISPLAY_HEIGHT 4

#define COMPARE_VALUE 15624

volatile uint8_t interrupt_timer_counter = 0;
volatile uint8_t usart_buffer_index = 0;

const char buffer_saludo[10] = { 'H','E','C','H','O',' ','P','O','R',':'};
const char buffer_nombre[14] = { 'R','A','T','T','A','M','A','Y','H','O','R','K','A','.'};


static uint8_t switch_next = 0; //next
static uint8_t switch_prev = 0; //prev
static uint8_t switch_play = 0; //play
static uint8_t switch_mute = 0; //mute

char artist_buffer[USART_BUFFER_SIZE];
char title_buffer[USART_BUFFER_SIZE];
char album_buffer[USART_BUFFER_SIZE];
char year_buffer[USART_BUFFER_SIZE];

char *buffers[NUM_BUFFERS] = {artist_buffer, title_buffer, album_buffer,year_buffer};

int current_buffer_index = 0;
int aState_1;
int aLastState_1;

int read_encoder(void){
    return ((PIND & (1 << PD2)) >> PD2) | (((PIND & (1 << PD3)) >> PD3) << 1);
}

void usart_init(void){
    UBRRH = (BAUDRATE>>8); // Configurar la velocidad de comunicación en 9600 bps
    UBRRL = BAUDRATE;
    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);    // Habilitar la transmisión y la recepción UART, así como la interrupción de recepción
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // Configurar el formato de 
}

void usart_transmit(unsigned char data){
    while (!(UCSRA & (1 << UDRE))); // Esperar a que el registro de transmisión esté vacío
    UDR = data; // Enviar el dato
}

void ScrollBuffer(char *buffer, int bufferSize, int j) {
    char first_buffer[21];
    strncpy(first_buffer, buffer, 20);  // Copia los primeros 20 caracteres de buffer a first_buffer
    first_buffer[20] = '\0';  // Asegura que first_buffer sea una cadena válida de C
    OLED_gotoxy(0, 0);

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
            _delay_ms(300);
            
        }
    }

    OLED_gotoxy(0, j);
    OLED_Puts(first_buffer);
}

void PrintBuffer(char *buffer, int j) {
    for (int i = 0; buffer[i] != '\0'; i++) {
        //correccion de caracteres
        if (buffer[i] == '{') {
            buffer[i] = 0xFD;
        } else if (buffer[i] == '}') {
            buffer[i] = 0xFF;
        } else if (buffer[i] == '[') {
            buffer[i] = 0xFA;
        } else if (buffer[i] == ']') {
            buffer[i] = 0xFC;
        } else if (buffer[i] == '_') {
            buffer[i] = 0xC4;
        } else if (buffer[i] == 0x17) {
            buffer[i] = 0x80;
        }
    }
    OLED_gotoxy(0, j);
    OLED_Puts(buffer);
}

void OLED_print(void){
    // Borra la pantalla LCD
    OLED_clrscr();
    OLED_gotoxy(0,0);

    PrintBuffer(artist_buffer, 0);
    PrintBuffer(title_buffer, 1);
    PrintBuffer(album_buffer, 2);
    PrintBuffer(year_buffer, 3);
    
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

void timer_init(void) {
    //TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler de 64
    TCCR1B |= (1 << CS10);
    
    TCNT1 = 65536 - 62500; // Valor inicial para 100 milisegundos con prescaler 64
    //TCNT1 = 65472; // (65536 - 64) para 1 Hz
    
    TIMSK |= (1 << TOIE1); // Habilita la interrupción por desbordamiento del temporizador
}

ISR(TIMER1_OVF_vect) {
    static uint16_t counter = 0;

    if (counter < 1) {
        PORTD |= (1 << PD6); // Enciende el LED
    } else {
        PORTD &= ~(1 << PD6); // Apaga el LED
    }

    counter++;

    if (counter == 300) {
        counter = 0; // Reinicia el contador
    }
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

void ports_init(void){
    DDRB |= (1 << PB6) | (1 << PB5) | (1 << PB1); //configurar PB1 as CS, PB5 [oled 8] as SDI, pb6 as SCL [oled 7] 

    DDRD |= (1 << PD6); //configurar PD6 como salida LED heartbeat
        
    DDRC &= ~(1 << PD7); // botones multimedia (play)
    DDRC &= ~(1 << PC0); // botones multimedia (next)
    DDRC &= ~(1 << PC1); // botones multimedia (prev)

    DDRD &= ~(1 << PD4);  // PD4 como entrada (SW - encoder)
    DDRD &= ~(1 << PD3);  // PD3 como entrada (DT - encoder)
    DDRD &= ~(1 << PD2);  // PD2 como entrada (CLK - encoder)

    PORTD |= (1 << PD2) | (1 << PD3);  // Habilitar resistencias de pull-up
}

void boot(void){ //funcion de inicio
    ports_init();
    OLED_Init(); // Inicializar OLED
    timer_init();
    boot_splash();
    usart_init(); // Inicializar USART    
    sei();
    PORTD |= (1 << PD6);    
}

int main(void){
    boot();
    
    OLED_gotoxy(0,0); OLED_Puts("Esperando");
    OLED_gotoxy(0,1); OLED_Puts("Conexion a PC...");
    //iniciar timer

    while (1) {
        aState_1 = read_encoder();

        if (aState_1 != aLastState_1) {
            if ((aLastState_1 == 0b00 && aState_1 == 0b01) || (aLastState_1 == 0b01 && aState_1 == 0b11) ||
    (aLastState_1 == 0b11 && aState_1 == 0b10) || (aLastState_1 == 0b10 && aState_1 == 0b00)) {
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

        }

        aLastState_1 = aState_1;
        if (!(PINC & (1 << PC0))) {
            if (switch_next == 0) {
                // El estado del interruptor ha cambiado a presionado
                char debug_message[20];
                sprintf(debug_message, "next\n");
                for (int i = 0; debug_message[i] != '\0'; i++) {
                    usart_transmit(debug_message[i]);
                }
                switch_next = 1;
            }
        } else {
            switch_next = 0;
        }

        if (!(PINC & (1 << PC1))) {
            if (switch_prev == 0) {
                // El estado del interruptor ha cambiado a presionado
                char debug_message[20];
                sprintf(debug_message, "prev\n");
                for (int i = 0; debug_message[i] != '\0'; i++) {
                    usart_transmit(debug_message[i]);
                }
                switch_prev = 1;
            }
        } else {
            switch_prev = 0;
        }

        if (!(PIND & (1 << PD4))) {
            if (switch_mute == 0) {
                // El estado del interruptor ha cambiado a presionado
                char debug_message[20];
                sprintf(debug_message, "-t 1\n");
                for (int i = 0; debug_message[i] != '\0'; i++) {
                    usart_transmit(debug_message[i]);
                }
                switch_mute = 1;
            }
        } else {
            switch_mute = 0;
        }

        if (!(PIND & (1 << PD7))) {
            if (switch_play == 0) {
                // El estado del interruptor ha cambiado a presionado
                char debug_message[20];
                sprintf(debug_message, "play\n");
                for (int i = 0; debug_message[i] != '\0'; i++) {
                    usart_transmit(debug_message[i]);
                }
                switch_play = 1;
            }
        } else {
            switch_play = 0;
        }
        _delay_ms(5);  // Pequeña pausa para evitar lecturas rápidas del encoder
        
    }
}