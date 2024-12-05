import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time
import socket
import sys
import threading
import json

# RFID 리더 초기화
reader = SimpleMFRC522()

# GPIO 모드 설정
GPIO.setmode(GPIO.BCM)

# PIR 센서 핀 번호
PIR_PIN1 = 22  # 첫 번째 센서
PIR_PIN2 = 27  # 두 번째 센서

# 핀 설정
GPIO.setup(PIR_PIN1, GPIO.IN)
GPIO.setup(PIR_PIN2, GPIO.IN)

def format_id(id):
    """RFID ID를 12자리 문자열로 변환"""
    return str(id).zfill(12)  # 빈자리는 0으로 채움

def connect_to_server(server_ip, server_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((server_ip, server_port))
        print(f"Server Connection Success: - {server_ip}:{server_port}")
        return sock
    except Exception as e:
        print(f"Server Connection Failed..: {e}")
        return None

def send_json(sock, id, status, direction=None):
    """JSON 메시지를 서버로 전송"""
    payload = {
        "id": id,
        "status": status,
        "direction": direction
    }
    message = json.dumps(payload)
    sock.send(message.encode())
    print(f"Sent to server: {message}")

def monitor_motion(sock, id):
    """모션 센서 동작 감지"""
    start_time = time.time()
    sensor1_triggered = False
    sensor2_triggered = False

    while time.time() - start_time < 10:  # 10초 동안 감지
        if GPIO.input(PIR_PIN1) and not sensor1_triggered:
            print("모션센서 1: 움직임 감지")
            sensor1_triggered = True

        if GPIO.input(PIR_PIN2) and not sensor2_triggered:
            print("모션센서 2: 움직임 감지")
            sensor2_triggered = True

        if sensor1_triggered and sensor2_triggered:
            direction = "entry" if GPIO.input(PIR_PIN2) else "exit"
            print(f"결과: {direction}")
            send_json(sock, id, "success", direction)
            return

        time.sleep(0.1)  # 빠른 루프 방지

    # 10초 후 결과 처리
    if not sensor1_triggered and not sensor2_triggered:
        print("결과: 실패 (두 센서 모두 감지되지 않음)")
        send_json(sock, id, "failure")
    elif sensor1_triggered != sensor2_triggered:
        print("결과: 실패 (한 센서만 감지됨)")
        send_json(sock, id, "failure")
    else:
        print("결과: 실패 (예외 상황)")
        send_json(sock, id, "failure")

def receive_messages(sock):
    """서버로부터 메시지를 수신하는 함수 (별도 스레드로 실행)"""
    try:
        while True:
            data = sock.recv(10000)
            if data:
                response = data.decode()
                print(f"Message from Server: {response}")
                if len(response) == 13:
                    rfid_id = response[:12]
                    action = response[-1]
                    if action == "1":
                        print("모션 센서 활성화")
                        monitor_motion(sock, rfid_id)
                    else:
                        print("모션 센서 비활성화")
            else:
                print("Server closed the connection.")
                break
    except Exception as e:
        print(f"Error while receiving messages: {e}")

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <서버IP> <포트>")
        sys.exit(1)

    server_ip = sys.argv[1]
    server_port = int(sys.argv[2])

    # 서버 연결
    sock = connect_to_server(server_ip, server_port)
    if not sock:
        sys.exit(1)

    # 메시지 수신 스레드 시작
    receive_thread = threading.Thread(target=receive_messages, args=(sock,))
    receive_thread.daemon = True  # 메인 프로그램 종료 시 함께 종료
    receive_thread.start()

    print("Please Touch RFID card...")

    try:
        while True:
            # RFID 카드 읽기
            id, text = reader.read()
            formatted_id = format_id(id)  # ID를 12자리로 포맷
            print(f"Card ID: {formatted_id}")
            print("-" * 30)

            try:
                # 포맷된 ID를 서버로 전송
                sock.send(formatted_id.encode())
                print(f"Success (ID transmit to Server): {formatted_id}")

            except Exception as e:
                print(f"Failed.. (ID transmit to Server): {e}")
                # 연결 재시도
                sock = connect_to_server(server_ip, server_port)
                if not sock:
                    break

            time.sleep(2)  # 2초 대기

    except KeyboardInterrupt:
        print("\nProgram End")
    finally:
        GPIO.cleanup()
        if sock:
            sock.close()

if __name__ == "__main__":
    main()
