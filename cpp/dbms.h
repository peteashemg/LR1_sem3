#ifndef DBMS_H
#define DBMS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <queue>
using namespace std;

enum Kind { ARR = 1, FLIST, LLIST, STACK, QUEUE, TREE };
struct Node {
    string val;
    Node* next = nullptr;
    Node* prev = nullptr;
};

struct TNode {
    string val;
    TNode* left = nullptr;
    TNode* right = nullptr;
};

struct Container {
    int kind = 0;
    vector<string> arr; // массив
    Node* head = nullptr; // список: голова; стек: вершина; очередь: начало
    Node* tail = nullptr; // хвост
    TNode* root = nullptr;// дерево
    int size = 0;
};

typedef map<string, Container> Db;

bool loadDb(Db& db, const string& file); // чтение из файла
void saveDb(const Db& db, const string& file); // запись в файл
string execute(Db& db, const string& query);// выполнить запрос
void clearDb(Db& db);// освободить память
bool isComplete(TNode* root); // проверка Complete Binary Tree

#endif
