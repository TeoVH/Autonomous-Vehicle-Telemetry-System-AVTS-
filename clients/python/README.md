# Cliente Python (Tkinter) — Telemática

Cliente gráfico para conectarse vía **TCP** a un servidor de telemetría, realizar **LOGIN** (`LOGIN <ROLE> <USER> <PASS>`),
visualizar **STATUS/DATA** en tiempo real y enviar comandos: **START→SPEED UP**, **TURN LEFT**, **TURN RIGHT**, **BATTERY RESET**.
Incluye **modo demo** que genera telemetría local cada 1s sin servidor.

## Requisitos
- Python 3.10+ (Tkinter incluido; en Linux: `sudo apt-get install python3-tk`)
- Sin dependencias externas.

## Ejecución
```bash
cd clients/python
python app.py
