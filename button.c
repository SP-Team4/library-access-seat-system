#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiringPi.h>

#define BUTTON_PIN 29  // GPIO 21

void receiveAndPrintData(int sock) {
    char buffer[4096];
    int str_len = read(sock, buffer, sizeof(buffer)-1);
    if (str_len <= 0) {
        printf("데이터 수신 실패\n");
        return;
    }
    buffer[str_len] = '\0';
    printf("\n최근 측정 데이터:\n%s\n", buffer);
}

int main() {
    // WiringPi 초기화
    if (wiringPiSetup() == -1) {
        printf("WiringPi 초기화 실패\n");
        return 1;
    }

    // 버튼 핀 설정
    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_UP);

    printf("버튼을 누르면 최근 측정 데이터를 조회합니다...\n");
    printf("프로그램을 종료하려면 Ctrl+C를 누르세요.\n");

    while (1) {
        if (digitalRead(BUTTON_PIN) == LOW) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                printf("소켓 생성 실패\n");
                delay(200);
                continue;
            }

            struct sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr("192.168.44.4"); // 서버 IP
            serv_addr.sin_port = htons(8080);

            if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
                printf("서버 연결 실패\n");
                close(sock);
                delay(200);
                continue;
            }

            // 데이터 요청 메시지 전송
            const char* request = "GET_DATA";
            write(sock, request, strlen(request));

            // 데이터 수신 및 출력
            receiveAndPrintData(sock);

            close(sock);
            delay(200);  // 디바운싱
        }
        delay(50);
    }

    return 0;
}