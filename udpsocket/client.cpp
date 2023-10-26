// Fredy Goolberth Quispe Neira
// Socket UDP Client

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
#include <chrono>
#include <ctime>
#include <functional>
#include <map>
#include "myclass.h"

#define SIZE 1024

using namespace std;

string nickname;
string nick_size;
int socketFD;
struct sockaddr_in server_addr;
// Game: stone, paper, scissors
map<string,int> game_state{
    {"piedra", 0},
    {"papel", 1},
    {"tijera", 2}
};

vector<unsigned char> encoding(string &buffer);
void decoding(vector<unsigned char> buffer);
void thread_receiver();
// Send functions
vector<unsigned char> send_message(stringstream &ss);
vector<unsigned char> send_struct(stringstream &ss);
vector<unsigned char> send_game(stringstream &ss);
// Receive functions
void recv_message(stringstream &ss);
void recv_notification(stringstream &ss);

typedef vector<unsigned char> (*send_ptr)(stringstream&);
typedef void (*recv_ptr)(stringstream&);

map<string, send_ptr> send_functions({
    {"send", &send_message},
    {"struct", &send_struct},
    {"game", &send_game}
});

map<char, recv_ptr> recv_functions({
    {'M', &recv_message},
    {'N', &recv_notification}
});

int main(){
    char recv_buffer[SIZE];
    int addr_len= sizeof(struct sockaddr_in);

    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) == -1){
        perror("inet_pton");
        exit(1);
    }

    cout << "Ingrese su nickname: ";
    getline(cin, nickname);
    cin.clear();
    ostringstream sz;
    sz << setw(2) << setfill('0') << nickname.size();
    nick_size = sz.str();

    thread(thread_receiver).detach();

    while(true){ // Wait for user input, encoding and sending to server
        string usr_input;
        getline(cin, usr_input);
        cin.clear();

        if (usr_input == "exit" || usr_input == "quit"){
            system("clear || cls");
            exit(EXIT_SUCCESS);
        } else if (usr_input == "clear" || usr_input == "cls"){
            system("clear || cls");
            continue;
        }
        
        vector<unsigned char> encoded = encoding(usr_input);
        if (encoded.size() == 0){
            cout << "Bad input, try again" << endl;
            continue;
        }
        sendto(socketFD, encoded.data(), encoded.size(), 0, (struct sockaddr *)&server_addr, addr_len);
    }
}

void thread_receiver(){
    vector<unsigned char> recv_buffer(SIZE);
    int bytes_readed, addr_len= sizeof(struct sockaddr_in);
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(socketFD, recv_buffer.data(), SIZE, 0, (struct sockaddr *)&server_addr, (socklen_t *)&addr_len);
        decoding(recv_buffer);
    }   
}

void decoding(vector<unsigned char> buffer){
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    char type;
    ss >> type;
    
    if (recv_functions.find(type) == recv_functions.end()){
        cout << "Bad type" << endl;
        return;
    }
    recv_functions[type](ss);
}

vector<unsigned char> encoding(string &buffer){
    stringstream ss(buffer);
    string action;
    vector<unsigned char> encoded;
    getline(ss, action, ' ');
    transform(action.begin(), action.end(), action.begin(), ::tolower);

    if (send_functions.find(action) == send_functions.end()){
        return {};
    }
    encoded = send_functions[action](ss);
    return encoded;
}

vector<unsigned char> send_message(stringstream &ss){
    // ss : receiver message
    string receiver, message;
    getline(ss, receiver, ' ');
    getline(ss, message, '\0');
    ostringstream size_receiver, size_message;
    size_receiver << setw(2) << setfill('0') << receiver.size();
    size_message << setw(3) << setfill('0') << message.size();
    string out_str = "M" + nick_size + nickname + size_receiver.str() + receiver + size_message.str() + message;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

vector<unsigned char> send_struct(stringstream &ss){
    // ss : number long
    string number, longint;
    getline(ss, number, ' ');
    getline(ss, longint, '\0');
    MyClass myclass;
    myclass.num = stoi(number);
    myclass.longint = stol(longint);
    string struct_str(sizeof(myclass), '0');
    memcpy(struct_str.data(), &myclass, sizeof(myclass));
    struct_str= "P" + struct_str;
    vector<unsigned char> out(struct_str.size()+1);
    copy(struct_str.begin(), struct_str.end(), out.begin());
    return out;
}

vector<unsigned char> send_game(stringstream &ss){
    // ss : state
    string state;
    getline(ss, state, '\0');
    if (game_state.find(state) == game_state.end()){
        cout << "Bad state" << endl;
        return {};
    }
    string out_str = "G" + nick_size + nickname + state;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

void recv_message(stringstream &ss){
    // ss : 00sender000message
    string size_sdr(2, '0'), size_msg(3, '0');
    ss.read(size_sdr.data(), size_sdr.size());
    string sender(stoi(size_sdr), '0');
    ss.read(sender.data(), sender.size());
    ss.read(size_msg.data(), size_msg.size());
    string message(stoi(size_msg), '0');
    ss.read(message.data(), message.size());
    cout << "Message received from " << sender << ": " << message << endl;    
}

void recv_notification(stringstream &ss){
    // ss : 00notification
    string size(2, '0');
    ss.read(size.data(), size.size());
    string notification(stoi(size), '0');
    ss.read(notification.data(), notification.size());
    cout << "Notification received: " << notification << endl;
}