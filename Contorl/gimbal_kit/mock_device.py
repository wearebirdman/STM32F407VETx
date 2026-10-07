"""云台设备仿真器:与 STM32 固件约定同一套协议行为。

用途:
1. 无硬件时全链路验证 PC 端逻辑。
2. 作为 MCU 固件的"协议参考实现"——固件行为应与本文件一致。

行为规格(固件必须实现):
- 纯请求-响应模型:不主动播报,LIM 只响应 PC 的 LIM 命令。
- M 越界 -> ERR 1 且位置不变;合法 -> OK 并更新位置。
- 未知命令/参数错误 -> ERR 2。
- 可选:jitter 模式模拟 15% 丢包,验证 PC 端重试逻辑。
"""
import random
import time

from .protocol import ERR_BAD_FRAME, ERR_OUT_OF_RANGE


class MockDevice:
    def __init__(self, limits=None, jitter: bool = False):
        # 水平/俯仰安全范围;机械臂阶段此处变为各关节限位表
        self.limits = limits if limits is not None else [(20, 160), (40, 140)]
        self.pos = [pair[0] for pair in self.limits]  # 上电归位到各轴下限
        self.jitter = jitter

    def boot_lines(self) -> str:
        nums = " ".join(str(v) for pair in self.limits for v in pair)
        return f"LIM {nums}\n"

    def handle(self, line: str) -> str:
        # 模拟丢包:返回空串,PC 端应触发超时重试
        if self.jitter and random.random() < 0.15:
            return ""
        toks = line.strip().split()
        if not toks:
            return f"ERR {ERR_BAD_FRAME} EMPTY"
        cmd = toks[0].upper()
        if cmd == "PING":
            return "OK PING"
        if cmd == "S":
            return "POS " + " ".join(str(p) for p in self.pos)
        if cmd == "LIM":
            return self.boot_lines().strip()
        if cmd == "M":
            try:
                want = [int(t) for t in toks[1:]]
            except ValueError:
                return f"ERR {ERR_BAD_FRAME} BAD_ANGLE"
            if len(want) != len(self.limits):
                return f"ERR {ERR_BAD_FRAME} AXIS_COUNT"
            for a, (lo, hi) in zip(want, self.limits):
                if not lo <= a <= hi:
                    return f"ERR {ERR_OUT_OF_RANGE} AXIS_LIMIT"  # 越界:拒绝且位置不变
            time.sleep(0.05)  # 模拟执行耗时
            self.pos = want
            return "OK " + " ".join(str(p) for p in self.pos)
        return f"ERR {ERR_BAD_FRAME} UNKNOWN_CMD"


class MockTransport:
    """进程内仿真传输,接口与 SerialTransport 一致。"""

    def __init__(self, jitter: bool = False):
        self._dev = MockDevice(jitter=jitter)
        self._inbox: list = []  # 纯请求-响应:无主动播报

    def send_line(self, line: str) -> None:
        self._inbox.append(self._dev.handle(line))

    def recv_line(self, timeout: float) -> str:
        # 仿真器立即返回空串表示丢包(超时),与真实串口行为对齐
        return self._inbox.pop(0) if self._inbox else ""

    def close(self) -> None:
        pass
