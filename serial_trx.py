import serial
import subprocess
import time
import threading

def enviar_activo(): # Función para enviar datos sin afectar con el tiempo de espera de la transmision
    prev_artist = ""
    prev_title = ""
    while True:
        try:
            artist = subprocess.check_output(
                'cmus-remote -Q 2> /dev/null | grep "tag artist " | cut -c12-',
                shell=True,
                text=True  # Asegura que la salida sea una cadena de texto (str)
            ).strip()  # Elimina espacios en blanco al principio y al final

            title = subprocess.check_output(
                'cmus-remote -Q 2> /dev/null | grep "tag title " | cut -c11-',
                shell=True,
                text=True
            ).strip() 

            album = subprocess.check_output(
                'cmus-remote -Q 2> /dev/null | grep "tag album " | cut -c11-',
                shell=True,
                text=True
            ).strip()

            tag_date = subprocess.check_output(
                'cmus-remote -Q 2> /dev/null | grep "tag date " | cut -c10-',
                shell=True,
                text=True
            ).strip()

            if artist != prev_artist or title != prev_title: # Verifica si artist o title han cambiado
                output = f"Artist:{artist}\nTrack:{title}\nAlbum:{album}\nYear:{tag_date}\n\n"  # Concatena la información
                print(output)  # Imprime la salida
                for char in output:
                    ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
                    time.sleep(0.015)  # Espera 15 ms entre cada carácter
                prev_artist = artist # Actualiza las variables prev_artist y prev_title
                prev_title = title
        except subprocess.CalledProcessError as e: # Maneja excepciones generadas por el comando cmus-remote
            print(f"Error ejecutando cmus-remote: {e}")
            # agregar un manejo de errores como si no se está ejecutando.

        except Exception as e:
            # Maneja otras excepciones
            print(f"Error inesperado: {e}")

        time.sleep(1) #espera un segundo para verificar si cambió la canción

ser = serial.Serial('/dev/ttyUSB1', 9600)  # Configuración de puerto serie y baudrate a conectarse / cambiar cuando sea unico el USB

# llamada de funcion / tipo interrupcion
activo_thread = threading.Thread(target=enviar_activo)
activo_thread.daemon = True  # El hilo se detendrá cuando el programa principal termine
activo_thread.start()

try:
    char_buffer = ""  # Buffer para acumular caracteres
    while True:
        byte = ser.read(1) # Lee un byte desde el puerto serie
        char_value = byte.decode('ascii', errors='ignore')  # Ignorar caracteres no ASCII, Convierte el byte a una cadena de caracteres utilizando ASCII

        if char_value.isdigit(): #verifica si es un digito (0-9)
            char_buffer += char_value # Acumula el carácter en el buffer
        else:
            if char_buffer:
                num_value = int(char_buffer)  # Convierte el buffer en un número entero
                if num_value > 99: # Ajusta num_value al rango de 0 a 99 aunque sea mayor
                    num_value = 99
                result_str = str(num_value) # Convierte num_value a un string para pasarlo al comando en linux
                subprocess.run(["pamixer", "--set-volume", result_str]) # comando para establecer el volumen en linux
                print(result_str)
                char_buffer = "" # Limpia el buffer

except KeyboardInterrupt:
    ser.close()    # salida por Ctrl+C
    print("\nPuerto serie cerrado.")
