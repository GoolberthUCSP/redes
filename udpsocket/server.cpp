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

#define SIZE 1024

using namespace std;

map<string, struct sockaddr_in> online_clients;

void processing(int &socketFD, string buffer, struct sockaddr_in &client_addr);
void send_message(int &socketFD, stringstream &ss);
void save_client(int &socketFD, stringstream &ss, struct sockaddr_in client_addr);

int main(){
    int bytes_readed;
    char recv_buffer[SIZE];
    struct sockaddr_in server_addr , client_addr;
    int addr_len= sizeof(struct sockaddr_in);
    int socketFD;

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
        memset(&recv_buffer, 0, sizeof(recv_buffer));
        bytes_readed = recvfrom(socketFD, recv_buffer, SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
        processing(socketFD, recv_buffer, client_addr);
    }
}


void processing(int &socketFD, string buffer, struct sockaddr_in &client_addr){
    stringstream ss(buffer);
    char type;
    ss >> type;
    if (type == 'N'){
        // N00nickname
        save_client(socketFD, ss, client_addr);        
    }
    else if (type == 'M'){
        // M00receiver000message
        send_message(socketFD, ss);
    }      
}

void send_message(int &socketFD, stringstream &ss){
    // ss : 00receiver000message
    string size_rcv(2, '0');
    ss.read(size_rcv.data(), size_rcv.size());
    string receiver(stoi(size_rcv), '0');
    ss.read(receiver.data(), receiver.size());

    string size_msg(3, '0');
    ss.read(size_msg.data(), size_msg.size());
    string message(stoi(size_msg), '0');
    ss.read(message.data(), message.size());

    if (online_clients.find(receiver) == online_clients.end()){ // Validation if receiver is online
        cout << "User " << receiver << " is not online. Message not sent" << endl;
        return;
    }
    string msg = "M" + size_msg + message;
    sendto(socketFD, msg.c_str(), msg.size(), MSG_CONFIRM, (struct sockaddr *)&online_clients[receiver], sizeof(struct sockaddr));
    cout << "Message sent to " << receiver << ": " << message << endl;
}

void save_client(int &socketFD, stringstream &ss, struct sockaddr_in client_addr){
    // ss : 00nickname
    string nick_size(2, '0');
    ss.read(nick_size.data(), nick_size.size());
    string nickname(stoi(nick_size), '0');
    ss.read(nickname.data(), nickname.size());
    online_clients[nickname] = client_addr;
    cout << "Client saved: " << nickname << endl;
}