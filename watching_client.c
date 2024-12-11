#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    // 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.46.2"); // 서버 IP
    serv_addr.sin_port = htons(port);

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("서버로부터 메시지를 수신하고 처리 후 다시 전송합니다.\n");
    printf("------------------------\n");

    while (1) {
    
        char msg[100]; // 서버로부터 수신할 메시지 버퍼
        printf("센서 %d번 활성화(%d) 명령 수신\n",  msg[0] - '0', msg[1] - '0');
        int read_bytes = read(sock, msg, sizeof(msg) - 1);
        if (read_bytes <= 0) {
            printf("서버와의 연결이 종료되었습니다.\n");
            break;
        }

        msg[read_bytes] = '\0'; // null-terminate
        printf("[수신] 서버로부터 메시지: %s\n", msg);

        // 메시지 처리: 좌석 번호와 감지 여부를 추출
        int seat = msg[0] - '0';    // 첫 번째 문자는 좌석 번호
        int detected = msg[1] - '0'; // 두 번째 문자는 감지 여부

        if (detected == 0)
            continue;

//------------ test용
        // 사용자로부터 메시지 입력 받기
        char modified_msg[100];
        printf("수동으로 전송할 메시지 입력 :");
        fgets(modified_msg, sizeof(modified_msg), stdin); // 사용자 입력 받기

        // 입력받은 문자열에서 끝의 개행 문자 제거
        modified_msg[strcspn(modified_msg, "\n")] = '\0';

        int modified_seat = modified_msg[0] - '0';    // 첫 번째 문자는 좌석 번호
        int modified_detected = modified_msg[1] - '0'; // 두 번째 문자는 감지 여부


//----------------
        printf("[Seat Watching] 수신된 메시지: %s (좌석 %d에서 %s)\n", 
               modified_msg, modified_seat, modified_detected ? "물체 감지됨" : "물체 없음");

        snprintf(modified_msg, sizeof(modified_msg), "%d%d", modified_seat, modified_detected);
        write(sock, modified_msg, strlen(modified_msg));

        printf("[전송] 수정된 메시지: %s\n", modified_msg);
    }

    close(sock);
    return 0;
}
