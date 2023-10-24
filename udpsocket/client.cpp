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

struct Server{
    int socketFD;
    struct sockaddr_in server_addr;
    Server(int &socketFD, struct sockaddr_in &server_addr) : socketFD(socketFD), server_addr(server_addr) {}
};

vector<unsigned char> encoding(string &buffer);
void decoding(vector<unsigned char> buffer);
void thread_receiver(Server &server);
// Send functions
vector<unsigned char> send_message(stringstream &ss);
vector<unsigned char> send_nickname(stringstream &ss);
vector<unsigned char> send_struct(stringstream &ss);
// Receive functions
void recv_message(stringstream &ss);

typedef vector<unsigned char> (*send_ptr)(stringstream&);
typedef void (*recv_ptr)(stringstream&);

map<string, send_ptr> send_functions({
    {"soy", &send_nickname},
    {"send", &send_message},
    {"struct", &send_struct}
});

map<char, recv_ptr> recv_functions({
    {'M', &recv_message}
});

int main(){
    int socketFD;
    struct sockaddr_in server_addr;
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

    Server server(socketFD, server_addr);

    thread(thread_receiver, ref(server)).detach();

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
            cout << "Bad input" << endl;
            continue;
        }
        sendto(socketFD, encoded.data(), encoded.size(), 0, (struct sockaddr *)&server_addr, addr_len);
    }
}

void thread_receiver(Server &server){
    vector<unsigned char> recv_buffer(SIZE);
    int bytes_readed, addr_len= sizeof(struct sockaddr_in);
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(server.socketFD, recv_buffer.data(), SIZE, 0, (struct sockaddr *)&server.server_addr, (socklen_t *)&addr_len);
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
    string out_str = "M" + size_receiver.str() + receiver + size_message.str() + message;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

vector<unsigned char> send_nickname(stringstream &ss){
    // ss :nickname
    string nickname;
    getline(ss, nickname, '\0');
    ostringstream size;
    size << setw(2) << setfill('0') << nickname.size();
    string out_str = "N" + size.str() + nickname;
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

void recv_message(stringstream &ss){
    // ss : 000message
    string size(3, '0');
    ss.read(size.data(), size.size());
    string message(stoi(size), '0');
    ss.read(message.data(), message.size());
    cout << "Message received: " << message << endl;    
}