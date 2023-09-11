/* Server code in CPP */

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
#include <iomanip>

#define SIZE 256

using namespace std;

void processing(int ConnectFD, string &nickname, char buff[SIZE], bool &kill);
void kill_connection(string &nickname);
void send_notification(string &nickname, string notification);

map<string, int> clientNames; //clientNames[clientName] = SocketFD

void thread_reader(int ConnectFD, string sender){
  string nickname(sender);
  bool kill = false;
  char buffer[SIZE];
  int n;
  while (!kill){
    bzero(buffer, SIZE);
    n = recv(ConnectFD, buffer, SIZE-1, 0);
    if (n < 0)
      perror("ERROR reading from socket");

    processing(ConnectFD, nickname, buffer, kill);
  }
}

void kill_connection(string &nickname){
  shutdown(clientNames[nickname], SHUT_RDWR);
  close(clientNames[nickname]);
  clientNames.erase(nickname);
  printf("Client disconnected: [%s]\n", nickname.c_str());
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
  char buffer[SIZE];
  
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

  client = sizeof(cli_addr);

  while (1){
    int ConnectFD = accept(SocketFD, (struct sockaddr *)&cli_addr, (socklen_t *)&client);

    if (ConnectFD < 0){
      perror("Accept error");
      exit(1);
    }

    //Send existent nicknames to verification from client
    string existent_nicknames= to_string(clientNames.size())+",";
    for (auto clientName : clientNames){
      existent_nicknames += clientName.first + ",";
    }
    existent_nicknames = existent_nicknames.substr(0, existent_nicknames.size()-1);
    int n = send(ConnectFD, existent_nicknames.c_str(), strlen(existent_nicknames.c_str()), 0);
    if (n < 0)
      perror("ERROR writing to socket");

    bzero(buffer, SIZE);

    //Receive username from client
    n= recv(ConnectFD, buffer, SIZE-1, 0);
    if (n < 0)
      perror("ERROR reading from socket");
    buffer[n] = '\0';
    
    clientNames[buffer] = ConnectFD;
    thread(thread_reader, ConnectFD, string(buffer)).detach();
    
    printf("Client connected: [%s]\n", buffer);
  }
  return 0;
}

void send_notification(string &nickname, string notification){
  ostringstream os, size_notification;
  size_notification << setw(2) << setfill('0') << notification.size();
  os << 'N' << size_notification.str() << notification;
  int n = send(clientNames[nickname], os.str().c_str(), strlen(os.str().c_str()), 0);
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Notification to %s: [%s]\n", nickname.c_str(), notification.c_str());
}

void processing(int ConnectFD, string &nickname, char buff[SIZE], bool &kill){
  stringstream ss(buff);
  ostringstream os;
  char type;
  ss >> type;

  if (type == 'N'){ //change nickname
    // N00nickname
    string size(2, '0');
    ss.read(size.data(), size.size());
    string new_nickname(stoi(size), '0');
    ss.read(new_nickname.data(), new_nickname.size());

    // Validate new nickname
    if (clientNames.find(new_nickname) != clientNames.end()){
      send_notification(nickname, "Nickname already in use");
      return;
    }

    clientNames.erase(nickname);
    clientNames[new_nickname] = ConnectFD;
    printf("Client nickname changed: [%s->%s]\n", nickname.c_str(), new_nickname.c_str());
    nickname = new_nickname;
    string notification= "Changed nickname to " + new_nickname;
    send_notification(nickname, notification);
  }
  else if (type == 'M'){ //message to one
    // M00receiver000message
    string size_rcv(2, '0'), size_msg(3, '0');
    ss.read(size_rcv.data(), size_rcv.size());

    string receiver(stoi(size_rcv), '0');
    ss.read(receiver.data(), receiver.size());

    if (clientNames.find(receiver) == clientNames.end()){ // Validation if receiver is online
      string notification= "User " + receiver + " is not online";
      send_notification(nickname, notification);
      return;
    }
    
    ss.read(size_msg.data(), size_msg.size());
    string message(stoi(size_msg), '0');
    
    ss.read(message.data(), message.size());
    printf("Message from %s to %s: [%s]\n", nickname.c_str(), receiver.c_str(), message.c_str());
    
    ostringstream size_nickname;
    size_nickname << setw(2) << setfill('0') << nickname.size();
    os << 'M' << size_nickname.str() << nickname << size_msg << message;
    int n = send(clientNames[receiver], os.str().c_str(), strlen(os.str().c_str()), 0); 
    if (n < 0)
      perror("ERROR writing to socket");
    printf("Message replay to %s: [%s]\n", receiver.c_str(), os.str().c_str());
  }
  else if (type == 'W'){ //message to all
    // W000message
    os << ss.str();
    for (auto client: clientNames){
      int n = send(client.second, os.str().c_str(), strlen(os.str().c_str()), 0);
      if (n < 0)
        perror("ERROR writing to socket");
      printf("Message replay to %s: [%s]\n", client.first.c_str(), os.str().c_str());
    }
    printf("Message replay to all: [DONE]\n");
  }
  else if (type == 'L'){ //list of online clients
    // L00
    string names;
    for (auto client: clientNames){
      names += client.first + ',';
    }
    names = names.substr(0, names.size()-1); //remove last comma
    ostringstream size_names;
    size_names << setw(3) << setfill('0') << names.size();
    os << 'L' << size_names.str() << names;
    int n = send(ConnectFD, os.str().c_str(), strlen(os.str().c_str()), 0);
    if (n < 0)
      perror("ERROR writing to socket");
    printf("List of online clients sent: [%s]\n", os.str().c_str());
  }
  else if (type == 'Q'){ //quit
    // Q00
    kill = true;
    kill_connection(nickname);
  }
}