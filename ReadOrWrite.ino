#include <SPI.h>
#include <MFRC522.h>

// Definición de pines para el módulo Mini RFID-RC522
#define RST_PIN 9   // Pin RST conectado al pin D9 del Nano
#define SS_PIN 10   // Pin SDA conectado al pin D10 del Nano

MFRC522 rfid(SS_PIN, RST_PIN); // Crear instancia del lector RFID

// Clave por defecto para autenticar (6 bytes con valor 0xFF)
MFRC522::MIFARE_Key key = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

void setup() {
  Serial.begin(9600); // Inicializar comunicación serial
  while (!Serial);

  SPI.begin();        // Inicializar comunicación SPI
  rfid.PCD_Init();    // Inicializar el lector RFID
  Serial.println("Mini RFID-RC522 listo. Use comandos para leer, escribir o restaurar datos...");
}

void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando.startsWith("leer")) {
      leerTarjeta();
    } else if (comando.startsWith("escribir")) {
      int espacio = comando.indexOf(' ');
      if (espacio > 0) {
        String resto = comando.substring(espacio + 1);
        int bloque = resto.substring(0, resto.indexOf(' ')).toInt();
        if (bloque >= 0 && bloque < 64 && bloque % 4 != 3) { // Bloques válidos
          String mensaje = resto.substring(resto.indexOf(' ') + 1);
          escribirEnTarjeta(bloque, mensaje);
        } else {
          Serial.println("Bloque inválido o reservado. No se puede escribir.");
        }
      } else {
        Serial.println("Error en el comando escribir. Formato: escribir <bloque> <mensaje>");
      }
    } else if (comando.startsWith("restaurar")) {
      restaurarTarjeta();
    } else {
      Serial.println("Comando desconocido. Use 'leer', 'escribir' o 'restaurar'.");
    }
  }
}

void leerTarjeta() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    Serial.println("No se detectó tarjeta. Acerque una tarjeta.");
    return;
  }

  // Muestra el UID de la tarjeta
  Serial.print("UID de la tarjeta: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Identificar tipo de tarjeta
  identificarTipoTarjeta();

  // Mostrar información de la tarjeta
  mostrarInfoTarjeta();

  // Detener comunicación con la tarjeta
  rfid.PICC_HaltA();
}

void escribirEnTarjeta(int bloque, String mensaje) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    Serial.println("No se detectó tarjeta. Acerque una tarjeta.");
    return;
  }

  if (!autenticarBloque(bloque)) {
    Serial.println("Error al autenticar el bloque para escribir.");
    return;
  }

  // Preparar datos para escribir (máximo 16 bytes)
  byte buffer[16] = {0};
  mensaje.getBytes(buffer, 16);

  MFRC522::StatusCode status = rfid.MIFARE_Write(bloque, buffer, 16);
  if (status == MFRC522::STATUS_OK) {
    Serial.print("Mensaje escrito en el bloque ");
    Serial.println(bloque);
  } else {
    Serial.print("Error al escribir en el bloque ");
    Serial.print(bloque);
    Serial.print(": ");
    Serial.println(rfid.GetStatusCodeName(status));
  }

  rfid.PICC_HaltA();
}

void restaurarTarjeta() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    Serial.println("No se detectó tarjeta. Acerque una tarjeta.");
    return;
  }

  Serial.println("Iniciando restauración de la tarjeta...");

  for (byte sector = 0; sector < 16; sector++) { // 16 sectores en MIFARE 1KB
    for (byte bloque = 0; bloque < 4; bloque++) {
      byte bloqueActual = sector * 4 + bloque;

      if (bloque == 3) {
        // Saltar bloques reservados (trailers)
        continue;
      }

      if (!autenticarBloque(bloqueActual)) {
        Serial.print("Error al autenticar el bloque ");
        Serial.println(bloqueActual);
        continue;
      }

      // Restaurar con datos predeterminados (todos ceros)
      byte buffer[16] = {0};
      MFRC522::StatusCode status = rfid.MIFARE_Write(bloqueActual, buffer, 16);
      if (status == MFRC522::STATUS_OK) {
        Serial.print("Bloque ");
        Serial.print(bloqueActual);
        Serial.println(" restaurado.");
      } else {
        Serial.print("Error al restaurar el bloque ");
        Serial.print(bloqueActual);
        Serial.print(": ");
        Serial.println(rfid.GetStatusCodeName(status));
      }
    }
  }

  Serial.println("Restauración completada.");
  rfid.PICC_HaltA();
}

bool autenticarBloque(byte bloque) {
  const int intentosMax = 3;
  for (int intento = 0; intento < intentosMax; intento++) {
    MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloque, &key, &(rfid.uid));
    if (status == MFRC522::STATUS_OK) {
      return true; // Éxito en la autenticación
    }
    Serial.print("Intento ");
    Serial.print(intento + 1);
    Serial.print(" de autenticación fallido para el bloque ");
    Serial.print(bloque);
    Serial.print(": ");
    Serial.println(rfid.GetStatusCodeName(status));
    delay(100); // Esperar antes de reintentar
  }
  return false; // Fallo tras varios intentos
}

void identificarTipoTarjeta() {
  MFRC522::PICC_Type tipo = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("Tipo de tarjeta detectada: ");
  Serial.println(rfid.PICC_GetTypeName(tipo));
}

void mostrarInfoTarjeta() {
  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  Serial.println("Leyendo bloques accesibles de la tarjeta...");
  int erroresConsecutivos = 0;

  for (byte sector = 0; sector < 16; sector++) { // 16 sectores en MIFARE 1KB
    for (byte bloque = 0; bloque < 4; bloque++) {
      byte bloqueActual = sector * 4 + bloque;

      if (bloque == 3) {
        // Saltar bloques reservados (trailers)
        continue;
      }

      if (!autenticarBloque(bloqueActual)) {
        Serial.print("Error al autenticar el bloque ");
        Serial.println(bloqueActual);
        erroresConsecutivos++;
        if (erroresConsecutivos >= 3) {
          Serial.println("Demasiados errores consecutivos. Deteniendo la lectura.");
          return;
        }
        continue;
      }

      erroresConsecutivos = 0; // Reiniciar si se autentica correctamente

      MFRC522::StatusCode status = rfid.MIFARE_Read(bloqueActual, buffer, &bufferSize);
      if (status == MFRC522::STATUS_OK) {
        Serial.print("Bloque ");
        Serial.print(bloqueActual);
        Serial.print(": ");

        // Imprimir como texto si es posible
        bool textoDetectado = false;
        for (byte i = 0; i < 16; i++) {
          if (buffer[i] >= 32 && buffer[i] <= 126) {
            textoDetectado = true;
            break;
          }
        }

        if (textoDetectado) {
          Serial.print("Texto: ");
          for (byte i = 0; i < 16; i++) {
            if (buffer[i] == 0) break; // Detener en el primer carácter nulo
            Serial.print((char)buffer[i]);
          }
        } else {
          for (byte i = 0; i < 16; i++) {
            Serial.print(buffer[i] < 0x10 ? " 0" : " ");
            Serial.print(buffer[i], HEX);
          }
        }

        Serial.println();
      } else {
        Serial.print("Error al leer el bloque ");
        Serial.print(bloqueActual);
        Serial.print(": ");
        Serial.println(rfid.GetStatusCodeName(status));
      }
    }
  }

  Serial.println("Lectura completada.");
}
