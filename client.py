import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time
import socket
import sys
import threading
import RPi_I2C_driver 


SEAT_GPIO_LED_OUT = [4, 5, 21, 12]
SEAT_GPIO_BUTTON_IN = [17, 6, 20, 24]
#SEAT_GPIO_BUTTON_OUT = [27, 13, 16, 23]

# RFID 리더 초기화
reader = SimpleMFRC522()


GPIO.cleanup()
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)

for i in SEAT_GPIO_BUTTON_IN:
    GPIO.setup(i, GPIO.IN, pull_up_down=GPIO.PUD_UP)
for i in range(4):
    GPIO.setup(SEAT_GPIO_LED_OUT[i], GPIO.OUT)
    GPIO.output(SEAT_GPIO_LED_OUT[i], False)

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

def printLCD(string1, string2, second):
    mylcd = RPi_I2C_driver.lcd() 
    mylcd.lcd_display_string(string1, 1) 
    mylcd.lcd_display_string(string2, 2) 
    time.sleep(second) 
    mylcd.lcd_clear() 
    time.sleep(1) 
    mylcd.backlight(0)
           

def rfid_reading(sock):
    try:
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
    except Exception as e:
        print(f"Error while receiving messages: {e}")                       

def receive_messages(sock):
    """
    서버로부터 메시지를 수신하는 함수
    """
    try:
        while True:
            print("waiting for message")
            data = sock.recv(1024)
            if data:
                print(f"Raw Data Length: {len(data)}")
                print(f"Raw Data: {data}")

                response = data.decode()
                print(f"Message from Server: {response}")


                rfid_id = 0

                #response = receive_messages(sock)
                print(f"response length: {len(response)}")
                if response == -1:
                    continue

                if len(response) == 13:
                    #학생 입력 정보인 경우
                    rfid_id = response[:12]
                    action = response[12]
                    if action == "0":
                        print(f"Student %s already reserved a seat", rfid_id)
                        printLCD("Student "+rfid_id, "Warning: Already reserved", 10)
                        continue
                
                elif response[0] == 's':
                    if (len(response) == 1):
                        printLCD("All seat reserved", " ", 5)
                        continue;
                        
                    response = str(response)
                    print(response)
                    print(f"Please choose a seat")
                    printLCD("Choose a seat", "1, 3, 4", 3)
                    
                    for i in range(4):
                        print("for")
                        GPIO.output(SEAT_GPIO_LED_OUT[i], False)
                        if (str(i+1) in response):
                            print(i)
                            GPIO.output(SEAT_GPIO_LED_OUT[i], True)
                        else:                           
                            GPIO.output(SEAT_GPIO_LED_OUT[i], False)
                            print("no")
                        
                    print(f"asdfff")    
                    while True:
                        btn1 = GPIO.input(SEAT_GPIO_BUTTON_IN[0])
                        btn2 = GPIO.input(SEAT_GPIO_BUTTON_IN[1])
                        btn3 = GPIO.input(SEAT_GPIO_BUTTON_IN[2])
                        btn4 = GPIO.input(SEAT_GPIO_BUTTON_IN[3])
                        print(f"choose a button")
                        #print(str(rfid_id))
                        
                        GPIO.output(SEAT_GPIO_LED_OUT[0], True)
                        GPIO.output(SEAT_GPIO_LED_OUT[2], True)
                        if ('1' in response) and btn1 == 0 :
                            print(f"1 selected")
                            GPIO.output(SEAT_GPIO_LED_OUT[0], True)
                            button_msg ='1'
                            break;
                        elif ('2' in response) and  btn2 == 0 :
                            GPIO.output(SEAT_GPIO_LED_OUT[1], False)
                            print(f"2 selected")
                            button_msg ='2'
                            break;
                        elif ('3' in response) and btn3 == 0 : 
                            GPIO.output(SEAT_GPIO_LED_OUT[2], False)
                            print(f"3 selected")
                            button_msg ='3'
                            break;
                        elif ('4' in response) and btn4 == 0 :
                            GPIO.output(SEAT_GPIO_LED_OUT[3], False)
                            print(f"4 selected")
                            button_msg ='4'
                            break;
                        else:
                            time.sleep(1)  
                    GPIO.output(SEAT_GPIO_LED_OUT[2], False)
                    printLCD("seat 3", "selected", 5)
                    print(button_msg)
                            
                    sock.send(button_msg.encode())


                elif len(response) == 3:
                    seat_number = response[1]
                    action = response[2]
                    if action == 0:
                        printLCD("Reserve Cancelation", rfid_id, 5)
                        #GPIO.output(SEAT_GPIO_LED_OUT[seat_number],False)
                    else:
                        printLCD("Successfully Reserved", "student: "+rfid_id+" seat: "+seat_number, 5)
                        GPIO.output(SEAT_GPIO_LED_OUT[seat_number],False)
                
            else:
                print("Server closed the connection.")
                #continue
    except Exception as e:
        print(f"Error while receiving messages: {e}")


'''
def button_LED_control(sock):
    rfid_id = 0
    while True:
        response = receive_messages(sock)
        print(f"response length: {len(response)}")
        if response == -1:
            continue

        if len(response) == 12:
            #학생 입력 정보인 경우
            rfid_id = response[:12]
            action = response[12]
            if action == "0":
                print("Student %s already reserved a seat", rfid_id)
                printLCD("Student "+rfid_id, "Warning: Already reserved", 10)
                continue
        
        elif response[0] == 's':
            # 서버로부터 좌석 정보 받기
            seat_list = response
            if (len(seat_list) == 1):
                printLCD("All seat reserved", "", 5)
                continue;
                
            print(seat_list)
            print("Please choose a seat")
            printLCD("Please choose a seat")
            
            while True:                 
                btn1 = GPIO.input(SEAT_GPIO_BUTTON_IN[0])
                btn2 = GPIO.input(SEAT_GPIO_BUTTON_IN[1])
                btn3 = GPIO.input(SEAT_GPIO_BUTTON_IN[2])
                btn4 = GPIO.input(SEAT_GPIO_BUTTON_IN[3])                

                if btn1 == 1 & response[0] == 0:
                    button_msg = rfid_id + '1'
                    break;
                elif btn2 == 1 & response[1] == 0:
                    button_msg = rfid_id + '2'
                    break;
                elif btn3 == 1 & response[2] == 0: 
                    button_msg = rfid_id + '3'
                    break;
                elif btn4 == 1 & response[3] == 0:
                    button_msg = rfid_id + '4'
                    break;
                else:
                    #error handling
                    print("unavailable")
                    printLCD("Unavailable seat", "Choose again", 5)
                    continue;
                    
            sock.send(button_msg.encode())


        elif len(response) == 2:
            seat_number = response[1]
            action = response[2]
            if action == 0:
                printLCD("Reserve Cancelation", rfid_id, 5)
                GPIO.output(SEAT_GPIO_LED_OUT[seat_number],False)
            else:
                printLCD("Successfully Reserved", "student: "+rfid_id+" seat: "+seat_number, 5)
                GPIO.output(SEAT_GPIO_LED_OUT[seat_number],True)
'''

        

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <서버IP> <포트>")
        sys.exit(1)

    """
    GPIO.setup(SEAT_GPIO_BUTTON_IN, GPIO.IN)
    GPIO.setup(SEAT_GPIO_BUTTON_OUT, GPIO.OUT)
    GPIO.setup(SEAT_GPIO_LED_OUT, GPIO.OUT)

    """
    for i in SEAT_GPIO_BUTTON_IN:
        GPIO.setup(i, GPIO.IN, GPIO.PUD_DOWN)
    #for i in SEAT_GPIO_BUTTON_OUT:
    #    GPIO.setup(i,GPIO.OUT)
    for i in SEAT_GPIO_LED_OUT:
        GPIO.setup(i,GPIO.OUT)


    server_ip = sys.argv[1]
    server_port = int(sys.argv[2])
    
    # 서버 연결
    sock = connect_to_server(server_ip, server_port)
    if not sock:
        sys.exit(1)

    # 스레드 시작receive_messages
    rfid_thread = threading.Thread(target=rfid_reading, args=(sock,))
    receive_messages_thread = threading.Thread(target=receive_messages, args=(sock,))

    rfid_thread.daemon = True 
    receive_messages_thread.daemon = True 

    rfid_thread.start()
    receive_messages_thread.start()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Program Terminated")
        GPIO.cleanup()
        if sock:
            sock.close()
        sys.exit(0)

if __name__ == "__main__":
    main()
