"""实时运动检测驱动舵机:有运动(有人) -> x轴动,静止(无人) -> y轴动。

用帧差法检测画面变化,无需额外模型依赖。
检测到状态变化立即切换运动轴。按 Ctrl+C 退出。
"""
import time
import cv2
import numpy as np
from gimbal_kit.transport import SerialTransport

PORT = "COM4"
CHECK_INTERVAL = 0.3      # 每隔多少秒检测一次运动
MOVE_STEP = 1             # 每次命令的角度变化量(度)
MOVE_DELAY = 0.02         # 每步间隔(秒)
SWEEP_LO = 50             # 摆动下限
SWEEP_HI = 130            # 摆动上限
FIXED_ANGLE = 90          # 非摆动轴的固定角度
MOTION_THRESH = 25        # 帧差灰度阈值
MOTION_PIXEL_RATIO = 0.02 # 变化像素占比超过此值视为有人

tp = SerialTransport(PORT)
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)


def has_motion(prev, curr):
    """比较两帧,返回是否有显著运动。"""
    if prev is None:
        return False
    diff = cv2.absdiff(prev, curr)
    gray = cv2.cvtColor(diff, cv2.COLOR_BGR2GRAY)
    _, mask = cv2.threshold(gray, MOTION_THRESH, 255, cv2.THRESH_BINARY)
    ratio = np.count_nonzero(mask) / mask.size
    return ratio > MOTION_PIXEL_RATIO


def set_angles(a1, a2):
    tp.send_line(f"M {a1} {a2}")
    try:
        tp.recv_line(0.5)
    except TimeoutError:
        pass


def main():
    x, y = FIXED_ANGLE, FIXED_ANGLE
    set_angles(x, y)

    x_dir = 1
    y_dir = 1
    last_check = 0
    person = False
    prev_frame = None

    print("开始实时检测(Ctrl+C 退出)...")
    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                continue

            now = time.time()
            if now - last_check >= CHECK_INTERVAL:
                person = has_motion(prev_frame, frame)
                prev_frame = frame.copy()
                last_check = now
                state = "有人(运动)" if person else "无人(静止)"
                axis = "x轴" if person else "y轴"
                print(f"[{time.strftime('%H:%M:%S')}] {state} -> {axis}动")

            if person:
                x += x_dir * MOVE_STEP
                if x >= SWEEP_HI:
                    x = SWEEP_HI
                    x_dir = -1
                elif x <= SWEEP_LO:
                    x = SWEEP_LO
                    x_dir = 1
                y = FIXED_ANGLE
            else:
                y += y_dir * MOVE_STEP
                if y >= SWEEP_HI:
                    y = SWEEP_HI
                    y_dir = -1
                elif y <= SWEEP_LO:
                    y = SWEEP_LO
                    y_dir = 1
                x = FIXED_ANGLE

            set_angles(x, y)
            time.sleep(MOVE_DELAY)

    except KeyboardInterrupt:
        print("\n退出,回到 90 90")
        set_angles(FIXED_ANGLE, FIXED_ANGLE)
    finally:
        cap.release()
        tp.close()


if __name__ == "__main__":
    main()
