from __future__ import annotations

from threading import Thread
from typing import Callable
import os

import cv2
import numpy as np

from .config import FaceConfig


class FaceRecognitionService:
    def __init__(self, config: FaceConfig, on_user_identified: Callable[[int], None]):
        self._config = config
        self._on_user_identified = on_user_identified
        self._loading = False
        self._identify = False
        self._person_num = 1
        self._thread: Thread | None = None

    def start(self) -> None:
        if not self._config.enabled or self._thread is not None:
            return

        self._thread = Thread(target=self._run, daemon=True)
        self._thread.start()

    def _camera_source(self):
        return int(self._config.camera_source) if self._config.camera_source.isdigit() else self._config.camera_source

    def _run(self) -> None:
        cap = cv2.VideoCapture(self._camera_source())
        face_cascade = cv2.CascadeClassifier(str(self._config.cascade_path))
        recognizer = cv2.face.LBPHFaceRecognizer_create()

        try:
            recognizer.read(str(self._config.model_path))
            labels = recognizer.getLabels().flatten()
            self._person_num = int(max(labels)) + 1
            self._identify = True
        except Exception:
            self._identify = False
            print("目前暫無模型，請按下空白鍵開始建立模型")

        while True:
            if self._loading:
                self._train_model(cap, face_cascade, recognizer)
                self._loading = False

            if self._identify:
                if not self._identify_frame(cap, face_cascade, recognizer):
                    break
            else:
                ret, _ = cap.read()
                if ret != 1:
                    print("沒抓到畫面")
                    break
                self._read_training_key()

        cap.release()
        cv2.destroyAllWindows()

    def _train_model(self, cap, face_cascade, recognizer) -> None:
        print("開始辨識，請對準人臉")
        loading_faces = []
        ids = []

        for _ in range(1, 50):
            ret, frame = cap.read()
            if ret != 1:
                print("沒抓到畫面")
                break

            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            image_np = np.array(gray, "uint8")
            face_rects = face_cascade.detectMultiScale(gray)
            for (x, y, w, h) in face_rects[:1]:
                loading_faces.append(image_np[y : y + h, x : x + w])
                ids.append(int(self._person_num))

        print(f"最終訓練面容數: {len(loading_faces)}, ID總數: {ids}")
        if len(loading_faces) <= 40:
            print("失敗 人臉捕獲數:", len(loading_faces))
            return

        self._config.model_path.parent.mkdir(parents=True, exist_ok=True)
        if os.path.exists(self._config.model_path):
            try:
                recognizer.read(str(self._config.model_path))
                recognizer.update(loading_faces, np.array(ids))
            except Exception:
                os.remove(self._config.model_path)
                recognizer.train(loading_faces, np.array(ids))
        else:
            recognizer.train(loading_faces, np.array(ids))

        recognizer.save(str(self._config.model_path))
        print("模型建立/更新完成 您的編號為NO.", self._person_num)
        self._person_num += 1
        self._identify = True

    def _identify_frame(self, cap, face_cascade, recognizer) -> bool:
        ret, frame = cap.read()
        if ret != 1:
            print("沒抓到畫面")
            return False

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = face_cascade.detectMultiScale(gray)
        user_mapping = {
            "1": 3,
            "2": 2,
            "3": 3,
            "4": 4,
            "5": 5,
            "6": 6,
        }

        for (x, y, w, h) in faces:
            identity, confidence = recognizer.predict(gray[y : y + h, x : x + w])
            if confidence < 30:
                user_id = user_mapping.get(str(identity))
                if user_id is not None:
                    self._on_user_identified(user_id)

        self._read_training_key()
        return True

    def _read_training_key(self) -> None:
        key = cv2.waitKey(1) & 0xFF
        if key == ord(" "):
            self._loading = True
