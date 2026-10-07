"""PC 摄像头采集:拍一帧存 JPEG。

不做任何识别,识别交由智能体(多模态模型直接读图)完成。
"""
import time


def capture(path: str = "snapshot.jpg", width: int = 640, height: int = 480,
            warmup: int = 5) -> str:
    """拍一帧存为 JPEG,返回文件路径。

    warmup:丢弃自动曝光/白平衡稳定前的帧,避免首张过暗。
    """
    import cv2
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        raise RuntimeError("无法打开摄像头(检查设备编号或系统隐私权限)")
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    for _ in range(warmup):
        cap.read()
        time.sleep(0.03)
    ok, frame = cap.read()
    cap.release()
    if not ok:
        raise RuntimeError("摄像头读帧失败")
    cv2.imwrite(path, frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
    return path
