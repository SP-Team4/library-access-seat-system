import socket
import sys

def main():
    if len(sys.argv) != 2:
        print("Usage: python reservation_client.py <port>")
        sys.exit(1)

    server_ip = '192.168.46.2'  # 서버 IP 주소
    server_port = int(sys.argv[1])  # 서버 포트 번호

    # 서버와 연결
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((server_ip, server_port))
    print(f"[클라이언트] 서버 ({server_ip}:{server_port})와 연결 성공")

    # RFID 전송
    rfid = "027812690245"
    client_socket.sendall(rfid.encode())  # RFID 전송
    print(f"[클라이언트] RFID 전송: {rfid}")

    # 가능한 좌석 정보 수신
    available_seats = client_socket.recv(6).decode('utf-8')
    print(f"[클라이언트] 가능한 좌석 정보 수신: {available_seats}")

    # 선택된 좌석 번호는 1로 가정
    seat_id = '1'  # 선택된 좌석 번호는 문자로 전송
    print(f"[클라이언트] 선택된 좌석 번호 전송: {seat_id}")

    # 선택된 좌석 번호 서버에 전송
    client_socket.sendall(seat_id.encode())  

    # 서버의 응답 받기
    response = client_socket.recv(10).decode('utf-8')
    print(f"[클라이언트] 서버 응답: {response}")

    # 연결 종료
    client_socket.close()

if __name__ == '__main__':
    main()
