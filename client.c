#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 23
#define PIN 24

static int GPIOExport(int pin) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}

static int GPIOUnexport(int pin) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}

static int GPIODirection(int pin, int dir) {
    static const char s_directions_str[] = "in\0out";
    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;
    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return (-1);
    }
    if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }
    close(fd);
    return (0);
}

static int GPIORead(int pin) {
    char path[VALUE_MAX];
    char value_str[3];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }
    if (-1 == read(fd, value_str, 3)) {
        fprintf(stderr, "Failed to read value!\n");
        return (-1);
    }
    close(fd);
    return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }
    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
        fprintf(stderr, "Failed to write value!\n");
        return (-1);
    }
    close(fd);
    return (0);
}


int main(int argc, char *argv[]) {
    clock_t start_t, end_t;
    double elapsed_time;
    int measurement_count = 0;
    
    // 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.44.4"); // DB 저장하는 라즈베리파이 IP
    serv_addr.sin_port = htons(8080);
    
    // 서버에 연결
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        printf("Connection Error\n");
        return 1;
    }

    // GPIO 설정
    if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN)) {
        printf("gpio export err\n");
        return 1;
    }
    
    // 나머지 GPIO 초기화 코드...

    printf("측정을 시작합니다. 10초마다 결과가 전송됩니다.\n");
    printf("50cm 이내: t, 50cm 초과: f\n");
    printf("------------------------\n");

    while (1) {
        measurement_count++;
        
        // 시간 측정 코드
        time_t now;
        struct tm *time_info;
        char time_str[9];
        time(&now);
        time_info = localtime(&now);
        strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);
        
        // 초음파 센서 측정
        if (-1 == GPIOWrite(POUT, 1)) {
            printf("gpio write/trigger err\n");
            return 3;
        }

        usleep(10);
        GPIOWrite(POUT, 0);

        while (GPIORead(PIN) == 0) {
            start_t = clock();
        }
        while (GPIORead(PIN) == 1) {
            end_t = clock();
        }

        elapsed_time = (double)(end_t - start_t) / CLOCKS_PER_SEC;
        double distance = elapsed_time / 2 * 34000;
        char result = (distance <= 50.0) ? 't' : 'f';

        // 데이터 전송
        char message[100];
        sprintf(message, "%.2f,%c", distance, result);
        write(sock, message, strlen(message));
        
        printf("[%s] 측정 #%d - 거리: %.2lfcm, 결과: %c\n", 
               time_str, measurement_count, distance, result);
        
        sleep(10);
    }

    close(sock);
    return 0;
}