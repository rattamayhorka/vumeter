import serial
import subprocess
import time

ser = serial.Serial('/dev/ttyUSB1', 9600)  # Configuración de puerto serie y baudios

try:
    char_buffer = ""  # Buffer para acumular caracteres

    while True:
        byte = ser.read(1) # Lee un byte desde el puerto serie
        char_value = byte.decode('ascii', errors='ignore')  # Ignorar caracteres no ASCII Convierte el byte a una cadena de caracteres utilizando ASCII

        if char_value.isdigit(): # Verifica si char_value es un dígito válido ('0'-'9')
            char_buffer += char_value # Acumula el carácter en el buffer
            #time.sleep(1)
        else:
            if char_buffer: # Si no es un dígito, verifica si hay caracteres en el buffer
                num_value = int(char_buffer)  # Convierte el buffer en un número entero
                if num_value > 99: # Ajusta num_value al rango de 0 a 99
                    num_value = 99
                result_str = str(num_value) # Convierte num_value a cadena sin formateo adicional
                subprocess.run(["pamixer", "--set-volume", result_str]) # Ejecuta el comando pamixer para establecer el volumen
                print(result_str)
                char_buffer = "" # Limpia el buffer
except KeyboardInterrupt:
    ser.close()    # Maneja la interrupción del teclado (Ctrl+C) para salir limpiamente
    print("\nPuerto serie cerrado. Adiós.")
