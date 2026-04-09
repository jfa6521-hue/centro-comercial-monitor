# Centro Comercial — Sistema de Monitoreo
## Campos y Ondas 2026-1 · Universidad del Quindío

---

## Estructura del proyecto

```
esp32_comercial/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── app_config.h        ← ⚠️ EDITAR primero
│   ├── main.c
│   ├── wifi_manager.c/h
│   ├── dht22.c/h
│   ├── sensors.c/h
│   └── cloud_sync.c/h
└── web/
    └── dashboard.html      ← Subir a GitHub Pages / Firebase Hosting
```

---

## Pasos para compilar y flashear

### 1. Requisitos
- ESP-IDF v5.x instalado (`idf.py --version`)
- Python 3.8+
- ESP32 con al menos 4 MB de flash

### 2. Configurar credenciales

Abre `main/app_config.h` y edita:

```c
#define STA_SSID         "TU_RED_WIFI"     // Red con Internet
#define STA_PASS         "TU_PASSWORD"
#define FIREBASE_HOST    "TU_PROYECTO-default-rtdb.firebaseio.com"
#define FIREBASE_SECRET  "TU_DATABASE_SECRET"
```

### 3. Compilar y flashear

```bash
cd esp32_comercial
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor   # Ajusta el puerto
```

---

## Configurar Firebase (gratuito)

1. Ve a https://console.firebase.google.com
2. Crea un proyecto → Realtime Database → Modo test (30 días)
3. Copia la URL: `https://TU_PROYECTO-default-rtdb.firebaseio.com`
4. Ve a Configuración → Cuentas de servicio → Secreto de base de datos
5. Pega el secreto en `app_config.h` → `FIREBASE_SECRET`

**Reglas de seguridad para el prototipo** (Realtime DB → Reglas):
```json
{
  "rules": {
    ".read": true,
    ".write": "auth != null"
  }
}
```
Para laboratorio puedes usar `.write: true` temporalmente.

---

## Publicar el Dashboard Web

### Opción A — GitHub Pages (gratis, fácil)
1. Crea un repositorio en GitHub
2. Sube `web/dashboard.html` como `index.html`
3. Settings → Pages → Branch: main → Guardar
4. URL: `https://TU_USUARIO.github.io/TU_REPO/`

### Opción B — Firebase Hosting
```bash
npm install -g firebase-tools
firebase login
firebase init hosting   # selecciona tu proyecto
# pon dashboard.html en la carpeta public/ como index.html
firebase deploy
```

Antes de subir, edita el bloque `firebaseConfig` en `dashboard.html`
con las credenciales de tu proyecto (Consola → Configuración → Tu app web).

---

## Conexiones de Hardware

| Componente        | GPIO ESP32 |
|-------------------|-----------|
| DHT22 (Data)      | GPIO 4    |
| Sensor IR People  | GPIO 5    |
| Señal NFC event   | GPIO 18   |

**DHT22:** VCC → 3.3 V, GND → GND, DATA → GPIO4 (resistencia pull-up 10 kΩ entre VCC y DATA).

**Sensor IR (e.g. FC-51):** OUT → GPIO5. El ESP32 cuenta personas en el flanco de bajada (rayo interrumpido).

**NFC event:** El módulo NFC (PN532/RC522) conecta GPIO18 a GND por ~100 ms cuando concede acceso.

---

## Datos en Firebase

El ESP32 escribe cada 5 s en `/monitor`:
```json
{
  "internet":        true,
  "connected_ap":    3,
  "temperature":     24.5,
  "humidity":        61.2,
  "people_inside":   8,
  "nfc_total":       23,
  "nfc_last_access": "2026-04-09T10:30:00",
  "dht_ok":          true,
  "timestamp":       1744200000
}
```

---

## Criterios del proyecto cubiertos

| Requisito                             | Estado |
|---------------------------------------|--------|
| AP WiFi ESP32 (mín. 3, máx. 4 disp.) | ✅     |
| Acceso a Internet verificable         | ✅     |
| Alimentación solar (TP4056 externo)   | 🔌 HW  |
| Temperatura ambiente (DHT22)          | ✅     |
| Conteo de personas (IR GPIO)          | ✅     |
| Dispositivos conectados en tiempo real| ✅     |
| Dashboard en la nube (sin memoria ESP)| ✅     |
| Eventos NFC contabilizados            | ✅     |
