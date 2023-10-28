#ifndef MYCLASS_H
#define MYCLASS_H

#include <iostream>
#include <string>
#include <map>

using namespace std;

struct MyClass {
    int num;
    long longint;
    char str[10]= "Goolberth";
    friend std::ostream& operator<<(std::ostream& os, const MyClass& obj){
        os << obj.num << " " << obj.longint << " " << obj.str;
        return os;
    }
};

map<string, string> win_to{
    {"piedra", "tijera"},
    {"papel", "piedra"},
    {"tijera", "papel"}
};
struct State{
    string nickname;
    string state;
    State(string nickname, string state) : nickname(nickname), state(state) {}
    State winner(State other) {
        if (win_to[state] == other.state)
            return *this;
        else
            return other;
    }
    State loser(State other) {
        if (win_to[state] == other.state)
            return other;
        else
            return *this;
    }
    string str(){
        return nickname + "->" + state;
    }
};

#endif // MYCLASS_H