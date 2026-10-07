"""gimbal-kit MCP Server(适配 mcp 2.x / MCPServer):把云台/摄像头能力暴露为智能体工具。

挂载(Trae 设置 -> MCP 添加):
  command: python
  args:    [d:\\.wm\\control\\mcp_server.py]
  env:     GIMBAL_MCP_MOCK=1       (无硬件时用仿真器)
           GIMBAL_MCP_PORT=COM5    (接真串口时指定,默认 auto)

设计:本文件只做"门面",业务逻辑全部在 gimbal_kit 包内,
     MCP 工具只是把函数结果包成智能体能理解的返回值。
"""
import base64
import os

from mcp import types
from mcp.server.mcpserver import MCPServer

from gimbal_kit import camera, GimbalController

mcp = MCPServer("gimbal-kit")
_dev: GimbalController | None = None


def _device() -> GimbalController:
    """懒加载设备连接,避免 server 启动时就占用串口。"""
    global _dev
    if _dev is None:
        _dev = GimbalController.connect(
            port=os.environ.get("GIMBAL_MCP_PORT", "auto"),
            mock=os.environ.get("GIMBAL_MCP_MOCK") == "1",
        )
    return _dev


@mcp.tool()
def camera_snapshot() -> list:
    """拍摄一张摄像头照片并以 JPEG 返回,用于视觉观察环境。"""
    path = camera.capture("mcp_snapshot.jpg")
    with open(path, "rb") as f:
        return [types.ImageContent(
            data=base64.b64encode(f.read()).decode(),
            mimeType="image/jpeg",
        )]


@mcp.tool()
def gimbal_ping() -> str:
    """测试与云台 MCU 的串口连通性。"""
    r = _device().ping()
    return f"{r.kind} {r.message}".strip()


@mcp.tool()
def gimbal_limits() -> str:
    """读取各轴安全角度范围(度),返回 'min max ...'。"""
    return " ".join(f"{lo} {hi}" for lo, hi in _device().limits)


@mcp.tool()
def gimbal_status() -> str:
    """查询当前各轴角度(度)。"""
    r = _device().status()
    return f"{r.kind} {' '.join(map(str, r.values))}"


@mcp.tool()
def gimbal_move(angles: list) -> str:
    """设置各轴目标角度(度,整数列表)。越界将被 MCU 拒绝并返回错误。"""
    r = _device().move(angles)
    return f"{r.kind} {' '.join(map(str, r.values))} {r.message}".strip()


if __name__ == "__main__":
    mcp.run()
