import serial
import subprocess
import time
import threading
from unidecode import unidecode

def enviar_activo(): # Función para enviar datos sin afectar con el tiempo de espera de la transmision
    prev_youtube_title = ""
    while True:
        try:
            # Ejecutar el comando dbus-monitor
            command = "dbus-monitor --session 'type=signal,interface=org.freedesktop.DBus.Properties'"
            process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            # Inicializar una variable para almacenar el valor de xesam:title
            youtube_title = None

            for line in process.stdout:
                if "xesam:title" in line:
                    # Encontramos una línea con "xesam:title", la siguiente línea contiene el valor que queremos
                    next_line = next(process.stdout)
                    youtube_title = next_line.strip().replace('variant                      string "', '')[:-1]
                    break
            # Cerrar el proceso
            process.terminate()

            if youtube_title != prev_youtube_title:
                output = f"\aNow playing:\n{youtube_title}\n\n\n"  # Concatena la información
            
                unicd_output = unidecode(output)                
                #print(unicd_output)  # Imprime la salida
                print(youtube_title)  # Imprime la salida
                for char in unicd_output:
                #for char in output:
                    ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
                    time.sleep(0.015)  # Espera 15 ms entre cada carácter  
                prev_youtube_title = youtube_title
        except subprocess.CalledProcessError as e: # Maneja excepciones generadas por el comando cmus-remote
            print(f"Error ejecutando comando: {e}")
            # agregar un manejo de errores como si no se está ejecutando.

        except Exception as e:
            # Maneja otras excepciones
            print(f"Error inesperado: {e}")

        time.sleep(1) #espera un segundo para verificar si cambió la canción

ser = serial.Serial('/dev/ttyUSB0', 9600)  # Configuración de puerto serie y baudrate a conectarse / cambiar cuando sea unico el USB

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
