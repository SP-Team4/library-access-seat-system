import RPi.GPIO as GPIO
import time

# GPIO 모드 설정
GPIO.setmode(GPIO.BCM)

# PIR 센서 핀 번호
PIR_PIN1 = 22  # 첫 번째 센서
PIR_PIN2 = 27  # 두 번째 센서

# 핀 설정
GPIO.setup(PIR_PIN1, GPIO.IN)
GPIO.setup(PIR_PIN2, GPIO.IN)

def monitor_motion():
    start_time = time.time()
    sensor1_triggered = False
    sensor2_triggered = False

    while time.time() - start_time < 10:  # 10초 동안 감지
        # 센서 1 감지
        if GPIO.input(PIR_PIN1) and not sensor1_triggered:
            print("모션센서 1: 움직임 감지")
            sensor1_triggered = True

        # 센서 2 감지
        if GPIO.input(PIR_PIN2) and not sensor2_triggered:
            print("모션센서 2: 움직임 감지")
            sensor2_triggered = True

        # 조건 만족 시 결과 출력
        if sensor1_triggered and sensor2_triggered:
            if sensor1_triggered and GPIO.input(PIR_PIN2):
                print("결과: 입장 방향")
            elif sensor2_triggered and GPIO.input(PIR_PIN1):
                print("결과: 퇴장 방향")
            break

        time.sleep(0.1)  # 0.1초 대기 (너무 빠른 루프 방지)

    # 10초 후 결과 처리
    if not sensor1_triggered and not sensor2_triggered:
        print("결과: 실패 (두 센서 모두 감지되지 않음)")
    elif (sensor1_triggered and not sensor2_triggered) or (sensor2_triggered and not sensor1_triggered):
        print("결과: 실패 (한 센서만 감지됨)")
    else:
        print("결과: 성공")

try:
    print("PIR 센서 동작 중...")
    while True:
        monitor_motion()
        print("재시작: 센서 동작을 기다리는 중...")
        time.sleep(1)  # 1초 대기 후 다시 감지 시작

except KeyboardInterrupt:
    print("\n프로그램 종료")

finally:
    GPIO.cleanup()
