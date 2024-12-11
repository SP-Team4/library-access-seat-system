#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define VALUE_MAX 256


/****** Socket ******/
int serv_sock = -1;
int clnt_sock_seat_watch = -1;

struct sockaddr_in serv_addr, clnt_addr;
socklen_t clnt_addr_size;

/****** Error Handling ******/
void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

/****** Watching Thread Function ******/
void *watching(void *data) {
    int clnt_sock_seat_watch = *(int *)data;
    char msg[3];
    memset(&msg, 0, sizeof(msg));
    
    // 초기 센서 활성화
    msg[0] = '4';
    msg[1] = '1';
    write(clnt_sock_seat_watch, msg, sizeof(msg));
    printf("[Seat Watching] 센서 %c번 활성화 명령 전송\n", msg[0]);

    while (1) {
        if (read(clnt_sock_seat_watch, &msg, sizeof(msg)) == -1) 
            error_handling("[Seat Watching] Unable to read \n");
       
        int count = 0;
        //Test용
        int seat = msg[0] - '0';
        int detected = msg[1] - '0';
        printf("[Seat Watching] 수신된 메시지: %s (좌석 %d에서 %s)\n", 
               msg, seat, detected ? "물체 감지됨" : "물체 없음");

        if (detected == 1) { //감지된 경우 initialize
            //initialize count
            printf("[Seat Watching] initialize count\n"); 
            count = 0;

        }
        else { //감지되지 않은 경우 increase count하고 만약 3회 이상이면 죽인다.
            //increase count
            printf("[Seat Watching] increase count\n"); 

            //여기에 count 세는 db 함수 새로 만들어야할듯...
            //이거 수는 test용으로 쓰십쇼
            count++;
            
            if(count >= 3){
                //센서 비활성화 명령 전달
                snprintf(msg, sizeof(msg), "%d0", seat);
                write(clnt_sock_seat_watch, msg, sizeof(msg));
                printf("[Seat Watching] 좌석 %d 센서 비활성화 명령 전송\n", seat);
          }     
        }
    }
}

/****** Main Function ******/
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
    clnt_sock_seat_watch = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_seat_watch == -1)
        error_handling("[Reservation] accept() error");
    printf("[Reservation] 클라이언트 연결 성공\n");

    pthread_t watching_thread;
    pthread_create(&watching_thread, NULL, watching, (void *)&clnt_sock_seat_watch);
    pthread_join(watching_thread, NULL);

    close(clnt_sock_seat_watch);
    close(serv_sock);

    return 0;
}
