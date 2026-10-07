"""设备门面:组合传输 + 协议,提供带重试和限幅缓存的高层操作。

连接时主动读一次 LIM 缓存限位,后续 move 在 PC 侧预校验越界(快速失败),
但 MCU 仍是最终安全权威——预校验只省一次往返。
"""
from . import protocol
from .mock_device import MockTransport
from .protocol import Response
from .transport import SerialTransport

TIMEOUT = 0.5
RETRY = 1


class GimbalController:
    def __init__(self, transport):
        self._tp = transport
        self.limits: list = []

    @classmethod
    def connect(cls, port: str = "auto", mock: bool = False, jitter: bool = False):
        if mock:
            self = cls(MockTransport(jitter=jitter))
        else:
            if port == "auto":
                import serial.tools.list_ports
                cands = list(serial.tools.list_ports.comports())
                if not cands:
                    raise RuntimeError("未发现串口设备,请指定 --port")
                port = cands[0].device
            self = cls(SerialTransport(port))
        self.limits = self.get_limits()  # 连接即缓存限位
        return self

    def _request(self, cmd: str) -> Response:
        for _ in range(RETRY + 1):
            self._tp.send_line(cmd)
            try:
                line = self._tp.recv_line(TIMEOUT)
            except TimeoutError:
                continue  # 超时重试
            resp = protocol.parse_response(line)
            if resp.kind != "ERR" or line.strip():
                return resp
        return Response("ERR", [], "重试后仍无回包")

    def ping(self) -> Response:
        return self._request("PING")

    def status(self) -> Response:
        return self._request("S")

    def get_limits(self) -> list:
        resp = self._request("LIM")
        if not resp.ok or len(resp.values) < 2 or len(resp.values) % 2:
            raise RuntimeError(f"限位读取失败: {resp.message}")
        return list(zip(resp.values[0::2], resp.values[1::2]))

    def move(self, angles: list) -> Response:
        if len(angles) != len(self.limits):
            return Response("ERR", [], f"需要 {len(self.limits)} 个轴角度")
        # PC 侧预校验:越界直接本地拒绝
        for a, (lo, hi) in zip(angles, self.limits):
            if not lo <= a <= hi:
                return Response("ERR", [], f"本地预检拒绝: {a} 超出 [{lo},{hi}]")
        return self._request(protocol.build_move(angles))

    def close(self):
        self._tp.close()
