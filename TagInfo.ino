// acerca cualquier tag al lector y te va a decir que tipo de tag es y su capacidad 

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9     // Pin de reset
#define SS_PIN 10     // Pin de selección SPI

MFRC522 mfrc522(SS_PIN, RST_PIN); // Crea el objeto RFID

void setup() {
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Escanea una tarjeta...");
}

void loop() {
    // Si no hay tarjeta, salir
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    Serial.print("Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    
    Serial.print("PICC Type: ");
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
