import serial
import struct
import time
import tkinter as tk


class ArduinoController:
    def __init__(self, port, baud=115200):
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)

    def send_command(self, cmd, value=0.0):
        packet = struct.pack('<Bf', cmd, float(value))
        self.ser.write(packet)

    def set_proportional(self, val):
        self.send_command(1, val)

    def set_integral(self, val):
        self.send_command(2, val)

    def set_derivative(self, val):
        self.send_command(3, val)

    def toggle_feedback(self):
        self.send_command(4, 0)


# arduino = ArduinoController("COM3")   # change if needed


def send_p():
    val = float(p_entry.get())
    arduino.set_proportional(val)

def send_i():
    val = float(i_entry.get())
    arduino.set_integral(val)

def send_d():
    val = float(d_entry.get())
    arduino.set_derivative(val)

def toggle_feedback():
    arduino.toggle_feedback()


root = tk.Tk()
root.title("PID Controller")


# --- P ---
tk.Label(root, text="Proportional (P)").grid(row=0, column=0, padx=10, pady=5)

p_entry = tk.Entry(root)
p_entry.grid(row=0, column=1)

tk.Button(root, text="Send", command=send_p).grid(row=0, column=2)


# --- I ---
tk.Label(root, text="Integral (I)").grid(row=1, column=0, padx=10, pady=5)

i_entry = tk.Entry(root)
i_entry.grid(row=1, column=1)

tk.Button(root, text="Send", command=send_i).grid(row=1, column=2)


# --- D ---
tk.Label(root, text="Derivative (D)").grid(row=2, column=0, padx=10, pady=5)

d_entry = tk.Entry(root)
d_entry.grid(row=2, column=1)

tk.Button(root, text="Send", command=send_d).grid(row=2, column=2)


# Toggle
tk.Button(root, text="Toggle Feedback", command=toggle_feedback).grid(row=3, column=0, columnspan=3, pady=20)


root.mainloop()