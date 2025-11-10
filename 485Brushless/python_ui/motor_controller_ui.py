import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import requests
import json
import threading
import time
from datetime import datetime
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import numpy as np

class M5MotorControllerUI:
    def __init__(self, root):
        self.root = root
        self.root.title("M5 PWR485 Motor Controller")
        self.root.geometry("1000x700")
        
        # Connection settings
        self.controller_ip = tk.StringVar(value="192.168.86.31")
        self.controller_port = tk.StringVar(value="80")
        self.connected = False
        self.auto_update = False
        self.update_interval = 1.0  # seconds
        
        # Motor status data
        self.motor_status = {}
        self.status_history = []
        self.max_history_points = 100
        
        # Create GUI
        self.create_widgets()
        
        # Start update thread
        self.update_thread = None
        self.running = True
        
    def create_widgets(self):
        # Create main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(2, weight=1)
        
        # Connection frame
        self.create_connection_frame(main_frame)
        
        # Control frame
        self.create_control_frame(main_frame)
        
        # Status and monitoring frame
        self.create_status_frame(main_frame)
        
    def create_connection_frame(self, parent):
        # Connection frame
        conn_frame = ttk.LabelFrame(parent, text="Connection", padding="5")
        conn_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        
        # IP Address
        ttk.Label(conn_frame, text="IP Address:").grid(row=0, column=0, sticky=tk.W, padx=(0, 5))
        ip_entry = ttk.Entry(conn_frame, textvariable=self.controller_ip, width=15)
        ip_entry.grid(row=0, column=1, sticky=tk.W, padx=(0, 10))
        
        # Port
        ttk.Label(conn_frame, text="Port:").grid(row=0, column=2, sticky=tk.W, padx=(0, 5))
        port_entry = ttk.Entry(conn_frame, textvariable=self.controller_port, width=8)
        port_entry.grid(row=0, column=3, sticky=tk.W, padx=(0, 10))
        
        # Connect button
        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=4, padx=(10, 0))
        
        # Status label
        self.status_label = ttk.Label(conn_frame, text="Disconnected", foreground="red")
        self.status_label.grid(row=0, column=5, padx=(10, 0))
        
        # Auto-update checkbox
        self.auto_update_var = tk.BooleanVar()
        auto_check = ttk.Checkbutton(conn_frame, text="Auto Update", 
                                   variable=self.auto_update_var, 
                                   command=self.toggle_auto_update)
        auto_check.grid(row=0, column=6, padx=(20, 0))
        
    def create_control_frame(self, parent):
        # Control frame
        control_frame = ttk.LabelFrame(parent, text="Motor Control", padding="5")
        control_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N), padx=(0, 10))
        
        # Speed control
        speed_frame = ttk.LabelFrame(control_frame, text="Speed Control", padding="5")
        speed_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 5))
        
        ttk.Label(speed_frame, text="Speed (RPM):").grid(row=0, column=0, sticky=tk.W)
        self.speed_var = tk.StringVar(value="100")
        speed_entry = ttk.Entry(speed_frame, textvariable=self.speed_var, width=10)
        speed_entry.grid(row=0, column=1, padx=(5, 10))
        
        ttk.Button(speed_frame, text="Set Speed", command=self.set_speed).grid(row=0, column=2)
        
        # Position control
        pos_frame = ttk.LabelFrame(control_frame, text="Position Control", padding="5")
        pos_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(pos_frame, text="Position:").grid(row=0, column=0, sticky=tk.W)
        self.position_var = tk.StringVar(value="1000")
        pos_entry = ttk.Entry(pos_frame, textvariable=self.position_var, width=10)
        pos_entry.grid(row=0, column=1, padx=(5, 10))
        
        ttk.Label(pos_frame, text="Speed:").grid(row=0, column=2, sticky=tk.W, padx=(10, 0))
        self.pos_speed_var = tk.StringVar(value="500")
        pos_speed_entry = ttk.Entry(pos_frame, textvariable=self.pos_speed_var, width=8)
        pos_speed_entry.grid(row=0, column=3, padx=(5, 10))
        
        ttk.Button(pos_frame, text="Set Position", command=self.set_position).grid(row=0, column=4)
        
        # Torque control
        torque_frame = ttk.LabelFrame(control_frame, text="Current Control", padding="5")
        torque_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(torque_frame, text="Current (mA):").grid(row=0, column=0, sticky=tk.W)
        self.torque_var = tk.StringVar(value="500")
        torque_entry = ttk.Entry(torque_frame, textvariable=self.torque_var, width=10)
        torque_entry.grid(row=0, column=1, padx=(5, 10))
        
        ttk.Button(torque_frame, text="Set Current", command=self.set_torque).grid(row=0, column=2)
        
        # Control buttons
        btn_frame = ttk.Frame(control_frame)
        btn_frame.grid(row=3, column=0, sticky=(tk.W, tk.E), pady=(10, 0))
        
        ttk.Button(btn_frame, text="Stop Motor", command=self.stop_motor).grid(row=0, column=0, padx=(0, 5))
        emergency_btn = ttk.Button(btn_frame, text="EMERGENCY STOP", command=self.emergency_stop)
        emergency_btn.grid(row=0, column=1, padx=5)
        emergency_btn.configure(style="Emergency.TButton")
        
        ttk.Button(btn_frame, text="Reset Position", command=self.reset_position).grid(row=0, column=2, padx=5)
        ttk.Button(btn_frame, text="Clear Errors", command=self.clear_errors).grid(row=0, column=3, padx=(5, 0))
        
        # Manual update button
        ttk.Button(control_frame, text="Update Status", command=self.update_status).grid(row=4, column=0, pady=(10, 0))
        
    def create_status_frame(self, parent):
        # Status frame
        status_frame = ttk.LabelFrame(parent, text="Motor Status & Monitoring", padding="5")
        status_frame.grid(row=1, column=1, rowspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        status_frame.columnconfigure(0, weight=1)
        status_frame.rowconfigure(1, weight=1)
        
        # Status display
        status_display_frame = ttk.Frame(status_frame)
        status_display_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        status_display_frame.columnconfigure(1, weight=1)
        
        # Current status
        current_frame = ttk.LabelFrame(status_display_frame, text="Current Status", padding="5")
        current_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N), padx=(0, 5))
        
        self.status_text = scrolledtext.ScrolledText(current_frame, width=30, height=15, wrap=tk.WORD)
        self.status_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Real-time plots
        plot_frame = ttk.LabelFrame(status_display_frame, text="Real-time Monitoring", padding="5")
        plot_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Create matplotlib figure
        self.fig = Figure(figsize=(8, 6), dpi=80)
        
        # Create subplots
        self.ax1 = self.fig.add_subplot(221)  # Speed
        self.ax2 = self.fig.add_subplot(222)  # Position
        self.ax3 = self.fig.add_subplot(223)  # Voltage/Current
        self.ax4 = self.fig.add_subplot(224)  # Temperature
        
        self.fig.tight_layout(pad=2.0)
        
        # Create canvas
        self.canvas = FigureCanvasTkAgg(self.fig, plot_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure plot frame
        plot_frame.columnconfigure(0, weight=1)
        plot_frame.rowconfigure(0, weight=1)
        
        # Log frame
        log_frame = ttk.LabelFrame(status_frame, text="Activity Log", padding="5")
        log_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(10, 0))
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=8, wrap=tk.WORD)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
    def get_base_url(self):
        return f"http://{self.controller_ip.get()}:{self.controller_port.get()}"
        
    def log_message(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)
        
    def toggle_connection(self):
        if not self.connected:
            try:
                # Test connection
                response = requests.get(f"{self.get_base_url()}/api/status", timeout=5)
                if response.status_code == 200:
                    self.connected = True
                    self.connect_btn.config(text="Disconnect")
                    self.status_label.config(text="Connected", foreground="green")
                    self.log_message("Connected to motor controller")
                    self.update_status()
                else:
                    messagebox.showerror("Connection Error", "Failed to connect to motor controller")
            except requests.exceptions.RequestException as e:
                messagebox.showerror("Connection Error", f"Connection failed: {str(e)}")
        else:
            self.connected = False
            self.auto_update = False
            self.auto_update_var.set(False)
            self.connect_btn.config(text="Connect")
            self.status_label.config(text="Disconnected", foreground="red")
            self.log_message("Disconnected from motor controller")
            
    def toggle_auto_update(self):
        self.auto_update = self.auto_update_var.get()
        if self.auto_update and self.connected:
            if self.update_thread is None or not self.update_thread.is_alive():
                self.update_thread = threading.Thread(target=self.auto_update_loop, daemon=True)
                self.update_thread.start()
            self.log_message("Auto-update enabled")
        else:
            self.auto_update = False
            self.log_message("Auto-update disabled")
            
    def auto_update_loop(self):
        while self.auto_update and self.connected and self.running:
            try:
                self.update_status()
                time.sleep(self.update_interval)
            except Exception as e:
                self.log_message(f"Auto-update error: {str(e)}")
                break
                
    def update_status(self):
        if not self.connected:
            return
            
        try:
            response = requests.get(f"{self.get_base_url()}/api/status", timeout=5)
            if response.status_code == 200:
                self.motor_status = response.json()
                self.display_status()
                self.update_plots()
            else:
                self.log_message(f"Status update failed: HTTP {response.status_code}")
        except requests.exceptions.RequestException as e:
            self.log_message(f"Status update error: {str(e)}")
            
    def display_status(self):
        status = self.motor_status
        
        # Clear and update status text
        self.status_text.delete(1.0, tk.END)
        
        status_text = f"""Motor Status:
Running: {status.get('isRunning', 'Unknown')}
Mode: {status.get('mode', 'Unknown')}

Current Readings:
Velocity: {status.get('velocity', 0)} RPM
Position: {status.get('position', 0)}

Power Measurements:
Voltage: {status.get('voltage', 0):.2f} V
Current: {status.get('current', 0):.3f} A
Temperature: {status.get('temperature', 0)} °C

Error Status:
Error Code: {status.get('errorCode', 0)}
Error Message: {status.get('errorMessage', 'None')}

Last Update: {datetime.now().strftime('%H:%M:%S')}"""
        
        self.status_text.insert(1.0, status_text)
        
    def update_plots(self):
        if not self.motor_status:
            return
            
        # Add current status to history
        current_time = time.time()
        self.status_history.append({
            'time': current_time,
            'speed': self.motor_status.get('velocity', 0),  # Changed from 'speed' to 'velocity'
            'position': self.motor_status.get('position', 0),
            'voltage': self.motor_status.get('voltage', 0),
            'current': self.motor_status.get('current', 0),
            'temperature': self.motor_status.get('temperature', 0)
        })
        
        # Limit history size
        if len(self.status_history) > self.max_history_points:
            self.status_history = self.status_history[-self.max_history_points:]
            
        if len(self.status_history) < 2:
            return
            
        # Extract data for plotting
        times = [(point['time'] - self.status_history[0]['time']) for point in self.status_history]
        speeds = [point['speed'] for point in self.status_history]
        positions = [point['position'] for point in self.status_history]
        voltages = [point['voltage'] for point in self.status_history]
        currents = [point['current'] for point in self.status_history]
        temperatures = [point['temperature'] for point in self.status_history]
        
        # Clear and update plots
        self.ax1.clear()
        self.ax1.plot(times, speeds, 'b-', linewidth=2)
        self.ax1.set_title('Speed (RPM)', fontsize=10)
        self.ax1.grid(True, alpha=0.3)
        
        self.ax2.clear()
        self.ax2.plot(times, positions, 'g-', linewidth=2)
        self.ax2.set_title('Position', fontsize=10)
        self.ax2.grid(True, alpha=0.3)
        
        self.ax3.clear()
        self.ax3.plot(times, voltages, 'r-', linewidth=2, label='Voltage (V)')
        ax3_twin = self.ax3.twinx()
        ax3_twin.plot(times, currents, 'm-', linewidth=2, label='Current (A)')
        self.ax3.set_title('Voltage & Current', fontsize=10)
        self.ax3.grid(True, alpha=0.3)
        self.ax3.legend(loc='upper left', fontsize=8)
        ax3_twin.legend(loc='upper right', fontsize=8)
        
        self.ax4.clear()
        self.ax4.plot(times, temperatures, 'orange', linewidth=2)
        self.ax4.set_title('Temperature (°C)', fontsize=10)
        self.ax4.grid(True, alpha=0.3)
        
        self.fig.tight_layout(pad=1.0)
        self.canvas.draw()
        
    def send_command(self, endpoint, data=None):
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to the motor controller first")
            return False
            
        try:
            if data:
                response = requests.post(f"{self.get_base_url()}/api/{endpoint}", 
                                       json=data, timeout=10)
            else:
                response = requests.post(f"{self.get_base_url()}/api/{endpoint}", timeout=10)
                
            if response.status_code == 200:
                result = response.json()
                if result.get('success', False):
                    self.log_message(f"Command successful: {result.get('message', '')}")
                    self.update_status()  # Refresh status after command
                    return True
                else:
                    self.log_message(f"Command failed: {result.get('message', 'Unknown error')}")
                    messagebox.showerror("Command Error", result.get('message', 'Unknown error'))
            else:
                self.log_message(f"HTTP Error: {response.status_code}")
                messagebox.showerror("HTTP Error", f"HTTP {response.status_code}")
        except requests.exceptions.RequestException as e:
            self.log_message(f"Command error: {str(e)}")
            messagebox.showerror("Connection Error", str(e))
        return False
        
    def set_speed(self):
        try:
            speed = int(self.speed_var.get())
            self.send_command("control/speed", {"speed": speed})
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter a valid speed value")
            
    def set_position(self):
        try:
            position = int(self.position_var.get())
            speed = int(self.pos_speed_var.get())
            self.send_command("control/position", {"position": position, "speed": speed})
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter valid position and speed values")
            
    def set_torque(self):
        try:
            torque = int(self.torque_var.get())
            # Send current in mA (torque value is treated as mA)
            self.send_command("control/current", {"current": torque})
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter a valid current value (mA)")
            
    def stop_motor(self):
        self.send_command("control/stop")
        
    def emergency_stop(self):
        if messagebox.askyesno("Emergency Stop", "Are you sure you want to perform an emergency stop?"):
            self.send_command("control/emergency_stop")
            
    def reset_position(self):
        self.send_command("control/reset_position")
        
    def clear_errors(self):
        self.send_command("control/clear_errors")
        
    def on_closing(self):
        self.running = False
        if self.update_thread and self.update_thread.is_alive():
            self.update_thread.join(timeout=1)
        self.root.destroy()

def main():
    root = tk.Tk()
    
    # Configure style for emergency button
    style = ttk.Style()
    style.configure("Emergency.TButton", background="red", foreground="white")
    
    app = M5MotorControllerUI(root)
    
    # Handle window closing
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    
    root.mainloop()

if __name__ == "__main__":
    main()