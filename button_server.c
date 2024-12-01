#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>

// 쿼리 결과를 처리하는 콜백 함수
static int callback(void *response_buffer, int argc, char **argv, char **azColName) {
    char *buffer = (char*)response_buffer;
    char row[256];
    
    snprintf(row, sizeof(row), "시간: %s, 거리: %s cm, 상태: %s\n", 
             argv[0] ? argv[0] : "NULL",
             argv[1] ? argv[1] : "NULL",
             argv[2] ? argv[2] : "NULL");
    
    strcat(buffer, row);
    return 0;
}

int main() {
    // 소켓 생성
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // 서버 주소 설정
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8080);
    
    // 바인딩
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        printf("Bind Error\n");
        return 1;
    }
    
    // 리스닝
    if (listen(serv_sock, 5) == -1) {
        printf("Listen Error\n");
        return 1;
    }

    printf("DB 쿼리 서버가 시작되었습니다... (포트: 8080)\n");
    
    // SQLite 데이터베이스 연결
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("/home/pi/Desktop/project/sensor_data.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    while (1) {
        // 클라이언트 연결 수락
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        
        // 클라이언트로부터 요청 수신
        char buffer[100];
        read(clnt_sock, buffer, sizeof(buffer)-1);
        
        // 최근 데이터 조회 쿼리
        char response[4096] = {0};
        const char *sql = "SELECT timestamp, distance, is_within_range FROM sensor_readings ORDER BY timestamp DESC LIMIT 5;";
        
        rc = sqlite3_exec(db, sql, callback, response, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            snprintf(response, sizeof(response), "데이터 조회 실패\n");
        }
        
        // 결과 전송
        write(clnt_sock, response, strlen(response));
        printf("클라이언트에게 데이터를 전송했습니다.\n");
        
        close(clnt_sock);
    }

    sqlite3_close(db);
    close(serv_sock);
    return 0;
}