/* Server code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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
    printf("Client replay: [%s]\n", buffer);
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

int main(void)
{
  struct sockaddr_in stSockAddr;
  struct sockaddr_in cli_addr;
  int client;
  int SocketFD;
  char buffer[256];
  char buffer2[256];
  int n;

  if ((SocketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("Socket");
    exit(1);
  }

  if (setsockopt(SocketFD, SOL_SOCKET, SO_REUSEADDR, "1", sizeof(int)) == -1){
    perror("Setsockopt");
    exit(1);
  }

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(1100);
  stSockAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(struct sockaddr)) == -1){
    perror("Unable to bind");
    exit(1);
  }

  if (listen(SocketFD, 5) == -1){
    perror("Listen error");
    exit(1);
  }

  for (;;){
    client = sizeof(cli_addr);
    int ConnectFD = accept(SocketFD, (struct sockaddr *)&cli_addr, (socklen_t *)&client);

    bzero(buffer, 256);
    bzero(buffer2, 256);

    while (1)
    {
      n = recv(ConnectFD, buffer, 255, 0);
      if (n < 0)
        perror("ERROR reading from socket");

      strcat(buffer2, "MSG:[");
      strcat(buffer2, buffer);
      strcat(buffer2, "]");
      printf("Responding:<%s>\n", buffer2);
      printf("Here is the message: [%s]\n", buffer);

      n = send(ConnectFD, "I got your message", 18, 0);
    }

    close(ConnectFD);
  }

  close(SocketFD);
  return 0;
}
