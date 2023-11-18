  


# Vumeter - Tablero con Control de Volumen y Display OLED multimedia 

Vumeter, es un proyecto con el que puedes mostrar en un OLED, información en tiempo real como Artista y Cancíón que se está reproduciendo en Linux, muy parecido a lo que era LCD Smartie, :D

Este script de Python interactúa con reproductores multimedia a traves de D-Bus y formatea la información y la envía al atmega32 que controla la pantalla OLED.

## Por implementar:
- cambiar control de volumen a encoder. (0%)
-   vumetro (30%)
- 
- notificaciónes (0%)

## Requisitos

- Python 3.x
- [pydbus](https://pypi.org/project/pydbus/)
- [unidecode](https://pypi.org/project/Unidecode/)
- [pamixer](https://man.archlinux.org/man/pamixer.1)

## Conexiones: 


## Configuración

Asegúrate de tener todos los requisitos instalados antes de ejecutar el script.

```bash
pip install pydbus unidecode
```
## Uso

Ejecuta el script de la siguiente manera:

```bash
python dbus_data_tx.py
```

El script detectará reproductores multimedia activos y mostrará información sobre la canción en el display OLED. También permite ajustar el volumen mediante un convertidor usb-serial conectado al microcontrolador.

## Notas

- Asegúrate de tener permisos para acceder al puerto serie (`/dev/ttyUSB0` en este caso).
- Ajusta el puerto serie y el baudrate según tu configuración.

## Contribuciones

¡Contribuciones son bienvenidas! Si encuentras errores o mejoras posibles, no dudes en crear un *issue* o enviar un *pull request*.

## Autor

[rattamayhorka]


