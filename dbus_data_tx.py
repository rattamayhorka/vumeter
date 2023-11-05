import serial
import subprocess
import pydbus
import time
from unidecode import unidecode
import threading

ser = serial.Serial('/dev/ttyUSB0', 9600)  # Configuración de puerto serie y baudrate a conectarse / cambiar cuando sea unico el USB

def obtener_reproductores_activos():
    # Ejecuta el comando y captura la salida para obtener una lista de reproductores activos
    command = "busctl --user --list | grep org.mpris.MediaPlayer2"
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()

    # Verifica si hubo errores
    if stderr:
        print(f"Error: {stderr.decode('utf-8')}")

    # Guarda la salida en la variable players
    players = stdout.decode('utf-8')

    # Divide la cadena en líneas
    player_lines = players.split('\n')
    return player_lines

def recopilar_data():
    artist = ""
    title = ""
    service_name = ""
    prev_output = ""
    reproductor = ""
    while True:
        try:
            player_lines = obtener_reproductores_activos()
            if not player_lines:
                print("No hay reproductores activos. Esperando...")
                time.sleep(3)  # Espera 10 segundos antes de volver a verificar
                continue

            # Itera a través de las líneas y muestra la primera parte de cada línea antes del primer espacio en blanco
            for line in player_lines:
                parts = line.split()
                if parts:
                    service_name = parts[0]  # Nombre del servicio del reproductor
                    #print(f'Reproductor: {service_name}')
                    reproductor = service_name
                    
                    # Conéctate al servicio D-Bus de un reproductor multimedia específico
                    session_bus = pydbus.SessionBus()
                    player = session_bus.get(service_name, '/org/mpris/MediaPlayer2')

                    # Obtiene los datos de metadatos del reproductor
                    metadata = player.Metadata
                    if 'xesam:artist' in metadata and 'xesam:title' in metadata:
                        artist = metadata["xesam:artist"][0]
                        title = metadata["xesam:title"]

            if title and service_name:
                output = f"\aNow playing:\n{artist} \n{title}\n{reproductor}\n"
                artist = ""
                title = ""
                reproductor = ""
            
            else:
                output = f"\awaiting...\n \n \n\n"

                  
            if output != prev_output:  # Verifica si la salida cambió
                unicd_output = unidecode(output)                
                print(output)  # Imprime la salida
                for char in unicd_output:
                
                    ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
                    time.sleep(0.015)  # Espera 15 ms entre cada carácter              
                prev_output = output  # Actualiza la salida anterior

        except Exception as e:
            print(f"Error: {e}") # Maneja otras excepciones

        time.sleep(1)

# llamada de funcion / tipo interrupcion
data_thread = threading.Thread(target=recopilar_data)
data_thread.daemon = True  # El hilo se detendrá cuando el programa principal termine
data_thread.start()

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