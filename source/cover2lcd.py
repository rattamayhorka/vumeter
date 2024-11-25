import dbus
import io
from PIL import Image, ImageEnhance
import serial
import time
import requests
import pyudev
import subprocess
import re

# Función para leer la API key desde un archivo
def leer_api_key(file):
    with open(file, 'r') as f:
        api_key = f.readline().strip()  # Lee la primera línea y quita espacios en blanco
    return api_key

# Uso de la función
LASTFM_API_KEY = leer_api_key('/home/acatl/Proyectos/vumeter/source/apikey.txt')

def find_ttyusb_by_model(model_name):
    context = pyudev.Context()
    tty_lcds = {device.device_node: device for device in context.list_devices(subsystem='tty') if 'USB' in device.device_node}

    for tty_lcd, tty_info in tty_lcds.items():
        if model_name in tty_info.get('ID_MODEL', ''):
            return tty_lcd

    print(f"No se encontró el modelo: {model_name}")
    return None

def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def invert_color(r, g, b):
    return (255 - r, 255 - g, 255 - b)

def send_image_to_lcd(image, ser):
    width, height = image.size
    for y in range(height):
        pixels = bytearray()
        for x in range(width):
            r, g, b = image.getpixel((x, y))
            r, g, b = invert_color(r, g, b)
            color = rgb565(r, g, b)
            pixels.extend(color.to_bytes(2, 'big'))
        ser.write(pixels)
        time.sleep(0.1)

def clean_title(title):
    # Elimina partes comunes de los títulos
    cleaned_title = re.sub(r'\s*\(.*?\)\s*', '', title)  # Eliminar texto entre paréntesis
    cleaned_title = cleaned_title.strip()
    return cleaned_title

def search_album_art_on_lastfm(artist, title):
    queries = [
        f"https://ws.audioscrobbler.com/2.0/?method=track.getInfo&api_key={LASTFM_API_KEY}&artist={artist}&track={title}&format=json",
        f"https://ws.audioscrobbler.com/2.0/?method=track.getInfo&api_key={LASTFM_API_KEY}&artist={artist}&track={clean_title(title)}&format=json"
    ]

    for query in queries:
        response = requests.get(query)
        if response.status_code == 200:
            data = response.json()
            if 'track' in data and 'album' in data['track']:
                image_url = data['track']['album']['image'][-1]['#text']
                if image_url:
                    image_data = requests.get(image_url).content
                    img = Image.open(io.BytesIO(image_data))
                    img = img.convert("RGB").resize((240, 240))
                    enhancer = ImageEnhance.Contrast(img)
                    img = enhancer.enhance(1.5)
                    return img
            else:
                print("No se encontró la pista con esa variante.")
    print("No se encontró la portada en Last.fm.")
    return None

def get_metadata(player_name):
    bus = dbus.SessionBus()
    player = bus.get_object(player_name, '/org/mpris/MediaPlayer2')
    properties = dbus.Interface(player, dbus_interface='org.freedesktop.DBus.Properties')
    metadata = properties.Get('org.mpris.MediaPlayer2.Player', 'Metadata')
    artist = metadata.get('xesam:artist')[0] if 'xesam:artist' in metadata else None
    title = metadata.get('xesam:title') if 'xesam:title' in metadata else None
    album_art_url = metadata.get('mpris:artUrl') if 'mpris:artUrl' in metadata else None
    
    return artist, title, album_art_url

def get_active_player():
    command = "busctl --user --list | grep org.mpris.MediaPlayer2 | head -n 1 | awk '{print $1}'"
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    return stdout.decode('utf-8').strip()

# Buscar el puerto serie correspondiente al modelo especificado
tty_lcd = find_ttyusb_by_model("CP2102_USB_to_UART_Bridge_Controller")

if tty_lcd is not None:
    try:
        # Configura el puerto serie para el ESP32
        ser = serial.Serial(tty_lcd, 115200, timeout=1)
        time.sleep(2)

        if not ser.is_open:
            print(f"Abriendo el puerto: {tty_lcd}")
            ser.open()

        player_name = get_active_player()

        if player_name:
            artist, title, album_art_url = get_metadata(player_name)
            album_art = None

            if artist and title:
                album_art = search_album_art_on_lastfm(artist, title)

            if not album_art and album_art_url:
                if album_art_url.startswith("file://"):
                    local_path = album_art_url.replace("file://", "")
                    try:
                        with open(local_path, "rb") as f:
                            image_data = f.read()
                        album_art = Image.open(io.BytesIO(image_data))
                        album_art = album_art.convert("RGB").resize((240, 240))
                    except FileNotFoundError:
                        print(f"No se encontró el archivo local: {local_path}")

            if album_art:
                print(f"Iniciando envío de portada: {artist} - {title}.")
                send_image_to_lcd(album_art, ser)
                print("Envío de portada completado.")
            else:
                print("No se pudo obtener la portada.")
        else:
            print("No hay reproductores activos en este momento.")

    except serial.SerialException as e:
        print(f"Error al abrir el puerto serie: {e}")
else:
    print("No se pudo abrir el puerto serie. Asegúrate de que el dispositivo esté conectado.")
