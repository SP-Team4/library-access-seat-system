import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time
import socket
import sys

# RFID 리더 초기화
reader = SimpleMFRC522()
# GPIO.setwarnings(False)

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