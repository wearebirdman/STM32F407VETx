"""gimbal_kit: AI 智能体控制云台的 PC 手脚层(本地零 AI,识别交由智能体完成)。"""
from .controller import GimbalController
from . import camera, protocol

__all__ = ["GimbalController", "camera", "protocol"]
