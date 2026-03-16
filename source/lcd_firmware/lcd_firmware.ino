#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
const int screenWidth = 240;
const int screenHeight = 240;
const int screenOffsetX = 0;

const int adcPin = 34;
const int vumeterHeight = 40;

int currentX = 0;
int currentY = 0;

// Variables para el vúmetro mejorado (sin bloqueos)
unsigned int peakMax = 0;
unsigned int peakMin = 4095;
unsigned long lastAudioUpdate = 0;
int lastActiveBlocks = -1;

//variables peak
int peakArray[10] = {0};      // Posición de los picos flotantes
int peakHoldCount[10] = {0};  // Contador para que el pico se quede arriba

// --- Variables para Vúmetro Circular (Arco) ---
int centerLine = 120; // Mitad de la pantalla (240/2)
int lastWidths[10] = {0}; // Para suavizar la caída de cada "ala"


void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(rgbTo565(0, 0, 0));
  pinMode(adcPin, INPUT);
}

void loop() {
  // 1. CAPTURA RÁPIDA (No bloqueante)
  // Tomamos una muestra en cada pasada del loop. Cero impacto en el serial.
  int sample = analogRead(adcPin);
  if (sample < 4095) {
    if (sample > peakMax) peakMax = sample;
    if (sample < peakMin) peakMin = sample;
  }

  // 2. ACTUALIZACIÓN DEL VÚMETRO (Cada 40ms)
  unsigned long currentMillis = millis();
  if (currentMillis - lastAudioUpdate >= 40) {
    int amplitude = peakMax - peakMin;
    
    // Procesar visualización
    drawVumeter(amplitude);

    //Dibujar arco
    //drawArc(amplitude);

    // Reiniciar picos para la siguiente ventana
    peakMax = 0;
    peakMin = 4095;
    lastAudioUpdate = currentMillis;
  }

  // 3. RECEPCIÓN DE IMAGEN (Igual a tu original que sí funciona)
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


void drawArc(int amplitude) {
int numBars = 8; // 8 niveles de "alas"
  int barHeight = vumeterHeight / numBars;
  int sensibilidad = 150; 
  
  // Mapeamos la amplitud al ancho total de las alas
  int totalWidth = map(amplitude, 20, sensibilidad, 0, 110); // Máximo 110px por lado
  totalWidth = constrain(totalWidth, 0, 110);

  for (int i = 0; i < numBars; i++) {
    // Cada "ala" tiene un desfase de tiempo para que se vea orgánico
    int targetW = totalWidth - (i * 10); 
    if (targetW < 0) targetW = 0;

    // Suavizado individual por barra
    if (targetW < lastWidths[i]) targetW = lastWidths[i] - 4;
    lastWidths[i] = targetW;

    int yPos = 250 + (i * barHeight); // Se dibujan hacia abajo de la imagen
    
    // Dibujamos el lado IZQUIERDO (del centro hacia la izquierda)
    // Borramos el rastro con el color de fondo (50,50,50 invertido)
    tft.fillRect(centerLine - 110, yPos, 110 - targetW, barHeight - 1, rgbTo565(50, 50, 50));
    // Dibujamos el ala activa
    tft.fillRect(centerLine - targetW, yPos, targetW, barHeight - 1, getWingColor(i));

    // Dibujamos el lado DERECHO (del centro hacia la derecha)
    // Ala activa
    tft.fillRect(centerLine, yPos, targetW, barHeight - 1, getWingColor(i));
    // Borramos el rastro
    tft.fillRect(centerLine + targetW, yPos, 110 - targetW, barHeight - 1, rgbTo565(50, 50, 50));
  }
}

void drawVumeter(int amplitude) {
  int numBlocks = 10;
  int blockWidth = screenWidth / numBlocks;

  // --- AJUSTE DE SENSIBILIDAD ---
  // Antes estaba en 800. Bajándolo a 150 hará que las barras suban más fácil.
  // Si sigue sin subir, baja el 150 a 80.
  int sensibilidad = 150; 
  
  int activeBlocks = map(amplitude, 20, sensibilidad, 0, numBlocks);
  activeBlocks = constrain(activeBlocks, 0, numBlocks);

  // Suavizado para que la caída no sea brusca
  if (activeBlocks < lastActiveBlocks) {
    activeBlocks = lastActiveBlocks - 1; 
  }
  if (activeBlocks < 0) activeBlocks = 0;

  if (lastActiveBlocks != activeBlocks) {
    for (int i = 0; i < numBlocks; i++) {
      uint16_t color = (i < activeBlocks) ? getColorForLevel(i, numBlocks) : rgbTo565(50, 50, 50);
      
      // Ajusté la posición Y a 250 para que no choque con la imagen de 240
      tft.fillRect(i * blockWidth, 250, blockWidth - 2, vumeterHeight, color);
    }
    lastActiveBlocks = activeBlocks;
  }
}

void drawPeaks(int amplitude) {
  int numBlocks = 10;
  int blockWidth = screenWidth / numBlocks;
  int sensibilidad = 150; 
  
  int activeBlocks = map(amplitude, 20, sensibilidad, 0, numBlocks);
  activeBlocks = constrain(activeBlocks, 0, numBlocks);

  // 1. Lógica de caída suave del vúmetro
  if (activeBlocks < lastActiveBlocks) activeBlocks = lastActiveBlocks - 1;
  if (activeBlocks < 0) activeBlocks = 0;
  lastActiveBlocks = activeBlocks;

  // 2. Lógica del Pico Flotante (Peak Hold)
  for (int i = 0; i < numBlocks; i++) {
    // Si el bloque actual está activo, movemos el pico hacia arriba
    if (i < activeBlocks) {
      if (i >= peakArray[i]) {
        peakArray[i] = i;
        peakHoldCount[i] = 12; // Cuántos ciclos se queda arriba (aprox 0.5s)
      }
    } else {
      // Si no está activo, el pico espera y luego cae
      if (peakHoldCount[i] > 0) {
        peakHoldCount[i]--;
      } else {
        if (peakArray[i] > 0) peakArray[i]--;
      }
    }
  }

  // 3. Dibujo en pantalla
  for (int i = 0; i < numBlocks; i++) {
    int x = i * blockWidth;
    int y = 250; // Posición vertical
    int w = blockWidth - 3;
    int h = vumeterHeight;

    // Color del bloque principal (Gradiente según la posición)
    uint16_t mainColor;
    if (i < activeBlocks) {
      mainColor = getColorForLevel(i, numBlocks);
    } else {
      mainColor = rgbTo565(30, 30, 30); // Apagado
    }

    // Dibujar el bloque base
    tft.fillRect(x, y, w, h, mainColor);

    // Dibujar el Pico Flotante (una rayita blanca o de color brillante arriba)
    if (peakArray[i] > 0) {
      uint16_t pColor = rgbTo565(255, 255, 255); // Blanco para el pico
      // Dibujamos el pico como una rayita de 3 píxeles de alto
      tft.fillRect(x, y + h - ((peakArray[i] + 1) * (h/numBlocks)), w, 3, pColor);
    }
  }
}

uint16_t getColorForLevel(int level, int totalLevels) {
  switch (level) {
    case 0: case 1: return rgbTo565(0, 0, 255);   // Azul profundo
    case 2: case 3: return rgbTo565(0, 255, 255); // Cian/Turquesa
    case 4: case 5: return rgbTo565(0, 255, 0);   // Verde neón
    case 6: case 7: return rgbTo565(255, 255, 0); // Amarillo
    case 8:         return rgbTo565(255, 128, 0); // Naranja
    case 9:         return rgbTo565(255, 0, 0);   // Rojo
    default:        return rgbTo565(50, 50, 50);
  }
}

// Colores para las alas (puedes personalizarlos)
uint16_t getWingColor(int line) {
  // Gradiente de abajo hacia arriba: Violeta -> Azul -> Cian
  switch(line) {
    case 0: return rgbTo565(255, 0, 255); // Magenta (Arriba)
    case 1: return rgbTo565(200, 0, 255);
    case 2: return rgbTo565(150, 50, 255);
    case 3: return rgbTo565(100, 100, 255);
    case 4: return rgbTo565(50, 150, 255);
    case 5: return rgbTo565(0, 200, 255);
    case 6: return rgbTo565(0, 255, 255);
    case 7: return rgbTo565(0, 255, 200); // Turquesa (Abajo)
    default: return rgbTo565(50, 50, 50);
  }
}

uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  r = 255 - r;
  g = 255 - g;
  b = 255 - b;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
