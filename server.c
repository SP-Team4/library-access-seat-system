// 서버 코드 (DB 저장하는 라즈베리파이)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>

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

    printf("서버가 시작되었습니다...\n");
    
    // SQLite 데이터베이스 연결
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("/home/pi/Desktop/project/sensor_data.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // 클라이언트 연결 수락
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    
    char buffer[100];
    while (1) {
        // 데이터 수신
        int str_len = read(clnt_sock, buffer, sizeof(buffer)-1);
        if (str_len <= 0) break;
        buffer[str_len] = '\0';
        
        // 데이터 파싱
        float distance;
        char result;
        sscanf(buffer, "%f,%c", &distance, &result);
        
        // DB에 저장
        char sql[200];
        snprintf(sql, sizeof(sql), 
                "INSERT INTO sensor_readings (distance, is_within_range) VALUES (%.2f, '%c');",
                distance, result);
        
        rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
        
        printf("received and saved: distance=%.2f, result=%c\n", distance, result);
    }

    sqlite3_close(db);
    close(clnt_sock);
    close(serv_sock);
    return 0;
}