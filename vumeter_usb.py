import serial

# Configura el puerto serie. Asegúrate de cambiar el puerto COMx según tu sistema.
ser = serial.Serial('/dev/ttyUSB0', 9600)  # Cambia a tu puerto serie y velocidad de baudios

try:
    while True:
        # Lee un byte desde el puerto serie
        byte = ser.read(1)

        # Convierte el byte a una cadena de caracteres utilizando ASCII
        char_value = byte.decode('ascii', errors='ignore')  # Ignorar caracteres no ASCII

        # Muestra el valor en la consola
        print(char_value, end='')
except KeyboardInterrupt:
    # Maneja la interrupción del teclado (Ctrl+C) para salir limpiamente
    ser.close()
    print("\nPuerto serie cerrado. Adiós.")
