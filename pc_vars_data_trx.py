import serial
import subprocess
import time
import threading
from unidecode import unidecode

def enviar_activo(): # Función para enviar datos sin afectar con el tiempo de espera de la transmision
    
    while True:
        try:
            temp1 = subprocess.check_output(
                "sensors | grep 'Core 0:' | awk '{print $3}' | cut -c2- | sed 's/..$//'",

                shell=True,
                text=True  # Asegura que la salida sea una cadena de texto (str)
            ).strip()  # Elimina espacios en blanco al principio y al final
            
            temp2 = subprocess.check_output(
                "sensors | grep 'Core 0:' | awk '{print $3}' | cut -c2- | sed 's/..$//'",
                
                shell=True,
                text=True  # Asegura que la salida sea una cadena de texto (str)
            ).strip()  # Elimina espacios en blanco al principio y al final

            #temp1 = float(temp1)
            #temp2 = float(temp2)
            
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
 



            output = f"\awifi: {wifi_essid}\n{wifi_signal}dB Mem:{free_memory}Mib\ntemp:{temp}\n/:{root_capacity} /home:{home_capacity}\n"  # Concatena la información
            #output = f"\a{artist}\n{title}\n{album}"  # Concatena la información
            print(output)  # Imprime la salida
            #print(output)  # Imprime la salida
                        
            for char in output:
                ser.write(char.encode())  # Convierte el carácter a bytes y envíalo por serial
                time.sleep(0.015)  # Espera 15 ms entre cada carácter  

        except subprocess.CalledProcessError as e: # Maneja excepciones generadas por el comando cmus-remote
            print(f"Error ejecutando comando: {e}")
            # agregar un manejo de errores como si no se está ejecutando.

        except Exception as e:
            # Maneja otras excepciones
            print(f"Error inesperado: {e}")

        time.sleep(5) #espera un segundo para verificar si cambió la canción

#ser = serial.Serial('/dev/ttyUSB1', 9600)  # Configuración de puerto serie y baudrate a conectarse / cambiar cuando sea unico el USB
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
