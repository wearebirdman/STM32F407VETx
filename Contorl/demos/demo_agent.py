"""端到端演示辅助脚本:调用 MCP server 的工具(相机+云台),把图片存盘。

用法:
  python demo_agent.py snap                  # 拍照存盘,返回文件路径
  python demo_agent.py move 90 60            # 控制云台
  python demo_agent.py pos                   # 查询角度
  python demo_agent.py limits                # 查询限位
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


async def _call(name: str, args: dict):
    params = StdioServerParameters(command="python", args=["mcp_server.py"], env=_env())
    async with stdio_client(params) as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()
            return await session.call_tool(name, args)


async def main():
    cmd = sys.argv[1]
    if cmd == "snap":
        r = await _call("camera_snapshot", {})
        img = next(c for c in r.content if getattr(c, "type", None) == "image")
        path = sys.argv[2] if len(sys.argv) > 2 else "demo_view.jpg"
        with open(path, "wb") as f:
            f.write(base64.b64decode(img.data))
        print(path)
    elif cmd == "move":
        angles = [int(x) for x in sys.argv[2:]]
        r = await _call("gimbal_move", {"angles": angles})
        print(next(c.text for c in r.content if hasattr(c, "text")))
    elif cmd == "pos":
        r = await _call("gimbal_status", {})
        print(next(c.text for c in r.content if hasattr(c, "text")))
    elif cmd == "limits":
        r = await _call("gimbal_limits", {})
        print(next(c.text for c in r.content if hasattr(c, "text")))
    elif cmd == "ping":
        r = await _call("gimbal_ping", {})
        print(next(c.text for c in r.content if hasattr(c, "text")))


if __name__ == "__main__":
    asyncio.run(main())
