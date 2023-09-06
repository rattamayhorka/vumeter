import serial
import subprocess

# Configura el puerto serie. Asegúrate de cambiar el puerto COMx según tu sistema.
ser = serial.Serial('/dev/ttyUSB0', 9600)  # Cambia a tu puerto serie y velocidad de baudios

try:
    while True:
        # Lee un byte desde el puerto serie
        byte = ser.read(1)

        # Convierte el byte a una cadena de caracteres utilizando ASCII
        char_value = byte.decode('ascii', errors='ignore')  # Ignorar caracteres no ASCII

        # Verifica si char_value es un dígito válido ('0'-'9')
        if char_value.isdigit():
            # Convierte el carácter ASCII a un número entero
            num_value = int(char_value)
            
            # Ajusta num_value al rango de 0 a 10
            if num_value > 10:
                num_value = 10

            # Multiplica num_value por 10 para obtener el resultado en el rango de 0 a 100
            result = num_value * 10

            # Formatea result como una cadena con dos dígitos
            result_str = "{:02d}".format(result)

            # Muestra el resultado en la consola
            print(result_str, end='')

            # Ejecuta el comando pamixer para establecer el volumen
            subprocess.run(["pamixer", "--set-volume", result_str])
        else:
            # Si no es un dígito, muestra el valor original
            print(char_value, end='')

except KeyboardInterrupt:
    # Maneja la interrupción del teclado (Ctrl+C) para salir limpiamente
    ser.close()
    print("\nPuerto serie cerrado. Adiós.")
