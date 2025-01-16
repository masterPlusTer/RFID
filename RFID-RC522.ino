// Incluye la biblioteca MFRC522
#include <SPI.h>
#include <MFRC522.h>

// Definición de pines para el módulo RFID-RC522
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
  Serial.println("Lector RFID-RC522 listo. Acerque una tarjeta...");
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

  // Leer o escribir datos en el bloque 1
  Serial.println("Seleccione una opción: R para leer, W para escribir seguido del mensaje.");
  while (!Serial.available()); // Espera entrada del usuario

  String input = Serial.readStringUntil('\n');
  char opcion = input[0];

  if (opcion == 'R' || opcion == 'r') {
    leerBloque(1);
  } else if (opcion == 'W' || opcion == 'w') {
    if (input.length() > 2) {
      String mensaje = input.substring(2);
      escribirBloque(1, mensaje);
    } else {
      Serial.println("Error: No se proporcionó un mensaje para escribir.");
    }
  } else {
    Serial.println("Opción no válida.");
  }

  // Detener comunicación con la tarjeta
  rfid.PICC_HaltA();
}

void leerBloque(byte bloque) {
  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  // Autenticar el bloque
  if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloque, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
    Serial.println("Error al autenticar el bloque.");
    return;
  }

  // Leer los datos del bloque
  if (rfid.MIFARE_Read(bloque, buffer, &bufferSize) == MFRC522::STATUS_OK) {
    Serial.print("Datos en el bloque ");
    Serial.print(bloque);
    Serial.print(": ");
    for (byte i = 0; i < 16; i++) {
      Serial.write(buffer[i]); // Mostrar datos como texto
    }
    Serial.println();
  } else {
    Serial.println("Error al leer el bloque.");
  }
}

void escribirBloque(byte bloque, String mensaje) {
  byte buffer[16] = {0};

  // Convertir el mensaje en un arreglo de bytes
  for (byte i = 0; i < mensaje.length() && i < 16; i++) {
    buffer[i] = mensaje[i];
  }

  // Autenticar el bloque
  if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloque, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
    Serial.println("Error al autenticar el bloque.");
    return;
  }

  // Escribir en el bloque
  MFRC522::StatusCode status = rfid.MIFARE_Write(bloque, buffer, 16);
  if (status == MFRC522::STATUS_OK) {
    Serial.println("Datos escritos correctamente!");
  } else {
    Serial.print("Error al escribir: ");
    Serial.println(rfid.GetStatusCodeName(status));
  }
}
