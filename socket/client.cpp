/* Client code in CPP */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <thread>

using namespace std;

void thread_reader(int SocketFD){
  char buffer[256];
  int n;
  while (1){
    bzero(buffer, 256);
    n = recv(SocketFD, buffer, 255, 0);
    
    if (n < 0)
      perror("ERROR reading from socket");

    if (n == 0){
      printf("Client disconnected\n");
      close(SocketFD);
      exit(EXIT_SUCCESS);
    }

    printf("Server replay: [%s]\n", buffer);
  }
}

int main(int argc, char *argv[]){
  if (argc != 2){
    printf("Bad number of arguments\n");
    exit(EXIT_FAILURE);
  }
  int Port = atoi(argv[1]);
  if (Port < 1024 || Port > 65535){
    printf("Bad port number\n");
    exit(EXIT_FAILURE);
  }

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
  stSockAddr.sin_port = htons(Port);
  Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr); //192.168.1.33

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
  else
    printf("Connected to server\n");

  bzero(buffer, 256);

  // Message getting username 
  printf("Enter your username: ");
  fgets(buffer, 255, stdin);
  n = send(SocketFD, buffer, strlen(buffer), 0);

  if (n < 0)
    perror("ERROR writing to socket");
  
  thread(thread_reader, SocketFD).detach();

  // Infinite loop waiting for messages from keyboard
  while(1){ 
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    n = send(SocketFD, buffer, strlen(buffer), 0);
    if (n < 0)
      perror("ERROR writing to socket");
      
  }
  return 0;
}
