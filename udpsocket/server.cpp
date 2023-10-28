// Fredy Goolberth Quispe Neira
// Socket UDP Server

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
#include <thread>
#include "myclass.h"

#define SIZE 1024

using namespace std;

int socketFD;
map<string, struct sockaddr_in> online_clients;
State *first_gamer = nullptr;

typedef void (*func_ptr)(stringstream&, string);

void processing(vector<unsigned char> buffer, struct sockaddr_in client_addr);
void send_message(stringstream &ss, string sender);
void process_struct(stringstream &ss, string sender);
void process_game(stringstream &ss, string sender);
void send_notification(string nickname, string message);

map<char, func_ptr> functions({
    {'M', &send_message},
    {'P', &process_struct},
    {'G', &process_game}
});

int main(){
    int bytes_readed;
    vector<unsigned char> recv_buffer(SIZE);
    struct sockaddr_in server_addr , client_addr;
    int addr_len= sizeof(struct sockaddr_in);

    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketFD,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
        perror("Bind");
        exit(1);
    }
	cout << "UDPServer Waiting for client on port 5000..." << endl;
    
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(socketFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
        thread(processing, recv_buffer, client_addr).detach();
    }
}


void processing(vector<unsigned char> buffer, struct sockaddr_in client_addr){
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    char type;
    ss >> type;
    if (functions.find(type) == functions.end()){
        cout << "Bad type" << endl;
        return;
    }
    string size_sdr(2, '0'), size_rcv(2, '0'), size_msg(3, '0');
    ss.read(size_sdr.data(), size_sdr.size());
    string sender(stoi(size_sdr), '0');
    ss.read(sender.data(), sender.size());

    if (online_clients.find(sender) == online_clients.end()){ // Validation if sender is not registered
        online_clients[sender] = client_addr;
        cout << "Client registered: " << sender << endl;
    }

    functions[type](ss, sender);
}

void send_message(stringstream &ss, string sender){
    // ss : 00receiver000message
    string size_rcv(2, '0'), size_msg(3, '0');
    
    ss.read(size_rcv.data(), size_rcv.size());
    string receiver(stoi(size_rcv), '0');
    ss.read(receiver.data(), receiver.size());

    ss.read(size_msg.data(), size_msg.size());
    string message(stoi(size_msg), '0');
    ss.read(message.data(), message.size());

    if (online_clients.find(receiver) == online_clients.end()){ // Validation if receiver is online
        cout << "User " << receiver << " is not online. Message not sent" << endl;
        return;
    }
    ostringstream size_sdr;
    size_sdr << setw(2) << setfill('0') << sender.size();
    string msg = "M" + size_sdr.str() + sender + size_msg + message;
    sendto(socketFD, msg.c_str(), msg.size(), MSG_CONFIRM, (struct sockaddr *)&online_clients[receiver], sizeof(struct sockaddr));
    cout << "Message sent to " << receiver << ": " << message << endl;
}

void process_struct(stringstream &ss, string sender){
    // ss : data_struct
    int size= sizeof(MyClass);
    vector<unsigned char> data(size);
    ss.read((char *)data.data(), size);
    MyClass myclass;
    memcpy(&myclass, data.data(), size);
    cout << "Struct received: " << myclass << endl;
}

void process_game(stringstream &ss, string sender){
    // ss : (6B state)
    
    string state(6, '0');
    ss.read(state.data(), state.size());

    if (state == "papel_") state = "papel";
    
    if (first_gamer == nullptr){
        first_gamer = new State(sender, state);
        send_notification(sender, "Done, wait for the next game.");
    }
    else {
        if (state == first_gamer->state){
            string out_str= "It's a draw.";
            send_notification(sender, out_str);
            send_notification(first_gamer->nickname, out_str);
        }
        else {
            State second(sender, state);
            State winner = first_gamer->winner(second);
            State loser = first_gamer->loser(second);
            string out_str = "Winner:" + winner.str() + ", Loser:" + loser.str();
            send_notification(winner.nickname, out_str); 
            send_notification(loser.nickname, out_str);
        }
        first_gamer = nullptr;
    }
}

void send_notification(string nickname, string message){
    // out : N00message
    ostringstream size;
    size << setw(2) << setfill('0') << message.size();
    string out_str = "N" + size.str() + message;
    sendto(socketFD, out_str.c_str(), out_str.size(), MSG_CONFIRM, (struct sockaddr *)&online_clients[nickname], sizeof(struct sockaddr));
}