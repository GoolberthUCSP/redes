/* Server code in CPP */

#include <sys/ioctl.h>
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
#include <vector>
#include <random>
#include <utility>
#include <algorithm>
#include "ttt.h"

#define SIZE 131072

using namespace std;

void processing(int ConnectFD, string &nickname, unsigned char buff[SIZE], bool &kill);
void kill_connection(string &nickname);
void send_notification(string &nickname, string notification);
void wrt_vec(vector<unsigned char> &vec, string data);

void change_nickname(stringstream &ss, int ConnectFD, string &nickname);
void send_message_to_one(stringstream &ss, int ConnectFD, string &nickname);
void send_message_to_all(stringstream &ss, int ConnectFD);
void send_online_clients(stringstream &ss, int ConnectFD, string &nickname);
void send_file_to_one(stringstream &ss, int ConnectFD, string &nickname);
void send_file_confirmation(stringstream &ss, int ConnectFD, string &nickname);
void initialize_game(int ConnectFD, string &nickname);
void play_game(stringstream &ss, int ConnectFD, string &nickname);
void view_board(int ConnectFD, string &nickname);
void send_board_state();
void reinitialize_game();

map<string, int> clientNames; //clientNames[clientName] = SocketFD
TicTacToe *ttt = nullptr;
vector<pair<string, char>> ttt_players;
vector<string> ttt_viewers;
char symbols[2]{'O','X'};
int rand_num;

void thread_reader(int ConnectFD, string sender){
  string nickname(sender);
  bool kill = false;
  unsigned char buffer[SIZE];
  int n;
  while (!kill){
    bzero(buffer, SIZE);
    n = recv(ConnectFD, buffer, SIZE-1, 0);
    if (n < 0)
      perror("ERROR reading from socket");

    if (n == 0){ //Disconnected, it's not the best way to do it
      kill_connection(nickname);
      return;
    }
    
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

  srand(time(NULL));
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
  rand_num = rand() % 2;

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

void processing(int ConnectFD, string &nickname, unsigned char buff[SIZE], bool &kill){
  stringstream ss;
  vector<unsigned char> os;

  ss.write((char *)buff, SIZE);
  char type;
  ss >> type;

  if (type == 'N'){ //change nickname
    // N00nickname
    change_nickname(ss, ConnectFD, nickname);
  }
  else if (type == 'M'){ //message to one
    // M00receiver000message
    send_message_to_one(ss, ConnectFD, nickname);
  }
  else if (type == 'W'){ //message to all
    // W000message
    send_message_to_all(ss, ConnectFD);
  }
  else if (type == 'L'){ //list of online clients
    // L00
    send_online_clients(ss, ConnectFD, nickname);
  }
  else if (type == 'F'){ //message file to one
    // F00receiver00000filename(10B size of file)(file)(10B hash value)(14B datetime of sent file in YYYYMMDDhhmmss)
    send_file_to_one(ss, ConnectFD, nickname);
  }
  else if (type == 'R'){ //file confirmation
    //R00receiver(10B hash)
    send_file_confirmation(ss, ConnectFD, nickname);
  }
  else if (type == 'B'){ //Receive initialize game
    // B000
    initialize_game(ConnectFD, nickname);
  }
  else if (type == 'P'){ //Play
    // P0
    play_game(ss, ConnectFD, nickname);
  }
  else if (type == 'V'){ //View board
    // V000
    view_board(ConnectFD, nickname);
  }
  else if (type == 'Q'){ //quit
    // Q00
    kill = true;
    kill_connection(nickname);
  }
}

// Process functions
void change_nickname(stringstream &ss, int ConnectFD, string &nickname){
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
void send_message_to_one(stringstream &ss, int ConnectFD, string &nickname){
  // M00receiver000message
  vector<unsigned char> os;
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
  //os << 'M' << size_nickname.str() << nickname << size_msg << message;
  wrt_vec(os, 'M' + size_nickname.str() + nickname + size_msg + message);
  int n = send(clientNames[receiver], (char*)os.data(), os.size(), 0); 
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Message replay to %s: [%s]\n", receiver.c_str(), os.data());
}
void send_message_to_all(stringstream &ss, int ConnectFD){
  // W000message
  vector<unsigned char> os;
  wrt_vec(os, ss.str());
  for (auto client: clientNames){
    int n = send(client.second, (char*)os.data(), os.size(), 0);
    if (n < 0)
      perror("ERROR writing to socket");
    printf("Message replay to %s: [%s]\n", client.first.c_str(), os.data());
  }
  printf("Message replay to all: [DONE]\n");
}
void send_online_clients(stringstream &ss, int ConnectFD, string &nickname){
  // L00
  vector<unsigned char> os;
  string names;
  for (auto client: clientNames){
    names += client.first + ',';
  }
  names = names.substr(0, names.size()-1); //remove last comma
  ostringstream size_names;
  size_names << setw(3) << setfill('0') << names.size();
  wrt_vec(os, 'L' + size_names.str() + names);
  int n = send(ConnectFD, (char*)os.data(), os.size(), 0);
  if (n < 0)
    perror("ERROR writing to socket");
  printf("List of online clients sent: [%s]\n", os.data());
}
void send_file_to_one(stringstream &ss, int ConnectFD, string &nickname){
  // F00receiver00000filename(10B size of file)(file)(10B hash value)(14B datetime of sent file in YYYYMMDDhhmmss)
  string size_rcv(2, '0'), size_fn(5, '0'), size_file(10, '0'), hash_data_str(10, '0'), datetime(14, '0');
  ss.read(size_rcv.data(), size_rcv.size());

  string receiver(stoi(size_rcv), '0');
  ss.read(receiver.data(), receiver.size());

  if (clientNames.find(receiver) == clientNames.end()){ // Validation if receiver is online
    string notification= "User " + receiver + " is not online";
    send_notification(nickname, notification);
    return;
  }

  ss.read(size_fn.data(), size_fn.size());
  string filename(stoi(size_fn), '0');

  ss.read(filename.data(), filename.size());

  ss.read(size_file.data(), size_file.size());
  vector<unsigned char> file(stoi(size_file));
  ss.read((char*)file.data(), file.size());
  
  ss.read(hash_data_str.data(), hash_data_str.size());

  ss.read(datetime.data(), datetime.size());

  ostringstream size_nickname;
  vector<unsigned char> replay_message;
  size_nickname << setw(2) << setfill('0') << nickname.size();

  wrt_vec(replay_message, 'F' + size_nickname.str() + nickname + size_fn + filename + size_file);
  copy(file.begin(), file.end(), back_inserter(replay_message));
  wrt_vec(replay_message, hash_data_str + datetime);
  string replay_print= 'F' + size_nickname.str() + nickname + size_fn + filename + size_file + "FILE" + hash_data_str + datetime;

  int n = write(clientNames[receiver], replay_message.data(), replay_message.size());
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Message replay to %s: [%s]\n", receiver.c_str(), replay_print.c_str());
}
void send_file_confirmation(stringstream &ss, int ConnectFD, string &nickname){
  //R00receiver(10B hash)
  string size_rcv(2, '0'), hash_code(10, '0');
  ss.read(size_rcv.data(), size_rcv.size());

  string receiver(stoi(size_rcv), '0');
  ss.read(receiver.data(), receiver.size());

  ss.read(hash_code.data(), hash_code.size());

  ostringstream replay_message, size_nickname;
  size_nickname << setw(2) << setfill('0') << nickname.size();

  replay_message << 'R' << size_nickname.str() << nickname << hash_code; 

  int n = send(clientNames[receiver], replay_message.str().c_str(), strlen(replay_message.str().c_str()), 0);
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Message replay to %s: [%s]\n", receiver.c_str(), replay_message.str().c_str());
}
void initialize_game(int ConnectFD, string &nickname){
  // B000
  if (ttt_players.size() == 0){ // If there is no one gamer
    ttt_players.push_back(make_pair(nickname, symbols[rand_num]));
    send_notification(nickname, "Success, your symbol is " + string(1, symbols[rand_num]) + ", waiting for your turn");
  }
  else if (ttt_players.size() == 1){ // If there is only one player
    ttt_players.push_back(make_pair(nickname, symbols[1-rand_num]));
    ttt = new TicTacToe(ttt_players[0], ttt_players[1], ttt_players[0].second);
    send_notification(nickname, "Success, your symbol is " + string(1, symbols[1-rand_num]) + ", waiting for your turn");
    send_board_state();
    send_notification(ttt_players[0].first, "You can play now. It's your turn");
  }
  else{ // If there are two ttt_players
    send_notification(nickname, "Error, there are already 2 ttt_players");
  }
}
void play_game(stringstream &ss, int ConnectFD, string &nickname){
  // P0
  if (ttt == nullptr){
    send_notification(nickname, "Error, the game has not been initialized");
    return;
  }
  if (!ttt->verify_player(nickname)){
    send_notification(nickname, "Error, you are not in the game");
    return;
  }
  if (!ttt->verify_turn(nickname)){
    send_notification(nickname, "Error, it's not your turn");
    return;
  }
  string position(1, '0');
  ss.read(position.data(), position.size());  
  bool done = ttt->play(stoi(position));
  if (!done){
    send_notification(nickname, "Error, invalid position");
    return;
  }
  else{
    
    send_board_state();
    
    if (ttt->verify_winner()){
      string winner = ttt->get_winner();
      string loser = ttt->get_loser();
      string result= "WINNER: " + winner + ", LOSER: " + loser;
      send_notification(winner, result);
      send_notification(loser, result);
      for (auto viewer : ttt_viewers){
        send_notification(viewer, result);
      }
      reinitialize_game();
    }
    else if (ttt->is_full()){
      string result = "The game is a draw";
      send_notification(ttt->player_1.first, result);
      send_notification(ttt->player_2.first, result);
      for (auto viewer : ttt_viewers){
        send_notification(viewer, result);
      }
      reinitialize_game();
    }
    else
      send_notification(ttt->get_turn_now(), "It's your turn");
  }
}
void view_board(int ConnectFD, string &nickname){
  // V000
  if (ttt == nullptr){
    send_notification(nickname, "Error, the game has not been initialized");
    return;
  }
  if (ttt->verify_player(nickname)){
    send_notification(nickname, "Error, you are a player, you cannot be a viewer");
    return;
  }
  if (auto viewer = find(ttt_viewers.begin(), ttt_viewers.end(), nickname) != ttt_viewers.end()){
    send_notification(nickname, "Error, you are already a viewer");
    return;
  }
  ttt_viewers.push_back(nickname);
  send_notification(nickname, "Success, you can now view the board");

  ostringstream replay_message, size_board, size_player_x, size_player_o;
  string board = ttt->get_board();
  string player_x= ttt->get_player_x();
  string player_o= ttt->get_player_o();

  size_board << setw(2) << setfill('0') << board.size();
  size_player_x << setw(2) << setfill('0') << player_x.size();
  size_player_o << setw(2) << setfill('0') << player_o.size();
  replay_message << 'V' << size_player_x.str() << player_x << size_player_o.str() << player_o << size_board.str() << board;
  int n = send(ConnectFD, replay_message.str().c_str(), strlen(replay_message.str().c_str()), 0);
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Message replay to %s: [%s]\n", nickname.c_str(), replay_message.str().c_str());
}

void wrt_vec(vector<unsigned char> &vec, string data){
  for (auto c: data){
    vec.push_back((unsigned char)c);
  }
}

void send_board_state(){
  ostringstream replay_message, size_board, size_player_x, size_player_o;
  string board = ttt->get_board();
  string player_x= ttt->get_player_x();
  string player_o= ttt->get_player_o();

  size_board << setw(2) << setfill('0') << board.size();
  size_player_x << setw(2) << setfill('0') << player_x.size();
  size_player_o << setw(2) << setfill('0') << player_o.size();
  replay_message << 'V' << size_player_x.str() << player_x << size_player_o.str() << player_o << size_board.str() << board;
  
  int n = send(clientNames[ttt_players[0].first], replay_message.str().c_str(), strlen(replay_message.str().c_str()), 0);
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Message replay to %s: [%s]\n", ttt_players[0].first.c_str(), replay_message.str().c_str());

  n = send(clientNames[ttt_players[1].first], replay_message.str().c_str(), strlen(replay_message.str().c_str()), 0);
  if (n < 0)
    perror("ERROR writing to socket");
  printf("Message replay to %s: [%s]\n", ttt_players[1].first.c_str(), replay_message.str().c_str());

  for (int i= 0; i< ttt_viewers.size(); i++){
    n = send(clientNames[ttt_viewers[i]], replay_message.str().c_str(), strlen(replay_message.str().c_str()), 0);
    if (n < 0)
      perror("ERROR writing to socket");
    printf("Message replay to %s: [%s]\n", ttt_viewers[i].c_str(), replay_message.str().c_str());
  }
}

void reinitialize_game(){
  ttt = nullptr;
  ttt_players.clear();
  ttt_viewers.clear();
  rand_num = rand() % 2;
}