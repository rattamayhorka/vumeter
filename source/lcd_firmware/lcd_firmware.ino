

#include <TFT_eSPI.h>  // Librería para la pantalla ILI9341
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI(); // Inicializa la pantalla
const int screenWidth = 240;  // Ancho de la imagen
const int screenHeight = 240; // Alto de la imagen
//const int screenOffsetX = 40; // Offset horizontal para centrar (320-240)/2
const int screenOffsetX = 0; // Offset horizontal para centrar (320-240)/2

void setup() {
  Serial.begin(115200);      // Iniciar la comunicación serial
  tft.init();                // Inicializar la pantalla
  tft.setRotation(0);        // Ajustar la rotación de la pantalla
  tft.fillScreen(TFT_WHITE); // Limpiar la pantalla al inicio
}

void loop() {
  static int currentY = 0; // Para rastrear la línea actual en el eje Y
  static int currentX = screenOffsetX; // Comienza en el centro horizontalmente

  if (Serial.available()) {
    // Leer datos en bloques de 40 píxeles para reducir llamadas al puerto serie
    for (int x = 0; x < 40; x++) {  // Enviar 40 píxeles a la vez
      // Leer 2 bytes del puerto serie (color RGB565)
      if (Serial.available() >= 2) { // Asegúrate de que haya suficientes datos disponibles
        uint16_t color = (Serial.read() << 8) | Serial.read();
        
        // Dibuja en la posición actual
        tft.drawPixel(currentX, currentY, color); // Dibuja en el eje X y Y

        currentX++; // Avanza en X
      }
    }

    // Reinicia currentX después de dibujar 40 píxeles
    if (currentX >= screenWidth + screenOffsetX) { // Si se alcanza el ancho de la imagen + offset
      currentX = screenOffsetX; // Reiniciar a la columna del centro (40)
      currentY++;    // Avanzar a la siguiente línea en Y
    }
    if (currentY >= screenHeight) { // Limitar a la altura de la imagen
      currentY = 0; // Reiniciar si se alcanza el final de la imagen
    }
  }
}
