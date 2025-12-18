# MQTT Test Client

Test script to simulate ESP32 PulseTracker sending data to MQTT broker.

## Setup

```bash
pip install -r requirements.txt
```

## Usage

### Interactive Mode (Menu)
```bash
python test_mqtt_client.py
```

### Auto Mode (Run all tests)
```bash
python test_mqtt_client.py --auto
```

## Features

- Send individual heart rate readings
- Simulate continuous heart rate monitoring
- Simulate complete workout sessions
- Simulate stopped workouts
- Send custom events
- Subscribe to mode topic

## Data Sent

**Route**: `pulsetracker/heartRate`
- Format: Integer string (e.g., "72")

**Route**: `pulsetracker/workout`
- Format: JSON events (start, lap, done, stop, status)

## Broker

- Host: 200.69.13.70
- Port: 1883
