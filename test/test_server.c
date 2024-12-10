
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define MAX_CLIENTS 5
#define DETECTION_THRESHOLD 3  // 감지 횟수 임계값 

void error_handling(char *message) {
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}

int main(int argc, char *argv[]) {
   int serv_sock;
   int clnt_sock;
   struct sockaddr_in serv_addr;
   struct sockaddr_in clnt_addr;
   socklen_t clnt_addr_size;
   int detection_count = 0;

   if (argc != 2) {
       printf("사용법: %s <포트>\n", argv[0]);
       exit(1);
   }

   // 소켓 생성
   serv_sock = socket(PF_INET, SOCK_STREAM, 0);
   if (serv_sock == -1)
       error_handling("소켓 생성 오류");

   // 서버 주소 설정
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(atoi(argv[1]));

   // 바인딩
   if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
       error_handling("바인드 오류");

   // 리스닝
   if (listen(serv_sock, MAX_CLIENTS) == -1)
       error_handling("리스닝 오류");

   printf("서버가 포트 %s에서 실행 중입니다...\n", argv[1]);

   while(1) {
       // 클라이언트 연결 수락
       clnt_addr_size = sizeof(clnt_addr);
       clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
       if (clnt_sock == -1)
           error_handling("연결 수락 오류");
       
       printf("클라이언트가 연결되었습니다\n");
       detection_count = 0;

       // 연결 직후 센서 활성화 메시지 전송 전 대기
       sleep(1);
       
       // 초기 센서 활성화 메시지 전송
       char msg[3] = "41";
       write(clnt_sock, msg, 2);
       printf("4번 센서 활성화 명령 전송: 41\n");

       while(1) {
           // 클라이언트로부터 메시지 수신
           memset(msg, 0, sizeof(msg));
           int str_len = read(clnt_sock, msg, 2);
           
           if (str_len <= 0) {
               printf("클라이언트 연결이 종료되었습니다\n");
               break;
           }

           // 수신된 메시지 출력
           printf("클라이언트로부터 수신: %s\n", msg);

           // 감지된 경우에만 카운트 증가 (메시지가 "41"인 경우)
           if (strcmp(msg, "41") == 0) {
               detection_count++;
               
               // 감지 횟수가 임계값에 도달하면 센서 중지 명령만 전송
               if (detection_count >= DETECTION_THRESHOLD) {
                   msg[0] = '4';
                   msg[1] = '0';
                   write(clnt_sock, msg, 2);
                   printf("4번 센서 비활성화 명령 전송: 40\n");
                   detection_count = 0;  // 카운트 리셋
               }
           }
       }

       close(clnt_sock);
   }

   close(serv_sock);
   return 0;
}