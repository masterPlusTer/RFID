//este codigo muestra info detallada de la tarjeta o tag rfid, solo acercalo al lector y dejalo ahi un ratito hasta que se complete la lectura




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
  Serial.println("Mini RFID-RC522 listo. Acerque una tarjeta...");
}

void loop() {
  // Verifica si hay una tarjeta nueva presente
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Verifica si se puede leer la tarjeta
  if (!rfid.PICC_ReadCardSerial()) {
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
