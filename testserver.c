
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#define VALUE_MAX 20

#define PY_SSIZE_T_CLEAN




/****** Socket ******/
int serv_sock = -1;
int clnt_sock_seat_reserv = -1;

struct sockaddr_in serv_addr, clnt_addr;
socklen_t clnt_addr_size;

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

/****** Thread ******/
pthread_t reservation_thread;

volatile int server_running = 1;  // 서버 종료 여부를 판단하는 플래그

void *reservation(void *data) {
    char msg[15];       //출입관리 서버와 통신 [0-11]rfid, [12]valid, [13]1:entry 0:exit, [14]NULL
    char seat_number[8];
    char rfid[13];
    int isValid = 0;  
    while(server_running){
        //RFID태그 들어옴
        if (read(clnt_sock_seat_reserv, &msg, sizeof(char)*12) == -1) 
            error_handling("[Seat Reservation] Unable to read\n");
        strncpy(rfid, msg, 12);
        printf("%s\n", rfid);

        int isValid = 0;
        if (isValid){
            //예약된 좌석이 있으면
            msg[12] = '0';
            write(clnt_sock_seat_reserv, msg, sizeof(char)*13);
            printf("Already reserved a seat \n");
            continue;
        }

        int available_seats = 123;
        if (available_seats == -1) {
            error_handling("[Seat Reservation] Failed to get available seats from Python\n");
        }
        char seat_str[6];  // 충분한 크기의 배열로 선언
        snprintf(seat_str, sizeof(seat_str), "%d", available_seats);
        write(clnt_sock_seat_reserv, seat_str, sizeof(char)*6);

        
        // 버튼 결과 받기
        memset(&msg, 0, sizeof(msg));  // 배열 초기화
        if (read(clnt_sock_seat_reserv, &msg, sizeof(char)*2) == -1)  // 1자리 숫자 + NULL 문자
            error_handling("[Seat Reservation] Unable to read seat id\n");

        printf("Received seat id: %s\n", msg);

        // DB -> RFID, seat으로 좌석 예약 진행
        printf("[C] Calling Python function reserve_seat with rfid: %s, seat_id: %d\n", rfid, msg);
        int result = 1;
        printf("[C] Python function result: %d\n", result);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_seat_reserv = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_seat_reserv == -1)
        error_handling("[Reservation] accept() error");
    printf("[Reservation] 클라이언트 연결 성공\n");

    pthread_create(&reservation_thread, NULL, reservation, NULL);

    pthread_join(reservation_thread, NULL);

    close(clnt_sock_seat_reserv);
    close(serv_sock);

    return 0;
}
