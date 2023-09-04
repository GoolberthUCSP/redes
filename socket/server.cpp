/* Server code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <thread>
#include <map>
#include <string>
#include <sstream>

using namespace std;

map<string, int> clientNames; //clientNames[clientName] = SocketFD

void thread_reader(int ConnectFD, string sender){
  char buffer[256];
  int n;
  while (1){
    bzero(buffer, 256);
    n = recv(ConnectFD, buffer, 255, 0);
    if (n < 0)
      perror("ERROR reading from socket");

    //kill process if message is "END"
    if (strcmp(buffer, "END") == 0){
      shutdown(ConnectFD, SHUT_RDWR);
      close(ConnectFD);
      clientNames.erase(sender);
      printf("Client disconnected: [%s]\n", sender.c_str());
      break;
    }

    //type of message: receiver,message
    stringstream ss(buffer);
    string receiver, message;
    getline(ss, receiver, ',');
    getline(ss, message);
    //message = message.substr(0, message.size()-1);

    //if receiver is online
    if (clientNames.find(receiver) != clientNames.end()){
      int receiverID = clientNames[receiver];
      message= sender + ": " + message;
      n = send(receiverID, message.c_str(), strlen(message.c_str()), 0);
      
      if (n < 0)
        perror("ERROR writing to socket");
      
      printf("Message replay to %s: [%s]\n", receiver.c_str(), message.c_str());
    }
    else{
      printf("Message not sent, receiver no exist: [%s]\n", receiver.c_str());
      
      receiver += " not online!";
      n = send(ConnectFD, receiver.c_str(), strlen(receiver.c_str()), 0);

      if (n < 0)
        perror("ERROR writing to socket");
    }
  }
}

void kill(int SocketFD){
  char tmp;
  scanf("%c", &tmp);
  shutdown(SocketFD, SHUT_RDWR);
  close(SocketFD);
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]){
  if (argc != 2){
    printf("Bad number of arguments\n");
    exit(1);
  }
  int Port = atoi(argv[1]);
  if (Port < 1024 || Port > 65535){
    printf("Bad port number\n");
    exit(1);
  }

  struct sockaddr_in stSockAddr;
  struct sockaddr_in cli_addr;
  int client;
  int SocketFD;
  char buffer[256];
  
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
  stSockAddr.sin_port = htons(Port);
  stSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(struct sockaddr)) == -1){
    perror("Unable to bind");
    exit(1);
  }
  if (listen(SocketFD, 5) == -1){
    perror("Listen error");
    exit(1);
  }

  //thread(kill, SocketFD).detach();
  client = sizeof(cli_addr);

  while (1){
    int ConnectFD = accept(SocketFD, (struct sockaddr *)&cli_addr, (socklen_t *)&client);

    if (ConnectFD < 0){
      perror("Accept error");
      exit(1);
      //kill(SocketFD); //Not sure if this is the right way
    }

    bzero(buffer, 256);

    //Receive username from client
    int n= recv(ConnectFD, buffer, 255, 0);
    if (n < 0)
      perror("ERROR reading from socket");
    buffer[n-1] = '\0';
    
    clientNames[buffer] = ConnectFD;
    thread(thread_reader, ConnectFD, string(buffer)).detach();
    
    printf("Client connected: [%s]\n", buffer);
  }
  return 0;
}
