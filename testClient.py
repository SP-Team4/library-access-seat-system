import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time
import socket
import sys
import threading
import RPi_I2C_driver 

SEAT_GPIO_LED_OUT = [4, 5, 21, 12]
SEAT_GPIO_BUTTON_IN = [17, 6, 20, 24]

#--------------------------#
# TOTAL 4 THREADS
# 1. RFID 
# 2. LED + button
# 3. LCD
# 4. 서버와 통신(main)
#--------------------------#


#--------------------------#
# SHARED DATA DEFINITION
# 1. RFID ID (RFID - 통신)
# 2. 좌석 정보 (LED - button - 통신; 무조건 s로 시작함)
# 3. is_ready -> 데이터 준비 완료 플래그
# 3. condition -> wait중인 스레드 깨우는 조건
#--------------------------#

rfid = 0
available_seat = None
is_ready = False
msg = None
string1 = None
string2 = None
second = 0 
LCD_ready = False

lock = threading.Lock()
condition = threading.Condition()


#--------------------------#
# INITIALIZATION
#--------------------------#

# RFID 리더 초기화
reader = SimpleMFRC522()

# GPIO 초기화
GPIO.cleanup()
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)

mylcd = RPi_I2C_driver.lcd() 
mylcd.lcd_clear() 
mylcd.backlight(0)


# LED초기화
for i in range(4):
    # button의 입력을 GPIO와 연결
    # LED의 출력을 GPIO와 연결한 뒤 모두 끄기
    # GPIO.PUD_UP => 음.. PUD DOWN으로 안되는지 한번만 더 테스트해보자
    GPIO.setup(SEAT_GPIO_BUTTON_IN[i], GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(SEAT_GPIO_LED_OUT[i], GPIO.OUT)
    GPIO.output(SEAT_GPIO_LED_OUT[i], False)

def format_id(id):
    """RFID ID를 12자리 문자열로 변환"""
    return str(id).zfill(12)  # 빈자리는 0으로 채움

#--------------------------#
# FUNCTION DEFINITION
#--------------------------#

# 서버와 소켓 연결
def connect_to_server(server_ip, server_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((server_ip, server_port))
        print(f"Server Connection Success: - {server_ip}:{server_port}")
        return sock
    except Exception as e:
        print(f"Server Connection Failed..: {e}")
        return None

def LED_control(available_seat):
    for i in available_seat:
        GPIO.output(SEAT_GPIO_LED_OUT[i-1],True)

def button_control(available_seat):
    # 호출되면 버튼입력받고 return
    button_msg = '0'
    while True:
        btn1 = GPIO.input(SEAT_GPIO_BUTTON_IN[0])
        btn2 = GPIO.input(SEAT_GPIO_BUTTON_IN[1])
        btn3 = GPIO.input(SEAT_GPIO_BUTTON_IN[2])
        btn4 = GPIO.input(SEAT_GPIO_BUTTON_IN[3])
        print(f"choose a button")

        if ('1' in available_seat) and btn1 == 0 :
            print(f"1 selected")
            printLCD("seat 1", "selected", 2)
            #GPIO.output(SEAT_GPIO_LED_OUT[0], True)
            button_msg ='1'
            break
        elif ('2' in available_seat) and  btn2 == 0 :
            #GPIO.output(SEAT_GPIO_LED_OUT[1], False)
            print(f"2 selected")
            printLCD("seat 1", "selected", 2)
            button_msg ='2'
            break
        elif ('3' in available_seat) and btn3 == 0 : 
            #GPIO.output(SEAT_GPIO_LED_OUT[2], False)
            print(f"3 selected")
            printLCD("seat 1", "selected", 2)
            button_msg ='3'
            break
        elif ('4' in available_seat) and btn4 == 0 :
            #GPIO.output(SEAT_GPIO_LED_OUT[3], False)
            print(f"4 selected")
            printLCD("seat 1", "selected", 2)
            button_msg ='4'
            break
        else:
            time.sleep(1)  
    print(f"%s", button_msg)
    return button_msg


#--------------------------#
# TREAD FUNCTIONS
#--------------------------#

# 서버로부터 메시지 수신


def rfid_reading(sock):
    while True:              
        try:
            print("Please Touch RFID card...")

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
                sock.close            
        #else:
            #  print("Server closed the connection.")

def printLCD(string1, string2, second):
    mylcd = RPi_I2C_driver.lcd() 
    mylcd.lcd_display_string(string1, 1) 
    mylcd.lcd_display_string(string2, 2) 
    time.sleep(second) 
    mylcd.lcd_clear() 
    mylcd.backlight(0)

def LCDController():
    global string1, string2, second, LCD_ready  
    while True:
        mylcd = RPi_I2C_driver.lcd() 
        mylcd.lcd_display_string(string1, 1) 
        mylcd.lcd_display_string(string2, 2) 
        time.sleep(second) 
        mylcd.lcd_clear() 
        mylcd.backlight(0)
        with lock:
            string1 = None
            string2 = None
            second = 0
            LCD_ready = False

def controller(sock):
    global msg, is_ready
    # msg 이용가능한지 busy waiting => 가능하면 while문 밖으로 빠져나가 다음 절차 실행
    while True:
        while (not is_ready):
            print(f"not ready for button input")
            time.sleep(1)
        
        # RFID찍고나서 예약 가능한지 1 or 0 수신
        # button으로 사용자 입력받기
        # 소켓으로 button number 전송(length: 1)
        try:
            if msg == 1:    
                seat = button_control()
                sock.send(seat.encode())
                with lock:
                    is_ready = False
            else:
                printLCD("Already reserved", "a seat", 3)
        except:
            print(f"failed to reserve a seat. Please try again from the first")
            continue
        
        # 소켓에서 전송받은 예약 정보(0, 1)에 따라 LED 결과값 출력
        while (not is_ready):
            print(f"not ready for control LED")
            time.sleep(1)

        try:
            if msg == 1:
                LED_control(available_seat)
                printLCD("Successfully reserved", "seat: "+seat, 5)
                with lock:
                    is_ready = False
            else:
                printLCD("Unable to reserve", "seat: "+seat, 5)
                continue
        except:
            print("failed to process a reservation. Please try again from the first")
        time.sleep(2)
    
    
def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <IP> <port>")
        sys.exit(1)

    server_ip = sys.argv[1]
    server_port = int(sys.argv[2])
    
    # 서버 연결
    sock = connect_to_server(server_ip, server_port)
    if not sock:
        print(f"unable to connect socket")
        sys.exit(1)

    # 스레드 시작receive_messages
    rfid_thread = threading.Thread(target=rfid_reading, args=(sock,))
    controller_thread = threading.Thread(target=controller, args=(sock,))

    rfid_thread.daemon = True 
    controller_thread.daemon = True 

    rfid_thread.start()
    controller_thread.start()


# def receive_messages(sock):
    global msg, is_ready
    try:
        while True:
            print("waiting for message")
            data = sock.recv(1024)
            if data:
                # 디버깅 코드
                print(f"Raw Data Length: {len(data)}")
                print(f"Raw Data: {data}")

                response = data.decode()
                print(f"Message from Server: {response}")
                if len(response) > 1:
                    with lock:
                        available_seat = response
                else:
                    with lock:
                        msg = response
                        is_ready = True
                time.sleep(1)

    except Exception as e:
        print(f"Error while receiving messages: {e}")

    finally:
        GPIO.cleanup()
        if sock:
            sock.close()
        sys.exit(0)

    # try:
    #     while True:
    #         time.sleep(1)
    # except KeyboardInterrupt:
    #     print("Program Terminated")
    #     GPIO.cleanup()
    #     if sock:
    #         sock.close()
    #     sys.exit(0)

if __name__ == "__main__":
    main()
