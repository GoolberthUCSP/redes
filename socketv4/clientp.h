#ifndef CLIENTP_H
#define CLIENTP_H

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
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <random>
#include <utility>
#include <algorithm>

#define SIZE 200000

using namespace std;

struct Clientp{ //For each client
    int ConnectFD;
    string nickname;
    stringstream ss;
    char recv_size[11];
    char type;
    bool disconnected;
    mutex mtx, mtx2;
    unsigned char tmp_recv[SIZE];
    vector<unsigned char> recv_buffer;
    vector<unsigned char> send_buffer;

    Clientp(int ConnectFD, string nickname){
        this->ConnectFD = ConnectFD;
        this->nickname = nickname;
        this->disconnected = false;
    }

    void send_response(string send_str){ //Send response to client
        vector<unsigned char> os;
        os.insert(os.begin(), send_str.begin(), send_str.end());
        send_response(os);
    }
    void send_response(vector<unsigned char> send_vect){ //Send response to client : ONLY USED TO SEND FILE TO CLIENT
        
        mtx.lock();
        send_buffer.clear();

        ostringstream send_buff_size;
        send_buff_size << setw(10) << setfill('0') << send_vect.size();
        string send_size = send_buff_size.str();
        send_buffer.insert(send_buffer.begin(), send_size.begin(), send_size.end());
        send_buffer.insert(send_buffer.end(), send_vect.begin(), send_vect.end());

        int bytes_sent = 0, bytes_to_send = send_buffer.size();
        while (bytes_to_send > 0){
            bytes_sent = send(ConnectFD, send_buffer.data() + bytes_sent, bytes_to_send, 0);
            if (bytes_sent < 0)
                perror("ERROR writing to socket");
            bytes_to_send -= bytes_sent;
        }
        mtx.unlock();
    }
    void recv_request(){ //Receive request from client

        mtx2.lock();
        recv_buffer.clear();
        memset(recv_size, 0, 11);

        int n = recv(ConnectFD, recv_size, 10, 0);
        if (n < 0)
            perror("ERROR reading from socket");
        int resp_size = stoi(recv_size);
        while(resp_size > 0){
            memset(tmp_recv, 0, SIZE);
            n = recv(ConnectFD, tmp_recv, resp_size, 0);
            if (n < 0)
                perror("ERROR reading from socket");
            
            recv_buffer.insert(recv_buffer.end(), tmp_recv, tmp_recv+n);
            resp_size -= n;
        }
        // Load recv_buffer into ss stringstream
        ss.clear();
        ss.write((char*)recv_buffer.data(), recv_buffer.size());
        mtx2.unlock();
    }

    void send_notification(string notification){
        // out: N00notification
        ostringstream size_notification;
        size_notification << setw(2) << setfill('0') << notification.size();
        string send_str = "N" + size_notification.str() + notification;
        send_response(send_str);
    }
};

#endif