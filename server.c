#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>



// #define BUF_SIZE 1024

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define PWM 0

// #define PIN 20
// #define POUT 21

typedef struct {
  int std_id[10]; //학번
  char rfid[13];
  int seat_number;
  int enter_tag_time;
  int enter_time;
  int out_tag_time;
  int out_time;
  int penalty;  //블랙리스트 구현 시
} STUDENT_DATA;

STUDENT_DATA students[10]; //10명까지..

void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

/****** Socket ******/

int serv_sock = -1;
int clnt_sock_seat_reserv = -1;
int clnt_sock_seat_watch = -1;
int clnt_sock_entrance = -1;

struct sockaddr_in serv_addr, clnt_addr;
socklen_t clnt_addr_size;

int thr_id;
int status;
int fd;
int ent_val;

/****** Thread ******/
//mutex?
pthread_t seat_reserv_thread;
pthread_t seat_watch_thread;
pthread_t entrance_thread;
pthread_t servo_thread;

/****** PWM settings ******/
static int PWMExport(int pwmnum) {
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  int fd, byte;

  fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for export!\n");
    return (-1);
  }

  byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, byte);
  close(fd);

  sleep(1);

  return (0);
}

static int PWMEnable(int pwmnum) {
  static const char s_enable_str[] = "1";

  char path[DIRECTION_MAX];
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in enable!\n");
    return -1;
  }

  write(fd, s_enable_str, strlen(s_enable_str));
  close(fd);

  return (0);
}

static int PWMWritePeriod(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in period!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in period!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}

static int PWMWriteDutyCycle(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in duty cycle!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in duty cycle!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}


void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

/****** Thread Functions ******/
void *servo(void *data){
    PWMExport(PWM);
    PWMWritePeriod(PWM, 5000000);
    PWMWriteDutyCycle(PWM, 0);
    PWMEnable(PWM);

    printf("Gate open\n");

    for (int i = 0; i < 300; i++) {
      PWMWriteDutyCycle(PWM, i * 10000);
      usleep(10000);
    }

    sleep(10);
    //10초 전에 모션센서로부터 결과 받으면 입력으로 침...

    for (int i = 300; i > 0; i++) {
      PWMWriteDutyCycle(PWM, i * 10000);
      usleep(10000);
    }

}

void *entrance(void *data){
  char msg[14];       //출입관리 서버와 통신 [0-11]rfid, [12]valid
  memset(&msg, 0, sizeof(msg));

  char connect_msg[] = "Entrance client socket\n";
  write(clnt_sock_entrance, connect_msg, sizeof(msg));

  STUDENT_DATA std;

  while (1){
    char rfid[12];
    memset(&rfid, 0, sizeof(rfid));

    for (int i=0; i<12; i++){
      if (read(clnt_sock_entrance, msg[i], sizeof(char)) == -1) 
        error_handling("Unable to read from Entrance_Server\n");
      //strncpy(rfid[i], msg[0], 1);
    }
    strncpy(rfid, msg, 12);

    printf("Successfully received RFID tag: %s \n", rfid);
    //DB -> 해당 ID로 확인되는 user 있는지 찾기
    //PYTHON -> 카메라 사진 찍고 대조
    
    int db = 0;     //DB 조회 결과
    int camera = 0; //카메라 조회 결과
    strncpy(msg, rfid, 12);
    
    if (!camera || !db ){
      printf("Invalid user \n");
      msg[12] = '0';
      write(clnt_sock_entrance, msg, sizeof(msg));
      continue; 
    }

    printf("Welcome \n");
    msg[12] = '1';
    write(clnt_sock_entrance, msg, sizeof(msg));

    //서보모터 열기 -> 스레드로 관리...

    //DB -> 학생 입장 결과 로그

    //모션센서 결과 받기
    for (int i=0; i<13; i++){
      if (read(clnt_sock_entrance, &msg, sizeof(char)) == -1) 
        error_handling("Unable to read from Entrance_Server\n");
      strncpy(rfid[i], msg[0], 1);
    }

    if (!msg[0]){
      int seat = 0;
      //DB -> 해당 rfid 학생에 대해 예약된 좌석이 있는지 확인
      if (seat){
        char cancel_msg[2];
        snprintf(cancel_msg, 2, "%d0", seat);
        write(clnt_sock_seat_reserv, cancel_msg, sizeof(cancel_msg));
        //DB -> 해당 rfid 학생에 대해 좌석 취소
      }
    }
  }
}

void *reservation(void *data){
  char msg[] = "Seat reservation client socket";
  write(clnt_sock_seat_reserv, msg, sizeof(msg));
  



  // printf("msg = %s\n", msg);
  //예약이 되면 seat_watch 소켓으로 키라고 전송
}

void *watching(void *data){
  int isValid = 0;

  char connect_msg[] = "Seat watching client socket";
  write(clnt_sock_entrance, connect_msg, sizeof(msg));

  char msg[2];       //좌석감시 서버와 통신 [0]좌석번호, [12]valid
  memset(&msg, 0, sizeof(msg));

  while(1){
    int count = 0;
    int seat = 0;

    for (int i=0; i<2; i++){
      if (read(clnt_sock_seat_watch, msg[i], sizeof(char)) == -1) 
        error_handling("Unable to read from Seat Watching Server\n");
      //strncpy(rfid[i], msg[0], 1);
    }
    seat = msg[0];
    printf("msg: %s", msg);

    //DB -> 해당 좌석에 대해 count 정보 받아와서 count 변수에 저장, 증가 후 update

    if (count >= 3){
    //DB -> 좌석 예약 취소 처리
      snprintf(msg, 2, "%d0", seat);
      write(clnt_sock_seat_watch, msg, sizeof(msg));
    }
    else{
      snprintf(msg, 2, "%d1", seat);
      write(clnt_sock_seat_watch, msg, sizeof(msg));
      //DB -> 해당 좌석 count 0으로 초기화
    }
  }
}



int main(int argc, char *argv[]) {
  int state = 1;
  int prev_state = 1;

  if (argc != 2) {
    printf("Usage : %s <port>\n", argv[0]);
  }

  // Enable GPIO pins
  // if (-1 == GPIOExport(PIN) || -1 == GPIOExport(POUT)) return (1);
  // // Set GPIO directions
  // if (-1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT, OUT))
  //   return (2);
  // if (-1 == GPIOWrite(POUT, 1)) return (3);


  //socket settings
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_sock == -1) error_handling("socket() error");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("bind() error");

  if (listen(serv_sock, 5) == -1) error_handling("listen() error");


  /***********************************
   *   출입 관리 client 소켓 연결     *
   **********************************/
  if (clnt_sock_entrance < 0){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_entrance = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_entrance == -1) error_handling("Entrance client: accept() error");
    printf("Entrance client: Connection established\n");
  }

  /***********************************
   *    좌석 예약 client 소켓 연결    *
   **********************************/
  if (clnt_sock_seat_reserv < 0){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_seat_reserv = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_seat_reserv == -1) error_handling("Seat reservation client: accept() error");
    printf("Seat reservation client: Connection established\n");
  }
  
  /***********************************
   *   좌석 감시 client 소켓 연결     *
   **********************************/
  if (clnt_sock_seat_watch < 0){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_seat_watch = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_seat_watch == -1) error_handling("Seat watching client: accept() error");
    printf("Seat watching client:  Connection established\n");
  }

  printf("### Start library-access-seat-system ###");
    
  pthread_create(&entrance_thread, NULL, entrance, (void *)data);
  pthread_create(&seat_watch_thread, NULL, reservation, (void *)data);    
  pthread_create(&seat_reserv_thread, NULL, watching, (void *)data);
  pthread_create(&servo_thread, NULL, servo, (void *)data);

  pthread_join(entrance_thread, (void **)&status);
  pthread_join(seat_watch_thread, (void **)&status);
  pthread_join(seat_reserv_thread, (void **)&status);
  pthread_join(servo_thread, (void **)&status);

  close(clnt_sock_entrance);
  close(clnt_sock_seat_reserv);
  close(clnt_sock_seat_watch);
  close(serv_sock);

  return (0);
}

