// Redes y Comunicación
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

#define SIZE 1024

using namespace std;

// Server variables
int socketFD;
struct sockaddr_in server_addr;

vector<unsigned char> encoding(string &buffer);
void decoding(vector<unsigned char> buffer);
void thread_receiver();
// CRUD request functions
vector<unsigned char> create_request(stringstream &ss);
vector<unsigned char> read_request(stringstream &ss);
vector<unsigned char> update_request(stringstream &ss);
vector<unsigned char> delete_request(stringstream &ss);
// Receive functions
void read_response(stringstream &ss);
void recv_notification(stringstream &ss);
void recv_ack(stringstream &ss);

// Automatic response functions
void send_ack(stringstream &ss);

typedef vector<unsigned char> (*req_ptr)(stringstream&);
typedef void (*recv_ptr)(stringstream&);

map<string, req_ptr> request_functions({
    {"create", &create_request},
    {"read", &read_request},
    {"update", &update_request},
    {"delete", &delete_request}
});

map<char, recv_ptr> recv_functions({
    {'R', &read_response},
    {'N', &recv_notification},
    {'A', &recv_ack}
});

int main(){
    int addr_len= sizeof(struct sockaddr_in);
    vector<unsigned char> send_buffer(SIZE);

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

    thread(thread_receiver).detach();

    while(true){ // Wait for user input, encoding and sending to server
        memset(send_buffer.data(), '-', SIZE);
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
        copy(encoded.begin(), encoded.end(), send_buffer.begin());
        sendto(socketFD, send_buffer.data(), SIZE, 0, (struct sockaddr *)&server_addr, addr_len);
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

    if (request_functions.find(action) == request_functions.end()){
        return {};
    }
    encoded = request_functions[action](ss);
    return encoded;
}


// CRUD request functions
vector<unsigned char> create_request(stringstream &ss){
    // ss : node1 node2
    // out : C00node100node2
    string node1, node2;
    getline(ss, node1, ' ');
    getline(ss, node2, '\0');
    ostringstream size1, size2;
    size1 << setw(2) << setfill('0') << node1.size();
    size2 << setw(2) << setfill('0') << node2.size();
    string out_str = "C" + size1.str() + node1 + size2.str() + node2;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

vector<unsigned char> read_request(stringstream &ss){
    // ss : node
    // out : R00node
    string node;
    getline(ss, node, '\0');
    ostringstream size;
    size << setw(2) << setfill('0') << node.size();
    string out_str = "R" + size.str() + node;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

vector<unsigned char> update_request(stringstream &ss){
    // ss : node1 node2 new1 new2
    // out : U00node100node200new100new2
    string node1, node2, new1, new2;
    getline(ss, node1, ' ');
    getline(ss, node2, ' ');
    getline(ss, new1, ' ');
    getline(ss, new2, '\0');
    ostringstream size1, size2, size3, size4;
    size1 << setw(2) << setfill('0') << node1.size();
    size2 << setw(2) << setfill('0') << node2.size();
    size3 << setw(2) << setfill('0') << new1.size();
    size4 << setw(2) << setfill('0') << new2.size();
    string out_str = "U" + size1.str() + node1 + size2.str() + node2 + size3.str() + new1 + size4.str() + new2;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

vector<unsigned char> delete_request(stringstream &ss){
    // ss : node
    // out : D00node
    string node;
    getline(ss, node, '\0');
    ostringstream size;
    size << setw(2) << setfill('0') << node.size();
    string out_str = "D" + size.str() + node;
    vector<unsigned char> out(out_str.size());
    copy(out_str.begin(), out_str.end(), out.begin());
    return out;
}

// Receive functions

void read_response(stringstream &ss){
    // ss : 00node000node1,node2,etc
    string node_size(2, '0'), nodes_size(3, '0');

    ss.read(node_size.data(), node_size.size());
    string node(stoi(node_size), '0');
    ss.read(node.data(), node.size());

    ss.read(nodes_size.data(), nodes_size.size());
    string nodes(stoi(nodes_size), '0');
    ss.read(nodes.data(), nodes.size());
    cout << "Read response: " << node << "->" << nodes << endl;
}

void recv_notification(stringstream &ss){
    // ss : 00notification
    string size(2, '0');

    ss.read(size.data(), size.size());
    string notification(stoi(size), '0');
    ss.read(notification.data(), notification.size());
    cout << "Notification received: " << notification << endl;
}

void recv_ack(stringstream &ss){
    // ss : ack_number(3 bytes)
    string ack_number(3, '0');
    ss.read(ack_number.data(), ack_number.size());
    cout << "Ack received: " << ack_number << endl;
}