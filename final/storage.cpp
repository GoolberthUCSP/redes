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
#include <set>
#include "cache.h"
#include "lib.h"


#define SIZE 1024

using namespace std;

int socketFD;
struct sockaddr_in primary_addr, this_addr;
string nickname;
int seq_number = 0;
int msg_id = 0;

Cache packets(100);
set<string> acks_to_recv;
map<string, set<string>> database; // database[node]: {nodes that are connected to node}
vector<int> storage_ports= {5001, 5002, 5003, 5004};
vector<string> storage_nicknames= {"storage0", "storage1", "storage2", "storage3"};
string nick_size = "08";

// CRUD request functions
void create_request(vector<unsigned char> data);
void read_request(vector<unsigned char> data);
void update_request(vector<unsigned char> data);
void delete_request(vector<unsigned char> data);

void processing(vector<unsigned char> buffer);
void send_message(string type, string data);
void resend_packet(string seq_num);
void process_ack(string seq_num);
void replay_ack(string seq_num);


typedef void (*func_ptr)(vector<unsigned char>);
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
        thread(processing, recv_buffer).detach();
    }
}


void processing(vector<unsigned char> buffer){
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    // ss : seq_num|hash|type|msg_id|flag|nick_size|nicknake|<data>
    string seq_num(2, 0), hash(6, 0), type(1, 0), msg_id(3, 0), flag(1, 0), nick_size(2, 0);
    ss.read(seq_num.data(), seq_num.size());
    ss.read(hash.data(), hash.size());
    ss.read(type.data(), type.size());
    ss.read(msg_id.data(), msg_id.size());
    ss.read(flag.data(), flag.size());
    ss.read(nick_size.data(), nick_size.size());
    string nickname(stoi(nick_size), 0);
    ss.read(nickname.data(), nickname.size());

    vector<unsigned char> data(buffer.size() - ss.tellg());
    ss.read((char *)data.data(), data.size());
    
    if (type == "A"){
        process_ack(seq_num);
        return;
    }
    
    // Push packet into packets(Cache) if it's not corrupted, else send NAK
    bool is_good= (hash == calc_hash(data))? true : false; // Calc hash only to data, without header
    replay_ack(seq_num);
    if (!is_good) 
        replay_ack(seq_num);
    else
        thread(crud_requests[type[0]], data).detach();
}


void send_message(string type, string data){
    return;
}

void resend_packet(string seq_num){
    return;
}

void process_ack(string seq_num){
    if (acks_to_recv.find(seq_num) == acks_to_recv.end())
        resend_packet(seq_num);
    else
        acks_to_recv.erase(seq_num);
}

void replay_ack(string seq_num){
    vector<unsigned char> packet(SIZE);
    memset(packet.data(), '-', SIZE);
    string header = seq_num + "000000" + "A" + "000" + "0" + nick_size + nickname;
    copy(header.begin(), header.end(), packet.begin());
    sendto(socketFD, packet.data(), packet.size(), MSG_CONFIRM, (struct sockaddr *)&primary_addr, sizeof(struct sockaddr));
}


// CRUD functions
void create_request(vector<unsigned char> data){
    // data : 00node100node2
    stringstream ss;
    ss.write((char *)data.data(), data.size());
    string size1(2, 0), size2(2, 0);
    ss.read(size1.data(), size1.size());
    string node1(stoi(size1), 0);
    ss.read(node1.data(), node1.size());

    ss.read(size2.data(), size2.size());
    string node2(stoi(size2), 0);
    ss.read(node2.data(), node2.size());
    
    database[node1].insert(node2);
    //Send notification of success
}
void read_request(vector<unsigned char> data){
    // data : 00node
    stringstream ss;
    ss.write((char *)data.data(), data.size());
    string size(2, 0);
    ss.read(size.data(), size.size());
    string node(stoi(size), 0);
    ss.read(node.data(), node.size());
    
    if (database.find(node) == database.end()){
        // Send notification of failure: node doesn't exist
        return;
    }
    //Return response to primary server
}

void update_request(vector<unsigned char> data){
    // data : 00node100node200new2
    stringstream ss;
    ss.write((char *)data.data(), data.size());
    string size1(2, 0), size2(2, 0), size3(2, 0);
    ss.read(size1.data(), size1.size());
    string node1(stoi(size1), 0);
    ss.read(node1.data(), node1.size());

    ss.read(size2.data(), size2.size());
    string node2(stoi(size2), 0);
    ss.read(node2.data(), node2.size());

    ss.read(size3.data(), size3.size());
    string new2(stoi(size3), 0);
    ss.read(new2.data(), new2.size());

    if (database.find(node1) == database.end()){
        // Send notification of failure: node doesn't exist
        return;
    }

    database[node1].erase(node2);
    database[node1].insert(new2);
    //Send notification of success
}

void delete_request(vector<unsigned char> data){
    // data : 00node
    stringstream ss;
    ss.write((char *)data.data(), data.size());
    string size(2, 0);
    ss.read(size.data(), size.size());
    string node(stoi(size), 0);
    ss.read(node.data(), node.size());
    
    if (database.find(node) == database.end()){
        // Send notification of failure: node doesn't exist
        return;
    }

    database.erase(node);
    //Send notification of success
}