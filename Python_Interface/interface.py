"""
ESP32 BLE Monitor
A GUI application to connect and monitor ESP32 BLE data.
Requires: pip install bleak
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import asyncio
import threading
from bleak import BleakClient, BleakScanner
from datetime import datetime

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

    # ── public API (called from GUI thread) ───────────────────────────────────
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

    # ── async internals ───────────────────────────────────────────────────────
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
                CHARACTERISTIC_UUID, message.encode(), response=True 
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
    """
    Parse: TENSION:v1,v2,v3|PWM:v1,v2,v3|SETPOINTS:v1,v2,v3|PID:p,i,d
    Returns dict of label → list[float], or empty dict on failure.
    """
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
    """A single labelled card showing up to 3 float values."""

    def __init__(self, parent, title, channel_labels, color=ACCENT, **kwargs):
        super().__init__(parent, bg=PANEL, highlightbackground=BORDER,
                         highlightthickness=1, **kwargs)

        self._color  = color
        self._vars   = []

        # Title bar
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
    """
    A collapsible panel with labelled float inputs and a Send button.
    on_send(values: list[float]) is called when the user submits.
    """

    def __init__(self, parent, title, field_labels, color, on_send, **kwargs):
        super().__init__(parent, bg=BG, **kwargs)
        self._color    = color
        self._on_send  = on_send
        self._expanded = True
        self._entries  = []
        self._field_labels = field_labels

        # ── header row (click to collapse) ────────────────────────────────
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

        # ── body ──────────────────────────────────────────────────────────
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

        # send button
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

        # reset button
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
                    f"'{raw}' is not a valid number for {self._field_labels[i]}."
                )
                return
        self._on_send(values)

    def _reset(self):
        for entry in self._entries:
            entry.delete(0, "end")
            entry.insert(0, "0.00")

    def set_values(self, values: list):
        """Pre-fill entries from incoming ESP32 data."""
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
class App(tk.Tk):

    def __init__(self):
        super().__init__()
        self.title("ESP32 BLE Monitor")
        self.configure(bg=BG)
        self.geometry("820x900")
        self.minsize(700, 700)
        self.resizable(True, True)

        self._ble = BLEManager(
            on_data=self._on_ble_data,
            on_status=self._on_ble_status,
        )
        self._build_ui()

    # ── UI construction ───────────────────────────────────────────────────────
    def _build_ui(self):
        # ── Header ─────────────────────────────────────────────────────────
        hdr = tk.Frame(self, bg=PANEL, height=54, highlightbackground=BORDER,
                       highlightthickness=1)
        hdr.pack(fill="x", side="top")
        hdr.pack_propagate(False)

        tk.Label(hdr, text="◈  ESP32 BLE MONITOR", font=FONT_TITLE,
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
        self._btn_clear.pack(side="left")

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

        # ── Divider ────────────────────────────────────────────────────────
        div = tk.Frame(scroll_frame, bg=BORDER, height=1)
        div.pack(fill="x", padx=16, pady=(4, 10))

        tk.Label(scroll_frame, text="  CONTROLS", font=("Courier New", 8, "bold"),
                 bg=BG, fg=TEXT_DIM).pack(anchor="w", padx=16, pady=(0, 6))

        # ── Setpoints control panel ────────────────────────────────────────
        self._ctrl_setpoints = ControlPanel(
            scroll_frame,
            title="Set Points",
            field_labels=["CH 1", "CH 2", "CH 3"],
            color=SUCCESS,
            on_send=self._on_send_setpoints,
        )
        self._ctrl_setpoints.pack(fill="x", padx=16, pady=(0, 8))
        self._ctrl_setpoints.set_enabled(False)

        # ── PID control panel ──────────────────────────────────────────────
        self._ctrl_pid = ControlPanel(
            scroll_frame,
            title="PID Values",
            field_labels=["P gain", "I gain", "D gain"],
            color="#e040fb",
            on_send=self._on_send_pid,
        )
        self._ctrl_pid.pack(fill="x", padx=16, pady=(0, 8))
        self._ctrl_pid.set_enabled(False)

        # ── Send command bar ───────────────────────────────────────────────
        div2 = tk.Frame(scroll_frame, bg=BORDER, height=1)
        div2.pack(fill="x", padx=16, pady=(4, 10))

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

    # ── BLE callbacks (called from BLE thread → schedule on GUI thread) ───────
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
        if parsed:
            if "TENSION"   in parsed: self._card_tension.update_values(parsed["TENSION"])
            if "PWM"       in parsed: self._card_pwm.update_values(parsed["PWM"])
            if "SETPOINTS" in parsed:
                self._card_setpoints.update_values(parsed["SETPOINTS"])
                # self._ctrl_setpoints.set_values(parsed["SETPOINTS"])
            if "PID"       in parsed:
                self._card_pid.update_values(parsed["PID"])
                # self._ctrl_pid.set_values(parsed["PID"])

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