"""测试摄像头并保存一张照片到 snapshots/。"""
import cv2
import os

snap_dir = os.path.join(os.path.dirname(__file__), "snapshots")
os.makedirs(snap_dir, exist_ok=True)

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

ok, frame = cap.read()
cap.release()

if ok:
    path = os.path.join(snap_dir, "test.jpg")
    cv2.imwrite(path, frame)
    print(f"拍照成功: {path} ({frame.shape[1]}x{frame.shape[0]})")
else:
    print("拍照失败:摄像头不可用")
