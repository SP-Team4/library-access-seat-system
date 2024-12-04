#!/usr/bin/env python

import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time

reader = SimpleMFRC522()

print("카드를 대주세요...")

try:
   while True:
       id, text = reader.read()
       print(f"카드 ID: {id}")
       print("-" * 30)
       time.sleep(2)  # 2초 대기
       
except KeyboardInterrupt:
   print("\n프로그램 종료")
finally:
   GPIO.cleanup()