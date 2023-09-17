import subprocess
import serial
import time

# Configura el puerto serial. Asegúrate de cambiar el puerto COMx según tu sistema.
ser = serial.Serial('/dev/ttyUSB1', 9600)  # Cambia 'COMx' al puerto serial correcto y ajusta la velocidad de baudios según sea necesario

# Variables para almacenar el artista y el título anteriores
prev_artist = ""
prev_title = ""

while True:
    try:
        # Ejecuta el comando y captura la salida estándar
        artist = subprocess.check_output(
            'cmus-remote -Q 2> /dev/null | grep "tag artist " | cut -c12-',
            shell=True,
            text=True  # Asegura que la salida sea una cadena de texto (str)
        ).strip()  # Elimina espacios en blanco al principio y al final

        # Ejecuta el comando y captura la salida estándar
        title = subprocess.check_output(
            'cmus-remote -Q 2> /dev/null | grep "tag title " | cut -c11-',
            shell=True,
            text=True  # Asegura que la salida sea una cadena de texto (str)
        ).strip()  # Elimina espacios en blanco al principio y al final

        if artist != prev_artist or title != prev_title: # Verifica si artist o title han cambiado
            output = f"Now playing: \n{artist} - {title} "  # Concatena la información
            print(output)  # Imprime la salida
            for char in output:
                ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
                time.sleep(0.015)  # Espera 15 ms entre cada carácter
            prev_artist = artist # Actualiza las variables prev_artist y prev_title
            prev_title = title
        time.sleep(1)  # Espera 1 segundo antes de verificar nuevamente
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")  # Maneja errores si ocurren
    except KeyboardInterrupt:
        ser.close()  # Cierra el puerto serial al salir limpiamente
        print("\nPrograma detenido por el usuario.")
        break  # Sale del bucle while
