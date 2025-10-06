import tkinter as tk
from tkinter import ttk, messagebox
import threading
import time
from typing import Dict

from networkx import center
from telemetry_client import TelemetryClient

APP_TITLE = "Telemática — Python Client (Tkinter)"

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("880x560")

        # --- Campos conexión ---
        self.host_var = tk.StringVar(value="127.0.0.1")
        self.port_var = tk.IntVar(value=5050)
        self.role_var = tk.StringVar(value="ADMIN")
        self.user_var = tk.StringVar(value="root")
        self.pass_var = tk.StringVar(value="password")
        self.demo_var = tk.BooleanVar(value=False)

        # --- Etiquetas telemetría ---
        self.speed_var = tk.StringVar(value="—")
        self.battery_var = tk.StringVar(value="—")
        self.dir_var = tk.StringVar(value="—")
        self.temp_var = tk.StringVar(value="—")
        self.cmd_status_var = tk.StringVar(value="—")

        self.client: TelemetryClient | None = None
        self.is_observer: bool = False
        self.cmd_frame: ttk.Frame | None = None
        self._demo_thread: threading.Thread | None = None
        self._demo_stop = threading.Event()

        self._build_ui()
        self._set_controls_enabled(False)

    # ---------------- UI ----------------
    def _build_ui(self):
        top = ttk.Frame(self, padding=8)
        top.pack(side=tk.TOP, fill=tk.X)

        def add_row(r, c0, w0, c1):
            ttk.Label(top, text=c0).grid(row=r, column=0, sticky="w", padx=4, pady=4)
            w0.grid(row=r, column=1, sticky="we", padx=4, pady=4)
            ttk.Label(top, text=c1).grid(row=r, column=2, sticky="w", padx=4, pady=4)

        host_entry = ttk.Entry(top, textvariable=self.host_var, width=18)
        port_entry = ttk.Spinbox(top, from_=1, to=65535, textvariable=self.port_var, width=8)

        ttk.Label(top, text="Host").grid(row=0, column=0, sticky="w"); host_entry.grid(row=0, column=1, sticky="we")
        ttk.Label(top, text="Puerto").grid(row=0, column=2, sticky="w"); port_entry.grid(row=0, column=3, sticky="we")

        ttk.Label(top, text="Role").grid(row=1, column=0, sticky="w")
        ttk.Entry(top, textvariable=self.role_var).grid(row=1, column=1, sticky="we")
        ttk.Label(top, text="Usuario").grid(row=1, column=2, sticky="w")
        ttk.Entry(top, textvariable=self.user_var).grid(row=1, column=3, sticky="we")

        ttk.Label(top, text="Password").grid(row=2, column=0, sticky="w")
        ttk.Entry(top, textvariable=self.pass_var, show="*").grid(row=2, column=1, sticky="we")
        ttk.Checkbutton(top, text="Demo sin servidor", variable=self.demo_var).grid(row=2, column=2, sticky="w")

        self.btn_login = ttk.Button(top, text="Login", command=self._on_login)
        self.btn_login.grid(row=2, column=3, sticky="we", padx=4)

        # Centro: telemetría + comandos
        center = ttk.Frame(self, padding=8)
        center.pack(side=tk.TOP, fill=tk.X)

        # Título Status
        ttk.Label(center, text="Status", font=("TkDefaultFont", 11, "bold")).pack(
            side=tk.TOP, anchor="w", pady=(0, 6)
        )

        # Telemetría
        grid = ttk.Frame(center)
        grid.pack(side=tk.TOP, fill=tk.X)

        def label_row(row, name, var):
            ttk.Label(grid, text=name).grid(row=row, column=0, sticky="w", padx=6, pady=6)
            ttk.Label(grid, textvariable=var, width=18).grid(row=row, column=1, sticky="w", padx=6, pady=6)

        label_row(0, "Speed", self.speed_var)
        ttk.Label(grid, text="Battery").grid(row=0, column=2, sticky="w", padx=6, pady=6)
        ttk.Label(grid, textvariable=self.battery_var, width=18).grid(row=0, column=3, sticky="w", padx=6, pady=6)
        label_row(1, "Dir", self.dir_var)
        ttk.Label(grid, text="Temp").grid(row=1, column=2, sticky="w", padx=6, pady=6)
        ttk.Label(grid, textvariable=self.temp_var, width=18).grid(row=1, column=3, sticky="w", padx=6, pady=6)

        # Botones de comandos
        self.cmd_frame = ttk.Frame(center)
        self.cmd_frame.pack(side=tk.TOP, fill=tk.X, pady=8)

        self.btn_start = ttk.Button(self.cmd_frame, text="START (SPEED UP)", command=self._cmd_start)
        self.btn_left = ttk.Button(self.cmd_frame, text="TURN LEFT", command=lambda: self._send_wire("TURN LEFT"))
        self.btn_right = ttk.Button(self.cmd_frame, text="TURN RIGHT", command=lambda: self._send_wire("TURN RIGHT"))
        self.btn_battery = ttk.Button(self.cmd_frame, text="BATTERY RESET", command=lambda: self._send_wire("BATTERY RESET"))

        for i, w in enumerate([self.btn_start, self.btn_left, self.btn_right, self.btn_battery]):
            w.grid(row=0, column=i, padx=6, pady=6, sticky="we")

        # Último comando
        status = ttk.Frame(center)
        status.pack(side=tk.TOP, fill=tk.X)
        ttk.Label(status, text="Último comando:").pack(side=tk.LEFT)
        ttk.Label(status, textvariable=self.cmd_status_var).pack(side=tk.LEFT, padx=8)

        # Log
        bottom = ttk.Frame(self, padding=8)
        bottom.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.log = tk.Text(bottom, height=10)
        self.log.pack(fill=tk.BOTH, expand=True)

        self.columnconfigure(0, weight=1)
        top.columnconfigure(1, weight=1)
        top.columnconfigure(3, weight=1)

        self.protocol("WM_DELETE_WINDOW", self._on_close)

    # ------------- Eventos -------------
    def _on_login(self):
        if self.client and self.client.is_connected():
            self._append_log("Ya conectado.")
            return

        self.btn_login.config(state=tk.DISABLED)

        if self.demo_var.get():
            self._append_log("Modo Demo: generando DATA cada 1s")
            self._apply_role_ui(True)
            self._start_demo()
            return

        host = self.host_var.get().strip()
        port = int(self.port_var.get())
        role = self.role_var.get().strip()
        self.is_observer = (role.upper() == "OBSERVER")
        user = self.user_var.get().strip()
        pwd  = self.pass_var.get()

        self.client = TelemetryClient(
            host, port, role, user, pwd,
            listener=dict(
                on_connected=lambda: self._append_log(f"Conectado a {host}:{port}"),
                on_login_result=self._on_login_result,
                on_status=self._update_status,
                on_server_message=lambda line: self._on_server_message(line),
                on_error=lambda msg, ex=None: self._on_error(msg, ex),
                on_closed=lambda: self._on_closed()
            )
        )
        self.client.connect_async()

    def _on_login_result(self, ok: bool, line: str):
        self._append_log(f"LOGIN → {line}")
        self._apply_role_ui(ok)
        if not ok:
            self.btn_login.config(state=tk.NORMAL)

    def _on_server_message(self, line: str):
        if not line:
            return
        self._append_log(f"SRV: {line}")
        u = line.strip().upper()
        if u.startswith("OK"):
            self.cmd_status_var.set("OK")
        elif u.startswith("ERR"):
            self.cmd_status_var.set("ERR")

    def _on_error(self, msg: str, ex: Exception | None):
        self._append_log(f"ERR: {msg}{(' — ' + str(ex)) if ex else ''}")
        self.btn_login.config(state=tk.NORMAL)
        self._set_controls_enabled(False)

    def _on_closed(self):
        self._append_log("Conexión cerrada")
        self.btn_login.config(state=tk.NORMAL)
        self._set_controls_enabled(False)

    def _apply_role_ui(self, logged_ok: bool):
        
        if not logged_ok:
            self._set_controls_enabled(False)
            if self.cmd_frame:
                self.cmd_frame.pack_forget()
            return

        if self.is_observer:
            
            if self.cmd_frame:
                self.cmd_frame.pack_forget()
            
            self._set_controls_enabled(False)
            self._append_log("Rol OBSERVER: solo visualización de Status.")
        else:
            
            if self.cmd_frame:
                
                self.cmd_frame.pack(side=tk.TOP, fill=tk.X, pady=8)
            self._set_controls_enabled(True)


    # -------- Demo loop --------
    def _start_demo(self):
        self._demo_stop.clear()
        def loop():
            speed = 0.0
            temp = 25.0
            battery = 100
            dirs = ["FORWARD", "LEFT", "RIGHT", "STOP"]
            d = 0
            while not self._demo_stop.is_set():
                speed = (speed + 3.0) % 120.0
                temp += (0.5 - time.time()%1) * 0.2
                battery = max(0, battery - (1 if int(time.time()) % 4 == 0 else 0))
                d = (d + 1) % len(dirs)
                self._update_status(dict(
                    speed=f"{speed:.1f}",
                    battery=str(battery),
                    dir=dirs[d],
                    temp=f"{temp:.1f}",
                ))
                time.sleep(1)
        self._demo_thread = threading.Thread(target=loop, name="demo-loop", daemon=True)
        self._demo_thread.start()

    # -------- Envío de comandos --------
    def _cmd_start(self):
        if self.is_observer:
            self._append_log("Rol OBSERVER: comandos deshabilitados.")
            return
        if self.demo_var.get():
            self.cmd_status_var.set("OK (demo)")
            self._append_log("CMD (demo): START → OK")
            return
        if not self.client or not self.client.is_connected():
            self._append_log("No conectado")
            return
        self.client.send("SPEED UP")
        self._append_log("CMD: SPEED UP")

    def _send_wire(self, wire: str):
        if self.is_observer:
            self._append_log("Rol OBSERVER: comandos deshabilitados.")
            return
        if self.demo_var.get():
            self.cmd_status_var.set("OK (demo)")
            self._append_log(f"CMD (demo): {wire} → OK")
            return
        if not self.client or not self.client.is_connected():
            self._append_log("No conectado")
            return
        self.client.send(wire)
        self._append_log(f"CMD: {wire}")

    # -------- Helpers --------
    def _set_controls_enabled(self, enabled: bool):
        state = (tk.NORMAL if enabled else tk.DISABLED)
        for w in [self.btn_start, self.btn_left, self.btn_right, self.btn_battery]:
            w.config(state=state)

    def _update_status(self, kv: Dict[str, str]):
        self.speed_var.set(kv.get("speed", "—"))
        self.battery_var.set(kv.get("battery", "—"))
        self.dir_var.set(kv.get("dir", "—"))
        self.temp_var.set(kv.get("temp", "—"))

    def _append_log(self, s: str):
        self.log.insert(tk.END, s + "\n")
        self.log.see(tk.END)

    def _on_close(self):
        try:
            self._demo_stop.set()
            if self.client:
                self.client.close()
        finally:
            self.destroy()

if __name__ == "__main__":
    App().mainloop()
