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

#define SIZE 256

using namespace std;

string encoding(char message[SIZE]);
void decoding(char message[SIZE]);

void thread_reader(int SocketFD){
  char buffer[SIZE];
  int n;
  while (1){
    bzero(buffer, SIZE);
    n = recv(SocketFD, buffer, SIZE-1, 0);
    
    if (n < 0)
      perror("ERROR reading from socket");

    if (n == 0){
      printf("Disconnected from server\n");
      close(SocketFD);
      exit(EXIT_SUCCESS);
    }
    
    decoding(buffer);
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
  char buffer[SIZE];

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

  // Message getting username 
  printf("Enter your username: ");
  fgets(buffer, SIZE-1, stdin);
  n = send(SocketFD, buffer, strlen(buffer), 0);

  if (n < 0)
    perror("ERROR writing to socket");
  
  thread(thread_reader, SocketFD).detach();

  // Infinite loop waiting for messages from keyboard
  while(1){ 
    bzero(buffer, SIZE);
    fgets(buffer, SIZE-1, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    string packet= encoding(buffer);
    if (packet == "E00"){ //Error message
      printf("Error in your message\n");
      continue;
    }
    
    n = send(SocketFD, packet.c_str(), strlen(packet.c_str()), 0);
    if (n < 0)
      perror("ERROR writing to socket");
      
  }
  return 0;
}

string encoding(char buff[SIZE]){
  // SIZE_RECEIVER => [01..99] = 2 char, SIZE_MESSAGE => [001..999] = 3 char
  stringstream ss;
  ostringstream output;

  ss << buff;
  char type, cm;
  ss >> type >> cm; //clearing the comma
  type= tolower(type);
  if (type == 'n') { //Change name
    // N,nickname
    string name(ss.str());
    ostringstream size;
    size << setw(2) << setfill('0') << name.size();
    output << toupper(type) << size.str() << name;
  } 
  else if (type == 'm') { //Message to one
    // M,receiver,message
    string rcv, message; //receiver, message
    getline(ss, rcv, ',');
    getline(ss, message);
    ostringstream size_rcv, size_msg;
    size_rcv << setw(2) << setfill('0') << rcv.size();
    size_msg << setw(3) << setfill('0') << message.size();
    output << toupper(type) << size_rcv.str() << rcv << size_msg.str() << message;
  } 
  else if (type == 'w') { //Message to all
    // W,message
    string message(ss.str());
    ostringstream size;
    size << setw(3) << setfill('0') << message.size();
    output << toupper(type) << size.str() << message;
  }
  else if (type == 'l') { //List all online
    // L || l
    output << toupper(type) << "00";
  } 
  else if (type == 'q') { //End connection
    // Q || q
    output << toupper(type) << "00";
  } 
  else { //Error in message
    output << "E00";
  }
  return output.str();
}

void decoding(char buff[SIZE]){
  // Decoding the packet and printing it
  // SIZE_RECEIVER => [01..99] = 2 char, SIZE_MESSAGE => [001..999] = 3 char

  stringstream ss;
  ss << buff;
  char type;
  ss >> type;

  if (type == 'N') { //Notification
    // N00notification 2 bytes for size of notification
    string size(2, '0');
    ss.read(size.data(), size.size());
    string notification(stoi(size), '0');
    ss.read(notification.data(), notification.size());
    cout << "Notification: " << notification << endl;
  }   
  else if (type == 'M') { //Message
    // M00sender000message 2 bytes for size of sender, 3 bytes for size of message
    string size_sdr(2, '0'), size_msg(3, '0');
    ss.read(size_sdr.data(), size_sdr.size());

    string sender(stoi(size_sdr), '0');
    ss.read(sender.data(), sender.size());
    
    ss.read(size_msg.data(), size_msg.size());
    string message(stoi(size_msg), '0');
    ss.read(message.data(), message.size());
    cout << "Message from " << sender << ": " << message << endl;
  } 
  else if (type == 'W') { //Message to all
    // W000message 3 bytes for size of message
    string size(3, '0');
    ss.read(size.data(), size.size());
    string message(stoi(size), '0');
    ss.read(message.data(), message.size());
    cout << "Message to all: " << message << endl;
  }
  else if (type == 'L') { //List all online
    // L000name1,name2,name3... 3 bytes for size of list of names
    string size(3, '0'), name;
    char c;
    vector<string> names;
    ss.read(size.data(), size.size());
    for (int i = 0; i < stoi(size); i++){
      ss >> c;
      if (c == ','){
        names.push_back(name);
        name = "";
      }
      else name += c;
    } 
    names.push_back(name);
    cout << "Online users: " << names.size() << endl;
    for (auto str: names){
      cout << "- " << str << endl; 
    }
  }
}