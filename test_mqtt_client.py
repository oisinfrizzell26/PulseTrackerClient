#!/usr/bin/env python3
"""
MQTT Test Client for PulseTracker
Simulates ESP32 device sending heart rate and workout data
"""

import paho.mqtt.client as mqtt
import time
import json
import random
import sys

# MQTT Configuration
BROKER = "200.69.13.70"
PORT = 1883
TOPIC_HEART = "pulsetracker/heartRate"
TOPIC_WORKOUT = "pulsetracker/workout"
TOPIC_MODE = "pulsetracker/mode"
TOPIC_BUZZER = "pulsetracker/buzzer"

# Simulation parameters
BASE_HR = 75
HR_VARIATION = 15


def on_connect(client, userdata, flags, rc):
    """Callback when connected to broker"""
    if rc == 0:
        print(f"✓ Connected to MQTT broker at {BROKER}:{PORT}")
        client.subscribe(TOPIC_MODE)
        print(f"✓ Subscribed to {TOPIC_MODE}")
    else:
        print(f"✗ Connection failed with code {rc}")


def on_message(client, userdata, msg):
    """Callback when message received"""
    print(f"← Received on {msg.topic}: {msg.payload.decode()}")


def send_heart_rate(client, bpm):
    """Send heart rate data"""
    payload = str(bpm)
    client.publish(TOPIC_HEART, payload)
    print(f"→ Heart Rate: {bpm} BPM")


def send_workout_event(client, event_data):
    """Send workout event as JSON"""
    payload = json.dumps(event_data)
    client.publish(TOPIC_WORKOUT, payload)
    print(f"→ Workout Event: {payload}")


def send_buzzer_trigger(client):
    """Trigger the buzzer on ESP32"""
    client.publish(TOPIC_BUZZER, "buzz")
    print(f"🔔 Buzzer triggered!")


def simulate_heart_rate(client, duration=30):
    """Simulate continuous heart rate monitoring"""
    print(f"\n═══ Simulating Heart Rate for {duration}s ═══")
    for i in range(duration):
        bpm = BASE_HR + random.randint(-HR_VARIATION, HR_VARIATION)
        bpm = max(50, min(200, bpm))  # Clamp to realistic range
        send_heart_rate(client, bpm)
        time.sleep(1)


def simulate_workout(client):
    """Simulate a complete workout session"""
    print("\n═══ Simulating Complete Workout ═══")
    
    # Start event
    start_event = {
        "event": "start",
        "mode": "Interval",
        "laps": 5
    }
    send_workout_event(client, start_event)
    time.sleep(2)
    
    # Simulate 5 laps
    total_time = 0
    for lap in range(1, 6):
        lap_time = random.randint(35000, 55000)  # 35-55 seconds per lap
        total_time += lap_time
        
        lap_event = {
            "event": "lap",
            "lap": lap,
            "lap_ms": lap_time,
            "split_ms": total_time
        }
        send_workout_event(client, lap_event)
        
        # Send some heart rates during lap
        for _ in range(3):
            bpm = BASE_HR + 20 + random.randint(-10, 20)  # Elevated HR during workout
            send_heart_rate(client, bpm)
            time.sleep(1)
    
    # Done event
    done_event = {
        "event": "done",
        "laps": 5,
        "total_ms": total_time
    }
    send_workout_event(client, done_event)
    print(f"✓ Workout complete: {total_time/1000:.1f}s total")


def simulate_stopped_workout(client):
    """Simulate a workout that gets stopped early"""
    print("\n═══ Simulating Stopped Workout ═══")
    
    # Start
    start_event = {
        "event": "start",
        "mode": "Freerun",
        "laps": 10
    }
    send_workout_event(client, start_event)
    time.sleep(2)
    
    # Complete 3 laps
    total_time = 0
    for lap in range(1, 4):
        lap_time = random.randint(40000, 50000)
        total_time += lap_time
        
        lap_event = {
            "event": "lap",
            "lap": lap,
            "lap_ms": lap_time,
            "split_ms": total_time
        }
        send_workout_event(client, lap_event)
        time.sleep(2)
    
    # Stop workout
    stop_event = {
        "event": "stop",
        "laps": 3,
        "total_ms": total_time
    }
    send_workout_event(client, stop_event)
    print(f"⏸ Workout stopped at lap 3")


def simulate_status_updates(client):
    """Simulate periodic status updates during workout"""
    print("\n═══ Simulating Status Updates ═══")
    
    elapsed = 0
    for lap in range(1, 4):
        elapsed += random.randint(40000, 50000)
        
        status_event = {
            "event": "status",
            "state": "running",
            "lap": lap,
            "elapsed_ms": elapsed
        }
        send_workout_event(client, status_event)
        time.sleep(2)


def interactive_menu(client):
    """Interactive menu for manual testing"""
    while True:
        print("\n" + "="*50)
        print("MQTT Test Client Menu")
        print("="*50)
        print("1. Send single heart rate")
        print("2. Simulate heart rate (30s)")
        print("3. Simulate complete workout")
        print("4. Simulate stopped workout")
        print("5. Send status updates")
        print("6. Send custom workout event")
        print("7. Send custom heart rate")
        print("8. 🔔 Trigger buzzer")
        print("9. 🔔 Test buzzer (5 beeps)")
        print("0. Exit")
        print("="*50)
        
        choice = input("Select option: ").strip()
        
        if choice == "1":
            bpm = BASE_HR + random.randint(-HR_VARIATION, HR_VARIATION)
            send_heart_rate(client, bpm)
        
        elif choice == "2":
            simulate_heart_rate(client, 30)
        
        elif choice == "3":
            simulate_workout(client)
        
        elif choice == "4":
            simulate_stopped_workout(client)
        
        elif choice == "5":
            simulate_status_updates(client)
        
        elif choice == "6":
            print("\nEvent types: start, lap, done, stop, status")
            event_type = input("Event type: ").strip()
            
            if event_type == "start":
                mode = input("Mode (Interval/Freerun): ").strip() or "Interval"
                laps = int(input("Total laps: ").strip() or "5")
                event = {"event": "start", "mode": mode, "laps": laps}
            
            elif event_type == "lap":
                lap = int(input("Lap number: ").strip() or "1")
                lap_ms = int(input("Lap time (ms): ").strip() or "45000")
                split_ms = int(input("Split time (ms): ").strip() or "45000")
                event = {"event": "lap", "lap": lap, "lap_ms": lap_ms, "split_ms": split_ms}
            
            elif event_type == "done":
                laps = int(input("Total laps: ").strip() or "5")
                total_ms = int(input("Total time (ms): ").strip() or "225000")
                event = {"event": "done", "laps": laps, "total_ms": total_ms}
            
            elif event_type == "stop":
                laps = int(input("Laps completed: ").strip() or "3")
                total_ms = int(input("Total time (ms): ").strip() or "135000")
                event = {"event": "stop", "laps": laps, "total_ms": total_ms}
            
            elif event_type == "status":
                state = input("State: ").strip() or "running"
                lap = int(input("Current lap: ").strip() or "2")
                elapsed_ms = int(input("Elapsed time (ms): ").strip() or "87340")
                event = {"event": "status", "state": state, "lap": lap, "elapsed_ms": elapsed_ms}
            
            else:
                print("Invalid event type")
                continue
            
            send_workout_event(client, event)
        
        elif choice == "7":
            bpm = int(input("BPM value (50-200): ").strip())
            send_heart_rate(client, bpm)
        
        elif choice == "8":
            send_buzzer_trigger(client)
        
        elif choice == "9":
            print("Sending 5 buzzer triggers...")
            for i in range(5):
                print(f"  Beep {i+1}/5")
                send_buzzer_trigger(client)
                time.sleep(1)  # Wait 1 second between beeps
            print("✓ Test complete")
        
        elif choice == "0":
            print("Exiting...")
            break
        
        else:
            print("Invalid option")


def main():
    """Main function"""
    print("PulseTracker MQTT Test Client")
    print(f"Target Broker: {BROKER}:{PORT}")
    print()
    
    # Create MQTT client
    client = mqtt.Client(client_id="PulseTrackerTest", clean_session=True)
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        # Connect to broker
        print(f"Connecting to {BROKER}:{PORT}...")
        client.connect(BROKER, PORT, 60)
        client.loop_start()
        time.sleep(1)
        
        # Check if we want auto mode or interactive
        if len(sys.argv) > 1 and sys.argv[1] == "--auto":
            print("\n🤖 AUTO MODE - Running test sequence\n")
            simulate_heart_rate(client, 10)
            time.sleep(2)
            simulate_workout(client)
            time.sleep(2)
            simulate_stopped_workout(client)
            time.sleep(2)
            simulate_status_updates(client)
            print("\n✓ Auto test sequence complete")
        else:
            interactive_menu(client)
        
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    except Exception as e:
        print(f"\n✗ Error: {e}")
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected")


if __name__ == "__main__":
    main()
