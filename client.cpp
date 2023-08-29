/* Client code in CPP */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include <thread>

using namespace std;

void thread_reader(int socketID){
  char buffer[256];
  int n;
  while (1){
    bzero(buffer, 256);
    n = recv(socketID, buffer, 255, 0);
    if (n < 0)
      perror("ERROR reading from socket");
    printf("Server replay: [%s]\n", buffer);
  }
}

void thread_writer(int socketID){
  int n;
  char buffer[256];
  while (1){
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    n = send(socketID, buffer, strlen(buffer), 0);
    if (n < 0)
      perror("ERROR writing to socket");
    printf("Client sent: [%s]\n", buffer);
  }
}

int main(int argc, char *argv[]){
  struct sockaddr_in stSockAddr;
  int Res;
  int SocketFD = socket(AF_INET, SOCK_STREAM, 0); // IPPROTO_TCP
  int n;
  char buffer[256];

  if (-1 == SocketFD){
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(1100);
  Res = inet_pton(AF_INET, "192.168.1.33", &stSockAddr.sin_addr);

  if (0 > Res){
    perror("error: first parameter is not a valid address family");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
  else if (0 == Res)
  {
    perror("char string (second parameter does not contain valid ipaddress");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
  {
    perror("connect failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
  printf("Sending:<%s>\n", argv[1]);
  n = send(SocketFD, argv[1], strlen(argv[1]), 0);

  bzero(buffer, 256);
  n = recv(SocketFD, buffer, 255, 0);
  if (n < 0)
    perror("ERROR reading from socket");
  printf("Server replay: [%s]\n", buffer);

  sleep(1000);

  shutdown(SocketFD, SHUT_RDWR);

  close(SocketFD);
  return 0;
}
