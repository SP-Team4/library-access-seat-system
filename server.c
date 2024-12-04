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

// #define PIN 20
// #define POUT 21

typedef struct student_data {
  int std_id[10]; //학번
  int seat_number;
  int enter_time;
  int out_time;
  int penalty;
} DATA;

typedef struct 

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

/****** Buffer ******/
void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

/****** Thread Functions ******/
void *entrance(void *data){
  char msg[] = "Entrance client socket";
  int isValid = 8;
  snprintf(msg, 2, "%d", isValid);
  write(clnt_sock_entrance, msg, sizeof(msg));
  printf("msg = %s\n", msg);
}

void *reservation(void *data){
  char msg[] = "Seat reservation client socket";
  int isValid = 8;
  snprintf(msg, 2, "%d", isValid);
  write(clnt_sock_seat_reserv, msg, sizeof(msg));
  printf("msg = %s\n", msg);
}

void *watching(void *data){
  //
  char msg[] = "Seat watching client socket";
  int isValid = 8;
  snprintf(msg, 2, "%d", isValid);
  write(clnt_sock_seat_watch, msg, sizeof(msg));
  printf("msg = %s\n", msg);
}



int main(int argc, char *argv[]) {
  int state = 1;
  int prev_state = 1;
  // int light = 0;

  // char msg[2] = {0};

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

  printf("도서관 출입 관리 시스템을 시작합니다.");
    
  pthread_create(&entrance_thread, NULL, entrance, (void *)data);
  pthread_create(&seat_watch_thread, NULL, reservation, (void *)data);    
  pthread_create(&seat_reserv_thread, NULL, watching, (void *)data);

  pthread_join(entrance_thread, (void **)&status);
  pthread_join(seat_watch_thread, (void **)&status);
  pthread_join(seat_reserv_thread, (void **)&status);

  // while (1) {
  //   state = GPIORead(PIN);

  //   if (prev_state == 0 && state == 1) {
  //     light = (light + 1) % 2;
  //     snprintf(msg, 2, "%d", light);
  //     write(clnt_sock, msg, sizeof(msg));
  //     printf("msg = %s\n", msg);
    
    
  //     pthread_create(&clnt_sock_entrance, NULL, entrance, (void *)data);
  //     pthread_create(&clnt_sock_seat_reserv, NULL, reservation, (void *)data);    
  //     pthread_create(&clnt_sock_seat_watch, NULL, watching, (void *)data);

  //   }

  //   prev_state = state;
  //   usleep(500 * 100);
  // }

  close(clnt_sock_entrance);
  close(clnt_sock_seat_reserv);
  close(clnt_sock_seat_watch);
  close(serv_sock);

  return (0);
}

