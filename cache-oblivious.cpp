//
// Created by Asus on 07/05/2026.
//
#include <iostream>
#include <vector>

using namespace std;

class Node{
};

int main() {

    // Crear vectores
    vector<int> vec10k;
    vector<int> vec100k;
    vector<int> vec1M;

    // Insertar valores secuencialmente en vector de 10k
    for (int i = 0; i < 10000; i++) {
        vec10k.push_back(i);
    }

    // Insertar valores secuencialmente en vector de 100k
    for (int i = 0; i < 100000; i++) {
        vec100k.push_back(i);
    }

    // Insertar valores secuencialmente en vector de 1M
    for (int i = 0; i < 1000000; i++) {
        vec1M.push_back(i);
    }

    cout << "Vectores llenados correctamente." << endl;

    return 0;
}