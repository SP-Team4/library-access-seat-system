import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time
import socket
import sys
import threading
import RPi_I2C_driver 

SEAT_GPIO_BUTTON_IN = [4, 5, 21, 25]
SEAT_GPIO_BUTTON_OUT = [17, 6, 20, 24]
SEAT_GPIO_LED_OUT = [27, 13, 16, 23]

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

def printLCD(string1, string2, time):
    mylcd = RPi_I2C_driver.lcd() 
    mylcd.lcd_display_string(string1, 1) 
    mylcd.lcd_display_string(string2, 2) 
    time.sleep(time) 
    mylcd.lcd_clear() 
    time.sleep(1) 
    mylcd.backlight(0)

def button_input():
    while True:
        btn1 = GPIO.input(SEAT_GPIO_BUTTON_IN[0])
        btn2 = GPIO.input(SEAT_GPIO_BUTTON_IN[1])
        btn3 = GPIO.input(SEAT_GPIO_BUTTON_IN[2])
        btn4 = GPIO.input(SEAT_GPIO_BUTTON_IN[3])

        if btn1 == 1: return 1
        elif btn2 == 1: return 2
        elif btn3 == 1: return 3
        elif btn4 == 1: return 4
        
def receive_messages(sock):
    """
    서버로부터 메시지를 수신하는 함수
    """
    try:
        while True:
            data = sock.recv(10000)
            if data:
                response = data.decode()
                print(f"Message from Server: {response}")
                return response
            
            else:
                print("Server closed the connection.")
                return -1
    except Exception as e:
        print(f"Error while receiving messages: {e}")
        return -1

def rfid_reading(sock):
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
                #sock = connect_to_server(server_ip, server_port)
                #if not sock:
                #    break
            
            time.sleep(2)  # 2초 대기
            
    except KeyboardInterrupt:
        print("\nProgram End")
    finally:
        GPIO.cleanup()
        if sock:
            sock.close()

def button_LED_control(sock):
    while True:
        response = receive_messages(sock)
        if response == -1:
            continue

        if len(response) == 13:
            #학생 입력 정보인 경우
            rfid_id = response[:12]
            action = response[12]
            if action == "0":
                print("Student %s already reserved a seat", rfid_id)
                printLCD("Student "+rfid_id, "Warning: Already reserved", 10)
                continue
        
        elif len(response) == 4:
            # 서버로부터 좌석 정보 받기
            seat_list = response
            
            print(seat_list)
            print("Please choose a seat")
            printLCD("Please choose a seat")
            #좌석정보 출력 : 0000으로 옴

            # 버튼 값 입력받고 서버에 전송
            # [0-11] rfid, [12] seat_num
            # button_input()
            button_msg = rfid_id + '3'
            sock.send(button_msg.encode())
                        

            # 서버로부터 받은 좌석 배열에서 가능한 값인지 확인

        elif len(response) == 2:
            # 좌석 제어 정보인 경우
            # action = response[12]
            seat_number = response[1]
            action = response[2]
            if action == 0:
                print("Reserve Cancelation: student "+rfid_id+" seat "+seat_number)
                GPIO.output(SEAT_GPIO_LED_OUT[seat_number],False)

                #해당 좌석번호 LED값 끄기
            else:
                printLCD("Successfully Reserved", "student: "+rfid_id+" seat: "+seat_number, 5)
                GPIO.output(SEAT_GPIO_LED_OUT[seat_number],True)

            # elif action == 2:
            #     #이선좌라고 lcd에 출력

def main():
    GPIO.setmode(GPIO.BCM)
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <서버IP> <포트>")
        sys.exit(1)

    for i in SEAT_GPIO_BUTTON_IN:
        GPIO.setup(i,GPIO.IN)
    for i in SEAT_GPIO_BUTTON_OUT:
        GPIO.setup(i,GPIO.OUT)
    for i in SEAT_GPIO_LED_OUT:
        GPIO.setup(i,GPIO.OUT)



    server_ip = sys.argv[1]
    server_port = int(sys.argv[2])
    
    # 서버 연결
    sock = connect_to_server(server_ip, server_port)
    if not sock:
        sys.exit(1)

    # 스레드 시작
    rfid_thread = threading.Thread(target=rfid_reading, args=(sock,))
    button_LED_thread = threading.Thread(target=button_LED_control, args=(sock,))

    rfid_thread.daemon = True 
    button_LED_thread.daemon = True 

    rfid_thread.start()
    button_LED_thread.start()


if __name__ == "__main__":
    main()