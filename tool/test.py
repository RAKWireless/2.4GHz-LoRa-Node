#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import json
import base64
from datetime import datetime
import threading
import time

# MQTT connection configuration
MQTT_BROKER = "140.179.175.182"  # IP from your screenshot
MQTT_PORT = 7483
MQTT_USERNAME = ""  # Fill in if username is required
MQTT_PASSWORD = ""  # Fill in if password is required

# ChirpStack MQTT topics
UPLINK_TOPIC = "application/+/device/+/event/up"  # Subscribe to uplink data from all applications and devices

class ChirpStackSubscriber:
    def __init__(self):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
        # Multi-device packet loss statistics
        self.devices = {}  # Dictionary to store per-device statistics
        
        # Simple logging mode
        self.simple_log = True  # Enable simplified logging
        
        # If authentication is needed, uncomment the following two lines
        # self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"Successfully connected to ChirpStack MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
            print(f"Subscribed to topic: {UPLINK_TOPIC}")
            client.subscribe(UPLINK_TOPIC)
        else:
            print(f"Connection failed, error code: {rc}")
            
    def on_disconnect(self, client, userdata, rc):
        print(f"Disconnected from MQTT Broker, code: {rc}")
        
    def on_message(self, client, userdata, msg):
        try:
            # Parse message
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            
            # Extract device information
            device_info = payload.get('deviceInfo', {})
            device_name = device_info.get('deviceName', 'Unknown')
            device_eui = device_info.get('devEui', 'Unknown')
            application_name = device_info.get('applicationName', 'Unknown')
            
            # Extract uplink data information
            uplink_id = payload.get('uplinkId', 'Unknown')
            f_cnt = payload.get('fCnt', 0)
            f_port = payload.get('fPort', 0)
            data_base64 = payload.get('data', '')
            
            # Decode Base64 data
            try:
                raw_data = base64.b64decode(data_base64)
                hex_data = raw_data.hex().upper()
            except:
                hex_data = "Decode failed"
                
            # Extract RF information
            rx_info = payload.get('rxInfo', [])
            if rx_info:
                rf_info = rx_info[0]  # Take the first reception information
                rssi = rf_info.get('rssi', 'N/A')
                snr = rf_info.get('snr', 'N/A')
                gateway_id = rf_info.get('gatewayId', 'Unknown')
            else:
                rssi = snr = gateway_id = 'N/A'
                
            # Get current time
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Simplified logging - only show basic info for non-Range Test data
            if not (len(raw_data) == 10 and raw_data[:3] == b'RAK'):
                if not self.simple_log:
                    # Print formatted uplink data (full details)
                    print("=" * 80)
                    print(f"Time: {timestamp}")
                    print(f"Device Name: {device_name}")
                    print(f"Device EUI: {device_eui}")
                    print(f"Application Name: {application_name}")
                    print(f"Frame Count: {f_cnt}")
                    print(f"FPort: {f_port}")
                    print(f"Raw Data (Base64): {data_base64}")
                    print(f"Hex Data: {hex_data}")
                    print(f"RSSI: {rssi} dBm")
                    print(f"SNR: {snr} dB")
                    print(f"Gateway ID: {gateway_id}")
                    print("=" * 80)
                    print()
                else:
                    # Simple log for non-Range Test data
                    log_line = f"[{timestamp}] Non-Range Test | DevEUI: {device_eui} | RSSI: {rssi} dBm | SNR: {snr} dB"
                    print(log_line)
                    print("-" * len(log_line))
                return  # Skip Range Test processing for non-Range Test data
            
            # If it's Range Test data, try to parse
            if len(raw_data) == 10 and raw_data[:3] == b'RAK':
                # Parse extended range test data
                extended_data = self.parse_extended_range_test_data(raw_data)
                
                if extended_data:
                    packet_counter = extended_data['packet_counter']
                    interval_seconds = extended_data.get('interval_seconds', 5)
                    target_count = extended_data.get('target_count', 100)
                    checksum_ok = extended_data.get('checksum_ok', False)
                    
                    # Get device statistics
                    device_stats = self.get_or_create_device_stats(device_eui)
                    device_stats['device_name'] = device_name
                    device_stats['last_rssi'] = rssi
                    device_stats['last_snr'] = snr
                    
                    # Update device configuration from packet data
                    if 'interval_seconds' in extended_data:
                        if interval_seconds > 0 and interval_seconds != device_stats['uplink_interval']:
                            device_stats['uplink_interval'] = interval_seconds
                        
                        if target_count > 0 and target_count != device_stats['expected_packet_count']:
                            device_stats['expected_packet_count'] = target_count
                    
                    # Update packet loss statistics for this device
                    packet_loss_rate, missing_packets = self.update_packet_statistics_for_device(device_eui, packet_counter)
                    
                    # Simplified Range Test log output with count info
                    print(f"[{timestamp}] Range Test | DevEUI: {device_eui} | Loss: {packet_loss_rate:.1f}% | Expected: {device_stats['total_expected']} | Received: {device_stats['total_received']} | RSSI: {rssi} dBm | SNR: {snr} dB | Count: {packet_counter}")
                    
                    # Show checksum status if available
                    if not checksum_ok and 'interval_seconds' in extended_data:
                        print(f"  ⚠️  Checksum ERROR for packet {packet_counter}")
                    
                    # Add separator line for clarity
                    print("=" * 150)
                    
                    # Update last packet time and restart timer for this device
                    device_stats['last_packet_time'] = time.time()
                    if device_stats['timer_enabled']:
                        self.start_timeout_timer_for_device(device_eui)
                    else:
                        # Start monitoring for new device
                        self.start_monitoring_for_device(device_eui)
                        self.start_timeout_timer_for_device(device_eui)
            
            # No need for separator lines in simple mode
            
        except json.JSONDecodeError:
            print(f"JSON parsing error: {msg.payload}")
        except Exception as e:
            print(f"Error processing message: {e}")
            print(f"Topic: {topic}")
            print(f"Message: {msg.payload}")
            
    def start(self):
        try:
            print(f"Starting ChirpStack uplink data subscriber...")
            print(f"Connecting to: {MQTT_BROKER}:{MQTT_PORT}")
            print("Multi-device packet loss monitoring enabled")
            
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_forever()
            
        except KeyboardInterrupt:
            print("\nUser interrupted, disconnecting...")
            self.stop_all_monitoring()
            self.client.disconnect()
        except Exception as e:
            print(f"Connection error: {e}")
            self.stop_all_monitoring()
            
    def reset_statistics_for_device(self, device_eui):
        """Reset packet loss statistics for specific device"""
        device_stats = self.get_or_create_device_stats(device_eui)
        device_stats['received_packets'].clear()
        device_stats['max_packet_number'] = -1
        device_stats['total_expected'] = 0
        device_stats['total_received'] = 0
        device_stats['last_packet_time'] = None
        
        # Cancel existing timer
        if device_stats['timer']:
            device_stats['timer'].cancel()
            device_stats['timer'] = None
            
        print(f"[RESET] DevEUI: {device_eui} - Statistics reset")
        
        # Enable timer monitoring for new session
        device_stats['timer_enabled'] = True
    
    def update_packet_statistics_for_device(self, device_eui, packet_number):
        """Update packet loss statistics for specific device"""
        device_stats = self.get_or_create_device_stats(device_eui)
        
        # If packet number is 0, reset statistics for this device
        if packet_number == 0:
            if device_stats['total_received'] > 0:  # Only show stats if we had previous data
                self.print_final_statistics_for_device(device_eui)
            self.reset_statistics_for_device(device_eui)
        
        # Update statistics
        device_stats['received_packets'].add(packet_number)
        device_stats['total_received'] += 1
        
        if packet_number > device_stats['max_packet_number']:
            device_stats['max_packet_number'] = packet_number
            
        # Calculate expected packets (from 0 to max_packet_number)
        device_stats['total_expected'] = device_stats['max_packet_number'] + 1
        
        # Calculate packet loss
        missing_packets = []
        for i in range(device_stats['total_expected']):
            if i not in device_stats['received_packets']:
                missing_packets.append(i)
                
        packet_loss_rate = (len(missing_packets) / device_stats['total_expected']) * 100 if device_stats['total_expected'] > 0 else 0
        
        return packet_loss_rate, missing_packets
    
    def print_final_statistics_for_device(self, device_eui):
        """Print final statistics for specific device"""
        device_stats = self.devices.get(device_eui)
        if not device_stats:
            return
            
        missing_packets = []
        for i in range(device_stats['total_expected']):
            if i not in device_stats['received_packets']:
                missing_packets.append(i)
                
        packet_loss_rate = (len(missing_packets) / device_stats['total_expected']) * 100 if device_stats['total_expected'] > 0 else 0
        
        print(f"[FINAL] DevEUI: {device_eui} | Loss: {packet_loss_rate:.1f}% | Expected: {device_stats['total_expected']} | Received: {device_stats['total_received']} | Last RSSI: {device_stats.get('last_rssi', 'N/A')} dBm | Last SNR: {device_stats.get('last_snr', 'N/A')} dB")
    
    def print_all_device_statistics(self):
        """Print statistics for all devices"""
        if not self.devices:
            print("No devices registered yet")
            return
            
        print(f"\n{'='*80}")
        print("SUMMARY - All Device Statistics")
        print(f"{'='*80}")
        
        for device_eui, device_stats in self.devices.items():
            missing_packets = []
            for i in range(device_stats['total_expected']):
                if i not in device_stats['received_packets']:
                    missing_packets.append(i)
            
            packet_loss_rate = (len(missing_packets) / device_stats['total_expected']) * 100 if device_stats['total_expected'] > 0 else 0
            
            print(f"Device: {device_stats['device_name']} ({device_eui})")
            print(f"  Loss Rate: {packet_loss_rate:.2f}% | Received: {device_stats['total_received']}/{device_stats['total_expected']} | Interval: {device_stats['uplink_interval']}s")
            
        print(f"{'='*80}")
    
    # Remove old single-device methods
    # (The old methods will be replaced by the new per-device methods)
    
    def parse_extended_range_test_data(self, raw_data):
        """Parse extended range test data with interval and target count"""
        if len(raw_data) >= 10 and raw_data[:3] == b'RAK':
            packet_counter = raw_data[3]
            
            # Parse extended data if available
            if len(raw_data) >= 9:
                interval_seconds = raw_data[5] | (raw_data[6] << 8)
                target_count = raw_data[7] | (raw_data[8] << 8)
                
                # Verify checksum if available
                if len(raw_data) >= 10:
                    received_checksum = raw_data[9]
                    expected_checksum = packet_counter ^ interval_seconds
                    checksum_ok = (received_checksum == expected_checksum)
                    
                    return {
                        'packet_counter': packet_counter,
                        'interval_seconds': interval_seconds,
                        'target_count': target_count,
                        'checksum_ok': checksum_ok
                    }
            
            return {'packet_counter': packet_counter}
        return None
    
    def get_or_create_device_stats(self, device_eui):
        """Get or create device statistics structure"""
        if device_eui not in self.devices:
            self.devices[device_eui] = {
                'received_packets': set(),
                'max_packet_number': -1,
                'total_expected': 0,
                'total_received': 0,
                'uplink_interval': 5,
                'last_packet_time': None,
                'timer': None,
                'timer_enabled': False,
                'expected_packet_count': 100,
                'device_name': 'Unknown'
            }
            print(f"New device registered: {device_eui}")
        return self.devices[device_eui]
    
    def start_timeout_timer_for_device(self, device_eui):
        """Start or restart timeout timer for specific device"""
        device_stats = self.devices.get(device_eui)
        if not device_stats:
            return
            
        # Cancel existing timer
        if device_stats['timer']:
            device_stats['timer'].cancel()
        
        if device_stats['timer_enabled']:
            # Start new timer with extra margin (1.5x interval + 2 seconds)
            timeout_seconds = device_stats['uplink_interval'] * 1.5 + 2
            device_stats['timer'] = threading.Timer(
                timeout_seconds, 
                lambda: self.on_packet_timeout_for_device(device_eui)
            )
            device_stats['timer'].start()
    
    def on_packet_timeout_for_device(self, device_eui):
        """Called when expected packet is not received within timeout period for specific device"""
        device_stats = self.devices.get(device_eui)
        if not device_stats or not device_stats['timer_enabled']:
            return
            
        current_time = time.time()
        if device_stats['last_packet_time']:
            time_since_last = current_time - device_stats['last_packet_time']
            
            # Calculate current statistics for this device
            missing_packets = []
            for i in range(device_stats['total_expected']):
                if i not in device_stats['received_packets']:
                    missing_packets.append(i)
            
            packet_loss_rate = (len(missing_packets) / device_stats['total_expected']) * 100 if device_stats['total_expected'] > 0 else 0
            
            print(f"[TIMEOUT] DevEUI: {device_eui} | No packet for {time_since_last:.1f}s | Loss: {packet_loss_rate:.1f}% | Expected: {device_stats['total_expected']} | Received: {device_stats['total_received']} | Last RSSI: {device_stats.get('last_rssi', 'N/A')} dBm | Last SNR: {device_stats.get('last_snr', 'N/A')} dB")
            print("=" * 150)
        
        # Restart timer for next expected packet
        if device_stats['timer_enabled']:
            self.start_timeout_timer_for_device(device_eui)
    
    def start_monitoring_for_device(self, device_eui):
        """Start packet monitoring with timeout detection for specific device"""
        device_stats = self.get_or_create_device_stats(device_eui)
        device_stats['timer_enabled'] = True
        timeout_seconds = device_stats['uplink_interval'] * 1.5 + 2
        print(f"Packet monitoring started for {device_eui} (timeout: {timeout_seconds}s)")
    
    def stop_monitoring_for_device(self, device_eui):
        """Stop packet monitoring and cancel timers for specific device"""
        device_stats = self.devices.get(device_eui)
        if device_stats:
            device_stats['timer_enabled'] = False
            if device_stats['timer']:
                device_stats['timer'].cancel()
                device_stats['timer'] = None
            print(f"Packet monitoring stopped for {device_eui}")
    
    def stop_all_monitoring(self):
        """Stop monitoring for all devices"""
        for device_eui in self.devices:
            self.stop_monitoring_for_device(device_eui)
        print("All device monitoring stopped")
    
def main():
    print("ChirpStack LoRaWAN Uplink Data Subscriber")
    print("=" * 50)
    print("Starting basic mode - Display Range Test data with packet loss monitoring")
    
    # Initialize subscriber
    subscriber = ChirpStackSubscriber()
    
    # Start subscription
    subscriber.start()

if __name__ == "__main__":
    main()
