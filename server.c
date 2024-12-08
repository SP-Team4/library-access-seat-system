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


//--------Python 코드 


void initialize_python() {
    Py_Initialize();
    PyObject *sys_path = PySys_GetObject("path");
    PyList_Append(sys_path, PyUnicode_DecodeFSDefault("."));
}

void finalize_python() {
    Py_Finalize();
}

int call_python_function(const char *python_name, const char *python_function, const char *parameter1, const char *parameter2) {
    // Python 모듈 로드
    PyObject *pModule = PyImport_ImportModule(python_name);
    if (pModule == NULL) {
        PyErr_Print();
        return -1;  // 모듈 로드 실패
    }

    // Python 함수 가져오기
    PyObject *pFunc = PyObject_GetAttrString(pModule, python_function);
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        return -1;  // 함수 가져오기 실패
    }

    // 파라미터 처리
    PyObject *pArgs;
    if (parameter1 != NULL && parameter2 == NULL) {
        pArgs = PyTuple_Pack(1, PyUnicode_DecodeFSDefault(parameter1));
    } else if (parameter1 != NULL && parameter2 != NULL) {
        pArgs = PyTuple_Pack(2, PyUnicode_DecodeFSDefault(parameter1), PyUnicode_DecodeFSDefault(parameter2));
    } else {
        pArgs = PyTuple_New(0);
    }

    if (pArgs == NULL) {
        PyErr_Print();
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return -1;  // 인자 패킹 실패
    }

    // 함수 호출
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    if (pValue == NULL) {
        PyErr_Print();
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return -1;  // 함수 호출 실패
    }

    // 반환값을 C의 int로 변환
    int result = (int)PyLong_AsLong(pValue);
    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_DECREF(pValue);
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return -1;  // 변환 오류
    }

    // 메모리 해제
    Py_DECREF(pValue);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    return result;  // 반환값
}


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
        for (int i = 30; i > 0; i--) {
          PWMWriteDutyCycle(PWM, i * 100000);
          usleep(10000);
        } 
      }
}

void *entrance(void *data){
  char msg[15];       //출입관리 서버와 통신 [0-11]rfid, [12]valid, [13]1:entry 0:exit, [14]NULL
  memset(&msg, 0, sizeof(msg));
  char *rfid = (char *)malloc(13 * sizeof(char));

  //STUDENT_DATA std;

  while (1){

    if (read(clnt_sock_entrance, &msg, sizeof(char)*15) == -1) 
      error_handling("[Entrance Server] Unable to read\n");

    strncpy(rfid, msg, 13);

    printf("[Entrance Server] Received RFID tag: %s \n", rfid);
    // DB -> 해당 ID로 확인되는 user 있는지 찾기
    //PYTHON -> 카메라 사진 찍고 대조

    int valid = call_python_function("database", "find_face_path", rfid, NULL);
    valid = 1;
    printf("valid = %d\n", valid);
    memset(&msg, 0, sizeof(msg));

    strncpy(msg, rfid, 12);
    
    if (!valid ){
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
    //안갔으면 entry_log_insert 실행 X 
    // DB -> 모션센서 결과에 따라 학생 입장 결과 로그

    int entered = msg[12] - '0';
    if(entered)
      call_python_function("database", "entry_log_insert", rfid, NULL);

    servo(0);

    if (!entered){
      //미입장시
      int seat = 0;
      // DB -> 해당 RFID로 예약된 좌석 있는지 확인
      seat = call_python_function("database", "check_reservation", rfid, NULL);

      if (seat){
        char cancel_msg[3];
        memset(&cancel_msg, 0, sizeof(cancel_msg));
        cancel_msg[1] = seat;
        cancel_msg[2] = '0';
        write(clnt_sock_seat_reserv, cancel_msg, sizeof(cancel_msg)*3);
        //DB -> 해당 rfid 학생에 대해 좌석 취소

        call_python_function("database", "delete_reservation", rfid, NULL);
        //reserv는 좌석취소하고LED끄기
      }
    }
    printf("end of entrance function");
  }
}

void *reservation(void *data){  
  char msg[15];       //좌석예약 서버와 통신 [0-11]rfid, [12]valid [13]좌석번호 [14]NULL
  char rfid[13];
  memset(rfid, 0, sizeof(rfid));
  memset(msg, 0, sizeof(msg));

  while(1){
    //RFID태그 들어옴
    if (read(clnt_sock_seat_reserv, &msg, sizeof(char)*12) == -1) 
      error_handling("[Seat Reservation] Unable to read\n");
    strncpy(rfid, msg, 12);
    printf("%s\n", rfid);

    int isValid = 0;
    //DB -> 해당 RFID로 예약된 좌석 있는지 확인 (있으면 1)
    isValid = call_python_function("database", "check_reservation", rfid, NULL);
    if (isValid){
      //예약된 좌석이 있으면
      msg[12] = '0';
      write(clnt_sock_seat_reserv, msg, sizeof(char)*13);
      printf("Already reserved a seat \n");
      continue;
    }

    //예약 가능한 좌석 목록 받기
    int available_seats = call_python_function("database", "get_available_seats", NULL, NULL);
    char seat_str[6];  // 충분한 크기의 배열로 선언
    snprintf(seat_str, sizeof(seat_str), "s%d", available_seats);
    write(clnt_sock_seat_reserv, seat_str, sizeof(char)*6);
    
    //버튼 결과 받기
    memset(&msg, 0, sizeof(msg));
    if (read(clnt_sock_seat_reserv, &msg, sizeof(char)*13) == -1) 
      error_handling("[Seat Reservation] Unable to read\n");

    strncpy(rfid, msg, 12);
    int seat = msg[12] - '0';  // 문자로 받은 좌석 번호를 정수로 변환
    
    //DB -> RFID, seat으로 좌석 예약 진행
    call_python_function("database", "reserve_seat", rfid, seat);

    snprintf(msg, sizeof(msg), "%d1", seat);  // 배열 크기를 정확하게 지정
    write(clnt_sock_seat_reserv, msg, sizeof(msg));
  
    //LED 키라고 전송
    char LED_message[3] = { 0 };
    write(clnt_sock_seat_reserv, LED_message, sizeof(char)*3);

    //좌석감시 서버에 초음파센서 키라고 전송
    char watch_messagge[3] = { 0 };
    watch_messagge[0] = seat;
    watch_messagge[1] = '1';
    write(clnt_sock_seat_watch, watch_messagge, sizeof(char)*3);
  }
}

// void *watching(void *data){
//   int isValid = 0;

//   char msg[3];       //좌석감시 서버와 통신 [0]좌석번호, [1]valid, [2]NULL
//   memset(&msg, 0, sizeof(msg));

//   msg[0] = '3';
//   msg[1] = '1';
//   write(clnt_sock_seat_watch, msg, sizeof(msg));


//   while(1){
//     int count = 0;
//     int seat = 0;

//     if (read(clnt_sock_seat_watch, &msg, sizeof(msg)) == -1) 
//       error_handling("[Seat Watching] Unable to read \n");

//     seat = msg[0] - '0';  // 문자를 숫자로 변환
//     count = msg[1] - '0';  // 문자를 숫자로 변환
//     printf("msg: %s\n", msg);

//     if (count >= 3){
//       call_python_function("database", "increase_count", seat, NULL);

//       snprintf(msg, sizeof(msg), "%d0", seat);  // 배열 크기를 정확하게 지정
//       write(clnt_sock_seat_watch, msg, sizeof(msg));
//       //DB -> 좌석 예약 취소 처리
//     }
//     else{
//       snprintf(msg, sizeof(msg), "%d1", seat);  // 배열 크기를 정확하게 지정
//       write(clnt_sock_seat_watch, msg, sizeof(msg));
//       //DB -> 해당 좌석 count 0으로 초기화
//       call_python_function("database", "zero_count", seat, NULL);
//     }
//   }
// }

void *watching(void *data){
  char msg[3];
  memset(&msg, 0, sizeof(msg));
  
  // 초기 센서 활성화
  msg[0] = '3';
  msg[1] = '1';
  write(clnt_sock_seat_watch, msg, sizeof(msg));
  printf("[Seat Watching] 센서 3번 활성화 명령 전송\n");

  while(1){
    if (read(clnt_sock_seat_watch, &msg, sizeof(msg)) == -1) 
      error_handling("[Seat Watching] Unable to read \n");

    int seat = msg[0] - '0';
    int detected = msg[1] - '0';
    printf("[Seat Watching] 수신된 메시지: %s (좌석 %d에서 %s)\n", 
           msg, seat, detected ? "물체 감지됨" : "물체 없음");

    // 감지 횟수 3회 이상이면
    if (detected >= 3){
      call_python_function("database", "increase_count", seat, NULL);
      // 센서 비활성화 명령 전송
      snprintf(msg, sizeof(msg), "%d0", seat);
      write(clnt_sock_seat_watch, msg, sizeof(msg));
      printf("[Seat Watching] 좌석 %d 센서 비활성화 명령 전송\n", seat);
    }
    else{
      // DB 카운트 초기화만 수행
      call_python_function("database", "zero_count", seat, NULL);
    }
  }
}


int main(int argc, char *argv[]) {

  initialize_python();


  call_python_function("database","make_db", NULL, NULL );
//  call_python_function("lcd","lcd_execute","ajou", "library");

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
   /*
  if (clnt_sock_entrance < 0){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_entrance = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_entrance == -1) error_handling("Entrance client: accept() error");
    printf("Entrance client: Connection established\n");
  }
  */


  /***********************************
   *    좌석 예약 client 소켓 연결    *
   **********************************/
   /*
    if (clnt_sock_seat_reserv < 0){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_seat_reserv = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock_seat_reserv == -1) error_handling("Seat reservation client: accept() error");
    printf("Seat reservation client: Connection established\n");
  }
*/
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
  
    
  //pthread_create(&entrance_thread, NULL, entrance, NULL);
  //pthread_create(&seat_reserv_thread, NULL, reservation, NULL);
    pthread_create(&seat_watch_thread, NULL, watching, NULL);    

  //pthread_join(entrance_thread, (void **)&status);
  //pthread_join(seat_reserv_thread, (void **)&status);
  pthread_join(seat_watch_thread, (void **)&status);

  close(clnt_sock_entrance);
  close(clnt_sock_seat_reserv);
  close(clnt_sock_seat_watch);
  close(serv_sock);
  finalize_python();

  return (0);
}

