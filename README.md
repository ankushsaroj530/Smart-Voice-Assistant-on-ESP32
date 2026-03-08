# ESP32 AI Voice Assistant 

An ESP32‑based AI voice assistant that connects to WiFi, listens to user questions via a browser interface, sends them to the Groq API for processing, and speaks the answers aloud through a speaker using I2S audio. The assistant always prompts the user to continue with “Please ask next question.” This project also includes optional embedded demo media (images and video) to illustrate functionality when viewing the repository.

-

## Features
- Voice recognition in the browser (Web Speech API)
- AI responses powered by Groq API (`llama-3.3-70b-versatile` model)
- Text‑to‑speech output directly from ESP32 using Google Translate TTS
- Clean, modern web interface served directly by ESP32
- Maximum volume output via I2S audio (`audio.setVolume(40)`)
- Lightweight design: runs entirely on ESP32, no external server required
- Optional embedded demo media:
  - Images of the hardware setup
  - Video demonstration of voice assistant interaction (plays in compatible PDF/PPT or web interface)
- Automatic prompt to encourage continued interaction



##  Hardware Requirements
- **ESP32 development board** (ESP32‑DevKitC, NodeMCU‑ESP32, etc.)
- **I2S DAC / amplifier module** (e.g., MAX98357A)
- **Speaker** (connected to DAC output pins)
- **Micro‑USB cable** for programming and power
- **WiFi network** (SSID and password configured in code)
- **Optional:** USB webcam or smartphone camera for video demonstration capture



##  Connections
ESP32 → I2S DAC (MAX98357A or similar):

| ESP32 Pin | DAC Pin   | Description          |
|-----------|-----------|--------------------|
| GPIO22    | DIN       | I2S Data Out       |
| GPIO26    | BCLK      | I2S Bit Clock      |
| GPIO25    | LRC (WS)  | I2S Word Select (LR) |
| 3.3V      | VIN       | Power              |
| GND       | GND       | Ground             |

Speaker connects to DAC output pins (L+, L‑).

---

