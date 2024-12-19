# CampiAgricoli_StazioneSerra

Progetto Arduino per monitorare e inviare dati ambientali da una serra tramite tecnologia LoRa. Il codice utilizza sensori per rilevare umidità e temperatura dell'aria e del suolo, visualizzando i dati su un display OLED e trasmettendoli via LoRa.

## Funzionalità principali

- **Monitoraggio ambientale**: Lettura di temperatura e umidità del suolo e dell'aria.
- **Display OLED**: Visualizzazione delle letture in tempo reale.
- **Trasmissione LoRa**: Invio dei dati raccolti tramite tecnologia LoRa.
- **Calibrazione sensore umidità suolo**: Facilità nella configurazione iniziale tramite guida sul display.

## Requisiti hardware

- **Scheda Arduino** compatibile con LoRa (es. Heltec LoRa).
- **Sensore DHT22** per umidità e temperatura dell'aria.
- **Sensore DS18B20** per la temperatura del suolo.
- **Sensore di umidità del suolo**.
- **Display OLED SSD1306**.
- **Modulo LoRa**.

## Dipendenze software

Assicurati di avere le seguenti librerie installate nell'IDE Arduino:
- [LoRaWAN_APP](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series_LoRaWAN)
- [DHTesp](https://github.com/beegee-tokyo/DHTesp)
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [OneWire](https://github.com/PaulStoffregen/OneWire)
- [HT_SSD1306Wire](https://github.com/ThingPulse/esp8266-oled-ssd1306)

## Configurazione

### Calibrazione del sensore di umidità del suolo
Durante l'avvio, il sistema guiderà l'utente nella calibrazione:
1. Posiziona il sensore in un terreno asciutto e attendi l'inizializzazione.
2. Posiziona il sensore in acqua o terreno molto bagnato e attendi la seconda calibrazione.

I valori minimo e massimo saranno utilizzati per calcolare l'umidità del suolo come percentuale.

### Configurazione LoRa
La banda di trasmissione è impostata su 915 MHz:
```cpp
#define BAND 915E6
