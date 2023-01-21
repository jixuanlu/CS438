#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <fstream>
#include <iomanip>

using namespace std;

// #define _DEBUG

default_random_engine gen;

typedef struct {
    int current_range;
    int id;
    int index;
    int current_val;
}node;

map<int, node> Nodes;

int N_node_number;
int L_packet_size;
int M_retransmission;
int T_total_time;
vector<int> R_random_range;

void loadFile(char* file);
void initializerNode();
double csma();

void loadFile(char* file) {
    ifstream infile;
    infile.open(file);

    if (!infile.is_open()) {
        cout << "Error, fail to open input file" << endl;
    }

    string line;

    int i = 1;
    while (getline(infile, line)) {
        stringstream ss(line);
        if (i == 4) {
            string var;
            ss >> var;
            for (int j = 0; j < M_retransmission; ++j) {
                int temp;
                ss >> temp;
                R_random_range.push_back(temp);
            }
        }
        else {
            string var;
            int temp;
            ss >> var >> temp;
            if (i == 1)
                N_node_number = temp;
            else if (i == 2)
                L_packet_size = temp;
            else if (i == 3)
                M_retransmission = temp;
            else if (i == 5)
                T_total_time = temp;
        }
        i++;
    }

    infile.close();

#ifdef _DEBUG
    cout << "----------------------------------------------------------------" << endl;
    cout << "print every element in the R_random_range list" << endl;
    for (int i=0; i < R_random_range.size(); i++)
        cout << R_random_range.at(i) << endl;
    cout << "----------------------------------------------------------------" << endl;
#endif
}

void initializerNode() {
    for (int i = 0; i < N_node_number; ++i) {
        node temp_node;
        temp_node.id = i;
        temp_node.current_range = R_random_range.at(0);
        temp_node.index = 0;
//        temp_node.current_val = rand() % temp_node.current_range;
        temp_node.current_val = i % 4;

        Nodes[i] = temp_node;
    }

#ifdef _DEBUG
    cout << "print every element in the Nodes map" << endl;
    for (auto i = Nodes.begin(); i != Nodes.end(); i++)
        cout << i->first << "   id: " << i->second.id << "   current_range: " << i->second.current_range << "    index: " << i->second.index << "    current_val: " << i->second.current_val << endl;
    cout << "----------------------------------------------------------------" << endl;
#endif
}

double csma() {
    int current_time = 0;
    int transmit_count = 0;
    cout << "round" << current_time << endl;
    for (int k = 0; k < N_node_number; ++k) {
        cout << Nodes[k].current_val << "    ";
    }
    cout << endl;
    int flag = 0;
    while (current_time < T_total_time) {
        current_time ++;

        int count = 0;
        for (int j = 0; j < N_node_number; ++j) {
            if (Nodes[j].current_val == 0) {
                count++;
            }
        }

        if (count == 1) {
            transmit_count ++;
            for (int i = 0; i < L_packet_size - 1; ++i) {
                cout << "round" << current_time << endl;
                for (int k = 0; k < N_node_number; ++k) {
                    cout << Nodes[k].current_val << "    ";
                }
                cout << endl;
                current_time ++;
                if (current_time > T_total_time) {
                    flag = 1;
                    break;
                }

                transmit_count ++;
            }
            if (flag == 1) {
                break;
            }
            int cur_process;
            for (int j = 0; j < N_node_number; ++j) {
                if (Nodes[j].current_val == 0) {
                    cur_process = j;
                }
            }
            Nodes[cur_process].index = 0;
            Nodes[cur_process].current_range = R_random_range.at(Nodes[cur_process].index);
            Nodes[cur_process].current_val = (current_time + Nodes[cur_process].id) % Nodes[cur_process].current_range;


            cout << "round" << current_time << endl;
            for (int k = 0; k < N_node_number; ++k) {
                cout << Nodes[k].current_val << "    ";
            }
            cout << endl;
            continue;
        }
        else if (count == 0) {
            for (int j = 0; j < N_node_number; ++j) {
                Nodes[j].current_val -= 1;
            }

            cout << "round" << current_time << endl;
            for (int k = 0; k < N_node_number; ++k) {
                cout << Nodes[k].current_val << "    ";
            }
            cout << endl;
        }
        else {

            for (int j = 0; j < N_node_number; ++j) {
                if (Nodes[j].current_val == 0) {
                    if (Nodes[j].index < M_retransmission - 1){
                        Nodes[j].index += 1;
                        Nodes[j].current_range = R_random_range.at(Nodes[j].index);
                        Nodes[j].current_val = (current_time + Nodes[j].id) % Nodes[j].current_range;

                    }
                    else {
                        Nodes[j].index = 0;
                        Nodes[j].current_range = R_random_range.at(Nodes[j].index);
                        Nodes[j].current_val = (current_time + Nodes[j].id) % Nodes[j].current_range;
                    }

                }
            }

            cout << "round" << current_time << endl;
            for (int l = 0; l < N_node_number; ++l) {
                cout << Nodes[l].current_val << "    ";
            }
            cout << endl;

        }
    }
    cout << transmit_count << endl;
    cout << T_total_time << endl;
    return double (transmit_count) / double (T_total_time);
}



int main(int argc, char** argv) {
    srand(time(0));
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }


    loadFile(argv[1]);
    initializerNode();
    float result = csma();
    cout.setf(ios::fixed);
    cout << setprecision(2);
    cout << result << endl;


    ofstream fpOut;
    fpOut.open("output.txt");
    fpOut.setf(ios::fixed);
    fpOut << setprecision(2);
    fpOut << result << endl;
    fpOut.close();

    return 0;
}

