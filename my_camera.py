import threading
from picamera2 import Picamera2
import cv2
import os
import numpy as np
import time

def find_image(database_img):
    def threaded_task():
        # 카메라 촬영
        picam2 = Picamera2()
        config = picam2.create_still_configuration()
        picam2.configure(config)
        picam2.start()
        print("Camera initialization complete.")
        
        time.sleep(5)  # 5초 대기
        print("Capturing image after 5-second delay.")

        image = picam2.capture_array()
        if image is None:
            print("Error: No image captured")
            picam2.close()
            return
        
        print(f"Captured image shape: {image.shape}")  # 이미지가 제대로 캡처되었는지 확인
        cv2.imwrite("captured_image.jpg", image)
        picam2.close()

        # 유사 이미지 검출
        folder_path = "./images"
        input_image = cv2.imread("./captured_image.jpg")
        input_gray = cv2.cvtColor(input_image, cv2.COLOR_BGR2GRAY)
        orb = cv2.ORB_create()
        keypoints1, descriptors1 = orb.detectAndCompute(input_gray, None)
        
        best_match = None
        highest_score = 0

        for filename in os.listdir(folder_path):
            if filename.endswith(".jpg") or filename.endswith(".png"):
                file_path = os.path.join(folder_path, filename)
                compare_image = cv2.imread(file_path)
                compare_gray = cv2.cvtColor(compare_image, cv2.COLOR_BGR2GRAY)
                keypoints2, descriptors2 = orb.detectAndCompute(compare_gray, None)
                bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
                matches = bf.match(descriptors1, descriptors2)
                matches = sorted(matches, key=lambda x: x.distance)
                score = len(matches)
                print(f"Image {filename} has {score} matches.")
                if score > highest_score:
                    highest_score = score
                    best_match = filename

        if database_img == best_match:
            print("Success!")
            print(f"The most similar image is: {best_match} with {highest_score} matches.")
        else:
            print("No similar images found.")
    
    # 쓰레드 생성 및 실행
    thread = threading.Thread(target=threaded_task)
    thread.start()
    thread.join()  # 쓰레드 작업이 끝날 때까지 대기

if __name__ == "__main__":
    find_image("brown.jpg")
