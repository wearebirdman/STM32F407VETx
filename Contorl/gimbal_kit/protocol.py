"""串口协议:行式 ASCII,轴无关(为多轴机械臂预留)。

PC -> MCU:
  PING              连通性测试
  S                 查询当前各轴角度
  LIM               查询各轴安全限位
  M <a1> <a2> ...   设置各轴目标角度(度,整数)
MCU -> PC(纯请求-响应,不主动播报):
  OK [参数...]                 执行成功
  POS <a1> <a2> ...            当前各轴角度
  LIM <min1> <max1> <min2> ...  各轴安全限位
  ERR <code> <说明>            拒绝执行,code 见下方错误码

设计原则:
- 轴数不写死,命令携带变长数组,2 轴云台和 6 轴机械臂共用同一协议。
- 安全权威在 MCU;PC 侧预校验只是快速失败,越界命令 MCU 仍会拒绝。
"""
from dataclasses import dataclass

# 错误码:与 MCU 固件约定一致,新增时双方同步
ERR_OUT_OF_RANGE = 1   # 角度越界
ERR_BAD_FRAME = 2      # 帧格式错误/解析失败
ERR_BUSY = 3           # 设备忙
ERR_INTERNAL = 4       # 其他内部错误


@dataclass
class Response:
    kind: str            # OK / POS / LIM / ERR
    values: list         # 数值载荷(角度/限位等)
    message: str = ""    # 文本说明(供调试/展示)

    @property
    def ok(self) -> bool:
        return self.kind != "ERR"


def build_move(angles: list) -> str:
    """构造 M 命令。PC 侧做类型校验快速失败;安全权威仍在 MCU。"""
    if not angles:
        raise ValueError("至少需要一个轴角度")
    parts = []
    for a in angles:
        if not isinstance(a, int) or isinstance(a, bool):
            raise ValueError(f"角度必须是整数(度): {a!r}")
        parts.append(str(a))
    return "M " + " ".join(parts)


def build(cmd: str) -> str:
    """构造非数据命令(PING/S/LIM),统一大写。"""
    return cmd.strip().upper()


def parse_response(line: str) -> Response:
    """解析 MCU 回包;无法识别的行按协议错误返回,绝不静默吞掉。"""
    line = line.strip()
    if not line:
        return Response("ERR", [], "空回包/超时")
    head, _, rest = line.partition(" ")
    toks = rest.split()
    nums = []
    for t in toks:
        try:
            nums.append(int(t))
        except ValueError:
            pass
    if head == "OK":
        return Response("OK", nums, rest)
    if head == "POS":
        return Response("POS", nums, rest)
    if head == "LIM":
        return Response("LIM", nums, rest)
    if head == "ERR":
        code = nums[0] if nums else ERR_INTERNAL
        return Response("ERR", nums[1:], f"错误码{code}: {' '.join(toks[1:])}")
    return Response("ERR", [], f"无法解析回包: {line!r}")
