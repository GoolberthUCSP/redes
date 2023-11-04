// Redes y Comunicación
// Socket UDP Primary Server

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
#include <set>
#include "cache.h"
#include "lib.h"

#define SIZE 1024

using namespace std;

int primaryFD;
int seq_number = 0;
int msg_id = 0;
Cache packets(100);
set<string> acks_to_recv;
set<string> storage_nicknames= {"storage0", "storage1", "storage2", "storage3"};

struct sockaddr_in primary_addr, connect_addr;
map<string, struct sockaddr_in> connects_addr; // nickname, addr

void verify_storage(int n_storage);
void send_message(Header &header, string type, vector<unsigned char> data);
void resend_packet(Header &header);
void process_ack(Header &header);

typedef void (*func_ptr)(Header &, vector<unsigned char>);
map<char, func_ptr> to_storage_functions({
});
map<char, func_ptr> to_client_functions({ 
});

int main(){
    int bytes_readed;
    vector<unsigned char> recv_buffer(SIZE);
    int addr_len= sizeof(struct sockaddr_in);

    if ((primaryFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket");
        exit(1);
    }

    memset(&primary_addr, 0, sizeof(primary_addr));
    primary_addr.sin_family = AF_INET;
    primary_addr.sin_port = htons(5000);
    primary_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(primaryFD,(struct sockaddr *)&primary_addr, sizeof(struct sockaddr)) == -1){
        perror("Bind");
        exit(1);
    }
	cout << "UDPServer Waiting for client on port 5000..." << endl;

    while(true){
        memset(recv_buffer.data(), 0, SIZE);

        bytes_readed = recvfrom(primaryFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&connect_addr, (socklen_t *)&addr_len);
        
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(connect_addr.sin_addr) << ":" << ntohs(connect_addr.sin_port) << endl;
        
        stringstream ss;
        ss.write((char *)recv_buffer.data(), recv_buffer.size());
        // ss : seq_num|hash|type|msg_id|flag|<data>
        string seq_num(2, 0), hash(6, 0), type(1, 0), msg_id(3, 0), flag(1, 0), nick_size(2, 0);
        ss.read(seq_num.data(), seq_num.size());
        ss.read(hash.data(), hash.size());
        ss.read(type.data(), type.size());
        ss.read(msg_id.data(), msg_id.size());
        ss.read(flag.data(), flag.size());
        ss.read(nick_size.data(), nick_size.size());
        string nickname(stoi(nick_size), 0);
        ss.read(nickname.data(), nickname.size());

        vector<unsigned char> data(recv_buffer.size() - ss.tellg());
        ss.read((char *)data.data(), data.size());
        
        if (connects_addr.find(nickname) == connects_addr.end()){ // Validation if nickname is saved
            connects_addr[nickname] = connect_addr;
        }

        Header header(nickname, seq_num);
        if (type == "A"){
            process_ack(header);
            continue;
        }
        
        // Push packet into packets(Cache) if it's not corrupted, else send NAK
        bool is_good= (hash == calc_hash(data))? true : false; // Calc hash only to data, without header
        send_message(header, "A", {});
        if (!is_good) // If packet is corrupted send NAK= 2 ACKs
            send_message(header, "A", {});
        else
            if (storage_nicknames.find(nickname) == storage_nicknames.end()) // If request is for storage
                thread(to_storage_functions[type[0]], ref(header), recv_buffer).detach();
            else // If request is for client
                thread(to_client_functions[type[0]], ref(header), recv_buffer).detach();
    }
}

void verify_storage(int n_storage){
    return;
}

void process_ack(Header &header){
    if (acks_to_recv.find(header.seq_num) == acks_to_recv.end()){
        // If it's not in acks_to_recv, it means that it's second ACK with the same seq_num = NAK
        resend_packet(header);
    }
    else
        acks_to_recv.erase(header.seq_num);
}

void send_message(Header &header, string type, vector<unsigned char> data){
    // packet: seq_num|hash|type|msg_id|flag|<data>
    return;
}

void resend_packet(Header &header){
    vector<unsigned char> packet = packets.get(stoi(header.seq_num));
    sendto(primaryFD, packet.data(), packet.size(), MSG_CONFIRM, (struct sockaddr *)&connects_addr[header.nickname], sizeof(struct sockaddr));
}