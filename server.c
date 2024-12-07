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

#define PY_SSIZE_T_CLEAN
#include <Python.h>



// #define BUF_SIZE 1024

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256
#define SEAT_NUM 4

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

void servo(int open){ 
    PWMExport(PWM);
    PWMWritePeriod(PWM, 5000000);
    PWMWriteDutyCycle(PWM, (open?0:30*100000));
    PWMEnable(PWM);


    if (open){
      printf("Gate open\n");
      for (int i = 0; i < 30; i++) {
        PWMWriteDutyCycle(PWM, i * 100000);
        usleep(10000);
      }
    }
      else{
        printf("Gate close\n");
        for (int i = 30; i > 0; i++) {
          PWMWriteDutyCycle(PWM, i * 100000);
          usleep(10000);
        } 
      }
}

void *entrance(void *data){
  char msg[15];       //출입관리 서버와 통신 [0-11]rfid, [12]valid, [13]1:entry 0:exit, [14]NULL
  memset(&msg, 0, sizeof(msg));

  //STUDENT_DATA std;

  while (1){
    char rfid[12];
    memset(&rfid, 0, sizeof(rfid));

    if (read(clnt_sock_entrance, &msg, sizeof(char)*15) == -1) 
      error_handling("[Entrance Server] Unable to read\n");
    strncpy(rfid, msg, 12);

    printf("[Entrance Server] Received RFID tag: %s \n", rfid);
    // DB -> 해당 ID로 확인되는 user 있는지 찾기

    PyObject *pFunc = PyObject_GetAttrString(pModule, "find_face_path");
    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject *pArgs = PyTuple_Pack(1, PyUnicode_DecodeFSDefault(rfid));
        PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
        if (pValue != NULL) {
            char *face_path = PyUnicode_AsUTF8(pValue);
            printf("Face path: %s\n", face_path);
            db = 1; // 사용자 찾음
            Py_DECREF(pValue);
        }
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
    }



    //PYTHON -> 카메라 사진 찍고 대조
    
    int db = 0;     //DB 조회 결과
    int camera = 0; //카메라 조회 결과
    memset(&msg, 0, sizeof(msg));
    strncpy(msg, rfid, 12);
    
    if (!camera || !db ){
      printf("[Entrance Server] Invalid user \n");
      msg[12] = '0';
      write(clnt_sock_entrance, msg, sizeof(char)*15);
      continue; 
    }

    printf("[Entrance Server] Welcome \n");
    msg[12] = '1';
    write(clnt_sock_entrance, msg, sizeof(char)*15);

    servo(1);
    //모션센서 결과 받기
    memset(&msg, 0, sizeof(msg));
    if (read(clnt_sock_entrance, &msg, sizeof(char)*15) == -1) 
      error_handling("[Entrance Server] Unable to read\n");
  
    int entered = msg[12];  //입장했는지
    int side = msg[13];     //방향(1: 입장; 0: 퇴장)

    // DB -> 모션센서 결과(entered, side0에 따라 학생 입장 결과 로그
    pFunc = PyObject_GetAttrString(pModule, "entry_log_insert");
    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject *pArgs = PyTuple_Pack(1, PyLong_FromLong(std_id)); // std_id는 적절히 설정해야 함
        PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
        if (pValue != NULL) {
            Py_DECREF(pValue);
        }
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
    }    servo(0);

    if (!entered){
      //미입장시
      int seat = 0;
      // DB -> 해당 RFID로 예약된 좌석 있는지 확인
      PyObject *pFunc = PyObject_GetAttrString(pModule, "check_reservation");
      if (pFunc && PyCallable_Check(pFunc)) {
          PyObject *pArgs = PyTuple_Pack(1, PyUnicode_DecodeFSDefault(rfid));
          PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
          if (pValue != NULL) {
              isValid = PyLong_AsLong(pValue);
              Py_DECREF(pValue);
          }
          Py_DECREF(pArgs);
          Py_DECREF(pFunc);
      }      
      if (seat){
        char cancel_msg[3];
        memset(&cancel_msg, 0, sizeof(cancel_msg));
        cancel_msg[1] = seat;
        cancel_msg[2] = '0';
        write(clnt_sock_seat_reserv, cancel_msg, sizeof(cancel_msg)*3);
        //DB -> 해당 rfid 학생에 대해 좌석 취소
        //reserv는 좌석취소하고LED끄기
      }
    }
    printf("end of entrance function");
  }
}

void *reservation(void *data){
  char connection_msg[] = "Seat reservation client socket \n";
  write(clnt_sock_seat_reserv, connection_msg, sizeof(connection_msg));
  
  char msg[15];       //좌석예약 서버와 통신 [0-11]rfid, [12]valid [13]좌석번호 [14]NULL
  char rfid[13];
  memset(&msg, 0, sizeof(msg));
  memset(&rfid, 0, sizeof(rfid));

  while(1){
    //RFID태그 들어옴
    if (read(clnt_sock_seat_reserv, &msg, sizeof(char)*12) == -1) 
      error_handling("[Seat Reservation] Unable to read\n");
    strncpy(rfid, msg, 12);

    int isValid = 0;
    //DB -> 해당 RFID로 예약된 좌석 있는지 확인 (있으면 isValid = 1)
    if (isValid){
      //예약된 좌석이 있으면
      strncpy(rfid, msg, 12);
      msg[12] = '0';
      write(clnt_sock_seat_reserv, msg, sizeof(char)*13);
      printf("Already reserved a seat \n");
      continue;
    }

    char seat_list[SEAT_NUM+1];
    //DB -> 예약 가능한 좌석 목록 받기 (string(0110) 형태)
    //좌석예약 서버로 좌석 목록 전송
    // snprintf(seat_msg,seat_list.length, "%s", seat_list);

    write(clnt_sock_seat_reserv, seat_list, sizeof(char)*(SEAT_NUM+1));

    //버튼 결과 받기 [0-11]rfid, [12]좌석번호
    memset(&msg, 0, sizeof(msg));

    if (read(clnt_sock_seat_reserv, &msg, sizeof(char)*13) == -1) 
      error_handling("[Seat Reservation] Unable to read\n");

    strncpy(rfid, msg, 12);
    int seat = msg[12];
    
    //DB -> RFID, seat으로 좌석 예약 진행


    //좌석예약서버에 LED키라고 전송 [0]좌석번호, [1]valid
    char LED_message[3];
    memset(&LED_message, 0, sizeof(LED_message));
    write(clnt_sock_seat_reserv, LED_message, sizeof(char)*3);

    //좌석감시 서버에 초음파센서 키라고 전송
    char watch_messagge[3];
    watch_messagge[0] = seat;
    watch_messagge[1] = '1';
    memset(&watch_messagge, 0, sizeof(watch_messagge));
    write(clnt_sock_seat_watch, watch_messagge, sizeof(char)*3);
  }
}

void *watching(void *data){
  int isValid = 0;

  char msg[3];       //좌석감시 서버와 통신 [0]좌석번호, [1]valid, [2]NULL
  memset(&msg, 0, sizeof(msg));

  while(1){
    int count = 0;
    int seat = 0;

    if (read(clnt_sock_seat_watch, &msg, sizeof(char)*3) == -1) 
      error_handling("[Seat Watching] Unable to read \n");

    seat = msg[0];
    printf("msg: %s", msg);


    if (count >= 3){
      snprintf(msg, 2, "%d0", seat);
      write(clnt_sock_seat_watch, msg, sizeof(msg));
      //DB -> 좌석 예약 취소 처리
    }
    else{
      snprintf(msg, 2, "%d1", seat);
      write(clnt_sock_seat_watch, msg, sizeof(msg));
      //DB -> 해당 좌석 count 0으로 초기화
    }
  }
}



int main(int argc, char *argv[]) {

  //python.h 초기화
  Py_Initialize();
  PyObject *pName = PyUnicode_DecodeFSDefault("database");
  PyObject *pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule == NULL) {
      PyErr_Print();
      fprintf(stderr, "Failed to load \"database\"\n");
      return 1;
  }




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

                                                                                                                                                                                                                                                                                                                                                                                                                 
  printf(" ####    ##  ###                              ### #                                     \n");
  printf(" ##     ##   ##                             ##  ##               #                      \n");
  printf(" ##          ##                             ##   #              ##                      \n");
  printf(" ##    ###   ## ##  ## # ###  ## # ### ##   ####   ### ##  ### ####  ###  ## ##  ##     \n");
  printf(" ##     ##   ### ## ####   ## ####  ## #      ####  ## #  ##    ##  ## ## ### ### ##    \n");
  printf(" ##  #  ##   ##  ## ##    ### ##    ## #    #   ##  ## #  ####  ##  ##### ##  ##  ##    \n");
  printf(" ##  #  ##   ##  ## ##   # ## ##     ##     ##  ##   ##     ##  ##  ##    ##  ##  ##    \n");
  printf(" ###### ####  # ###  ##   #### ##     ##     # ###    ##   ###    ##  #### ##  ##  ##   \n");
  printf("                                    ###             ###                                 \n");
  printf("                                    ##              ##                                  \n");
  
    
  pthread_create(&entrance_thread, NULL, entrance, (void *)data);
  pthread_create(&seat_watch_thread, NULL, reservation, (void *)data);    
  pthread_create(&seat_reserv_thread, NULL, watching, (void *)data);
  //pthread_create(&servo_thread, NULL, servo, (void *)data);

  pthread_join(entrance_thread, (void **)&status);
  pthread_join(seat_watch_thread, (void **)&status);
  pthread_join(seat_reserv_thread, (void **)&status);
  //pthread_join(servo_thread, (void **)&status);

  close(clnt_sock_entrance);
  close(clnt_sock_seat_reserv);
  close(clnt_sock_seat_watch);
  close(serv_sock);

  return (0);
}

