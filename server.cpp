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
#include <map>
#include <string>

using namespace std;

vector<int> clients;
map<string, int> users;

void thread_reader(int socketID){
  char buffer[256];
  int n;
  while (1){
    bzero(buffer, 256);
    n = recv(socketID, buffer, 255, 0);
    if (n < 0)
      perror("ERROR reading from socket");
    if (buffer == "END"){
      shutdown(socketID, SHUT_RDWR);
      close(socketID);
      break;
    }
    string sender, receiver, message;
    //type of message: sender,receiver,message
    int i = 0;
    for (; buffer[i] != ','; i++)
      sender += buffer[i];
    i++;
    for (; buffer[i] != ','; i++)
      receiver += buffer[i];
    i++;
    for (; buffer[i] != '\0'; i++)
      message += buffer[i];
    if (users.find(receiver) != users.end()){
      int receiverID = users[receiver];
      n = send(receiverID, message.c_str(), strlen(message.c_str()), 0);
      if (n < 0)
        perror("ERROR writing to socket");
      printf("Client sent: [%s]\n", buffer);
    }
    else{
      n = send(socketID, "User not found", strlen("User not found"), 0);
      if (n < 0)
        perror("ERROR writing to socket");
      printf("Client sent: [%s]\n", buffer);
    }
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

    if (ConnectFD < 0){
      perror("Accept error");
      exit(1);
    }

    clients.push_back(ConnectFD);
    thread(thread_reader, ConnectFD).detach();
    thread(thread_writer, ConnectFD).detach();

    printf("Client connected: %s\n", inet_ntoa(cli_addr.sin_addr));
  }

  close(SocketFD);
  return 0;
}
