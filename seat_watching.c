#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <string.h> 

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256
#define PORT 8080
#define MAX_BUFFER 1024
#define DISTANCE_THRESHOLD 50.0 

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

// 4개 센서
#define TRIG1 4
#define ECHO1 18
#define TRIG2 17
#define ECHO2 23
#define TRIG3 27
#define ECHO3 24
#define TRIG4 6
#define ECHO4 12

// 로그 출력을 위한 매크로 
#define SENSOR1_LOG "[센서 1 쓰레드] "
#define SENSOR2_LOG "[센서 2 쓰레드] "
#define SENSOR3_LOG "[센서 3 쓰레드] "
#define SENSOR4_LOG "[센서 4 쓰레드] "

typedef struct {
    int trig;
    int echo;
    int sensor_num;
} sensor_params;

// 전역 변수들
int sensor_active[4] = {0, 0, 0, 0};
pthread_mutex_t sensor_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
int sock;

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
    char path[DIRECTION_MAX];
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
        close(fd);
        return (-1);
    }
    
    close(fd);
    return (0);
}

double measure_distance(int trig, int echo, int sensor_num) {
    struct timespec start_time, end_time;
    double time_diff;
    const char* thread_log;
    
    switch(sensor_num) {
        case 1: thread_log = SENSOR1_LOG; break;
        case 2: thread_log = SENSOR2_LOG; break;
        case 3: thread_log = SENSOR3_LOG; break;
        case 4: thread_log = SENSOR4_LOG; break;
        default: thread_log = "[ Unknown ] "; break;
    }

    // TRIG 신호 전송
    if (GPIOWrite(trig, LOW) == -1) {
        printf("%sGPIO write LOW 오류\n", thread_log);
        return -1;
    }
    usleep(10);
    
    if (GPIOWrite(trig, HIGH) == -1) {
        printf("%sGPIO write HIGH 오류\n", thread_log);
        return -1;
    }
    usleep(10);
    
    if (GPIOWrite(trig, LOW) == -1) {
        printf("%sGPIO write LOW 오류\n", thread_log);
        return -1;
    }

    // ECHO 신호 대기 및 측정
    while (GPIORead(echo) == 0) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
    }
    
    while (GPIORead(echo) == 1) {
        clock_gettime(CLOCK_MONOTONIC, &end_time);
    }

    // 시간 차이 계산 (나노초 단위까지 고려)
    time_diff = (end_time.tv_sec - start_time.tv_sec) + 
                (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    // 거리 계산 (음속 = 340m/s = 34000cm/s)
    double distance = time_diff * 17000.0; // 34000/2 = 17000

    return distance;
}

void* sensor_thread(void* arg) {
    sensor_params* params = (sensor_params*)arg;
    char result[3] = {0};
    const char* thread_log;
    struct timespec last_measurement_time, current_time, activation_time;
    int was_active = 0;  // 이전 상태를 추적하기 위한 변수
    
    switch(params->sensor_num) {
        case 1: thread_log = SENSOR1_LOG; break;
        case 2: thread_log = SENSOR2_LOG; break;
        case 3: thread_log = SENSOR3_LOG; break;
        case 4: thread_log = SENSOR4_LOG; break;
        default: thread_log = "[ Unknown ] "; break;
    }
    
    printf("%s쓰레드 시작\n", thread_log);
    clock_gettime(CLOCK_MONOTONIC, &last_measurement_time);
    
    while(1) {
        pthread_mutex_lock(&sensor_mutex);
        int is_active = sensor_active[params->sensor_num - 1];
        pthread_mutex_unlock(&sensor_mutex);
        
        // 센서가 새롭게 활성화된 경우
        if(is_active && !was_active) {
            clock_gettime(CLOCK_MONOTONIC, &activation_time);
            printf("%s센서 활성화. 10초 후 측정 시작\n", thread_log);
            was_active = 1;
        }
        // 센서가 비활성화된 경우
        else if(!is_active && was_active) {
            was_active = 0;
            printf("%s센서 비활성화\n", thread_log);
        }
        
        if(is_active) {
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            
            // 센서 활성화 후 경과 시간 계산
            double activation_elapsed = (current_time.tv_sec - activation_time.tv_sec) + 
                                     (current_time.tv_nsec - activation_time.tv_nsec) / 1e9;
            
            // 마지막 측정 후 경과 시간 계산
            double measurement_elapsed = (current_time.tv_sec - last_measurement_time.tv_sec) + 
                                      (current_time.tv_nsec - last_measurement_time.tv_nsec) / 1e9;
            
            // 활성화 후 10초 이상 경과했고, 마지막 측정 후 10초 이상 경과한 경우
            if(activation_elapsed >= 10.0 && measurement_elapsed >= 10.0) {
                double distance = measure_distance(params->trig, params->echo, params->sensor_num);

                if(distance >= 0) {
                    int detected = (distance <= DISTANCE_THRESHOLD) ? 1 : 0;
                    snprintf(result, sizeof(result), "%d%d", params->sensor_num, detected);
                    
                    pthread_mutex_lock(&socket_mutex);
                    ssize_t sent = send(sock, result, 2, MSG_NOSIGNAL);
                    if (sent <= 0) {
                        printf("%s결과 전송 실패\n", thread_log);
                    } else {
                        printf("%s거리: %.2fcm, %s 감지됨, 결과: %s 전송됨\n", 
                               thread_log, distance, detected ? "물체" : "물체 미", result);
                    }
                    pthread_mutex_unlock(&socket_mutex);
                    
                    clock_gettime(CLOCK_MONOTONIC, &last_measurement_time);
                } else {
                    printf("%s거리 측정 실패\n", thread_log);
                }
            }
        }
        usleep(50000); // 50ms 대기
    }
    return NULL;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    pthread_t threads[4];
    sensor_params sensors[4] = {
        {TRIG1, ECHO1, 1},
        {TRIG2, ECHO2, 2},
        {TRIG3, ECHO3, 3},
        {TRIG4, ECHO4, 4}
    };
    char recv_msg[3] = {0};
    int str_len;

    if (argc != 3) {
        printf("사용법: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    printf("초음파 센서 프로그램을 시작합니다...\n");
    
    // GPIO 핀 초기화
    if (-1 == GPIOExport(TRIG1) || -1 == GPIOExport(ECHO1) ||
        -1 == GPIOExport(TRIG2) || -1 == GPIOExport(ECHO2) ||
        -1 == GPIOExport(TRIG3) || -1 == GPIOExport(ECHO3) ||
        -1 == GPIOExport(TRIG4) || -1 == GPIOExport(ECHO4)) {
        printf("GPIO 내보내기 오류\n");
        return 1;
    }
    usleep(100000);

    // GPIO 방향 설정
    if (-1 == GPIODirection(TRIG1, OUT) || -1 == GPIODirection(ECHO1, IN) ||
        -1 == GPIODirection(TRIG2, OUT) || -1 == GPIODirection(ECHO2, IN) ||
        -1 == GPIODirection(TRIG3, OUT) || -1 == GPIODirection(ECHO3, IN) ||
        -1 == GPIODirection(TRIG4, OUT) || -1 == GPIODirection(ECHO4, IN)) {
        printf("GPIO 방향 설정 오류\n");
        return 2;
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) 
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("서버 연결 성공\n");
    printf("GPIO 초기화 완료. 쓰레드를 시작합니다...\n");

    // 센서 쓰레드 생성
    for(int i = 0; i < 4; i++) {
        if(pthread_create(&threads[i], NULL, sensor_thread, &sensors[i]) != 0) {
            printf("센서 %d번 쓰레드 생성 실패\n", i+1);
            return 3;
        }
        printf("센서 %d번 쓰레드가 생성되었습니다\n", i+1);
        usleep(100000);
    }

    // 메인 쓰레드에서 서버로부터 메시지 수신
    fd_set readfds;
    struct timeval tv;
    int max_fd = sock + 1;

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        
        // timeout 설정 (1초)
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(max_fd, &readfds, NULL, NULL, &tv);
        
        if (activity < 0) {
            printf("select error\n");
            break;
        }
        
        if (activity > 0 && FD_ISSET(sock, &readfds)) {
            memset(recv_msg, 0, sizeof(recv_msg));
            str_len = read(sock, recv_msg, 2);
            
            if (str_len <= 0) {
                printf("연결 종료 또는 오류\n");
                break;
            }
            
            if (str_len == 2) {
                printf("서버로부터 메시지 수신: %s\n", recv_msg);
                
                int sensor_num = recv_msg[0] - '0';
                int action = recv_msg[1] - '0';
                
                if (sensor_num >= 1 && sensor_num <= 4 && (action == 0 || action == 1)) {
                    pthread_mutex_lock(&sensor_mutex);
                    sensor_active[sensor_num-1] = action;
                    pthread_mutex_unlock(&sensor_mutex);
                    
                printf("센서 %d번 %s\n", sensor_num, action ? "켜짐" : "꺼짐");
                }
            }
        }
    }

    // 정리
    close(sock);
    
    // GPIO 정리
    GPIOUnexport(TRIG1); GPIOUnexport(ECHO1);
    GPIOUnexport(TRIG2); GPIOUnexport(ECHO2);
    GPIOUnexport(TRIG3); GPIOUnexport(ECHO3);
    GPIOUnexport(TRIG4); GPIOUnexport(ECHO4);
    
    // 뮤텍스 정리
    pthread_mutex_destroy(&socket_mutex);
    pthread_mutex_destroy(&sensor_mutex);

    return 0;
}