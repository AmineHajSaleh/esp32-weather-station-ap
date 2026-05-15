# 🌤️ ESP32 Multi-Sensor Weather Station (AP Mode)

## 📝 Description
Ce projet est une station météorologique embarquée totalement autonome basée sur un microcontrôleur ESP32. Le système configure l'ESP32 en mode **Point d'Accès (Soft-AP)**, lui permettant d'émettre son propre réseau Wi-Fi de manière isolée.

Il intègre l'agrégation de données de deux bus de communication différents :
* **Bus 1-Wire** : Lecture précise de la température via un capteur DS18B20.
* **Bus I2C** : Lecture de la température, de l'humidité relative, de la pression atmosphérique et calcul de l'altitude via un capteur BME280.

L'interface web intégrée utilise du JavaScript asynchrone (Fetch API) pour actualiser les mesures toutes les 5 secondes via un endpoint JSON (`/sensors`), sans nécessiter le rechargement de la page.

## 🛠️ Matériel Utilisé
* **Microcontrôleur :** ESP32
* **Capteur 1 :** DS18B20 (Sonde de température étanche)
* **Capteur 2 :** BME280 (Capteur environnemental)
* **Passifs :** 1x Résistance de tirage 4.7 kΩ

## 🔌 Câblage Électrique

**DS18B20 (1-Wire)**
* `VCC` ➔ `3.3V`
* `GND` ➔ `GND`
* `DATA` ➔ `GPIO 13` *(avec résistance pull-up 4.7kΩ vers le 3.3V)*

**BME280 (I2C)**
* `VIN` ➔ `3.3V`
* `GND` ➔ `GND`
* `SDA` ➔ `GPIO 21`
* `SCL` ➔ `GPIO 22`
## 🔌 Schéma Électrique
![Schéma de câblage Fritzing/Wokwi](https://github.com/AmineHajSaleh/esp32-weather-station-ap/blob/main/Screenshot%202026-05-15%20113403.png)

## 🚀 Installation et Configuration

1. Cloner ce dépôt :
   ```bash
   git clone [https://github.com/AmineHajSaleh/esp32-weather-station-ap.git](https://github.com/AmineHajSaleh/esp32-weather-station-ap.git)
