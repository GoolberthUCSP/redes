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
#include <sstream>

using namespace std;

map<string, int> clientNames; //clientNames[clientName] = SocketFD

void thread_reader(int ConnectFD){
  char buffer[256];
  int n;
  while (1){
    bzero(buffer, 256);
    n = recv(ConnectFD, buffer, 255, 0);
    if (n < 0)
      perror("ERROR reading from socket");
    if (buffer == "END"){
      shutdown(ConnectFD, SHUT_RDWR);
      close(ConnectFD);
      break;
    }
    stringstream ss(buffer);
    string sender, receiver, message;
    //type of message: sender,receiver,message
    getline(ss, sender, ',');
    getline(ss, receiver, ',');
    getline(ss, message);

    if (clientNames.find(receiver) != clientNames.end()){
      int receiverID = clientNames[receiver];
      n = send(receiverID, message.c_str(), strlen(message.c_str()), 0);
      if (n < 0)
        perror("ERROR writing to socket");
      printf("Client sent: [%s]\n", buffer);
    }
    else{
      n = send(ConnectFD, "User not online", strlen("User not online"), 0);
      if (n < 0)
        perror("ERROR writing to socket");
      printf("Client sent: [%s]\n", buffer);
    }
    printf("Client replay: [%s]\n", buffer);
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

  while (1){
    client = sizeof(cli_addr);
    int ConnectFD = accept(SocketFD, (struct sockaddr *)&cli_addr, (socklen_t *)&client);

    if (ConnectFD < 0){
      perror("Accept error");
      exit(1);
    }

    // Enviar mensaje al cliente pidiendo su nombre
    char buffer[256];
    bzero(buffer, 256);



    string clientIP= inet_ntoa(cli_addr.sin_addr);
    clientNames[clientIP] = ConnectFD;
    thread(thread_reader, ConnectFD).detach();
    
    printf("Client connected: %s\n", clientIP.c_str());
  }

  close(SocketFD);
  return 0;
}
