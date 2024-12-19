#include "HT_SSD1306Wire.h"
#include "DHTesp.h"  // Libreria per il sensore di temperatura e umidità DHT22
#include <OneWire.h> // Libreria per il sensore di temperatura del suolo ds18b20, modificata per incompatibilità GPIO_IS_VALID_PIN
#include <DallasTemperature.h> // Libreria per il sensore di temperatura del suolo ds18b20

// Definizione della banda LoRa
#define BAND    915E6  // Banda di frequenza (915 MHz in questo caso)
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // Display OLED (Indirizzo I2C, frequenza, SDA, SCL, risoluzione, reset)
DHTesp dht;  // Oggetto per il sensore DHT
OneWire oneWire(17); // Imposta la connessione OneWire sul pin GPIO17
DallasTemperature ds18b20(&oneWire); // Oggetto per il sensore di temperatura del suolo DS18B20

// Variabili per la calibrazione del sensore di umidità del suolo
int minMoistureValue = 0;  // Inizializzazione del valore minimo (suolo asciutto)
int maxMoistureValue = 0;  // Inizializzazione del valore massimo (suolo bagnato)

// SEZIONE LORA

#include "LoRaWan_APP.h"
#include "Arduino.h"

// Parametri di configurazione LoRa
#define RF_FREQUENCY 915000000  // Frequenza LoRa (915 MHz)
#define TX_OUTPUT_POWER 14      // Potenza di trasmissione in dBm
#define LORA_BANDWIDTH 0        // Larghezza di banda (125 kHz)
#define LORA_SPREADING_FACTOR 7 // Fattore di spreading (SF7)
#define LORA_CODINGRATE 1       // Codifica (4/5)
#define LORA_PREAMBLE_LENGTH 8  // Lunghezza del preambolo
#define LORA_SYMBOL_TIMEOUT 0   // Timeout simboli
#define LORA_FIX_LENGTH_PAYLOAD_ON false  // Lunghezza fissa del payload
#define LORA_IQ_INVERSION_ON false  // Inversione IQ disabilitata

#define RX_TIMEOUT_VALUE 1000  // Timeout di ricezione
#define BUFFER_SIZE 100        // Dimensione del buffer per il payload

char txpacket[BUFFER_SIZE];  // Pacchetto da inviare

static RadioEvents_t RadioEvents;  // Gestione eventi radio

int16_t txNumber;  // Numero di trasmissioni

int16_t rssi, rxSize;  // RSSI (potenza del segnale ricevuto) e dimensione del pacchetto ricevuto

bool lora_idle = true;  // Flag per verificare se LoRa è inattivo

// Funzioni di callback per la gestione della trasmissione
void OnTxDone(void) {
    Serial.println("TX done......");
    lora_idle = true;
}

void OnTxTimeout(void) {
    Radio.Sleep();
    Serial.println("TX Timeout......");
    lora_idle = true;
}

// FINE SEZIONE LORA

// FUNZIONI DI APPOGGIO

int numeroLettura = 1;  // Numero della lettura per visualizzare sul display
float soilTemperature, airHumidity, airTemperature;  // Variabili per temperatura e umidità
int soilMoisture;  // Umidità del suolo

void showCalibrationGuide(String message, int yPosition) {
    display.clear();
    display.display();
    
    // Dividi il messaggio in righe di massimo 21 caratteri (approssimativamente adatti per lo schermo OLED 128x64)
    int maxCharsPerLine = 21;
    int lineSpacing = 12;  // Distanza verticale tra le righe

    int currentY = yPosition;
    for (int i = 0; i < message.length(); i += maxCharsPerLine) {
        String line = message.substring(i, min(i + maxCharsPerLine, (int)message.length()));
        display.drawString(0, currentY, line);
        currentY += lineSpacing;
    }

    display.display();
}

// Funzione per visualizzare le informazioni sul display OLED
void showInfoDisplay(int numeroLettura, float soilMoisture, float soilTemp, float airHumidity, float airTemp) {
    display.clear();
    display.display();

    // Mostra il numero della lettura
    display.drawString(0, 0, "Lettura: " + String(numeroLettura));

    // Mostra l'umidità del suolo
    display.drawString(0, 12, "Umid Suolo: " + (soilMoisture >= 0 ? String(soilMoisture) + "%" : "--"));

    // Mostra la temperatura del suolo
    display.drawString(0, 24, "Temp Suolo: " + (soilTemp >= 0 ? String(soilTemp) + "C" : "--"));

    // Mostra l'umidità dell'aria
    display.drawString(0, 36, "Umid Aria: " + (airHumidity >= 0 ? String(airHumidity) + "%" : "--"));

    // Mostra la temperatura dell'aria
    display.drawString(0, 48, "Temp Aria: " + (airTemp >= 0 ? String(airTemp) + "C" : "--"));

    display.display();  // Aggiorna il display con le nuove informazioni
}

// Funzione di calibrazione del sensore di umidità del suolo
void calibrateSoilMoisture(int soilMoistureValue) {
    // Se il valore letto è inferiore al minimo, aggiorna il valore minimo
    if (soilMoistureValue < minMoistureValue) {
        minMoistureValue = soilMoistureValue;
        Serial.print("Nuovo Minimo Umidità: ");
        Serial.println(minMoistureValue);
    }

    // Se il valore letto è superiore al massimo, aggiorna il valore massimo
    if (soilMoistureValue > maxMoistureValue) {
        maxMoistureValue = soilMoistureValue;
        Serial.print("Nuovo Massimo Umidità: ");
        Serial.println(maxMoistureValue);
    }
}

// Funzione per leggere l'umidità del suolo
int readSoilMoisture() {
    int soilMoistureValue = analogRead(A0);  // Legge il valore analogico dal sensore

    // Esegue la calibrazione passando il valore appena letto
    calibrateSoilMoisture(soilMoistureValue);

    // Mappa il valore da 0 a 100 (percentuale di umidità)
    long mappedSoilMoistureValue = map(soilMoistureValue, maxMoistureValue, minMoistureValue, 0, 100);

    return mappedSoilMoistureValue;  // Ritorna il valore calibrato in percentuale
}

// Funzione per leggere la temperatura del suolo
float readSoilTemperature() {
    ds18b20.requestTemperatures();  // Richiesta di lettura della temperatura

    // Restituisce la temperatura in Celsius dal sensore DS18B20
    float soilTemperatureCelsius = ds18b20.getTempCByIndex(0);

    return soilTemperatureCelsius;  // Ritorna la temperatura del suolo
}

// Funzione per leggere l'umidità dell'aria
float readAirHumidity() {
    float humidity = dht.getHumidity();  // Legge l'umidità dell'ambiente

    // Se il valore non è valido, restituisce -999 per indicare errore
    return isnan(humidity) ? -999 : humidity;
}

// Funzione per leggere la temperatura dell'aria
float readAirTemperature() {
    float temperature = dht.getTemperature();  // Legge la temperatura dell'ambiente

    // Se il valore non è valido, restituisce -999 per indicare errore
    return isnan(temperature) ? -999 : temperature;
}

// FINE FUNZIONI

void setup() {
    Serial.begin(115200);  // Inizializza la comunicazione seriale

    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);  // Inizializzazione della board HELTEC

    display.init(); // Inizializza il display OLED
    display.flipScreenVertically(); // Ribalta sull'asse verticale lo schermo
    delay(2000); // Piccola pausa per inizializzazione display e pausa pre calibrazione

    dht.setup(2, DHTesp::DHT22);  // Configura il sensore DHT22 sul pin 2
    ds18b20.begin();  // Inizializza il sensore di temperatura del suolo DS18B20

    // Guida calibrazione iniziale sul display
    showCalibrationGuide("Inizio calibrazione...", 0);
    delay(2000);

    showCalibrationGuide("Fai in modo che la sonda sia asciutta.", 15);
    delay(5000);  // Pausa di 5 secondi per posizionare il sensore nel terreno asciutto
    minMoistureValue = analogRead(A0);  // Imposta il valore minimo (asciutto)
    Serial.print("Valore Minimo Iniziale: ");
    Serial.println(minMoistureValue);

    showCalibrationGuide("Posiziona la sonda nell'acqua.", 15);
    delay(5000);  // Pausa di 5 secondi per posizionare il sensore nel terreno bagnato
    maxMoistureValue = analogRead(A0);  // Imposta il valore massimo (bagnato)
    Serial.print("Valore Massimo Iniziale: ");
    Serial.println(maxMoistureValue);

    // Dopo la calibrazione, mostra il messaggio finale
    showCalibrationGuide("Calibrazione completata!", 15);
    delay(2000);
    display.clear();  // Pulisce il display
    display.display();

    // Configurazione della trasmissione LoRa
    txNumber = 0;
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;

    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
                      LORA_FIX_LENGTH_PAYLOAD_ON, true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void loop() {
    // Se LoRa è inattivo, invia i dati ogni 10 secondi
    if (lora_idle == true) {
        delay(10000);

        // Legge i valori dai sensori
        soilMoisture = readSoilMoisture();
        soilTemperature = readSoilTemperature();
        airHumidity = readAirHumidity();
        airTemperature = readAirTemperature();

        // Incrementa numero lettura
        numeroLettura = numeroLettura + 1;

        // Formatta i dati da inviare
        sprintf(txpacket, "SoilMoisture:%d SoilTemp:%.2f AirHum:%.2f AirTemp:%.2f",
                soilMoisture, soilTemperature, airHumidity, airTemperature);

        Serial.printf("\r\nSending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));


        // Invia il pacchetto via LoRa
        Radio.Send((uint8_t *)txpacket, strlen(txpacket));
        lora_idle = false;

        // Chiama la funzione per aggiornare il display con le letture
        showInfoDisplay(numeroLettura, soilMoisture, soilTemperature, airHumidity, airTemperature);
    }

    

    Radio.IrqProcess();  // Gestisce le interruzioni LoRa
}
