# Vumeter

Vumeter, es un proyecto con el que puedes mostrar en un LCD hd44780 información en tiempo real como Artista y Cancíón que se está reproduciendo en CMUS, muy parecido a lo que era LCD Smartie, :D

Por implementar:
- vumetro (30%)
- notificaciónes (0%)
  
## Conexiones: 

los pines 3,4,5,6 del atmega, van a los 4 bits mas significativos (4,5,6,7) del lcd 

pin LCD_PORT1         PORTB        /**< port for the LCD lines   */

pin LCD_PORT2         PORTB


pin LCD_DATA0_PORT   LCD_PORT2     // port for 4bit data bit 0 

pin LCD_DATA1_PORT   LCD_PORT2     // port for 4bit data bit 1

pin LCD_DATA2_PORT   LCD_PORT2     // port for 4bit data bit 2

pin LCD_DATA3_PORT   LCD_PORT2     // port for 4bit data bit 3

pin LCD_DATA0_PIN    3            // pin for 4bit data bit 0  
pin LCD_DATA1_PIN    4            // pin for 4bit data bit 1  
pin LCD_DATA2_PIN    5            // pin for 4bit data bit 2  
pin LCD_DATA3_PIN    6            // pin for 4bit data bit 3  
pin LCD_RS_PORT      LCD_PORT1    // port for RS line        
pin LCD_RS_PIN       0            // pin  for RS line         
pin LCD_RW_PORT      LCD_PORT1    // port for RW line        
pin LCD_RW_PIN       1            // pin  for RW line         
pin LCD_E_PORT       LCD_PORT1    // port for Enable line    
pin LCD_E_PIN        2            // pin  for Enable line

# Vumeter - Tablero con Control de Volumen y Display OLED multimedia 

Vumeter, es un proyecto con el que puedes mostrar en un OLED, información en tiempo real como Artista y Cancíón que se está reproduciendo en Linux, muy parecido a lo que era LCD Smartie, :D

Este script de Python interactúa con reproductores multimedia en D-Bus y controla el volumen a través de un microcontrolador atmega32, tambien muestra información sobre el artista y el título de la canción.

 ## Requisitos

 - Python 3.x
 - [pydbus](https://pypi.org/project/pydbus/)
 - [unidecode](https://pypi.org/project/Unidecode/)
 - [pamixer](https://man.archlinux.org/man/pamixer.1)

 ## Configuración

 Asegúrate de tener todos los requisitos instalados antes de ejecutar el script.

 ```bash
 pip install pydbus unidecode
 ```

 ## Uso

 Ejecuta el script de la siguiente manera:

 ```bash
 python dbus_data_tx.py
 ```

 El script detectará reproductores multimedia activos y mostrará información sobre la canción en el display OLED. También permite ajustar el volumen mediante un puerto serie.

 ## Notas

 - Asegúrate de tener permisos para acceder al puerto serie (`/dev/ttyUSB0` en este caso).
 - Ajusta el puerto serie y el baudrate según tu configuración.
 - Ajusta la frecuencia de verificación y actualización según tus preferencias.

 ## Contribuciones

 ¡Contribuciones son bienvenidas! Si encuentras errores o mejoras posibles, no dudes en crear un *issue* o enviar un *pull request*.

 ## Autor

 [Nombre del autor]


