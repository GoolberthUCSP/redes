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
#include <iomanip>
#include <vector>
#include <sstream>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <functional>
#include "connection.h"

using namespace std;
int SocketFD = socket(AF_INET, SOCK_STREAM, 0); // IPPROTO_TCP

void thread_reader(Connection &conn){
  while(!conn.disconnected){
    conn.recv_and_decode();
  }
}

void thread_sender(Connection &conn){
  string usr_input;
  while(!conn.disconnected){
    getline(cin, usr_input);
    conn.encoding(usr_input);
    cin.clear();
  }
}

bool validate_nickname(string nickname, unsigned char buff[SIZE]){
  string size, nick;
  vector<string> nicknames;
  stringstream ss;
  ss << buff;
  getline(ss, size, ',');
  for (int i = 0; i < stoi(size); i++){
    getline(ss, nick, ',');
    nicknames.push_back(nick);
  }
  for (auto it: nicknames){
    if (it == nickname)
      return false;
  }
  return true;
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
  int n;
  unsigned char buffer[SIZE];
  
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

  bzero(buffer, SIZE);
  
  // Get existent usernames to validate nickname
  n = recv(SocketFD, buffer, SIZE-1, 0);
  if (n < 0)
    perror("ERROR reading from socket");
  
  // Getting a nickname no repeated
  char nickname[50];
  do{
    bzero(nickname, 50); 
    printf("Enter an username no repeated: ");
    fgets(nickname, 49, stdin);
    nickname[strcspn(nickname, "\n")] = '\0';
  }while(!validate_nickname(string(nickname), buffer));
  
  n = send(SocketFD, nickname, strlen(nickname), 0);

  if (n < 0)
    perror("ERROR writing to socket");
  
  Connection conn(SocketFD);

  thread reader = thread(thread_reader, ref(conn));
  thread sender = thread(thread_sender, ref(conn));
  reader.join();
  sender.join();
  return 0;
}