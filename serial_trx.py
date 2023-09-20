import serial
import subprocess
import time
import threading




# Función para enviar "activo" cada segundo
def enviar_activo():

    # Variables para almacenar el artista y el título anteriores
    prev_artist = ""
    prev_title = ""
    while True:
        try:
            #ser.write("activo\n".encode())
            #ser.flush()
            #time.sleep(3)
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
        except subprocess.CalledProcessError as e:
            # Maneja excepciones generadas por el comando cmus-remote
            print(f"Error ejecutando cmus-remote: {e}")
            # Puedes agregar una lógica para manejar este error, como volver a intentar o esperar antes de volver a intentar.

        except Exception as e:
            # Maneja otras excepciones que puedan ocurrir
            print(f"Error inesperado: {e}")

        time.sleep(1)

ser = serial.Serial('/dev/ttyUSB1', 9600)  # Configuración de puerto serie y baudios


# Inicia el hilo para enviar "activo"
activo_thread = threading.Thread(target=enviar_activo)
activo_thread.daemon = True  # El hilo se detendrá cuando el programa principal termine
activo_thread.start()

try:
    char_buffer = ""  # Buffer para acumular caracteres

    while True:
        byte = ser.read(1) # Lee un byte desde el puerto serie
        char_value = byte.decode('ascii', errors='ignore')  # Ignorar caracteres no ASCII Convierte el byte a una cadena de caracteres utilizando ASCII

        if char_value.isdigit(): # Verifica si char_value es un dígito válido ('0'-'9')
            char_buffer += char_value # Acumula el carácter en el buffer
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
