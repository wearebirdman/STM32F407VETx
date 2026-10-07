"""传输层抽象:控制信道与物理层解耦。

当前实现:SerialTransport(USB 串口)。
预留扩展:TcpTransport(H7 阶段以太网)实现同一 Transport 接口,
controller 与协议层无需改动即可切换物理层。
"""
import time
from abc import ABC, abstractmethod


class Transport(ABC):
    @abstractmethod
    def send_line(self, line: str) -> None:
        """发送一行命令(自动追加换行)。"""

    @abstractmethod
    def recv_line(self, timeout: float) -> str:
        """接收一行回包;超时抛 TimeoutError。"""

    @abstractmethod
    def close(self) -> None: ...


class SerialTransport(Transport):
    """USB 串口,115200-8N1,行帧以 \\n 分隔。"""

    def __init__(self, port: str, baudrate: int = 115200):
        import serial
        self._ser = serial.Serial(port, baudrate, timeout=0.2)
        self._buf = b""

    def _read_until_nl(self, deadline: float) -> bytes:
        while time.monotonic() < deadline:
            if b"\n" in self._buf:
                line, self._buf = self._buf.split(b"\n", 1)
                return line
            chunk = self._ser.read(64)
            if chunk:
                self._buf += chunk
        raise TimeoutError("串口回包超时")

    def send_line(self, line: str) -> None:
        self._ser.write((line.strip() + "\n").encode("ascii"))

    def recv_line(self, timeout: float) -> str:
        return self._read_until_nl(time.monotonic() + timeout).decode("ascii", "replace")

    def close(self) -> None:
        self._ser.close()
