#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import json
import base64
from datetime import datetime
import threading
import time
import tkinter as tk
from tkinter import ttk, scrolledtext
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from collections import deque
import numpy as np

# MQTT connection configuration
MQTT_BROKER = "140.179.175.182"
MQTT_PORT = 7483
MQTT_USERNAME = ""
MQTT_PASSWORD = ""

# ChirpStack MQTT topics
UPLINK_TOPIC = "application/+/device/+/event/up"

class RangeTestGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RAK3183 Range Test Monitor")
        self.root.geometry("1400x900")
        
        # Configure dark theme colors
        self.colors = {
            'bg_dark': '#1a1a1a',           # 深黑色背景
            'bg_medium': '#2d2d2d',         # 中等深色
            'bg_light': '#3d3d3d',          # 较浅深色
            'accent_blue': '#00bfff',       # 科技蓝
            'accent_green': '#00ff7f',      # 科技绿
            'accent_orange': '#ff6b35',     # 警告橙
            'text_white': '#ffffff',        # 白色文字
            'text_gray': '#cccccc',         # 灰色文字
            'success': '#28a745',           # 成功绿
            'danger': '#dc3545',            # 危险红
            'warning': '#ffc107'            # 警告黄
        }
        
        # Apply dark theme
        self.setup_dark_theme()
        
        # Data storage for plotting
        self.devices_data = {}  # {device_eui: device_data}
        self.max_packets = 100  # Maximum packets to display
        
        # MQTT subscriber
        self.subscriber = None
        
        # Create GUI elements
        self.create_widgets()
        
        # Start MQTT connection
        self.start_mqtt_connection()
        
        # Initialize summary view
        if hasattr(self, 'summary_tree'):
            self.refresh_summary()
    
    def setup_dark_theme(self):
        """Setup dark theme for the application"""
        # Configure root window
        self.root.configure(bg=self.colors['bg_dark'])
        
        # Configure ttk styles
        style = ttk.Style()
        style.theme_use('clam')  # Use clam theme as base
        
        # Configure Frame styles
        style.configure('Dark.TFrame', 
                       background=self.colors['bg_dark'],
                       borderwidth=0)
        
        style.configure('Medium.TFrame', 
                       background=self.colors['bg_medium'],
                       borderwidth=1,
                       relief='solid')
        
        # Configure LabelFrame styles
        style.configure('Dark.TLabelframe', 
                       background=self.colors['bg_dark'],
                       foreground=self.colors['accent_blue'],
                       borderwidth=2,
                       relief='solid')
        
        style.configure('Dark.TLabelframe.Label',
                       background=self.colors['bg_dark'],
                       foreground=self.colors['accent_blue'],
                       font=('Arial', 10, 'bold'))
        
        # Configure Label styles
        style.configure('Dark.TLabel',
                       background=self.colors['bg_dark'],
                       foreground=self.colors['text_white'],
                       font=('Arial', 9))
        
        style.configure('Status.TLabel',
                       background=self.colors['bg_dark'],
                       foreground=self.colors['accent_green'],
                       font=('Arial', 9, 'bold'))
        
        # Configure Button styles
        style.configure('Dark.TButton',
                       background=self.colors['bg_light'],
                       foreground=self.colors['text_white'],
                       borderwidth=1,
                       focuscolor='none',
                       font=('Arial', 9))
        
        style.map('Dark.TButton',
                 background=[('active', self.colors['accent_blue']),
                           ('pressed', self.colors['bg_medium'])])
        
        # Configure Combobox styles
        style.configure('Dark.TCombobox',
                       fieldbackground=self.colors['bg_light'],
                       background=self.colors['bg_light'],
                       foreground=self.colors['text_white'],
                       borderwidth=1,
                       arrowcolor=self.colors['accent_blue'])
        
        # Configure Notebook styles
        style.configure('Dark.TNotebook',
                       background=self.colors['bg_dark'],
                       borderwidth=0)
        
        style.configure('Dark.TNotebook.Tab',
                       background=self.colors['bg_medium'],
                       foreground=self.colors['text_gray'],
                       padding=[12, 8],
                       borderwidth=1)
        
        style.map('Dark.TNotebook.Tab',
                 background=[('selected', self.colors['accent_blue']),
                           ('active', self.colors['bg_light'])],
                 foreground=[('selected', self.colors['text_white']),
                           ('active', self.colors['text_white'])])
        
        # Configure Treeview styles
        style.configure('Dark.Treeview',
                       background=self.colors['bg_medium'],
                       foreground=self.colors['text_white'],
                       fieldbackground=self.colors['bg_medium'],
                       borderwidth=1,
                       font=('Arial', 9))
        
        style.configure('Dark.Treeview.Heading',
                       background=self.colors['bg_light'],
                       foreground=self.colors['accent_blue'],
                       borderwidth=1,
                       font=('Arial', 9, 'bold'))
        
        style.map('Dark.Treeview',
                 background=[('selected', self.colors['accent_blue'])])
        
        # Configure PanedWindow styles
        style.configure('Dark.TPanedwindow',
                       background=self.colors['bg_dark'])
        
        style.configure('Dark.Sash',
                       sashthickness=3,
                       background=self.colors['accent_blue'])
        
        # Configure Checkbutton styles
        style.configure('Dark.TCheckbutton',
                       background=self.colors['bg_dark'],
                       foreground=self.colors['text_white'],
                       focuscolor='none',
                       font=('Arial', 9))
        
        style.map('Dark.TCheckbutton',
                 background=[('active', self.colors['bg_light'])])
        
    def create_widgets(self):
        # Create main notebook for different views
        self.main_notebook = ttk.Notebook(self.root, style='Dark.TNotebook')
        self.main_notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create detailed view (original interface)
        self.create_detailed_view()
        
        # Create summary view (new interface)
        self.create_summary_view()
    
    def create_detailed_view(self):
        """Create the detailed view (original interface)"""
        # Create detailed frame
        detailed_frame = ttk.Frame(self.main_notebook, style='Dark.TFrame')
        self.main_notebook.add(detailed_frame, text="Detailed View")
        
        # Create main frame with paned window
        main_paned = ttk.PanedWindow(detailed_frame, orient=tk.HORIZONTAL, style='Dark.TPanedwindow')
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left panel for charts
        left_frame = ttk.Frame(main_paned, style='Dark.TFrame')
        main_paned.add(left_frame, weight=3)
        
        # Right panel for device tables and logs
        right_frame = ttk.Frame(main_paned, style='Dark.TFrame')
        main_paned.add(right_frame, weight=1)
        
        # Create charts frame
        self.create_charts_frame(left_frame)
        
        # Create device tables frame
        self.create_device_tables_frame(right_frame)
        
        # Create log frame
        self.create_log_frame(right_frame)
        
        # Create control frame
        self.create_control_frame(right_frame)
    
    def create_summary_view(self):
        """Create the summary view for all devices"""
        # Create summary frame
        summary_frame = ttk.Frame(self.main_notebook, style='Dark.TFrame')
        self.main_notebook.add(summary_frame, text="Summary View")
        
        # Create summary table (full height)
        self.create_summary_table(summary_frame)
        
        # Create summary control frame
        self.create_summary_control(summary_frame)
    
    def create_summary_table(self, parent):
        """Create summary statistics table"""
        table_frame = ttk.LabelFrame(parent, text="All Devices Summary", style='Dark.TLabelframe')
        table_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create treeview for summary statistics
        columns = ('Device', 'Name', 'Total Packets', 'Received', 'Loss Rate', 'Avg RSSI', 'Avg SNR', 'Last Update')
        self.summary_tree = ttk.Treeview(table_frame, columns=columns, show='headings', height=12, style='Dark.Treeview')
        
        # Configure columns
        self.summary_tree.heading('Device', text='Device EUI')
        self.summary_tree.heading('Name', text='Device Name')
        self.summary_tree.heading('Total Packets', text='Total Packets')
        self.summary_tree.heading('Received', text='Received')
        self.summary_tree.heading('Loss Rate', text='Loss Rate (%)')
        self.summary_tree.heading('Avg RSSI', text='Avg RSSI (dBm)')
        self.summary_tree.heading('Avg SNR', text='Avg SNR (dB)')
        self.summary_tree.heading('Last Update', text='Last Update')
        
        # Configure column widths
        self.summary_tree.column('Device', width=120, anchor='center')
        self.summary_tree.column('Name', width=100, anchor='center')
        self.summary_tree.column('Total Packets', width=100, anchor='center')
        self.summary_tree.column('Received', width=80, anchor='center')
        self.summary_tree.column('Loss Rate', width=100, anchor='center')
        self.summary_tree.column('Avg RSSI', width=100, anchor='center')
        self.summary_tree.column('Avg SNR', width=100, anchor='center')
        self.summary_tree.column('Last Update', width=120, anchor='center')
        
        # Add scrollbar for summary table
        summary_scrollbar = ttk.Scrollbar(table_frame, orient=tk.VERTICAL, command=self.summary_tree.yview)
        self.summary_tree.configure(yscrollcommand=summary_scrollbar.set)
        
        # Pack widgets
        self.summary_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        summary_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def create_summary_control(self, parent):
        """Create summary control frame"""
        control_frame = ttk.Frame(parent, style='Dark.TFrame')
        control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Auto refresh toggle
        self.auto_refresh_var = tk.BooleanVar(value=True)
        auto_refresh_check = ttk.Checkbutton(control_frame, text="Auto Refresh", 
                                           variable=self.auto_refresh_var, style='Dark.TCheckbutton')
        auto_refresh_check.pack(side=tk.LEFT, padx=5)
        
        # Manual refresh button
        ttk.Button(control_frame, text="Refresh Summary", command=self.refresh_summary,
                  style='Dark.TButton').pack(side=tk.LEFT, padx=5)
        
        # Total devices count
        self.total_devices_var = tk.StringVar(value="Total Devices: 0")
        ttk.Label(control_frame, textvariable=self.total_devices_var, style='Dark.TLabel').pack(side=tk.RIGHT, padx=5)
        
    def create_charts_frame(self, parent):
        charts_frame = ttk.LabelFrame(parent, text="RSSI & SNR Charts", style='Dark.TLabelframe')
        charts_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create matplotlib figure with dark theme
        plt.style.use('dark_background')
        self.fig = Figure(figsize=(12, 8), dpi=100, facecolor=self.colors['bg_dark'])
        self.fig.suptitle('RAK3183 Range Test - RSSI & SNR Monitor', 
                         fontsize=16, fontweight='bold', color=self.colors['accent_blue'])
        
        # Create subplots
        self.ax_rssi = self.fig.add_subplot(211, facecolor=self.colors['bg_medium'])
        self.ax_snr = self.fig.add_subplot(212, facecolor=self.colors['bg_medium'])
        
        # Configure RSSI plot
        self.ax_rssi.set_title('RSSI (dBm)', fontweight='bold', color=self.colors['accent_green'], fontsize=12)
        self.ax_rssi.set_ylabel('RSSI (dBm)', color=self.colors['text_white'])
        self.ax_rssi.grid(True, alpha=0.3, color=self.colors['bg_light'])
        self.ax_rssi.set_ylim(-120, -40)
        self.ax_rssi.tick_params(colors=self.colors['text_gray'])
        
        # Configure SNR plot
        self.ax_snr.set_title('SNR (dB)', fontweight='bold', color=self.colors['accent_green'], fontsize=12)
        self.ax_snr.set_xlabel('Packet Number', color=self.colors['text_white'])
        self.ax_snr.set_ylabel('SNR (dB)', color=self.colors['text_white'])
        self.ax_snr.grid(True, alpha=0.3, color=self.colors['bg_light'])
        self.ax_snr.set_ylim(-20, 20)
        self.ax_snr.tick_params(colors=self.colors['text_gray'])
        
        # Create canvas
        self.canvas = FigureCanvasTkAgg(self.fig, charts_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self.canvas.get_tk_widget().configure(bg=self.colors['bg_dark'])
        
        # Device selection frame
        device_frame = ttk.Frame(charts_frame, style='Dark.TFrame')
        device_frame.pack(fill=tk.X, padx=5, pady=2)
        
        # Left side - device selection
        left_side = ttk.Frame(device_frame, style='Dark.TFrame')
        left_side.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        ttk.Label(left_side, text="Selected Device:", style='Dark.TLabel').pack(side=tk.LEFT)
        self.device_var = tk.StringVar()
        self.device_combo = ttk.Combobox(left_side, textvariable=self.device_var, 
                                       state="readonly", style='Dark.TCombobox', width=15)
        self.device_combo.pack(side=tk.LEFT, padx=5)
        self.device_combo.bind('<<ComboboxSelected>>', self.on_device_selected)
        
        # Middle - current device statistics
        self.stats_frame = ttk.Frame(device_frame, style='Dark.TFrame')
        self.stats_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)
        
        # Initialize current device stats display
        self.current_stats = {
            'total': tk.StringVar(value="Total: -/-"),
            'loss_rate': tk.StringVar(value="Loss: -.-%"),
            'avg_rssi': tk.StringVar(value="Avg RSSI: - dBm"),
            'avg_snr': tk.StringVar(value="Avg SNR: - dB")
        }
        
        # Create current stats labels
        for var in self.current_stats.values():
            label = ttk.Label(self.stats_frame, textvariable=var, style='Dark.TLabel', font=('Arial', 8))
            label.pack(side=tk.LEFT, padx=8)
        
        # Right side - clear button
        ttk.Button(device_frame, text="Clear Data", command=self.clear_selected_device,
                  style='Dark.TButton').pack(side=tk.RIGHT, padx=5)
        
    def create_device_tables_frame(self, parent):
        # Device tables frame with notebook for multiple devices
        tables_frame = ttk.LabelFrame(parent, text="Device Statistics", style='Dark.TLabelframe')
        tables_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create notebook for device tabs
        self.device_notebook = ttk.Notebook(tables_frame, style='Dark.TNotebook')
        self.device_notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Dictionary to store treeviews for each device
        self.device_trees = {}
        
    def create_log_frame(self, parent):
        log_frame = ttk.LabelFrame(parent, text="Activity Log", style='Dark.TLabelframe')
        log_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create scrolled text widget for logs with dark theme
        self.log_text = scrolledtext.ScrolledText(log_frame, height=8, wrap=tk.WORD,
                                                 bg=self.colors['bg_medium'],
                                                 fg=self.colors['text_white'],
                                                 insertbackground=self.colors['accent_blue'],
                                                 selectbackground=self.colors['accent_blue'],
                                                 font=('Consolas', 9))
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
    def create_control_frame(self, parent):
        control_frame = ttk.LabelFrame(parent, text="Controls", style='Dark.TLabelframe')
        control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Connection status
        self.status_var = tk.StringVar(value="Disconnected")
        ttk.Label(control_frame, text="Status:", style='Dark.TLabel').pack(side=tk.LEFT, padx=5)
        self.status_label = ttk.Label(control_frame, textvariable=self.status_var, style='Status.TLabel')
        self.status_label.pack(side=tk.LEFT, padx=5)
        
        # Clear all button
        ttk.Button(control_frame, text="Clear All", command=self.clear_all_devices,
                  style='Dark.TButton').pack(side=tk.RIGHT, padx=5)
        
    def log_message(self, message):
        """Add message to log with timestamp"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        
        self.log_text.insert(tk.END, log_entry)
        self.log_text.see(tk.END)
        
        # Limit log size
        lines = self.log_text.get("1.0", tk.END).split('\n')
        if len(lines) > 100:
            self.log_text.delete("1.0", f"{len(lines)-100}.0")
    
    def get_or_create_device_data(self, device_eui):
        """Get or create device data structure"""
        if device_eui not in self.devices_data:
            self.devices_data[device_eui] = {
                'device_name': 'Unknown',
                'packets': deque(maxlen=self.max_packets),  # Store packet data
                'rssi_values': deque(maxlen=self.max_packets),
                'snr_values': deque(maxlen=self.max_packets),
                'packet_numbers': deque(maxlen=self.max_packets),
                'received_packets': set(),
                'max_packet_number': -1,
                'total_expected': 0,
                'total_received': 0,
                'last_update': time.time()
            }
            
            # Add device to combo box
            current_devices = list(self.device_combo['values'])
            if device_eui not in current_devices:
                current_devices.append(device_eui)
                self.device_combo['values'] = current_devices
                if not self.device_var.get():
                    self.device_var.set(device_eui)
            
            # Create device tab and table
            self.create_device_tab(device_eui)
            
            self.log_message(f"New device registered: {device_eui}")
            
        return self.devices_data[device_eui]
    
    def create_device_tab(self, device_eui):
        """Create a new tab for device statistics"""
        # Create frame for this device
        device_frame = ttk.Frame(self.device_notebook, style='Dark.TFrame')
        self.device_notebook.add(device_frame, text=f"{device_eui[-8:]}")  # Show last 8 chars
        
        # Create treeview for packet statistics
        columns = ('Packet', 'Status', 'RSSI', 'SNR', 'Time')
        tree = ttk.Treeview(device_frame, columns=columns, show='headings', height=10, style='Dark.Treeview')
        
        # Configure columns
        tree.heading('Packet', text='Packet #')
        tree.heading('Status', text='Status')
        tree.heading('RSSI', text='RSSI (dBm)')
        tree.heading('SNR', text='SNR (dB)')
        tree.heading('Time', text='⏰ Time')
        
        tree.column('Packet', width=80, anchor='center')
        tree.column('Status', width=90, anchor='center')
        tree.column('RSSI', width=90, anchor='center')
        tree.column('SNR', width=90, anchor='center')
        tree.column('Time', width=100, anchor='center')
        
        # Add scrollbar
        scrollbar = ttk.Scrollbar(device_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scrollbar.set)
        
        # Pack widgets
        tree.pack(fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Store tree reference
        self.device_trees[device_eui] = tree
        
        # Bind tab selection event to update device combobox
        self.device_notebook.bind('<<NotebookTabChanged>>', self.on_tab_changed)
    
    def update_device_data(self, device_eui, packet_number, rssi, snr, device_name):
        """Update device data and refresh displays"""
        device_data = self.get_or_create_device_data(device_eui)
        device_data['device_name'] = device_name
        device_data['last_update'] = time.time()
        
        # Reset statistics if packet number is 0
        if packet_number == 0:
            if device_data['total_received'] > 0:
                self.log_message(f"Test reset for {device_eui} - Clearing all data")
            
            # Clear all device statistics
            device_data['received_packets'].clear()
            device_data['max_packet_number'] = -1
            device_data['total_expected'] = 0
            device_data['total_received'] = 0
            
            # Clear plotting data
            device_data['packet_numbers'].clear()
            device_data['rssi_values'].clear()
            device_data['snr_values'].clear()
            device_data['packets'].clear()
            
            # Clear the treeview
            if device_eui in self.device_trees:
                for item in self.device_trees[device_eui].get_children():
                    self.device_trees[device_eui].delete(item)
            
            # Clear and redraw charts if this device is currently selected
            if self.device_var.get() == device_eui:
                self.clear_charts()
            
            # Update summary to show reset state
            self.update_device_summary(device_eui)
        
        # Update statistics (包括packet 0)
        device_data['received_packets'].add(packet_number)
        device_data['total_received'] += 1
        
        if packet_number > device_data['max_packet_number']:
            device_data['max_packet_number'] = packet_number
        
        device_data['total_expected'] = device_data['max_packet_number'] + 1
        
        # Add to deques for plotting
        device_data['packet_numbers'].append(packet_number)
        device_data['rssi_values'].append(rssi)
        device_data['snr_values'].append(snr)
        
        # Add to treeview
        self.update_device_table(device_eui, packet_number, rssi, snr)
        
        # Update summary
        self.update_device_summary(device_eui)
        
        # Update chart if this device is selected
        if self.device_var.get() == device_eui:
            self.update_charts()
        
        # Auto refresh summary view if enabled
        if hasattr(self, 'auto_refresh_var') and self.auto_refresh_var.get():
            self.refresh_summary()
    
    def update_device_table(self, device_eui, packet_number, rssi, snr):
        """Update the device table with new packet data"""
        if device_eui not in self.device_trees:
            return
            
        tree = self.device_trees[device_eui]
        device_data = self.devices_data[device_eui]
        
        # Add received packet
        timestamp = datetime.now().strftime("%H:%M:%S")
        tree.insert('', 'end', values=(
            packet_number,
            'Received',
            f"{rssi:.1f}",
            f"{snr:.1f}",
            timestamp
        ), tags=('received',))
        
        # Configure tags for styling with dark theme colors
        tree.tag_configure('received', background=self.colors['success'], foreground=self.colors['text_white'])
        tree.tag_configure('missing', background=self.colors['danger'], foreground=self.colors['text_white'])
        
        # Add missing packets
        for i in range(device_data['total_expected']):
            if i not in device_data['received_packets']:
                # Check if missing packet already exists in tree
                exists = False
                for item in tree.get_children():
                    if tree.item(item)['values'][0] == i:
                        exists = True
                        break
                
                if not exists:
                    tree.insert('', 'end', values=(
                        i,
                        'MISSING',
                        'N/A',
                        'N/A',
                        'N/A'
                    ), tags=('missing',))
        
        # Sort tree by packet number
        items = [(tree.item(item)['values'][0], item) for item in tree.get_children()]
        items.sort(key=lambda x: int(x[0]) if str(x[0]).isdigit() else float('inf'))
        
        for index, (_, item) in enumerate(items):
            tree.move(item, '', index)
        
        # Scroll to bottom
        if tree.get_children():
            tree.see(tree.get_children()[-1])
    
    def update_device_summary(self, device_eui):
        """Update device summary statistics"""
        if device_eui not in self.devices_data:
            return
            
        device_data = self.devices_data[device_eui]
        
        # Calculate statistics
        missing_packets = []
        for i in range(device_data['total_expected']):
            if i not in device_data['received_packets']:
                missing_packets.append(i)
        
        loss_rate = (len(missing_packets) / device_data['total_expected']) * 100 if device_data['total_expected'] > 0 else 0
        
        # Update current stats display if this is the selected device
        if self.device_var.get() == device_eui:
            self.current_stats['total'].set(f"Total: {device_data['total_received']}/{device_data['total_expected']}")
            self.current_stats['loss_rate'].set(f"Loss: {loss_rate:.1f}%")
            
            if device_data['rssi_values']:
                avg_rssi = sum(device_data['rssi_values']) / len(device_data['rssi_values'])
                self.current_stats['avg_rssi'].set(f"Avg RSSI: {avg_rssi:.1f} dBm")
            else:
                self.current_stats['avg_rssi'].set("Avg RSSI: - dBm")
                
            if device_data['snr_values']:
                avg_snr = sum(device_data['snr_values']) / len(device_data['snr_values'])
                self.current_stats['avg_snr'].set(f"Avg SNR: {avg_snr:.1f} dB")
            else:
                self.current_stats['avg_snr'].set("Avg SNR: - dB")
    
    def on_device_selected(self, event=None):
        """Handle device selection change"""
        selected_device = self.device_var.get()
        
        # Switch to corresponding device tab in notebook
        if selected_device:
            # Find the tab index for the selected device
            for index, tab_id in enumerate(self.device_notebook.tabs()):
                tab_text = self.device_notebook.tab(tab_id, "text")
                # Extract device EUI from tab text (remove emoji and whitespace)
                device_suffix = selected_device[-8:]  # Last 8 characters
                if device_suffix in tab_text:
                    self.device_notebook.select(index)
                    break
        
        # Update charts
        self.update_charts()
        
        # Update current stats display
        if selected_device and selected_device in self.devices_data:
            self.update_device_summary(selected_device)
        else:
            # Reset stats display if no device selected
            self.current_stats['total'].set("Total: -/-")
            self.current_stats['loss_rate'].set("Loss: -.-%")
            self.current_stats['avg_rssi'].set("Avg RSSI: - dBm")
            self.current_stats['avg_snr'].set("Avg SNR: - dB")
    
    def on_tab_changed(self, event=None):
        """Handle notebook tab change event"""
        try:
            # Get currently selected tab
            current_tab = self.device_notebook.select()
            if not current_tab:
                return
                
            # Get tab text to find corresponding device EUI
            tab_text = self.device_notebook.tab(current_tab, "text")
            
            # Extract device suffix from tab text (remove emoji and whitespace)
            device_suffix = tab_text.strip()
            
            # Find matching device EUI
            for device_eui in self.devices_data.keys():
                if device_eui.endswith(device_suffix):
                    # Update combobox selection without triggering its event
                    self.device_combo.unbind('<<ComboboxSelected>>')
                    self.device_var.set(device_eui)
                    self.device_combo.bind('<<ComboboxSelected>>', self.on_device_selected)
                    
                    # Update charts and stats for the selected device
                    self.update_charts()
                    self.update_device_summary(device_eui)
                    break
                    
        except Exception as e:
            self.log_message(f"Error in tab change: {e}")
    
    def update_charts(self):
        """Update RSSI and SNR charts for selected device"""
        selected_device = self.device_var.get()
        if not selected_device or selected_device not in self.devices_data:
            return
        
        device_data = self.devices_data[selected_device]
        
        # Clear previous plots
        self.ax_rssi.clear()
        self.ax_snr.clear()
        
        # Configure plots with dark theme
        self.ax_rssi.set_title(f'RSSI (dBm) - {device_data["device_name"]} ({selected_device[-8:]})', 
                              fontweight='bold', color=self.colors['accent_green'], fontsize=12)
        self.ax_rssi.set_ylabel('RSSI (dBm)', color=self.colors['text_white'])
        self.ax_rssi.grid(True, alpha=0.3, color=self.colors['bg_light'])
        self.ax_rssi.tick_params(colors=self.colors['text_gray'])
        
        self.ax_snr.set_title(f'SNR (dB) - {device_data["device_name"]} ({selected_device[-8:]})', 
                             fontweight='bold', color=self.colors['accent_green'], fontsize=12)
        self.ax_snr.set_xlabel('Packet Number', color=self.colors['text_white'])
        self.ax_snr.set_ylabel('SNR (dB)', color=self.colors['text_white'])
        self.ax_snr.grid(True, alpha=0.3, color=self.colors['bg_light'])
        self.ax_snr.tick_params(colors=self.colors['text_gray'])
        
        if device_data['packet_numbers'] and device_data['rssi_values'] and device_data['snr_values']:
            packet_nums = list(device_data['packet_numbers'])
            rssi_vals = list(device_data['rssi_values'])
            snr_vals = list(device_data['snr_values'])
            
            # Plot RSSI with tech colors
            self.ax_rssi.plot(packet_nums, rssi_vals, color=self.colors['accent_blue'], 
                             marker='o', markersize=4, linewidth=2, label='RSSI',
                             markerfacecolor=self.colors['accent_blue'], 
                             markeredgecolor=self.colors['text_white'], markeredgewidth=0.5)
            self.ax_rssi.set_ylim(min(rssi_vals) - 5, max(rssi_vals) + 5)
            
            # Plot SNR with tech colors
            self.ax_snr.plot(packet_nums, snr_vals, color=self.colors['accent_green'], 
                            marker='o', markersize=4, linewidth=2, label='SNR',
                            markerfacecolor=self.colors['accent_green'], 
                            markeredgecolor=self.colors['text_white'], markeredgewidth=0.5)
            self.ax_snr.set_ylim(min(snr_vals) - 2, max(snr_vals) + 2)
            
            # Mark missing packets with red dashed lines
            all_packets = set(range(device_data['total_expected']))
            missing_packets = all_packets - device_data['received_packets']
            
            if missing_packets:
                missing_list = sorted(list(missing_packets))
                # Mark missing packets on both plots
                for missing_num in missing_list:
                    self.ax_rssi.axvline(x=missing_num, color=self.colors['danger'], 
                                        linestyle='--', alpha=0.8, linewidth=2)
                    self.ax_snr.axvline(x=missing_num, color=self.colors['danger'], 
                                       linestyle='--', alpha=0.8, linewidth=2)
        
        # Set x-axis limits
        if device_data['total_expected'] > 0:
            self.ax_rssi.set_xlim(-1, max(device_data['total_expected'], 10))
            self.ax_snr.set_xlim(-1, max(device_data['total_expected'], 10))
        
        self.canvas.draw()
    
    def clear_selected_device(self):
        """Clear data for selected device"""
        selected_device = self.device_var.get()
        if selected_device in self.devices_data:
            # Clear device data
            device_data = self.devices_data[selected_device]
            device_data['packets'].clear()
            device_data['rssi_values'].clear()
            device_data['snr_values'].clear()
            device_data['packet_numbers'].clear()
            device_data['received_packets'].clear()
            device_data['max_packet_number'] = -1
            device_data['total_expected'] = 0
            device_data['total_received'] = 0
            
            # Clear treeview
            if selected_device in self.device_trees:
                for item in self.device_trees[selected_device].get_children():
                    self.device_trees[selected_device].delete(item)
            
            # Update summary
            self.update_device_summary(selected_device)
            
            # Clear and reset charts
            self.clear_charts()
            
            # Refresh summary view
            if hasattr(self, 'auto_refresh_var') and self.auto_refresh_var.get():
                self.refresh_summary()
            
            self.log_message(f"🗑️ Cleared data for device: {selected_device}")
    
    def clear_all_devices(self):
        """Clear all device data"""
        self.devices_data.clear()
        self.device_combo['values'] = []
        self.device_var.set('')
        
        # Clear all tabs
        for tab in self.device_notebook.tabs():
            self.device_notebook.forget(tab)
        
        self.device_trees.clear()
        
        # Reset current stats display
        self.current_stats['total'].set("📊 Total: -/-")
        self.current_stats['loss_rate'].set("📉 Loss: -.-%")
        self.current_stats['avg_rssi'].set("📡 Avg RSSI: - dBm")
        self.current_stats['avg_snr'].set("📊 Avg SNR: - dB")
        
        # Clear and reset charts
        self.clear_charts()
        
        # Clear summary view
        if hasattr(self, 'summary_tree'):
            for item in self.summary_tree.get_children():
                self.summary_tree.delete(item)
            self.total_devices_var.set("📱 Total Devices: 0")
            self.clear_summary_charts()
        
        self.log_message("🗑️ Cleared all device data")
    
    def clear_charts(self):
        """Clear both RSSI and SNR charts"""
        # Clear previous plots
        self.ax_rssi.clear()
        self.ax_snr.clear()
        
        # Reset chart configuration with dark theme
        self.ax_rssi.set_title('📡 RSSI (dBm) - Waiting for data...', 
                              fontweight='bold', color=self.colors['accent_green'], fontsize=12)
        self.ax_rssi.set_ylabel('RSSI (dBm)', color=self.colors['text_white'])
        self.ax_rssi.grid(True, alpha=0.3, color=self.colors['bg_light'])
        self.ax_rssi.set_ylim(-120, -40)
        self.ax_rssi.tick_params(colors=self.colors['text_gray'])
        
        self.ax_snr.set_title('📊 SNR (dB) - Waiting for data...', 
                             fontweight='bold', color=self.colors['accent_green'], fontsize=12)
        self.ax_snr.set_xlabel('Packet Number', color=self.colors['text_white'])
        self.ax_snr.set_ylabel('SNR (dB)', color=self.colors['text_white'])
        self.ax_snr.grid(True, alpha=0.3, color=self.colors['bg_light'])
        self.ax_snr.set_ylim(-20, 20)
        self.ax_snr.tick_params(colors=self.colors['text_gray'])
        
        # Set default x-axis limits
        self.ax_rssi.set_xlim(-1, 10)
        self.ax_snr.set_xlim(-1, 10)
        
        # Redraw canvas
        self.canvas.draw()
    
    def refresh_summary(self):
        """Refresh summary data"""
        try:
            # Clear existing summary data
            for item in self.summary_tree.get_children():
                self.summary_tree.delete(item)
            
            # Update total devices count
            self.total_devices_var.set(f"📱 Total Devices: {len(self.devices_data)}")
            
            if not self.devices_data:
                return
            
            for device_eui, device_data in self.devices_data.items():
                # Calculate statistics
                total_expected = device_data['total_expected']
                total_received = device_data['total_received']
                
                if total_expected > 0:
                    missing_count = 0
                    for i in range(total_expected):
                        if i not in device_data['received_packets']:
                            missing_count += 1
                    loss_rate = (missing_count / total_expected) * 100
                else:
                    loss_rate = 0
                
                # Calculate averages
                avg_rssi = sum(device_data['rssi_values']) / len(device_data['rssi_values']) if device_data['rssi_values'] else 0
                avg_snr = sum(device_data['snr_values']) / len(device_data['snr_values']) if device_data['snr_values'] else 0
                
                # Format last update time
                last_update = datetime.fromtimestamp(device_data['last_update']).strftime("%H:%M:%S")
                
                # Add to summary table
                self.summary_tree.insert('', 'end', values=(
                    device_eui[-8:],  # Short device ID
                    device_data['device_name'],
                    total_expected,
                    total_received,
                    f"{loss_rate:.1f}%",
                    f"{avg_rssi:.1f}" if avg_rssi != 0 else "N/A",
                    f"{avg_snr:.1f}" if avg_snr != 0 else "N/A",
                    last_update
                ))
            
        except Exception as e:
            self.log_message(f"❌ Error refreshing summary: {e}")
    
    def start_mqtt_connection(self):
        """Start MQTT connection in separate thread"""
        self.subscriber = ChirpStackSubscriber(self)
        mqtt_thread = threading.Thread(target=self.subscriber.start, daemon=True)
        mqtt_thread.start()
        
        # Start periodic summary refresh
        self.schedule_summary_refresh()
    
    def schedule_summary_refresh(self):
        """Schedule periodic summary refresh"""
        if hasattr(self, 'auto_refresh_var') and self.auto_refresh_var.get():
            self.refresh_summary()
        
        # Schedule next refresh in 5 seconds
        self.root.after(5000, self.schedule_summary_refresh)

class ChirpStackSubscriber:
    def __init__(self, gui):
        self.gui = gui
        # Use the old client initialization to avoid compatibility issues
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.gui.status_var.set("🟢 Connected")
            # Update status label color to green
            self.gui.status_label.configure(foreground=self.gui.colors['success'])
            self.gui.log_message(f"✅ Connected to MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
            client.subscribe(UPLINK_TOPIC)
        else:
            self.gui.status_var.set("🔴 Connection Failed")
            # Update status label color to red
            self.gui.status_label.configure(foreground=self.gui.colors['danger'])
            self.gui.log_message(f"❌ Connection failed, error code: {rc}")
    
    def on_disconnect(self, client, userdata, rc):
        self.gui.status_var.set("🟡 Disconnected")
        # Update status label color to orange
        self.gui.status_label.configure(foreground=self.gui.colors['accent_orange'])
        self.gui.log_message(f"⚠️ Disconnected from MQTT Broker")
    
    def on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            
            # Extract device information
            device_info = payload.get('deviceInfo', {})
            device_name = device_info.get('deviceName', 'Unknown')
            device_eui = device_info.get('devEui', 'Unknown')
            
            # Extract frame counter for debugging
            f_cnt = payload.get('fCnt', 0)
            
            # Extract data
            data_base64 = payload.get('data', '')
            try:
                raw_data = base64.b64decode(data_base64)
            except Exception as decode_error:
                self.gui.root.after(0, self.gui.log_message, 
                                   f"❌ Base64 decode error: {decode_error}")
                return
            
            # Extract RF information
            rx_info = payload.get('rxInfo', [])
            if rx_info:
                rf_info = rx_info[0]
                rssi = rf_info.get('rssi', 0)
                snr = rf_info.get('snr', 0)
            else:
                self.gui.root.after(0, self.gui.log_message, 
                                   f"⚠️ No RX info available for {device_name}")
                return
            
            # Debug log for raw data (show first 20 bytes for large packets)
            debug_hex = raw_data.hex() if len(raw_data) <= 20 else raw_data[:20].hex() + '...'
            self.gui.root.after(0, self.gui.log_message, 
                               f"🔍 Debug | {device_name} | FCnt: {f_cnt} | Raw data len: {len(raw_data)} | Hex: {debug_hex}")
            
            # Check if it's Range Test data (新格式: 100字节，RAK开头)
            if len(raw_data) >= 7 and raw_data[:3] == b'RAK':
                # 解析新的数据包格式
                packet_counter = raw_data[3]  # 包计数器在第4字节
                packet_counter_high = raw_data[4]  # 包计数器高字节（预留）
                interval_low = raw_data[5]  # 上传间隔低字节
                interval_high = raw_data[6]  # 上传间隔高字节
                
                # 计算完整的包计数器和间隔
                full_packet_counter = packet_counter + (packet_counter_high << 8)
                upload_interval = interval_low + (interval_high << 8)
                
                # 使用低字节作为主要计数器（0-99循环）
                display_packet_counter = packet_counter
                
                # 特殊处理第一包异常
                if f_cnt == 1 and display_packet_counter != 0:
                    self.gui.root.after(0, self.gui.log_message, 
                                       f"⚠️ First frame anomaly | FCnt: {f_cnt} | Packet Counter: {display_packet_counter} | Expected: 0")
                
                # 额外的调试信息
                self.gui.root.after(0, self.gui.log_message, 
                                   f"📦 Packet Details | Counter: {display_packet_counter} | Full Counter: {full_packet_counter} | Interval: {upload_interval}s")
                
                # Update GUI in main thread
                self.gui.root.after(0, self.gui.update_device_data, 
                                   device_eui, display_packet_counter, rssi, snr, device_name)
                
                # Log message with enhanced information
                self.gui.root.after(0, self.gui.log_message, 
                                   f"📡 Range Test | {device_name} | FCnt: {f_cnt} | 📦 Packet: {display_packet_counter} | ⏱️ Interval: {upload_interval}s | 📊 RSSI: {rssi:.1f} dBm | 📈 SNR: {snr:.1f} dB")
            else:
                # Log non-Range Test data for debugging
                self.gui.root.after(0, self.gui.log_message, 
                                   f"🔍 Non-Range Test data | {device_name} | FCnt: {f_cnt} | Len: {len(raw_data)} | Data: {debug_hex}")
            
        except Exception as e:
            self.gui.root.after(0, self.gui.log_message, 
                               f"❌ Error processing message: {e} | Topic: {msg.topic}")
            
        except Exception as e:
            self.gui.root.after(0, self.gui.log_message, f"Error processing message: {e}")
    
    def start(self):
        try:
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_forever()
        except Exception as e:
            self.gui.root.after(0, self.gui.log_message, f"Connection error: {e}")

def main():
    root = tk.Tk()
    app = RangeTestGUI(root)
    
    try:
        root.mainloop()
    except KeyboardInterrupt:
        print("Application closed by user")

if __name__ == "__main__":
    main()
