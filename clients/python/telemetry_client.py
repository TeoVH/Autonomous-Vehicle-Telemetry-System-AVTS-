import socket
import threading
from typing import Callable, Dict, Optional
from status_parser import parse

Listener = Dict[str, Callable]

class TelemetryClient:
    """
    Cliente TCP simple con un hilo receptor.
    Callbacks esperadas en 'listener':
      - on_connected()
      - on_login_result(ok: bool, line: str)
      - on_status(kv: Dict[str, str])
      - on_server_message(line: str)
      - on_error(msg: str, ex: Exception | None)
      - on_closed()
    """

    def __init__(self, host: str, port: int, role: str, user: str, password: str, listener: Listener):
        self.host = host
        self.port = port
        self.role = role
        self.user = user
        self.password = password
        self.listener = listener

        self._sock: Optional[socket.socket] = None
        self._send_lock = threading.Lock()
        self._recv_thread: Optional[threading.Thread] = None
        self._stop_flag = threading.Event()

    def is_connected(self) -> bool:
        return self._sock is not None

    def connect_async(self) -> None:
        t = threading.Thread(target=self._run, name="net-recv", daemon=True)
        t.start()
        self._recv_thread = t

    def _run(self) -> None:
        try:
            self._sock = socket.create_connection((self.host, self.port), timeout=15.0)
            self._sock_file = self._sock.makefile("r", encoding="utf-8", newline="\n")
            self._call('on_connected')

            # LOGIN
            login_line = f"LOGIN {self.role} {self.user} {self.password}\n"
            self.send(login_line, raw=True)

            # Primera línea
            first = self._sock_file.readline()
            ok = bool(first) and first.upper().startswith("OK")
            is_telemetry = bool(first) and (first.startswith("STATUS ") or first.startswith("DATA "))
            if not is_telemetry:
                self._call('on_login_result', ok, first.strip() if first else "(sin respuesta)")
            else:
                self._call('on_status', parse(first))

            # Loop receptor
            while not self._stop_flag.is_set():
                line = self._sock_file.readline()
                if not line:
                    break
                line = line.rstrip("\r\n")
                if line.startswith("STATUS ") or line.startswith("DATA "):
                    self._call('on_status', parse(line))
                else:
                    self._call('on_server_message', line)

        except Exception as ex:
            self._call('on_error', "Fallo de conexión/IO", ex)
        finally:
            self.close()

    def send(self, text: str, raw: bool = False) -> None:
        if not self._sock:
            return
        data = text if raw else (text + "\n")
        with self._send_lock:
            try:
                self._sock.sendall(data.encode("utf-8"))
            except Exception as ex:
                self._call('on_error', "Fallo enviando datos", ex)

    def close(self) -> None:
        self._stop_flag.set()
        try:
            if hasattr(self, "_sock_file") and self._sock_file:
                self._sock_file.close()
        except Exception:
            pass
        try:
            if self._sock:
                self._sock.close()
        except Exception:
            pass
        finally:
            self._sock = None
            self._call('on_closed')

    # Helper para llamar callbacks si existen
    def _call(self, name: str, *args):
        fn = self.listener.get(name)
        if callable(fn):
            try:
                fn(*args)
            except Exception:
                pass
