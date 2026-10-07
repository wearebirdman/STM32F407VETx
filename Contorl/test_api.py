"""硅基流动 API 连通性测试。

用法:
    python test_api.py

会测试几个常用的视觉模型是否可用,并打印模型回复。
"""
import os
from openai import OpenAI

API_KEY = "sk-rbpbwjobnndtwbghnmkkcdwwhhnskhoeaqmvmztwglsnhioo"
BASE_URL = "https://api.siliconflow.cn/v1"

# 硅基流动上常用的视觉模型(带 VL/V 的才能看图)
VISION_MODELS = [
    "Qwen/Qwen2.5-VL-72B-Instruct",
    "Qwen/Qwen2.5-VL-7B-Instruct",
    "deepseek-ai/DeepSeek-VL2",
    "zhipuai/glm-4v",
]


def main():
    client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

    print("=" * 50)
    print("硅基流动 API 连通性测试")
    print("=" * 50)

    # 1. 先测一个纯文本模型(最快),确认 Key 有效
    print("\n[1] 测试 Key 有效性(纯文本)...")
    try:
        resp = client.chat.completions.create(
            model="Qwen/Qwen2.5-7B-Instruct",
            messages=[{"role": "user", "content": "回复两个字:收到"}],
            max_tokens=10,
        )
        print(f"    OK: {resp.choices[0].message.content.strip()}")
    except Exception as e:
        print(f"    失败: {e}")
        return

    # 2. 逐个测试视觉模型是否可用
    print("\n[2] 测试视觉模型可用性...")
    available = []
    for model in VISION_MODELS:
        try:
            resp = client.chat.completions.create(
                model=model,
                messages=[{"role": "user", "content": "回复:ok"}],
                max_tokens=5,
            )
            print(f"    [可用] {model}")
            available.append(model)
        except Exception as e:
            print(f"    [不可用] {model}: {str(e)[:60]}")

    print("\n" + "=" * 50)
    if available:
        print(f"可用的视觉模型: {available}")
        print("建议:测试效果时优先用 7B(快、便宜),正式用 72B(效果好)")
    else:
        print("没有可用的视觉模型,请检查 Key 权限或模型名")


if __name__ == "__main__":
    main()
