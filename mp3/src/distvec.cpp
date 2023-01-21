#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <utility>
#include <list>

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
void initialize();
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

void initialize() {
    for (int key : Nodes) {
        for (int item : Nodes) {
            if (key == item) {
                forward_table[key][item] = make_pair(item, 0);
            }
            else if (Costs[key].count(item) == 0 || Costs[key][item] == -999) {
                forward_table[key][item] = make_pair(-1, -999);
            }
            else {
                forward_table[key][item] = make_pair(item, Costs[key][item]);
            }
        }
    }
#ifdef _DEBUG
    printForwardTable();
#endif

}

void printForwardTable() {
    // print out the forward_table map
    // cout << "The Forward Table: " << endl;
    for (auto src_id_it = Nodes.begin(); src_id_it != Nodes.end(); src_id_it++) {
        auto src_id = *src_id_it;
        for (auto dst_id_it = Nodes.begin(); dst_id_it != Nodes.end(); dst_id_it++) {
            auto dst_id = *dst_id_it;
            //cout << "src: " << src_id << " dst: " << dst_id << " next: " << forward_table[src_id][dst_id].first << " cost: " << forward_table[dst_id][src_id].second << endl;
            if(forward_table[dst_id][src_id].second != -999) {
                cout << dst_id << " " << forward_table[src_id][dst_id].first << " " << forward_table[dst_id][src_id].second << endl;
            }
        }
    }

    for (auto src_id_it = forward_table.begin(); src_id_it != forward_table.end(); src_id_it++) {
        int source = src_id_it -> first;
        map<int, pair<int, int>> ft = src_id_it -> second;
        for (auto dst_id_it = ft.begin(); dst_id_it != ft.end(); dst_id_it++) {
            if(dst_id_it->second.second != -999) {
                outfile << dst_id_it->first << " " << dst_id_it->second.first << " " << dst_id_it->second.second << endl;
            }
        }
    }
}

void distanceVector() {
    // calculate for each node
    for (int node : Nodes) {
        // for each src node
        for (int one : Nodes) {
            // for each dst node
            for (int two : Nodes) {
                if (one == two) {
                    continue;
                }
                int min_next = forward_table[one][two].first;
                int min_cost = forward_table[one][two].second;
                //
                for (int three : Nodes) {
                    if (Costs[one].count(three) != 0 && Costs[one][three] != -999 && forward_table[three][two].second >= 0) {
                        int current_cost = Costs[one][three] + forward_table[three][two].second;
                        if (min_cost < 0 || current_cost < min_cost) {
                            min_next = three;
                            min_cost = current_cost;
                        }
                    }
                }
                forward_table[one][two].first = min_next;
                forward_table[one][two].second = min_cost;
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
        Costs[node1][node2] = cost;
        Costs[node2][node1] = cost;
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
        initialize();
        distanceVector();
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

        if (forward_table.count(cur_node) == 0 || forward_table[cur_node].count(end) == 0 || forward_table[item.src][item.dst].second == -999) {
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
    initialize();
    distanceVector();
    printForwardTable();
    loadMessage(argv[2]);
    sendMessage();
    changeTopo(argv[3]);

    outfile.close();

    return 0;
}
