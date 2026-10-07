"""测试 Qwen3-VL 视觉模型能否正确理解图片。"""
import base64
from openai import OpenAI

API_KEY = "sk-rbpbwjobnndtwbghnmkkcdwwhhnskhoeaqmvmztwglsnhioo"
BASE_URL = "https://api.siliconflow.cn/v1"
MODEL = "zai-org/GLM-4.5V"

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

# 用一张本地测试图,没有的话跳过图片只测纯文本
import os
test_img = os.path.join(os.path.dirname(__file__), "snapshots", "test.jpg")

messages = [{"role": "user", "content": [
    {"type": "text", "text": "这张图里有人吗?只回答 有 或 没有,不要其他字。"}
]}]

if os.path.exists(test_img):
    with open(test_img, "rb") as f:
        b64 = base64.b64encode(f.read()).decode()
    messages[0]["content"].append({
        "type": "image_url",
        "image_url": {"url": f"data:image/jpeg;base64,{b64}"}
    })
    print(f"使用测试图片: {test_img}")
else:
    print(f"无测试图片({test_img} 不存在),仅测纯文本能力")

try:
    resp = client.chat.completions.create(
        model=MODEL,
        messages=messages,
        max_tokens=10,
    )
    print(f"模型回复: {resp.choices[0].message.content.strip()}")
    print("API 接入成功!")
except Exception as e:
    print(f"失败: {e}")
