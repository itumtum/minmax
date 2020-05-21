#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <bitset>
#include <algorithm>
#include <string>

using namespace std;

extern size_t maxRecursion;
extern size_t maxCombinaties;

extern int AANTAL;
extern vector<vector<bool>> MAT;
extern vector<int> MIN;
extern int MAX;

string filename;

int aantalTrue(const vector<bool>& vec) {
    int grootte = 0;
    for (const bool& v : vec) {
        if (v) {
            grootte++;
        }
    }
    return grootte;
}

bool isAEenSubSetVanB(const vector<bool>& A, const vector<bool>& B) {
    bool subset = true;
    if (A == B) {
        return true;
    }
    if ((A.size() == B.size()) && (aantalTrue(A) < aantalTrue(B))) {
        for (size_t n = 0; n < A.size() && subset; n++) {
            if (A[n]) {
                subset = B[n];
            }
        }
    }
    else {
        return false;
    }
    return subset;
}

static size_t berekenCombinatie(double grootte, double totaal) {
    double result = 1;
    double tellerVan, tellerTot;
    double noemer;
    tellerVan = totaal;
    if ((totaal - grootte) > grootte) {
        tellerTot = (totaal - grootte) + 1;
        noemer = grootte;
    }
    else {
        tellerTot = grootte + 1;
        noemer = (totaal - grootte);
    }
    for (double i = 0; i <= (tellerVan - tellerTot); i++) {
        result = result * (tellerVan - i);
        if (i < noemer) {
            result = result / (noemer - i);
        }
    }
    return (size_t)result;
}

bool combinatieKleinerDan(double grootte, double totaal, double dezeWaarde) {
    bool kleinerDan = true;
    double result = 1;
    double tellerVan, tellerTot;
    double noemer;
    tellerVan = totaal;
    if ((totaal - grootte) > grootte) {
        tellerTot = (totaal - grootte) + 1;
        noemer = grootte;
    }
    else {
        tellerTot = grootte + 1;
        noemer = (totaal - grootte);
    }
    for (double i = 0; i <= (tellerVan - tellerTot) && kleinerDan; i++) {
        result = result * (tellerVan - i);
        if (i < noemer) {
            result = result / (noemer - i);
            kleinerDan = (result < dezeWaarde);
        }
    }
    return kleinerDan;
}


void coutBoolVector(vector<bool>& vec) {
    for (bool b : vec) {
        cout << (b ? "*" : "-");
    }
}

void printBoolVector(const vector<bool>& vec, ofstream& f) {
    for (size_t n = 0; n < vec.size(); n++) {
        if (vec[n]) {
            f << n + 1 << " ";
        }
    }
}

void printBoolVectorStars(vector<bool>& vec, ofstream& f) {
    for (bool b : vec) {
        f << (b ? "*" : "-");
    }
}

void unionVectors(const vector<bool>& v1, const vector<bool>& v2, vector<bool>& vUnion) {
    _ASSERT(v1.size() == v2.size());
    for (size_t nx = 0; nx < v1.size(); nx++) {
        vUnion[nx] = v1[nx] || v2[nx];
    }
}

bool intersectionExists(const vector<bool>& v1, const vector<bool>& v2) {
    _ASSERT(v1.size() == v2.size());
    bool intersect = false;
    for (size_t nx = 0; nx < v1.size() && !intersect; nx++) {
        intersect = (v1[nx] && v2[nx]);
    }
    return intersect;
}

vector<vector<int>>& leesInputFile(const string& filename, vector<vector<int>>& matrix) {
    vector<int> oneLine;
    std::ifstream inFile;
    inFile.open(filename);
    char test[1000];
    char num[10];
    if (!inFile.bad()) {
        matrix.clear();
        while (!inFile.fail()) {
            bool lineEnd = false;
            inFile.getline(&test[0], 1000);
            oneLine.clear();
            size_t x = 0;
            while (!lineEnd) {
                size_t c = 0;
                bool readNum = false;
                while ((int)test[x] >= (int)'0' && (int)test[x] <= (int)'9') {
                    readNum = true;
                    num[c++] = test[x++];
                }
                lineEnd = (test[x] == '\0');
                if (readNum) {
                    num[c] = '\0';
                    oneLine.push_back((int)_atoi64(num));
                }
                x++;
            }
            if (oneLine.size() > 0) {
                matrix.push_back(oneLine);
            }
            cout << test;
            cout << std::endl;
        }
    }
    inFile.close();
    return matrix;
}

void schrijfInputFile(const string& filename) {
    ofstream outFile(filename, ios::out);
    if (!outFile.bad()) {
        outFile << maxCombinaties << " " << maxRecursion << std::endl;
        outFile << MAT[0].size() << " " << MAX << " " << AANTAL << std::endl;
        for (const int& i : MIN) {
            outFile << i << " ";
        }
        outFile << std::endl;
        for (const vector<bool>& x : MAT) {
            printBoolVector(x, outFile);
            outFile << std::endl;
        }
    }
    outFile.close();
}

bool checkInputData(const vector<vector<int>>& inputData) {
    bool fileOK = (inputData.size() >= 4);
    if (fileOK) {
        //parameterlijnen lengte OK
        fileOK = (inputData[0].size() == 2) && (inputData[1].size() == 3);
    }
    if (fileOK) {
        //aantal uurlijsten OK
        fileOK = (inputData.size() - 3 == inputData[1][2]);
    }
    if (fileOK) {
        //aantal te plaatsen uur kleiner of gelijk aan lengte van het rooster
        fileOK = (inputData[1][1] <= inputData[1][0]);
    }
    if (fileOK) {
        //aantal minima komt overeen met aantal uurlijsten
        fileOK = (inputData[1][2] == inputData[2].size());
    }
    if (fileOK) {
        //alle uren tussen 1 en roosterlengte
        for (size_t m = 3; m < inputData.size() && fileOK; m++) {
            for (size_t n = 0; n < inputData[m].size() && fileOK; n++) {
                fileOK = (inputData[m][n] > 0 && inputData[m][n] <= inputData[1][0]);
                if (fileOK && n > 0) {
                    fileOK = (inputData[m][n - 1] < inputData[m][n]);
                }
            }
        }
    }
    return fileOK;
}
