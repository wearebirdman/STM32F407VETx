"""AI 控制器:拍照 -> 视觉模型决策 -> function calling -> 控制 MCU。

闭环流程:
    用户指令 + 摄像头照片 -> 大模型(带工具) -> 工具调用(gimbal_move 等)
        -> 执行串口命令 -> 结果回传模型 -> 模型生成自然语言回复

用法:
    python ai_controller.py "让云台转到 90 60"
    python ai_controller.py --interactive          # 交互模式
"""
import os
import sys
import base64
import cv2
from openai import OpenAI

# 把 pc_ai 目录加入 sys.path,方便 import gimbal_kit
sys.path.insert(0, os.path.dirname(__file__))
from gimbal_kit.transport import SerialTransport
from gimbal_kit.mock_device import MockDevice


class MockTransport:
    """用 MockDevice 模拟串口,接口与 SerialTransport 一致。"""
    def __init__(self):
        self.dev = MockDevice()
    def send_line(self, line):
        self._resp = self.dev.handle(line)
    def recv_line(self, timeout):
        return self._resp
    def close(self):
        pass

# ===================== 配置 =====================
API_KEY = "sk-rbpbwjobnndtwbghnmkkcdwwhhnskhoeaqmvmztwglsnhioo"
BASE_URL = "https://api.siliconflow.cn/v1"
MODEL = "zai-org/GLM-4.5V"                   # 视觉+function calling 均支持
SERIAL_PORT = "COM4"
CAMERA_INDEX = 0
SNAPSHOT_DIR = os.path.join(os.path.dirname(__file__), "snapshots")
os.makedirs(SNAPSHOT_DIR, exist_ok=True)

# ===================== 工具定义(给模型看的 JSON Schema) =====================
TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "gimbal_move",
            "description": "控制云台各轴转到指定角度。角度范围:x轴20~160, y轴40~140。",
            "parameters": {
                "type": "object",
                "properties": {
                    "angles": {
                        "type": "array",
                        "items": {"type": "integer"},
                        "description": "各轴目标角度,如 [90, 60] 表示 x=90, y=60",
                    },
                },
                "required": ["angles"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "gimbal_status",
            "description": "查询云台当前各轴角度和限位。",
            "parameters": {"type": "object", "properties": {}},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "camera_snapshot",
            "description": "拍摄一张当前摄像头照片,返回照片描述。",
            "parameters": {"type": "object", "properties": {}},
        },
    },
]

# ===================== 硬件接口 =====================
_tp = None


def get_transport():
    global _tp
    if _tp is None:
        if "--mock" in sys.argv:
            _tp = MockTransport()
        else:
            _tp = SerialTransport(SERIAL_PORT)
    return _tp


def send_cmd(cmd: str, timeout: float = 1.0) -> str:
    """发送命令并等待回包。"""
    tp = get_transport()
    tp.send_line(cmd)
    try:
        return tp.recv_line(timeout).strip()
    except TimeoutError:
        return "TIMEOUT"


# ===================== 工具实现 =====================
def tool_gimbal_move(angles):
    if not isinstance(angles, list) or len(angles) != 2:
        return "ERR: angles 必须是 2 个整数"
    a1, a2 = int(angles[0]), int(angles[1])
    resp = send_cmd(f"M {a1} {a2}")
    return resp


def tool_gimbal_status():
    pos = send_cmd("S")
    lim = send_cmd("LIM")
    return f"{pos}; {lim}"


def tool_camera_snapshot():
    """拍照并保存,返回照片路径(模型通过多模态消息直接看图,此处仅记录)。"""
    cap = cv2.VideoCapture(CAMERA_INDEX)
    ok, frame = cap.read()
    cap.release()
    if not ok:
        return "ERR: 摄像头不可用"
    path = os.path.join(SNAPSHOT_DIR, "snapshot.jpg")
    cv2.imwrite(path, frame)
    return f"已拍摄照片: {path}"


TOOL_HANDLERS = {
    "gimbal_move": tool_gimbal_move,
    "gimbal_status": tool_gimbal_status,
    "camera_snapshot": tool_camera_snapshot,
}


# ===================== AI 主循环 =====================
def run_once(user_text: str) -> str:
    """执行一轮:拍照 -> 模型决策 -> 执行工具 -> 返回模型回复。"""
    client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

    # 1. 拍照
    cap = cv2.VideoCapture(CAMERA_INDEX)
    ok, frame = cap.read()
    cap.release()

    messages = [
        {"role": "system", "content": (
            "你是一个云台控制助手。你可以通过工具控制2轴云台(x轴20~160度, y轴40~140度)。"
            "用户会给你指令或描述场景,你需要决定是否调用工具。"
            "回复要简洁,控制指令要果断。"
        )},
        {"role": "user", "content": []},
    ]

    if ok:
        path = os.path.join(SNAPSHOT_DIR, "snapshot.jpg")
        cv2.imwrite(path, frame)
        with open(path, "rb") as f:
            b64 = base64.b64encode(f.read()).decode()
        messages[1]["content"].append({
            "type": "image_url",
            "image_url": {"url": f"data:image/jpeg;base64,{b64}"},
        })
    messages[1]["content"].append({"type": "text", "text": user_text})

    # 2. 模型决策(可能多轮工具调用)
    for _ in range(5):   # 最多 5 轮工具调用,防止死循环
        resp = client.chat.completions.create(
            model=MODEL,
            messages=messages,
            tools=TOOLS,
            tool_choice="auto",
        )
        msg = resp.choices[0].message

        # 没有工具调用 -> 结束
        if not msg.tool_calls:
            return msg.content or "(无回复)"

        # 执行所有工具调用
        messages.append(msg)   # 把模型的 tool_calls 消息加入历史
        for tc in msg.tool_calls:
            name = tc.function.name
            args = _parse_args(tc.function.arguments)
            print(f"  [工具调用] {name}({args})")

            handler = TOOL_HANDLERS.get(name)
            if handler:
                result = handler(**args)
            else:
                result = f"未知工具: {name}"

            print(f"  [执行结果] {result}")
            messages.append({
                "role": "tool",
                "tool_call_id": tc.id,
                "content": str(result),
            })

    return "已达到最大工具调用轮次"


def _parse_args(s: str) -> dict:
    import json
    try:
        return json.loads(s)
    except Exception:
        return {}


def main():
    args = [a for a in sys.argv[1:] if a != "--mock"]
    if args and args[0] != "--interactive":
        # 单次模式:python ai_controller.py "你的指令"
        user_text = " ".join(args)
        print(f"你: {user_text}")
        reply = run_once(user_text)
        print(f"AI: {reply}")
    else:
        # 交互模式
        print("AI 云台控制器(输入 quit 退出)")
        while True:
            try:
                user_text = input("\n你: ").strip()
            except (EOFError, KeyboardInterrupt):
                break
            if user_text.lower() in ("quit", "exit", "q"):
                break
            if not user_text:
                continue
            reply = run_once(user_text)
            print(f"AI: {reply}")

    if _tp:
        _tp.close()


if __name__ == "__main__":
    main()
