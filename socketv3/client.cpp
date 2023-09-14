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

#define SIZE 131072

using namespace std;

ifstream infile;
ofstream outfile;
int SocketFD = socket(AF_INET, SOCK_STREAM, 0); // IPPROTO_TCP

string encoding(char message[SIZE]);
void decoding(char message[SIZE]);
void replay_hash(string &nickname, string &size_nickname, string &hash_file);
string get_hash(string buffer);
string get_datetime();

void thread_reader(int SocketFD){
  char buffer[SIZE];
  int n;
  while (1){
    bzero(buffer, SIZE);
    n = recv(SocketFD, buffer, SIZE-1, 0);
    
    if (n < 0)
      perror("ERROR reading from socket");

    if (n == 0){ //Disconnected
      printf("Disconnected from server\n");
      close(SocketFD);
      exit(EXIT_SUCCESS);
    }
    
    decoding(buffer);
  }
}

bool validate_nickname(string nickname, char buff[SIZE]){
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
  
  thread(thread_reader, SocketFD).detach();

  // Infinite loop waiting for messages from keyboard
  while(1){ 
    bzero(buffer, SIZE);
    fgets(buffer, SIZE-1, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    string packet= encoding(buffer);
    if (packet[0] == 'E'){ //Error message
      printf("%s\n", packet.c_str());
      continue;
    }
    else if (packet[0] == 'C'){ //Clear screen
      system("clear || cls");
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
    string name;
    getline(ss, name);
    ostringstream size;
    size << setw(2) << setfill('0') << name.size();
    output << 'N' << size.str() << name;
  } 
  else if (type == 'm') { //Message to one
    // M,receiver,message
    string rcv, message; //receiver, message
    getline(ss, rcv, ',');
    getline(ss, message);
    ostringstream size_rcv, size_msg;
    size_rcv << setw(2) << setfill('0') << rcv.size();
    size_msg << setw(3) << setfill('0') << message.size();
    output << 'M' << size_rcv.str() << rcv << size_msg.str() << message;
  } 
  else if (type == 'w') { //Message to all
    // W,message
    string message;
    getline(ss, message);
    ostringstream size;
    size << setw(3) << setfill('0') << message.size();
    output << 'W' << size.str() << message;
  }
  else if (type == 'f'){ //Send file
    // F || f,receiver,filename
    string rcv, filename;
    getline(ss, rcv, ',');
    getline(ss, filename);
    ostringstream size_rcv, size_fn;
    size_rcv << setw(2) << setfill('0') << rcv.size();
    size_fn << setw(5) << setfill('0') << filename.size();
    filename= "data/" + filename;
    infile.open(filename, ios::binary);
    if (!infile.good()){
      output << "ERROR: File not found in " << filename;
      return output.str();
    }
    // Open existent file
    infile.seekg(0, infile.end);
    int size_file = infile.tellg();
    ostringstream size;
    size << setw(10) << setfill('0') << size_file;
    infile.seekg(0, infile.beg);
    string buffer(size_file, '0');
    infile.read(buffer.data(), size_file);
    infile.close();
    string hash_data, datetime;
    hash_data= get_hash(buffer);
    datetime= get_datetime();
    output << 'F' << size_rcv.str() << rcv << size_fn.str() << filename << size.str() << buffer << hash_data << datetime;
  }
  else if (type == 'l') { //List all online
    // L || l
    output << 'L' << "00";
  } 
  else if (type == 'q') { //End connection
    // Q || q
    output << 'Q' << "00";
  } 
  else if (type == 'c') { //Clear terminal
    output << "CLEAR";
  }
  else { //Error in message
    output << "ERROR: Invalid message type.";
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
  else if (type == 'F') { //File received
    // F00sender00000filename(10B size of file)(file)(10B hash value)(14B datetime of sent file in YYYYMMDDhhmmss)
    string size_sdr(2, '0'), size_fn(5, '0'), size_file(10, '0'), hash_data_str(10, '0'), datetime(14, '0');
    ss.read(size_sdr.data(), size_sdr.size());
    string sender(stoi(size_sdr), '0');
    ss.read(sender.data(), sender.size());

    ss.read(size_fn.data(), size_fn.size());
    string filename(stoi(size_fn), '0');
    ss.read(filename.data(), filename.size());
    
    ss.read(size_file.data(), size_file.size());
    string file_buffer(stoi(size_file), '0');

    ss.read(hash_data_str.data(), hash_data_str.size());
    string hash_file_rcv= get_hash(file_buffer);

    replay_hash(sender, size_sdr, hash_file_rcv);

    // Validate hash value
    if (hash_file_rcv == hash_data_str){
      ss.read(datetime.data(), datetime.size());
      cout << "File received from " << sender << ": " << filename << endl;
      // Save file
      outfile.open("out/" + filename, ios::binary);
      outfile.write(file_buffer.data(), file_buffer.size());
      outfile.close();
    }
  }
}

string get_hash(string buffer){
  // Hash value is only the sum of the characters of the message
  ostringstream hash_value;
  int value;
  for (int i = 0; i < buffer.size(); i++){
    value += buffer[i];
  }
  hash_value << setw(10) << setfill('0') << value;
  return hash_value.str();
}

string get_datetime(){
  // Return the current date and time in YYYYMMDDHHMMSS format
  time_t now = time(0);
  tm *ltm = localtime(&now);
  ostringstream datetime;
  datetime << to_string(1900 + ltm->tm_year);
  datetime << setw(2) << setfill('0') << to_string(1 + ltm->tm_mon);
  datetime << setw(2) << setfill('0') << to_string(ltm->tm_mday);
  datetime << setw(2) << setfill('0') << to_string(ltm->tm_hour);
  datetime << setw(2) << setfill('0') << to_string(ltm->tm_min);
  datetime << setw(2) << setfill('0') << to_string(ltm->tm_sec);
  return datetime.str();
}

void replay_hash(string &sender, string &size_sdr, string &hash){
  ostringstream output;
  output << 'R' << size_sdr << sender << hash;
  int n = send(SocketFD, output.str().c_str(), output.str().size(), 0);
  if ( n < 0)
    perror("ERROR writing to socket");
}