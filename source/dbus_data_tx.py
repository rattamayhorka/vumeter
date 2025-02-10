import os
import time
import serial
import subprocess
import pydbus  #dependencia
import threading 
import logging
import sys
import pyudev 

from unidecode import unidecode #dependencia
from daemonize import Daemonize #dependencia

pid = "/tmp/vumeter.pid"
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
logger.propagate = False
log_format = '%(asctime)s - %(levelname)s - %(message)s'
fh = logging.FileHandler("/tmp/vumeter.log", "w")
fh.setLevel(logging.DEBUG)
fh.setFormatter(logging.Formatter(log_format))
logger.addHandler(fh)
keep_fds = [fh.stream.fileno()]

def find_ttyusb_by_model(model_name):
    context = pyudev.Context()
    tty_devices = {device.device_node: device for device in context.list_devices(subsystem='tty') if 'USB' in device.device_node}
    for tty_device, tty_info in tty_devices.items():
        if model_name in tty_info.get('ID_MODEL', ''):
            logger.debug(f"Puerto oled: {tty_device}")
            return tty_device
    logger.debug(f"No se encontró el modelo: {model_name}")
    return None

def check_usb_port(model_name="USB-Serial_Controller"):
    while True:
        tty_oled = find_ttyusb_by_model(model_name)
        if tty_oled:
            return tty_oled
        else:
            logger.debug(f"Esperando el puerto USB para el modelo: {model_name}...")
            time.sleep(5)

def main():
    tty_oled = check_usb_port()
    if tty_oled:
        logger.debug(f"Puerto oled {tty_oled} presente.")
        ser = serial.Serial(tty_oled, 9600)
        delay_music = 80
        delay_pc_vars = 6

    def obtener_reproductores_activos():
        try:
            # Ejecuta el comando y captura la salida para obtener una lista de reproductores activos
            command = "busctl --user --list | grep org.mpris.MediaPlayer2 | head -n 1"
            process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            # Verifica si hubo errores
            if stderr:
                logger.error(f"Error: {stderr.decode('utf-8')}")

            # Guarda la salida en la variable players
            players = stdout.decode('utf-8')

            # Divide la cadena en líneas
            player_lines = players.split('\n')

            # Verifica si no hay reproductores activos
            if not player_lines:
                logger.info("No se encontraron reproductores multimedia activos.")
                return []  # Retorna una lista vacía indicando que no se encontraron reproductores activos

            return player_lines  # Retorna la lista de reproductores activos

        except subprocess.CalledProcessError as e:
            logger.error(f"Error al obtener reproductores activos: {e}")
            return []  # Retorna una lista vacía en caso de error

    def send_serial(dato_envio):
        unicd_output = unidecode(dato_envio)
        for char in unicd_output:
            ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
            time.sleep(0.002)  # Espera 2 ms entre cada carácter

    def send_serial_sin_unicode(dato_envio):
        for char in dato_envio:
            ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
            time.sleep(0.002)  # Espera 2 ms entre cada carácter

    def recopilar_data():
        contador = 0
        artist = ""
        title = ""
        service_name = ""
        reproductor = ""
        prev_artist = ""
        prev_title = ""
        time_until_print_pc_vars = delay_music
        wait_printing_pc_vars = delay_pc_vars
        while True:
            try:
                
                player_lines = obtener_reproductores_activos()
                if not player_lines:
                    logger.info("recopilar_data No hay reproductores activos. Esperando...")
                    time.sleep(3)  # Espera 3 segundos antes de volver a verificar
                    continue
                
                temp1 = subprocess.check_output(
                    "sensors | grep 'Core 0:' | awk '{print $3}' | cut -c2- | sed 's/..$//'",

                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final

                temp2 = subprocess.check_output(
                    "sensors | grep 'Core 1:' | awk '{print $3}' | cut -c2- | sed 's/..$//'",
                    
                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final

                temp = ( float(temp1) + float(temp2) ) / 2

                root_capacity = subprocess.check_output(
                    "df -h | grep nvme0n1p5 | awk '{print $4}'",
                    
                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final

                home_capacity = subprocess.check_output(
                    "df -h | grep nvme0n1p3 | awk '{print $4}'",
                    
                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final

                wifi_signal = subprocess.check_output(
                    "cat /proc/net/wireless | grep wlan0 | awk '{print $4}'",
                    
                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final

                wifi_essid = subprocess.check_output(
                    "iwgetid -r",
                    
                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final

                free_memory = subprocess.check_output(
                    "free -m | grep Mem: | awk '{print $3}'",
                    
                    shell=True,
                    text=True  # Asegura que la salida sea una cadena de texto (str)
                ).strip()  # Elimina espacios en blanco al principio y al final
                
                # Itera a través de las líneas y muestra la primera parte de cada línea antes del primer espacio en blanco
                for line in player_lines:
                    parts = line.split()
                    if parts:
                        service_name = parts[0]  # Nombre del servicio del reproductor
                        #print(f'Reproductor: {service_name}')
                        reproductor = service_name
                        reproductor = reproductor.replace("org.mpris.MediaPlayer2.", "")
                        reproductor = reproductor.split('.')[0]
                        reproductor = reproductor[0].capitalize() + reproductor[1:]

                        # Conéctate al servicio D-Bus de un reproductor multimedia específico
                        session_bus = pydbus.SessionBus()
                        player = session_bus.get(service_name, '/org/mpris/MediaPlayer2')
                        playback_status = player.PlaybackStatus

                        # Obtiene los datos de metadatos del reproductor
                        metadata = player.Metadata
                        if 'xesam:artist' in metadata and 'xesam:title' in metadata:
                            artist = metadata["xesam:artist"][0]
                            title = metadata["xesam:title"]

                            if ' - ' in title: # Verifica si el título contiene " - " y divide en artist y title
                                artist, title = title.split(' - ', 1)
                                artist = artist.strip()
                                title = title.strip()

                            if ' - Topic' in artist: # Verifica si el título contiene " - " y divide en artist y title
                                artist = artist.replace(' - Topic',"")

                if artist != prev_artist or title != prev_title:
                    subprocess.Popen(['python3', '/home/acatl/Proyectos/vumeter/source/cover2lcd.py'])
                    logger.info(f"Canción cambiada: {artist} - {title}")
                    prev_artist = artist
                    prev_title = title

                if title and service_name and (contador < time_until_print_pc_vars):
                    max_len = max(len(reproductor), len(artist), len(title)) + 1  # Ajuste aquí
                    if max_len > 21: # Verifica si max_len es mayor a 20
                        for i in range(max_len - 20):  # Ajuste aquí
                            # Traducción del estado de reproducción
                            if playback_status == "Playing":
                                playback_status = "Reproduciending:"
                            elif playback_status == "Paused":
                                playback_status = "Pausado..."
                            #linea_1 = f"{reproductor}...: {playback_status}"
                            linea_1 = f"{playback_status}"
                            linea_2 = artist
                            linea_3 = title
                            # Obtener el volumen en tiempo real
                            get_volume = subprocess.check_output(
                                "pamixer --get-volume",
                                shell=True,
                                text=True
                            ).strip()

                            get_mute = subprocess.check_output(
                                "pamixer --get-mute",
                                shell=True,
                                text=True
                            ).strip()
                            if get_mute == "true":
                                get_mute = "On"
                            else:
                                get_mute = "Off" 

                            linea_4 = f"Vol:{get_volume}     Mute:{get_mute}"

                            if len(linea_1) > 20:
                                linea_1 = linea_1[i:i + 20]

                            if len(linea_2) > 20:
                                linea_2 = linea_2[i:i + 20]

                            if len(linea_3) > 20:
                                linea_3 = linea_3[i:i + 20]

                            if len(linea_4) > 20:
                                linea_4 = linea_4[i:i + 20]

                            output = f"\a{linea_1}\n{linea_2}\n{linea_3}\n{linea_4}\n"
                            logger.info(f"enviando datos mayores de 20 caracteres... contador: {contador}")
                            send_serial(output)
                            time.sleep(0.3)

                        artist = ""
                        title = ""
                        reproductor = ""
                    else:
                        # Si max_len no es mayor a 20, imprime directamente
                        if playback_status == "Playing":
                            playback_status = "Reproduciending:"
                        elif playback_status == "Paused":
                            playback_status = "Pausado..."
                        #linea_1 = f"{reproductor}...: {playback_status}"
                        linea_1 = f"{playback_status}"
                        linea_2 = artist
                        linea_3 = title
                        # Obtener el volumen en tiempo real
                        get_volume = subprocess.check_output(
                            "pamixer --get-volume",
                            shell=True,
                            text=True
                        ).strip()

                        get_mute = subprocess.check_output(
                            "pamixer --get-mute",
                            shell=True,
                            text=True
                        ).strip()

                        if get_mute == "true":
                            get_mute = "On"
                        else:
                            get_mute = "Off" 

                        linea_4 = f"Vol:{get_volume}      Mute:{get_mute}"

                        output = f"\a{linea_1}\n{linea_2}\n{linea_3}\n{linea_4}\n"
                        send_serial(output)
                        time.sleep(0.3)

                        artist = ""
                        title = ""
                        reproductor = ""
                        logger.info(f"enviando datos menores de 20 caracteres... contador: {contador}")

                else:
                    output = f"\aEsperando\nReproductor...\n\n\n"

                if contador >= time_until_print_pc_vars:
                    output = f"\awifi:{wifi_essid}\n{wifi_signal}dB Mem:{free_memory}Mib\nTemp:{temp}\x17C\n/:{root_capacity} /home:{home_capacity}\n"  # Concatena la información
                    logger.info(f"inicio de impresion de variables pc: {contador}")
        
                if contador >= time_until_print_pc_vars + wait_printing_pc_vars:
                    contador = 0
                    logger.info(f"fin de impresion de variables: {contador}")
        
                send_serial(output)

            except Exception as e:
                if "No player is being controlled by playerctld" in str(e):
                    logger.error("exception No hay reproductor multimedia activo siendo controlado por playerctld.")
                    output = f"\aEsperando\nReproductor...\n\n\n"
                    send_serial(output)
                else:
                    logger.error(f"Error Exception: {e}")  # Maneja otras excepciones

            contador += 1
            time.sleep(1)

    # llamada de funcion / tipo interrupcion
    data_thread = threading.Thread(target=recopilar_data)
    data_thread.daemon = True  # El hilo se detendrá cuando el programa principal termine
    data_thread.start()

    try:
        char_buffer = ""  # Buffer para acumular caracteres
        while True:
            try:    
                byte = ser.read(1)  # Lee un byte desde el puerto serie
                char_value = byte.decode('ascii', errors='ignore')  # Convierte el byte a una cadena de caracteres utilizando ASCII
                char_buffer += char_value
                # Si se recibe un carácter de nueva línea, ejecuta el comando y reinicia el buffer
                #print(char_buffer) 
                if char_value == '\n':
                    if "-" in char_buffer:
                        # Dividir el comando y el valor
                        command, value = char_buffer.strip().split(' ')
                        subprocess.run(["pamixer", command, value])
                        if "-i" in command:
                            logger.info("received command increment volume")
                        elif "-d" in command:
                            logger.info("received command decrement volume")
                        elif "-t" in command:
                            logger.info("received command mute/unmute")
                        char_buffer = ""  # Limpia el buffer
                    elif "prev" in char_buffer:
                        logger.info("recived prev command")
                        subprocess.run(["playerctl", "previous"])
                        char_buffer = ""  # Limpia el buffer
                    elif "play" in char_buffer:
                        logger.info("received next command")
                        subprocess.run(["playerctl", "next"])
                        char_buffer = ""  # Limpia el buffer
                    elif "next" in char_buffer:
                        logger.info("received play command")
                        subprocess.run(["playerctl", "play-pause"])
                        char_buffer = ""  # Limpia el buffer

            except serial.SerialException as e:
                logger.error(f"1Error de puerto serie: {e}")
                logger.info("2El puerto serie se ha desconectado. Esperando...")
                ser.close()  # Cierra el puerto serie
                while True:
                    if check_usb_port():
                        logger.debug(f"Puerto serie reconectado: {tty_oled}")
                        ser = serial.Serial(tty_oled, 9600)
                        break  # Sale del bucle interno para continuar con el programa principal
                    else:
                        logger.info("4Esperando a que se reconecte el puerto serie...")
                        time.sleep(5)  # Espera 5 segundos antes de volver a verificar el puerto USB

    except KeyboardInterrupt:
        logger.info("\nSalida por Ctrl+C")
        logger.info("\nPuerto serie cerrado.")
        ser.close()    # salida por Ctrl+C

def run_as_daemon():
    #daemon = Daemonize(app="vumeter", pid="/tmp/vumeter.pid", action=main)
    #daemon.start()
    daemon = Daemonize(app="test_app", pid=pid, action=main, keep_fds=keep_fds)
    daemon.start()

if __name__ == "__main__":
    run_as_daemon()
    