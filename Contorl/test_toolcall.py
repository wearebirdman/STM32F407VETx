"""最小测试:纯文本(不带图片) + 工具调用,看模型是否正常。"""
import json
from openai import OpenAI

API_KEY = "sk-rbpbwjobnndtwbghnmkkcdwwhhnskhoeaqmvmztwglsnhioo"
BASE_URL = "https://api.siliconflow.cn/v1"
MODEL = "zai-org/GLM-4.5V"

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

tools = [{
    "type": "function",
    "function": {
        "name": "gimbal_status",
        "description": "查询云台状态",
        "parameters": {"type": "object", "properties": {}},
    },
}]

messages = [
    {"role": "user", "content": "查询云台状态"},
]

resp = client.chat.completions.create(
    model=MODEL,
    messages=messages,
    tools=tools,
    tool_choice="auto",
)
msg = resp.choices[0].message
print(f"content: {repr(msg.content)}")
print(f"tool_calls: {msg.tool_calls}")
if msg.tool_calls:
    for tc in msg.tool_calls:
        print(f"  call: {tc.function.name}({tc.function.arguments})")
