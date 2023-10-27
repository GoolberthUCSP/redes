// Redes y Comunicación
// Socket UDP Storage Server

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
#include <algorithm>

#define SIZE 1024

using namespace std;

int socketFD;
struct sockaddr_in primary_addr, this_addr;
string nickname;

map<string, vector<string>> database; // database[node]: {nodes that are connected to node}
vector<int> storage_ports= {5001, 5002, 5003, 5004};
vector<string> storage_nicknames= {"00", "01", "02", "03"};
int nick_size = 2;

typedef void (*func_ptr)(stringstream&);

// CRUD request functions
void create_request(stringstream &ss);
void read_request(stringstream &ss);
void update_request(stringstream &ss);
void delete_request(stringstream &ss);

map<char, func_ptr> crud_requests({
    {'C', &create_request},
    {'R', &read_request},
    {'U', &update_request},
    {'D', &delete_request}
});

int main(int argc, char *argv[]){
    if (argc != 2){
        cout << "Bad number of arguments" << endl;
        return 1;
    }
    int port = storage_ports[atoi(argv[1])%4];
    nickname = storage_nicknames[atoi(argv[1])%4];

    int bytes_readed;
    vector<unsigned char> recv_buffer(SIZE);

    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Storage: socket");
        exit(1);
    }
    // Define the primary server's address
    memset(&primary_addr, 0, sizeof(primary_addr));
    primary_addr.sin_family = AF_INET;
    primary_addr.sin_port = htons(5000);
    if (inet_pton(AF_INET, "127.0.0.1", &primary_addr.sin_addr) == -1){
        perror("Storage: inet_pton");
        exit(1);
    }
    // Define this server's address
    memset(&this_addr, 0, sizeof(this_addr));
    this_addr.sin_family = AF_INET;
    this_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &this_addr.sin_addr) == -1){
        perror("Storage: inet_pton");
        exit(1);
    }
    // Bind this server's address to the socket
    if (bind(socketFD, (struct sockaddr *)&this_addr, sizeof(struct sockaddr)) == -1){
        perror("Storage: bind");
        exit(1);
    }
    // Register in principal server
    // sendto primary_addr : nickname

	cout << "UDPServer Waiting for primary socket on port " << port << "..." << endl;
    
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(socketFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&primary_addr, (socklen_t *)sizeof(struct sockaddr_in));
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(primary_addr.sin_addr) << ":" << ntohs(primary_addr.sin_port) << endl;
        // Process the header and then the data
        thread(processing, recv_buffer).detach();
    }
}


void processing(vector<unsigned char> buffer){
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    char type;
    ss >> type;
    if (crud_requests.find(type) == crud_requests.end()){
        cout << "Bad type" << endl;
        return;
    }
    crud_requests[type](ss);
}