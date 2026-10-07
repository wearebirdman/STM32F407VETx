"""单次会话内完成完整闭环:拍照 -> 移动 -> 查位置。

用于演示 AI 控制云台的端到端链路(mock 设备)。
同一会话内多次调用工具,设备状态保持一致。
"""
import asyncio
import base64
import os
import sys

from mcp.client.stdio import StdioServerParameters, stdio_client
from mcp import ClientSession


def _env():
    e = dict(os.environ)
    e["GIMBAL_MCP_MOCK"] = "1"
    e["PYTHONIOENCODING"] = "utf-8"
    return e


async def main():
    target = [int(x) for x in sys.argv[1:3]] if len(sys.argv) >= 3 else [90, 80]
    params = StdioServerParameters(command="python", args=["mcp_server.py"], env=_env())

    async with stdio_client(params) as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()

            # 1. 拍照存盘(供 AI 观察)
            r = await session.call_tool("camera_snapshot", {})
            img = next(c for c in r.content if getattr(c, "type", None) == "image")
            with open("demo_view.jpg", "wb") as f:
                f.write(base64.b64decode(img.data))
            print("[1] 已拍照 -> demo_view.jpg")

            # 2. 移动前位置
            r = await session.call_tool("gimbal_status", {})
            before = next(c.text for c in r.content if hasattr(c, "text"))
            print(f"[2] 移动前: {before}")

            # 3. 执行移动(AI 决策的目标角度)
            r = await session.call_tool("gimbal_move", {"angles": target})
            move_resp = next(c.text for c in r.content if hasattr(c, "text"))
            print(f"[3] MOVE {target} -> {move_resp}")

            # 4. 移动后位置(同一会话,状态保持)
            r = await session.call_tool("gimbal_status", {})
            after = next(c.text for c in r.content if hasattr(c, "text"))
            print(f"[4] 移动后: {after}")

            # 5. 越界拒绝测试
            r = await session.call_tool("gimbal_move", {"angles": [200, 60]})
            oor = next(c.text for c in r.content if hasattr(c, "text"))
            print(f"[5] 越界测试 MOVE [200,60] -> {oor}")

            r = await session.call_tool("gimbal_status", {})
            final = next(c.text for c in r.content if hasattr(c, "text"))
            print(f"[6] 越界后位置(应不变): {final}")


if __name__ == "__main__":
    asyncio.run(main())
