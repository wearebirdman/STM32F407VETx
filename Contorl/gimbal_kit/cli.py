"""命令行入口:阶段 A 调试用,MCP 接入前先跑通业务逻辑。

用法:
  python -m gimbal_kit.cli --mock ping
  python -m gimbal_kit.cli --mock move 90 60
  python -m gimbal_kit.cli --mock move 200 60       # 越界,应返回 ERR
  python -m gimbal_kit.cli --mock --jitter move 45 80  # 验证重试
  python -m gimbal_kit.cli snap                       # 拍一张照片
"""
import argparse
import json
import sys

from gimbal_kit import camera, GimbalController
from gimbal_kit.protocol import Response


def main(argv=None) -> int:
    p = argparse.ArgumentParser(prog="gimbal_kit")
    p.add_argument("--mock", action="store_true", help="使用协议仿真器(无硬件)")
    p.add_argument("--jitter", action="store_true", help="仿真器注入 15%% 丢包")
    p.add_argument("--port", default="auto")
    p.add_argument("cmd", choices=["ping", "pos", "limits", "move", "snap"])
    p.add_argument("args", nargs="*")
    a = p.parse_args(argv)

    if a.cmd == "snap":
        out = a.args[0] if a.args else "snapshot.jpg"
        print(camera.capture(out))
        return 0

    dev = GimbalController.connect(port=a.port, mock=a.mock, jitter=a.jitter)
    if a.cmd == "ping":
        r = dev.ping()
    elif a.cmd == "pos":
        r = dev.status()
    elif a.cmd == "limits":
        r = Response("OK", [v for pair in dev.limits for v in pair])
    else:  # move
        try:
            r = dev.move([int(x) for x in a.args])
        except ValueError:
            print("move 需要整数角度列表", file=sys.stderr)
            return 2
    dev.close()
    print(json.dumps(
        {"kind": r.kind, "values": r.values, "message": r.message},
        ensure_ascii=False))
    return 0 if r.ok else 1


if __name__ == "__main__":
    sys.exit(main())
