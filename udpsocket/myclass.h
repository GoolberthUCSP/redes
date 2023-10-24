#ifndef MYCLASS_H
#define MYCLASS_H

#include <iostream>

struct MyClass {
    int num;
    long longint;
    char str[10]= "Goolberth";
    friend std::ostream& operator<<(std::ostream& os, const MyClass& obj){
        os << obj.num << " " << obj.longint << " " << obj.str;
        return os;
    }
};

#endif // MYCLASS_H