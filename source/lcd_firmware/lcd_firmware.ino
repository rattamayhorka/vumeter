#include <TFT_eSPI.h>  // Librería para la pantalla ILI9341
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI(); // Inicializa la pantalla
const int screenWidth = 240;  // Ancho de la imagen
const int screenHeight = 240; // Alto de la imagen
const int screenOffsetX = 0;  // Offset horizontal para centrar (320-240)/2

const int adcPin = 34;        // Pin ADC
const int vumeterHeight = 40; // Altura del vúmetro

unsigned long previousMillis = 0;
const long interval = 100;    // Intervalo de actualización del vúmetro (ms)

int currentX = 0; // Posición actual en X para la portada
int currentY = 0; // Posición actual en Y para la portada


// Variables para el valor mínimo y máximo
int minValue = 4095; // Inicialmente el valor máximo posible
int maxValue = 0;    // Inicialmente el valor mínimo posible

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0); // Asegura orientación correcta
  tft.fillScreen(rgbTo565(0, 0, 0));
  pinMode(adcPin, INPUT); // Configura el pin 34 como entrada analógica
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Lectura del ADC en el pin 34
    int adcValue = analogRead(adcPin);
    drawVumeter(adcValue);
  }

  // Recepción de la imagen desde la PC
  if (Serial.available()) {
    for (int x = 0; x < 40; x++) {
      if (Serial.available() >= 2) {
        uint16_t color = (Serial.read() << 8) | Serial.read();
        tft.drawPixel(currentX + screenOffsetX, currentY, color);

        currentX++;
        if (currentX >= screenWidth) {
          currentX = 0;
          currentY++;
          if (currentY >= screenHeight) {
            currentY = 0;
          }
        }
      }
    }
  }
}

/*
void drawVumeter(int value) {
  int numBlocks = 10; // Número de bloques simulando LEDs
  int blockWidth = screenWidth / numBlocks;

  // Actualizar los valores mínimo y máximo
  if (value < minValue) {
    minValue = value; // Si el valor actual es menor que el mínimo, actualiza
  }
  if (value > maxValue) {
    maxValue = value; // Si el valor actual es mayor que el máximo, actualiza
  }

  // Asegurarse de que los valores mínimo y máximo no sean iguales (para evitar divisiones por cero)
  if (maxValue > minValue) {
    // Mapeo de la señal al rango de bloques (0-10)
    int activeBlocks = map(value, minValue, maxValue, 0, numBlocks);

    // Dibuja los bloques del vúmetro según el valor mapeado
    static int lastActiveBlocks = -1;
    if (lastActiveBlocks != activeBlocks) {
      for (int i = 0; i < numBlocks; i++) {
        uint16_t color = (i < activeBlocks) ? getColorForLevel(i, numBlocks) : rgbTo565(50, 50, 50);
        tft.fillRect(i * blockWidth, screenHeight + 20, blockWidth - 2, vumeterHeight, color);
      }
      lastActiveBlocks = activeBlocks;
    }
  }
  // Si los valores mínimo y máximo son iguales, no se hace nada para evitar el error de mapeo
}
*/

void drawVumeter(int value) {
  int numBlocks = 10; // Número de bloques simulando LEDs
  int blockWidth = screenWidth / numBlocks;
  static unsigned long lastResetTime = 0; // Última vez que se reiniciaron los valores

  // Reiniciar min y max cada 6 segundos
  if (millis() - lastResetTime >= 6000) {
    minValue = 4095;
    maxValue = 0;
    lastResetTime = millis(); // Actualizar tiempo del último reinicio
  }

  // Actualizar los valores mínimo y máximo
  if (value < minValue) {
    minValue = value; // Si el valor actual es menor que el mínimo, actualiza
  }
  if (value > maxValue) {
    maxValue = value; // Si el valor actual es mayor que el máximo, actualiza
  }

  // Asegurarse de que los valores mínimo y máximo no sean iguales (para evitar divisiones por cero)
  if (maxValue > minValue) {
    // Mapeo de la señal al rango de bloques (0-10)
    int activeBlocks = map(value, minValue, maxValue, 0, numBlocks);

    // Dibuja los bloques del vúmetro según el valor mapeado
    static int lastActiveBlocks = -1;
    if (lastActiveBlocks != activeBlocks) {
      for (int i = 0; i < numBlocks; i++) {
        uint16_t color = (i < activeBlocks) ? getColorForLevel(i, numBlocks) : rgbTo565(50, 50, 50);
        tft.fillRect(i * blockWidth, screenHeight + 20, blockWidth - 2, vumeterHeight, color);
      }
      lastActiveBlocks = activeBlocks;
    }
  }
}


uint16_t getColorForLevel(int level, int totalLevels) {
  switch (level) {
    case 0: return rgbTo565(0, 0, 255);
    case 1: return rgbTo565(0, 51, 255);
    case 2: return rgbTo565(0, 73, 255);
    case 3: return rgbTo565(0, 102, 255);
    case 4: return rgbTo565(0, 153, 255);
    case 5: return rgbTo565(0, 176, 255);
    case 6: return rgbTo565(0, 204, 255);
    case 7: return rgbTo565(0, 226, 255);
    case 8: return rgbTo565(0, 255, 255);
    case 9: return rgbTo565(0, 255, 192);
    default: return rgbTo565(50, 50, 0);
  }
}

uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  r = 255 - r;
  g = 255 - g;
  b = 255 - b;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
