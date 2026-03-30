"""
ESP32 BLE Monitor
A GUI application to connect and monitor ESP32 BLE data.
Requires: pip install bleak matplotlib
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import asyncio
import threading
from bleak import BleakClient, BleakScanner
from datetime import datetime
import csv
import collections

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.ticker as ticker

import serial

# ── BLE config ────────────────────────────────────────────────────────────────
DEVICE_NAME         = "ESP32-BLE"
CHARACTERISTIC_UUID = "abcd1234-ab12-ab12-ab12-abcdef123456"

# ── Colour palette ────────────────────────────────────────────────────────────
BG          = "#0e0e0f"
PANEL       = "#161618"
BORDER      = "#2a2a2e"
ACCENT      = "#00e5ff"
ACCENT_DIM  = "#007a8a"
SUCCESS     = "#00e676"
WARNING     = "#ffab40"
DANGER      = "#ff1744"
TEXT        = "#e8e8ec"
TEXT_DIM    = "#666672"
FONT_MONO   = ("Courier New", 10)
FONT_LABEL  = ("Courier New", 9, "bold")
FONT_TITLE  = ("Courier New", 13, "bold")
FONT_VALUE  = ("Courier New", 14, "bold")

# ── Graph config ──────────────────────────────────────────────────────────────
MAX_POINTS      = 300          # rolling window length
CH_COLORS       = ["#00e5ff", "#00e676", "#ffab40"]   # CH1, CH2, CH3
SETPOINT_COLORS = ["#4fc3f7", "#69f0ae", "#ffd54f"]   # lighter dashed variants
GRAPH_BG        = "#111113"
GRAPH_AXES_BG   = "#161618"


import serial
import re

class SerialManager:
    def __init__(self, port, baudrate, on_data, on_status):
        self.port = port
        self.baudrate = baudrate
        self.on_data = on_data
        self.on_status = on_status
        self._running = False
        self.ser = None

    def start(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            self._running = True
            threading.Thread(target=self._read_loop, daemon=True).start()
            self.on_status("connected", f"Serial connected on {self.port}")
        except Exception as e:
            self.on_status("error", f"Serial error: {e}")

    def stop(self):
        self._running = False
        if self.ser:
            self.ser.close()
        self.on_status("disconnected", "Serial disconnected")

    def _read_loop(self):
        while self._running:
            try:
                line = self.ser.readline().decode(errors="ignore").strip()
                if line:
                    self.on_data(line)
            except Exception as e:
                self.on_status("error", str(e))
                break

def parse_tension(line: str):
    pattern = r"Tension 1:\s*([\d\.\-eE]+),\s*Tension 2:\s*([\d\.\-eE]+),\s*Tension 3:\s*([\d\.\-eE]+)"
    match = re.search(pattern, line)
    if not match:
        return None
    return list(map(float, match.groups()))

# ══════════════════════════════════════════════════════════════════════════════
class BLEManager:
    """Handles all BLE communication in a background thread."""

    def __init__(self, on_data, on_status):
        self.on_data    = on_data
        self.on_status  = on_status
        self.client     = None
        self.loop       = asyncio.new_event_loop()
        self._connected = False

        t = threading.Thread(target=self._run_loop, daemon=True)
        t.start()

    def _run_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def connect(self):
        asyncio.run_coroutine_threadsafe(self._connect(), self.loop)

    def disconnect(self):
        asyncio.run_coroutine_threadsafe(self._disconnect(), self.loop)

    def send(self, message: str):
        if self._connected:
            asyncio.run_coroutine_threadsafe(self._send(message), self.loop)

    @property
    def connected(self):
        return self._connected

    async def _connect(self):
        self.on_status("scanning", f"Scanning for {DEVICE_NAME}…")
        try:
            device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10)
            if not device:
                self.on_status("error", f"{DEVICE_NAME} not found. Is it powered on?")
                return
            self.on_status("connecting", f"Found {device.address} — connecting…")
            self.client = BleakClient(device, disconnected_callback=self._on_disconnect)
            await self.client.connect()
            await self.client.start_notify(CHARACTERISTIC_UUID, self._on_notify)
            self._connected = True
            self.on_status("connected", f"Connected to {device.address}")
        except Exception as e:
            self.on_status("error", f"Connection failed: {e}")

    async def _disconnect(self):
        if self.client and self._connected:
            await self.client.stop_notify(CHARACTERISTIC_UUID)
            await self.client.disconnect()
        self._connected = False
        self.on_status("disconnected", "Disconnected")

    async def _send(self, message: str):
        try:
            await self.client.write_gatt_char(
                CHARACTERISTIC_UUID,
                message.encode(),
                response=False   # 🔥 change this
            )
        except Exception as e:
            self.on_status("error", f"Send failed: {e}")

    def _on_notify(self, sender, data: bytearray):
        self.on_data(data.decode("utf-8", errors="replace"))

    def _on_disconnect(self, client):
        self._connected = False
        self.on_status("disconnected", "Device disconnected unexpectedly")


# ══════════════════════════════════════════════════════════════════════════════
def parse_payload(raw: str) -> dict:
    result = {}
    try:
        for section in raw.split("|"):
            if ":" not in section:
                continue
            label, values = section.split(":", 1)
            result[label.strip()] = [float(v) for v in values.split(",")]
    except Exception:
        pass
    return result


# ══════════════════════════════════════════════════════════════════════════════
class ValueCard(tk.Frame):
    def __init__(self, parent, title, channel_labels, color=ACCENT, **kwargs):
        super().__init__(parent, bg=PANEL, highlightbackground=BORDER,
                         highlightthickness=1, **kwargs)
        self._color = color
        self._vars  = []

        title_bar = tk.Frame(self, bg=color, height=3)
        title_bar.pack(fill="x", side="top")

        inner = tk.Frame(self, bg=PANEL, padx=14, pady=10)
        inner.pack(fill="both", expand=True)

        tk.Label(inner, text=title, font=FONT_LABEL,
                 bg=PANEL, fg=color).pack(anchor="w")

        row = tk.Frame(inner, bg=PANEL)
        row.pack(fill="x", pady=(8, 0))

        for lbl in channel_labels:
            col = tk.Frame(row, bg=PANEL)
            col.pack(side="left", expand=True, fill="x", padx=4)
            tk.Label(col, text=lbl, font=("Courier New", 8),
                     bg=PANEL, fg=TEXT_DIM).pack()
            var = tk.StringVar(value="—")
            self._vars.append(var)
            tk.Label(col, textvariable=var, font=FONT_VALUE,
                     bg=PANEL, fg=TEXT).pack()

    def update_values(self, values: list):
        for i, var in enumerate(self._vars):
            try:
                var.set(f"{values[i]:.2f}")
            except (IndexError, ValueError):
                var.set("—")

    def reset(self):
        for var in self._vars:
            var.set("—")


# ══════════════════════════════════════════════════════════════════════════════
class ControlPanel(tk.Frame):
    def __init__(self, parent, title, field_labels, color, on_send, **kwargs):
        super().__init__(parent, bg=BG, **kwargs)
        self._color        = color
        self._on_send      = on_send
        self._expanded     = True
        self._entries      = []
        self._field_labels = field_labels

        hdr = tk.Frame(self, bg=PANEL, highlightbackground=BORDER,
                       highlightthickness=1)
        hdr.pack(fill="x")

        accent_bar = tk.Frame(hdr, bg=color, width=4)
        accent_bar.pack(side="left", fill="y")

        self._toggle_lbl = tk.Label(hdr, text="▾", font=("Courier New", 11),
                                    bg=PANEL, fg=color, padx=8)
        self._toggle_lbl.pack(side="left")

        tk.Label(hdr, text=title, font=FONT_LABEL,
                 bg=PANEL, fg=color).pack(side="left", pady=10)

        hdr.bind("<Button-1>", self._toggle)
        self._toggle_lbl.bind("<Button-1>", self._toggle)

        self._body = tk.Frame(self, bg=PANEL, highlightbackground=BORDER,
                              highlightthickness=1, padx=14, pady=12)
        self._body.pack(fill="x", pady=(1, 0))

        fields_row = tk.Frame(self._body, bg=PANEL)
        fields_row.pack(fill="x")

        for lbl in field_labels:
            col = tk.Frame(fields_row, bg=PANEL)
            col.pack(side="left", expand=True, fill="x", padx=6)
            tk.Label(col, text=lbl, font=("Courier New", 8),
                     bg=PANEL, fg=TEXT_DIM).pack(anchor="w")
            entry = tk.Entry(col, bg=BG, fg=TEXT, insertbackground=color,
                             relief="flat", font=FONT_MONO,
                             highlightbackground=BORDER, highlightthickness=1,
                             justify="center", width=10)
            entry.insert(0, "0.00")
            entry.pack(fill="x", ipady=6)
            entry.bind("<FocusIn>",  lambda e, en=entry: en.config(highlightbackground=color))
            entry.bind("<FocusOut>", lambda e, en=entry: en.config(highlightbackground=BORDER))
            entry.bind("<Return>",   lambda e: self._send())
            self._entries.append(entry)

        btn_row = tk.Frame(self._body, bg=PANEL)
        btn_row.pack(fill="x", pady=(10, 0))

        self._send_btn = tk.Button(
            btn_row, text=f"↑  SEND {title.upper()}",
            command=self._send,
            bg=PANEL, fg=color, activebackground=color, activeforeground=BG,
            relief="flat", font=FONT_LABEL, padx=14, pady=6,
            highlightbackground=color, highlightthickness=1, cursor="hand2",
        )
        self._send_btn.pack(side="right")
        self._send_btn.bind("<Enter>", lambda e: self._send_btn.config(bg=color, fg=BG))
        self._send_btn.bind("<Leave>", lambda e: self._send_btn.config(bg=PANEL, fg=color))

        reset_btn = tk.Button(
            btn_row, text="⟳  RESET",
            command=self._reset,
            bg=PANEL, fg=TEXT_DIM, activebackground=BORDER, activeforeground=TEXT,
            relief="flat", font=FONT_LABEL, padx=10, pady=6,
            highlightbackground=BORDER, highlightthickness=1, cursor="hand2",
        )
        reset_btn.pack(side="right", padx=(0, 8))

    def _toggle(self, event=None):
        self._expanded = not self._expanded
        if self._expanded:
            self._body.pack(fill="x", pady=(1, 0))
            self._toggle_lbl.config(text="▾")
        else:
            self._body.pack_forget()
            self._toggle_lbl.config(text="▸")

    def _send(self):
        values = []
        for i, entry in enumerate(self._entries):
            raw = entry.get().strip()
            try:
                values.append(float(raw))
                entry.config(highlightbackground=BORDER)
            except ValueError:
                entry.config(highlightbackground=DANGER)
                messagebox.showerror(
                    "Invalid input",
                    f"'{raw}' is not a valid number for {self._field_labels[i]}.")
                return
        self._on_send(values)

    def _reset(self):
        for entry in self._entries:
            entry.delete(0, "end")
            entry.insert(0, "0.00")

    def set_values(self, values: list):
        for i, entry in enumerate(self._entries):
            try:
                entry.delete(0, "end")
                entry.insert(0, f"{values[i]:.3f}")
            except IndexError:
                pass

    def set_enabled(self, enabled: bool):
        state = "normal" if enabled else "disabled"
        for entry in self._entries:
            entry.config(state=state)
        self._send_btn.config(state=state)


# ══════════════════════════════════════════════════════════════════════════════
class LiveGraph(tk.Frame):
    """
    Three vertically stacked subplots — one per tension channel.
    Each subplot also shows the corresponding setpoint as a dashed line.
    """

    def __init__(self, parent, **kwargs):
        super().__init__(parent, bg=PANEL, highlightbackground=BORDER,
                         highlightthickness=1, **kwargs)

        # ── header bar ────────────────────────────────────────────────────
        hdr = tk.Frame(self, bg=BORDER, height=24)
        hdr.pack(fill="x")
        hdr.pack_propagate(False)

        tk.Label(hdr, text="  LIVE GRAPH  —  STRING TENSION",
                 font=("Courier New", 8, "bold"),
                 bg=BORDER, fg=TEXT_DIM).pack(side="left", pady=4)

        # acquisition controls inside the header
        ctrl = tk.Frame(hdr, bg=BORDER)
        ctrl.pack(side="right", padx=8)

        self._acq_running = False

        self._btn_start = tk.Button(
            ctrl, text="▶  START ACQ",
            command=self._start_acq,
            bg=BORDER, fg=SUCCESS, activebackground=SUCCESS, activeforeground=BG,
            relief="flat", font=("Courier New", 8, "bold"), padx=8, pady=2,
            highlightbackground=SUCCESS, highlightthickness=1, cursor="hand2",
        )
        self._btn_start.pack(side="left", padx=(0, 4))
        self._btn_start.bind("<Enter>", lambda e: self._btn_start.config(bg=SUCCESS, fg=BG))
        self._btn_start.bind("<Leave>", lambda e: self._btn_start.config(bg=BORDER, fg=SUCCESS))

        self._btn_stop = tk.Button(
            ctrl, text="■  STOP",
            command=self._stop_acq,
            bg=BORDER, fg=DANGER, activebackground=DANGER, activeforeground=BG,
            relief="flat", font=("Courier New", 8, "bold"), padx=8, pady=2,
            highlightbackground=BORDER, highlightthickness=1, cursor="hand2",
            state="disabled",
        )
        self._btn_stop.pack(side="left", padx=(0, 4))
        self._btn_stop.bind("<Enter>", lambda e: self._btn_stop.config(bg=DANGER, fg=BG)
                            if self._acq_running else None)
        self._btn_stop.bind("<Leave>", lambda e: self._btn_stop.config(bg=BORDER, fg=DANGER)
                            if self._acq_running else None)

        # status pill
        self._acq_dot = tk.Label(ctrl, text="●", font=("Courier New", 10),
                                 bg=BORDER, fg=BORDER)
        self._acq_dot.pack(side="left", padx=(2, 0))

        # ── matplotlib figure ──────────────────────────────────────────────
        self._fig = Figure(figsize=(8, 3.8), dpi=96,
                           facecolor=GRAPH_BG, tight_layout=True)
        self._fig.subplots_adjust(hspace=0.12, left=0.08,
                                  right=0.97, top=0.96, bottom=0.10)

        self._axes = []
        for i in range(3):
            ax = self._fig.add_subplot(3, 1, i + 1)
            ax.set_facecolor(GRAPH_AXES_BG)
            ax.tick_params(colors=TEXT_DIM, labelsize=7)
            ax.set_ylabel(f"CH {i+1} (N)", color=CH_COLORS[i],
                          fontsize=7, labelpad=4)
            for spine in ax.spines.values():
                spine.set_edgecolor(BORDER)
            ax.xaxis.set_major_locator(ticker.AutoLocator())
            ax.yaxis.set_major_locator(ticker.MaxNLocator(4))
            ax.grid(True, color=BORDER, linewidth=0.5, linestyle="--", alpha=0.6)
            self._axes.append(ax)

        self._axes[-1].set_xlabel("samples", color=TEXT_DIM, fontsize=7)
        for ax in self._axes[:-1]:
            ax.tick_params(labelbottom=False)

        self._canvas = FigureCanvasTkAgg(self._fig, master=self)
        self._canvas.get_tk_widget().pack(fill="both", expand=True, padx=2, pady=(0, 2))

        # ── rolling data buffers ───────────────────────────────────────────
        self._tension   = [collections.deque(maxlen=MAX_POINTS) for _ in range(3)]
        self._setpoints = [collections.deque(maxlen=MAX_POINTS) for _ in range(3)]

        # matplotlib line objects
        self._lines_t  = []
        self._lines_sp = []
        for i, ax in enumerate(self._axes):
            lt, = ax.plot([], [], color=CH_COLORS[i],       linewidth=1.4, label=f"Tension CH{i+1}")
            ls, = ax.plot([], [], color=SETPOINT_COLORS[i], linewidth=0.9,
                          linestyle="--", alpha=0.75, label=f"Setpoint CH{i+1}")
            ax.legend(loc="upper right", fontsize=6,
                      facecolor=PANEL, edgecolor=BORDER, labelcolor=TEXT_DIM)
            self._lines_t.append(lt)
            self._lines_sp.append(ls)

        # ── blink animation for recording dot ─────────────────────────────
        self._blink_state = False
        self._blink_job   = None

    # ── public API ────────────────────────────────────────────────────────────
    @property
    def acquiring(self):
        return self._acq_running

    def push(self, tension: list, setpoints: list):
        """Called by App whenever new data arrives and acquisition is active."""
        if not self._acq_running:
            return
        for i in range(3):
            try:
                self._tension[i].append(tension[i])
            except IndexError:
                self._tension[i].append(float("nan"))
            try:
                self._setpoints[i].append(setpoints[i])
            except IndexError:
                self._setpoints[i].append(float("nan"))

    def refresh(self):
        """Redraw; call periodically from the GUI thread."""
        if not self._acq_running:
            return
        for i in range(3):
            yt  = list(self._tension[i])
            ysp = list(self._setpoints[i])
            xs  = list(range(len(yt)))
            self._lines_t[i].set_data(xs, yt)
            xsp = list(range(len(ysp)))
            self._lines_sp[i].set_data(xsp, ysp)
            self._axes[i].relim()
            self._axes[i].autoscale_view()
        self._canvas.draw_idle()

    def clear_data(self):
        for i in range(3):
            self._tension[i].clear()
            self._lines_t[i].set_data([], [])
            self._lines_sp[i].set_data([], [])
        self._canvas.draw_idle()

    def get_series(self):
        """Return (tension_lists, setpoint_lists) as plain lists."""
        return (
            [list(d) for d in self._tension],
            [list(d) for d in self._setpoints],
        )

    # ── acquisition control ───────────────────────────────────────────────────
    def _start_acq(self):
        self._acq_running = True
        self._btn_start.config(state="disabled")
        self._btn_stop.config(state="normal")
        self._acq_dot.config(fg=DANGER)
        self._blink()

    def _stop_acq(self):
        self._acq_running = False
        self._btn_start.config(state="normal")
        self._btn_stop.config(state="disabled")
        if self._blink_job:
            self.after_cancel(self._blink_job)
            self._blink_job = None
        self._acq_dot.config(fg=BORDER)

    def _blink(self):
        if not self._acq_running:
            return
        self._blink_state = not self._blink_state
        self._acq_dot.config(fg=DANGER if self._blink_state else BORDER)
        self._blink_job = self.after(600, self._blink)


# ══════════════════════════════════════════════════════════════════════════════
class App(tk.Tk):

    def __init__(self):
        super().__init__()
        self.title("ESP32 BLE Monitor")
        self.configure(bg=BG)
        self.geometry("860x1100")
        self.minsize(700, 800)
        self.resizable(True, True)

        # acquisition log rows
        self._acq_rows: list[dict] = []
        self._last_pwm       = [0.0, 0.0, 0.0]
        self._last_setpoints = [0.0, 0.0, 0.0]

        self._ble = BLEManager(
            on_data=self._on_ble_data,
            on_status=self._on_ble_status,
        )

        # Serial added in parallel
        self._serial = SerialManager(
            port="COM3",       # change to your PC port
            baudrate=115200,
            on_data=self._on_serial_data,
            on_status=self._on_serial_status  # can reuse BLE status handler
        )
        self._build_ui()
        self._graph_tick()

    def _on_serial_status(self, state: str, message: str):
        # For example, just log it
        ts = datetime.now().strftime("%H:%M:%S")
        self._log_write(f"[{ts}] SERIAL STATUS: {message}\n", "accent")

    # ── UI construction ───────────────────────────────────────────────────────
    def _build_ui(self):
        # ── Header ─────────────────────────────────────────────────────────
        hdr = tk.Frame(self, bg=PANEL, height=54, highlightbackground=BORDER,
                       highlightthickness=1)
        hdr.pack(fill="x", side="top")
        hdr.pack_propagate(False)

        tk.Label(hdr, text="◈  Rétroaction haptique", font=FONT_TITLE,
                 bg=PANEL, fg=ACCENT).pack(side="left", padx=20)

        self._status_dot = tk.Label(hdr, text="●", font=("Courier New", 16),
                                    bg=PANEL, fg=DANGER)
        self._status_dot.pack(side="right", padx=(0, 8))
        self._status_label = tk.Label(hdr, text="DISCONNECTED",
                                      font=FONT_LABEL, bg=PANEL, fg=TEXT_DIM)
        self._status_label.pack(side="right", padx=(0, 4))

        # ── Control bar ────────────────────────────────────────────────────
        ctrl = tk.Frame(self, bg=BG, pady=12)
        ctrl.pack(fill="x", padx=16)

        self._btn_connect = self._make_button(
            ctrl, "⚡  CONNECT", self._on_connect, ACCENT)
        self._btn_connect.pack(side="left", padx=(0, 8))

        self._btn_disconnect = self._make_button(
            ctrl, "✕  DISCONNECT", self._on_disconnect, DANGER)
        self._btn_disconnect.pack(side="left", padx=(0, 8))
        self._btn_disconnect.config(state="disabled")

        self._btn_clear = self._make_button(
            ctrl, "⌫  CLEAR LOG", self._clear_log, TEXT_DIM)
        self._btn_clear.pack(side="left", padx=(0, 8))

        self._btn_save = self._make_button(
            ctrl, "💾  SAVE CSV", self._save_csv, SUCCESS)
        self._btn_save.pack(side="left", padx=(0, 8))

        self._btn_delete = self._make_button(
            ctrl, "🗑  DELETE DATA", self._delete_data, DANGER)
        self._btn_delete.pack(side="left")

        # ── Scrollable body ────────────────────────────────────────────────
        body_outer = tk.Frame(self, bg=BG)
        body_outer.pack(fill="both", expand=True)

        canvas = tk.Canvas(body_outer, bg=BG, highlightthickness=0)
        scrollbar = tk.Scrollbar(body_outer, orient="vertical",
                                 command=canvas.yview)
        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)

        scroll_frame = tk.Frame(canvas, bg=BG)
        win_id = canvas.create_window((0, 0), window=scroll_frame, anchor="nw")

        def _on_frame_configure(e):
            canvas.configure(scrollregion=canvas.bbox("all"))
        def _on_canvas_configure(e):
            canvas.itemconfig(win_id, width=e.width)

        scroll_frame.bind("<Configure>", _on_frame_configure)
        canvas.bind("<Configure>", _on_canvas_configure)
        canvas.bind_all("<MouseWheel>",
                        lambda e: canvas.yview_scroll(-1*(e.delta//120), "units"))

        # ── Data cards ─────────────────────────────────────────────────────
        cards_frame = tk.Frame(scroll_frame, bg=BG)
        cards_frame.pack(fill="x", padx=16, pady=(0, 8))
        cards_frame.columnconfigure(0, weight=1)
        cards_frame.columnconfigure(1, weight=1)

        self._card_tension = ValueCard(
            cards_frame, "STRING TENSION (N)",
            ["CH 1", "CH 2", "CH 3"], color=ACCENT)
        self._card_tension.grid(row=0, column=0, padx=(0, 6), pady=4, sticky="nsew")

        self._card_pwm = ValueCard(
            cards_frame, "PWM VALUES",
            ["CH 1", "CH 2", "CH 3"], color=WARNING)
        self._card_pwm.grid(row=0, column=1, padx=(6, 0), pady=4, sticky="nsew")

        self._card_setpoints = ValueCard(
            cards_frame, "SET POINTS",
            ["CH 1", "CH 2", "CH 3"], color=SUCCESS)
        self._card_setpoints.grid(row=1, column=0, padx=(0, 6), pady=4, sticky="nsew")

        self._card_pid = ValueCard(
            cards_frame, "PID VALUES",
            ["P", "I", "D"], color="#e040fb")
        self._card_pid.grid(row=1, column=1, padx=(6, 0), pady=4, sticky="nsew")

        # ── Graph + Controls side by side ──────────────────────────────────
        side_frame = tk.Frame(scroll_frame, bg=BG)
        side_frame.pack(fill="x", padx=16, pady=(0, 10))

        self._graph = LiveGraph(side_frame)
        self._graph.pack(side="left", fill="both", expand=True, padx=(0, 6))

        # right column: controls
        right_col = tk.Frame(side_frame, bg=BG)
        right_col.pack(side="left", fill="y", padx=(6, 0))

        tk.Label(right_col, text="  CONTROLS", font=("Courier New", 8, "bold"),
                 bg=BG, fg=TEXT_DIM).pack(anchor="w", pady=(0, 6))

        # ── Setpoints control panel ────────────────────────────────────────
        self._ctrl_setpoints = ControlPanel(
            right_col,
            title="Set Points",
            field_labels=["CH 1", "CH 2", "CH 3"],
            color=SUCCESS,
            on_send=self._on_send_setpoints,
        )
        self._ctrl_setpoints.pack(fill="x", pady=(0, 8))
        self._ctrl_setpoints.set_enabled(False)

        # ── PID control panel ──────────────────────────────────────────────
        self._ctrl_pid = ControlPanel(
            right_col,
            title="PID Values",
            field_labels=["P gain", "I gain", "D gain"],
            color="#e040fb",
            on_send=self._on_send_pid,
        )
        self._ctrl_pid.pack(fill="x", pady=(0, 8))
        self._ctrl_pid.set_enabled(False)

        # ── Send command bar ───────────────────────────────────────────────
        tk.Frame(scroll_frame, bg=BORDER, height=1).pack(
            fill="x", padx=16, pady=(4, 10))

        send_frame = tk.Frame(scroll_frame, bg=PANEL, highlightbackground=BORDER,
                              highlightthickness=1)
        send_frame.pack(fill="x", padx=16, pady=(0, 10))

        tk.Label(send_frame, text="TX ›", font=FONT_LABEL,
                 bg=PANEL, fg=ACCENT_DIM, padx=10).pack(side="left")

        self._cmd_var = tk.StringVar()
        cmd_entry = tk.Entry(send_frame, textvariable=self._cmd_var,
                             bg=BG, fg=TEXT, insertbackground=ACCENT,
                             relief="flat", font=FONT_MONO,
                             highlightthickness=0)
        cmd_entry.pack(side="left", fill="x", expand=True, ipady=8, padx=(0, 4))
        cmd_entry.bind("<Return>", lambda e: self._on_send())

        self._make_button(send_frame, "SEND", self._on_send, ACCENT,
                          padx=16, pady=6).pack(side="right", padx=8, pady=6)

        # ── Log ────────────────────────────────────────────────────────────
        log_outer = tk.Frame(scroll_frame, bg=PANEL, highlightbackground=BORDER,
                             highlightthickness=1, height=200)
        log_outer.pack(fill="both", expand=True, padx=16, pady=(0, 16))
        log_outer.pack_propagate(False)

        log_hdr = tk.Frame(log_outer, bg=BORDER, height=24)
        log_hdr.pack(fill="x")
        log_hdr.pack_propagate(False)
        tk.Label(log_hdr, text="  LOG", font=("Courier New", 8, "bold"),
                 bg=BORDER, fg=TEXT_DIM).pack(side="left", pady=4)

        self._log = scrolledtext.ScrolledText(
            log_outer, bg=BG, fg=TEXT_DIM, font=("Courier New", 9),
            relief="flat", wrap="word", state="disabled",
            insertbackground=ACCENT, padx=10, pady=6)
        self._log.pack(fill="both", expand=True)

        self._log.tag_config("accent",  foreground=ACCENT)
        self._log.tag_config("success", foreground=SUCCESS)
        self._log.tag_config("warning", foreground=WARNING)
        self._log.tag_config("error",   foreground=DANGER)
        self._log.tag_config("dim",     foreground=TEXT_DIM)
        self._log.tag_config("data",    foreground=TEXT)

    def _make_button(self, parent, text, cmd, color, padx=14, pady=6):
        btn = tk.Button(
            parent, text=text, command=cmd,
            bg=PANEL, fg=color, activebackground=color,
            activeforeground=BG, relief="flat", font=FONT_LABEL,
            padx=padx, pady=pady,
            highlightbackground=color, highlightthickness=1,
            cursor="hand2",
        )
        btn.bind("<Enter>", lambda e: btn.config(bg=color, fg=BG))
        btn.bind("<Leave>", lambda e: btn.config(bg=PANEL, fg=color))
        return btn


    def _on_serial_data(self, raw: str):
        self.after(0, self._handle_serial_data, raw)

    def _handle_serial_data(self, raw: str):
        print(raw)
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]

        self._log_write(f"[{ts}] {raw}\n", "data")

        tension = parse_tension(raw)
        if not tension:
            return

        # update UI
        self._card_setpoints.update_values(tension)

        self._on_send_setpoints(tension)

        # push to graph (reuse your system)
        self._graph.push(tension, self._last_setpoints)

    # ── Graph refresh loop ────────────────────────────────────────────────────
    def _graph_tick(self):
        self._graph.refresh()
        self.after(100, self._graph_tick)   # ~10 fps

    # ── BLE callbacks ─────────────────────────────────────────────────────────
    def _on_ble_data(self, raw: str):
        self.after(0, self._handle_data, raw)

    def _on_ble_status(self, state: str, message: str):
        self.after(0, self._handle_status, state, message)

    # ── GUI-thread handlers ───────────────────────────────────────────────────
    def _handle_data(self, raw: str):
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self._log_write(f"[{ts}] ", "dim")
        self._log_write(raw + "\n", "data")

        parsed = parse_payload(raw)
        if not parsed:
            return

        tension   = parsed.get("TENSION",   [])
        setpoints = parsed.get("SETPOINTS", self._last_setpoints)

        if "TENSION"   in parsed: self._card_tension.update_values(parsed["TENSION"])
        if "PWM"       in parsed: self._card_pwm.update_values(parsed["PWM"])
        # if "SETPOINTS" in parsed:
        #     self._card_setpoints.update_values(parsed["SETPOINTS"])
        #     self._last_setpoints = list(parsed["SETPOINTS"])
        if "PID"       in parsed:
            self._card_pid.update_values(parsed["PID"])
            self._last_pwm = list(parsed.get("PWM", self._last_pwm))

        # push to graph
        self._graph.push(tension, setpoints)

        # record acquisition row
        if self._graph.acquiring:
            row = {
                "timestamp":   datetime.now().isoformat(timespec="milliseconds"),
                "pwm_ch1":     self._last_pwm[0],
                "pwm_ch2":     self._last_pwm[1],
                "pwm_ch3":     self._last_pwm[2],
                "sp_ch1":      setpoints[0] if len(setpoints) > 0 else "",
                "sp_ch2":      setpoints[1] if len(setpoints) > 1 else "",
                "sp_ch3":      setpoints[2] if len(setpoints) > 2 else "",
                "tension_ch1": tension[0] if len(tension) > 0 else "",
                "tension_ch2": tension[1] if len(tension) > 1 else "",
                "tension_ch3": tension[2] if len(tension) > 2 else "",
            }
            self._acq_rows.append(row)

    def _handle_status(self, state: str, message: str):
        ts = datetime.now().strftime("%H:%M:%S")
        colour_map = {
            "connected":    (SUCCESS, SUCCESS, "CONNECTED"),
            "disconnected": (DANGER,  "warning", "DISCONNECTED"),
            "scanning":     (WARNING, "warning", "SCANNING…"),
            "connecting":   (WARNING, "warning", "CONNECTING…"),
            "error":        (DANGER,  "error",   "ERROR"),
        }
        dot_color, tag, label = colour_map.get(state, (TEXT_DIM, "dim", state.upper()))

        self._status_dot.config(fg=dot_color)
        self._status_label.config(text=label, fg=dot_color)
        self._log_write(f"[{ts}] {message}\n", tag)

        if state == "connected":
            self._btn_connect.config(state="disabled")
            self._btn_disconnect.config(state="normal")
            self._ctrl_setpoints.set_enabled(True)
            self._ctrl_pid.set_enabled(True)
        elif state in ("disconnected", "error"):
            self._btn_connect.config(state="normal")
            self._btn_disconnect.config(state="disabled")
            self._ctrl_setpoints.set_enabled(False)
            self._ctrl_pid.set_enabled(False)
            for card in (self._card_tension, self._card_pwm,
                         self._card_setpoints, self._card_pid):
                card.reset()

    # ── Button handlers ───────────────────────────────────────────────────────
    def _on_connect(self):
        self._btn_connect.config(state="disabled")
        self._ble.connect()
        self._serial.start()

    def _on_disconnect(self):
        self._btn_disconnect.config(state="disabled")
        self._ble.disconnect()

    def _on_send(self):
        msg = self._cmd_var.get().strip()
        if not msg:
            return
        if not self._ble.connected:
            self._log_write("Not connected.\n", "error")
            return
        self._ble.send(msg)
        ts = datetime.now().strftime("%H:%M:%S")
        self._log_write(f"[{ts}] TX › {msg}\n", "accent")
        self._cmd_var.set("")

    def _on_send_setpoints(self, values: list):
        msg = "SET_SETPOINTS:" + ",".join(f"{v:.3f}" for v in values)
        self._last_setpoints = values
        self._ble.send(msg)
        ts = datetime.now().strftime("%H:%M:%S")
        self._log_write(f"[{ts}] TX › {msg}\n", "success")

    def _on_send_pid(self, values: list):
        msg = "SET_PID:" + ",".join(f"{v:.4f}" for v in values)
        self._ble.send(msg)
        ts = datetime.now().strftime("%H:%M:%S")
        self._log_write(f"[{ts}] TX › {msg}\n", "accent")

    def _clear_log(self):
        self._log.config(state="normal")
        self._log.delete("1.0", "end")
        self._log.config(state="disabled")

    def _delete_data(self):
        if not self._acq_rows and not any(self._graph._tension[i] for i in range(3)):
            messagebox.showinfo("No data", "There is no acquisition data to delete.")
            return
        if not messagebox.askyesno("Delete data",
                                   "This will permanently clear all recorded acquisition data\nand the live graph. Continue?"):
            return
        self._acq_rows.clear()
        self._last_pwm       = [0.0, 0.0, 0.0]
        self._last_setpoints = [0.0, 0.0, 0.0]
        self._graph.clear_data()
        ts = datetime.now().strftime("%H:%M:%S")
        self._log_write(f"[{ts}] Acquisition data cleared.\n", "warning")

    def _save_csv(self):
        if not self._acq_rows:
            messagebox.showinfo("No data", "No acquisition data to save.\n"
                                "Start acquisition and receive some data first.")
            return

        path = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
            title="Save acquisition data",
            initialfile=f"esp32_acq_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
        )
        if not path:
            return

        fieldnames = ["timestamp", "pwm_ch1", "pwm_ch2", "pwm_ch3",
                      "sp_ch1", "sp_ch2", "sp_ch3",
                      "tension_ch1", "tension_ch2", "tension_ch3"]
        try:
            with open(path, "w", newline="") as f:
                writer = csv.DictWriter(f, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(self._acq_rows)
            self._log_write(f"[{datetime.now().strftime('%H:%M:%S')}] "
                            f"Saved {len(self._acq_rows)} rows → {path}\n", "success")
            messagebox.showinfo("Saved", f"Data saved to:\n{path}")
        except Exception as e:
            messagebox.showerror("Save failed", str(e))

    # ── Helpers ───────────────────────────────────────────────────────────────
    def _log_write(self, text: str, tag: str = ""):
        self._log.config(state="normal")
        self._log.insert("end", text, tag)
        self._log.see("end")
        self._log.config(state="disabled")


# ══════════════════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    
    app = App()
    app.mainloop()
