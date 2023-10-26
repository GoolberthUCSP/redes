#ifndef CACHE_H
#define CACHE_H

#include <iostream>
#include <unordered_map>
#include <list>
#include <utility>
const int CACHE_SIZE = 100000; //Tamaño de la caché

using namespace std;

template<class T>
struct Cache{
    int size; //Tamaño de la caché
    list<pair<int, T>> cache_list; //Lista de elementos de la caché
    unordered_map<int, list<pair<int, T>>::iterator> cache_map; //Mapa de elementos de la caché
    Cache(int size = CACHE_SIZE){
        this->size = size;
        cache_list.clear();
        cache_map.clear();
    }
    ~Cache(){
        cache_list.clear();
        cache_map.clear();
    }
    //Insertar un elemento en la caché
    void insert(int key, T value){
        //Si la caché está llena, eliminar el primer elemento
        if (cache_list.size() == size){
            int key_to_delete = cache_list.back().first;
            cache_list.pop_back();
            cache_map.erase(key_to_delete);
        }
        //Insertar el elemento en la caché
        cache_list.push_front(make_pair(key, value));
        cache_map[key] = cache_list.begin();
    }
    //Obtener un elemento de la caché
    T get(int key){
        //Si el elemento no está en la caché, devolver NULL
        if (cache_map.find(key) == cache_map.end())
            return -1;
        //Si el elemento está en la caché, devolver su valor
        auto it = cache_map[key];
        T value = it->second;
        cache_list.erase(it);
        cache_list.push_front(make_pair(key, value));
        cache_map[key] = cache_list.begin();
        return value;
    }
    //Sobrecarga del operador <<
    friend ostream& operator<<(ostream& os, const Cache& cache){
        for (auto it = cache.cache_list.begin(); it != cache.cache_list.end(); it++){
            os << "Newest -> " << it->first << " - " << it->second << " -> Oldest\n";
        }
        return os;
    }
    //Sobrecarga del operador >>
    friend istream& operator>>(istream& is, Cache& cache){
        int key;
        T value;
        is >> key >> value;
        cache.insert(key, value);
        return is;
    }
};

#endif //CACHE_H