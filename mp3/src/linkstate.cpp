#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <list>
#include <unordered_map>
#include <bits/stdc++.h>

using namespace std;

// #define _DEBUG

// Set for all the nodes
set<int> Nodes;

// The cost table
// map<current_node, map<target_node, cost>>
map<int, map<int, int>> Costs;

// The forward table
// map<source_node, map<target_node, pair<next_node, cost>>>
map<int, map<int, pair<int, int>>> forward_table;

// message
typedef struct {
    int src;
    int dst;
    string msg;
}message;

// message list
list<message> message_table;

//file out put
ofstream outfile;

void loadTopofile(char* file);
void printForwardTable();
void distanceVector();
void changeTopo(char* file);
void loadMessage(char* file);
void sendMessage();

void loadTopofile(char* file) {

    ifstream infile;
    infile.open(file);

    if (!infile.is_open()) {
        cout << "Error, fail to open topofile" << endl;
    }

    string line;

    while (getline(infile, line)) {
        stringstream ss(line);
        int node1, node2, cost;
        ss >> node1 >> node2 >> cost;
        Nodes.insert(node1);
        Nodes.insert(node2);
        Costs[node1][node2] = cost;
        Costs[node2][node1] = cost;
    }

    infile.close();

#ifdef _DEBUG
    // print out the Costs map
    cout << "The Cost Table: " << endl;
    for (map<int, map<int, int> >::iterator it1 = Costs.begin(); it1 != Costs.end(); it1++)
    {
        for (map<int, int>::iterator it2 = (it1->second).begin(); it2 != (it1->second).end(); it2++)
        {
            cout << it1->first << " to ";
            cout << it2->first << " cost ";
            cout << it2->second << endl;
        }
        cout << "----------------------" << endl;
    }
    cout << endl;
    cout << endl;
    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
    cout << endl;
    cout << endl;
#endif
}

void printForwardTable() {
    // print out the forward_table map
    // cout << "The Forward Table: " << endl;
    for (auto src_id_it = forward_table.begin(); src_id_it != forward_table.end(); src_id_it++) {
        map<int, pair<int, int>> ft = src_id_it -> second;
        for (auto dst_id_it = ft.begin(); dst_id_it != ft.end(); dst_id_it++) {
            outfile << dst_id_it->first << " " << dst_id_it->second.first << " " << dst_id_it->second.second << endl;
        }
    }
}

void dijkstra() {
    unordered_map<int, int> prev_nodes;
    for (int node : Nodes) {
        set<int> N;
        map<int, int> D;

        N.insert(node);

        for (int v : Nodes) {
            if (v == node) {
                D[v] = 0;
                prev_nodes[v] = node;
            }
            else if (Costs[node].count(v) != 0) {
                D[v] = Costs[node][v];
                prev_nodes[v] = node;
            }
            else {
                D[v] = INT_MAX;
            }

        }

        list<int> min_nodes;

#ifdef _DEBUG
        //print out the initialization for all the D(v)
        int count = 1;
        for (auto i : D) {
            cout << i.first << "   " << i.second << endl;
            if (count == 5) {
                cout << endl;
                count = 1;
            }
            count++;
        }
#endif
        bool end = false;
        while (end == false) {
            end = true;
            int mindis = INT_MAX;
            int mindis_node = INT_MAX;
            for (int w : Nodes) {
                if (N.count(w) > 0) {
                    continue;
                }
                if (D[w] < mindis) {
                    mindis = D[w];
                    mindis_node = w;
                }
            }
            if (mindis_node != INT_MAX) {
                N.insert(mindis_node);
                min_nodes.push_back(mindis_node);
                end = false;

#ifdef _DEBUG
                cout << "Min distance is: " << mindis << " min distance node is: " << mindis_node << endl;
#endif

                for (pair<int, int> adjacent : Costs[mindis_node]) {
                    int cur_node = adjacent.first;
                    int cur_cost = adjacent.second;

                    if (D[cur_node] > D[mindis_node] + cur_cost) {
                        D[cur_node] = D[mindis_node] + cur_cost;
                        prev_nodes[cur_node] = mindis_node;
                    }
                    else if (D[mindis_node] + cur_cost == D[cur_node] && mindis_node < prev_nodes[cur_node]) {
                        D[cur_node] = D[cur_node];
                        prev_nodes[cur_node] = mindis_node;


                    }
                }
            }
        }


        forward_table[node][node].first = node;
        forward_table[node][node].second = 0;
        while (!min_nodes.empty()) {
            int cur = min_nodes.front();
            min_nodes.pop_front();

            if (prev_nodes[cur] == node) {
                forward_table[node][cur].first = cur;
                forward_table[node][cur].second = D[cur];
            }
            else {
                forward_table[node][cur].first = forward_table[node][prev_nodes[cur]].first;
                forward_table[node][cur].second = D[cur];
            }
        }


    }
}
void changeTopo(char* file) {
    ifstream infile;
    infile.open(file);

    if (!infile.is_open()) {
        cout << "Error, fail to open changefile" << endl;
    }

    string line;

    while (getline(infile, line)) {
        stringstream ss(line);
        int node1, node2, cost;
        ss >> node1 >> node2 >> cost;
        if (cost == -999) {
            Costs[node1].erase(node2);
            Costs[node2].erase(node1);
            if (Costs.count(node1) <= 0) {
                Nodes.erase(node1);
            }
            if (Costs.count(node2) <= 0) {
                Nodes.erase(node2);
            }
        }
        else {
            Costs[node1][node2] = cost;
            Costs[node2][node1] = cost;
        }
#ifdef _DEBUG
        // print out the Costs map
        cout << "The Cost Table: " << endl;
        for (map<int, map<int, int> >::iterator it1 = Costs.begin(); it1 != Costs.end(); it1++)
        {
            for (map<int, int>::iterator it2 = (it1->second).begin(); it2 != (it1->second).end(); it2++)
            {
                cout << it1->first << " to ";
                cout << it2->first << " cost ";
                cout << it2->second << endl;
            }
            cout << "----------------------" << endl;
        }
        cout << endl;
        cout << endl;
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
        cout << endl;
        cout << endl;
#endif

        forward_table.clear();
        dijkstra();
        printForwardTable();
        sendMessage();
    }
    infile.close();
}


void loadMessage(char* file) {
    ifstream infile;
    infile.open(file);

    if (!infile.is_open()) {
        cout << "Error, fail to open messagefile" << endl;
    }

    string line;

    while (getline(infile, line)) {
        stringstream ss(line);
        int node1, node2;
        string cur_msg;
        ss >> node1 >> node2;
        cur_msg = line.substr(line.find(" "));
        cur_msg = cur_msg.substr(line.find(" ") + 2);
        message new_msg;
        new_msg.src = node1;
        new_msg.dst = node2;
        new_msg.msg = cur_msg;
        message_table.push_back(new_msg);
    }
    infile.close();
}

void sendMessage() {
    for (message item : message_table) {

        list<int> path;
        int cur_node = item.src;
        int end = item.dst;

        if (forward_table.count(cur_node) == 0 || forward_table[cur_node].count(end) == 0 || forward_table[item.src][item.dst].second == INT_MAX) {
            outfile << "from " << cur_node << " to " << end << " cost infinite hops unreachable message " << item.msg << endl;
        }
        else {
            while (cur_node != end) {
                path.push_back(cur_node);
                cur_node = forward_table[cur_node][end].first;
            }

            outfile << "from " << item.src << " to " << item.dst << " cost " << forward_table[item.src][item.dst].second << " hops ";
            for (int i : path) {
                outfile << i << " ";
            }
            outfile << "message ";
            outfile << item.msg << endl;
        }
    }
}



int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    FILE* fpOut;
    fpOut = fopen("output.txt", "w");

    fclose(fpOut);

    outfile.open("output.txt");

    loadTopofile(argv[1]);
    dijkstra();
    printForwardTable();
    loadMessage(argv[2]);
    sendMessage();
    changeTopo(argv[3]);

    outfile.close();

    return 0;
}
