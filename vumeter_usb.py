import serial
import subprocess

# Configura el puerto serie. Asegúrate de cambiar el puerto COMx según tu sistema.
ser = serial.Serial('/dev/ttyUSB0', 9600)  # Cambia a tu puerto serie y velocidad de baudios

try:
    char_buffer = ""  # Buffer para acumular caracteres

    while True:
        # Lee un byte desde el puerto serie
        byte = ser.read(1)

        # Convierte el byte a una cadena de caracteres utilizando ASCII
        char_value = byte.decode('ascii', errors='ignore')  # Ignorar caracteres no ASCII

        # Verifica si char_value es un dígito válido ('0'-'9')
        if char_value.isdigit():
            # Acumula el carácter en el buffer
            char_buffer += char_value
        else:
            # Si no es un dígito, verifica si hay caracteres en el buffer
            if char_buffer:
                # Convierte el buffer en un número entero
                num_value = int(char_buffer)
                
                # Ajusta num_value al rango de 0 a 99
                if num_value > 99:
                    num_value = 99

                # Convierte num_value a cadena sin formateo adicional
                result_str = str(num_value)

                # Muestra el resultado en la consola
                print(result_str, end='')

                # Ejecuta el comando pamixer para establecer el volumen
                subprocess.run(["pamixer", "--set-volume", result_str])

                # Limpia el buffer
                char_buffer = ""

            # Muestra el valor original si no es un dígito
            print(char_value, end='')

except KeyboardInterrupt:
    # Maneja la interrupción del teclado (Ctrl+C) para salir limpiamente
    ser.close()
    print("\nPuerto serie cerrado. Adiós.")
