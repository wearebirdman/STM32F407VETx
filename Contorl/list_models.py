"""列出硅基流动上所有可用模型,筛选视觉模型。"""
from openai import OpenAI

API_KEY = "sk-rbpbwjobnndtwbghnmkkcdwwhhnskhoeaqmvmztwglsnhioo"
BASE_URL = "https://api.siliconflow.cn/v1"

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

models = client.models.list()

print("所有可用模型:")
vision_models = []
for m in models:
    mid = m.id
    print(f"  {mid}")
    # 视觉模型通常名字里带 vl 或 v
    if "vl" in mid.lower() or "-v" in mid.lower() or "vision" in mid.lower():
        vision_models.append(mid)

print("\n" + "=" * 50)
print("可能的视觉模型:")
for vm in vision_models:
    print(f"  {vm}")
