#include <unistd.h>
#include <sstream>
#include <set>

#include "zmq_functions.h"
#include "topology.h"

int main() {
    topology topology_container;
    std::vector<zmq::socket_t> list_heads;
    zmq::context_t context;

    std::string operation;
    while (std::cin >> operation) {

        if (operation == "create") {
            int node_id, parent_id;
            std::cin >> node_id >> parent_id;

            if (topology_container.find(node_id) != -1) {
                std::cout << "Error: already exists" << std::endl;
            }
            else if (parent_id == -1) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("Can't create new process");
                    return -1;
                }
                if (pid == 0) {
                    execl("./count_node", "./count_node", std::to_string(node_id).c_str(), NULL);
                    perror("Can't execute new process");
                    return -2;
                }
                
                list_heads.emplace_back(context, ZMQ_REQ);
                list_heads[list_heads.size() - 1].setsockopt(ZMQ_SNDTIMEO, 5000);
                bind(list_heads[list_heads.size() - 1], node_id);
                send_message(list_heads[list_heads.size() - 1], std::to_string(node_id) + "pid");
                
                std::string reply = receive_message(list_heads[list_heads.size() - 1]);
                std::cout << reply << std::endl;
                topology_container.insert(node_id, parent_id);
            }
            else if (topology_container.find(parent_id) == -1) {
                std::cout << "Error: parent not found" << std::endl;
            }
            else {
                int list_num = topology_container.find(parent_id);
                send_message(list_heads[list_num], std::to_string(parent_id) + "create " + std::to_string(node_id));

                std::string reply = receive_message(list_heads[list_num]);
                std::cout << reply << std::endl;
                topology_container.insert(node_id, parent_id);
            }
        }
        else if (operation == "exec") {
            int dest_id;
            int n;
            std::cin >> dest_id >> n;
            std::vector<int> v(n);
            std::string s;
            int list_num = topology_container.find(dest_id);
            for (int i = 0; i < n; ++i) {
                std::cin >> v[i];
                s = s + std::to_string(v[i]) + ' ';
            }
            if (list_num == -1) {
                std::cout << "ERROR: incorrect node id" << std::endl;
            }
            else {
                send_message(list_heads[list_num], std::to_string(dest_id) + "exec " + std::to_string(n) + ' ' + s);

                std::string reply = receive_message(list_heads[list_num]);
                std::cout << reply << std::endl;
            }

        }
        else if (operation == "kill") {
            int id;
            std::cin >> id;
            int list_num = topology_container.find(id);
            if (list_num == -1) {
                std::cout << "ERROR: incorrect node id" << std::endl;
            }
            else {
                bool is_first = (topology_container.get_first_id(list_num) == id);
                send_message(list_heads[list_num], std::to_string(id) + " kill");

                std::string reply = receive_message(list_heads[list_num]);
                std::cout << reply << std::endl;
                topology_container.erase(id);
                if (is_first) {
                    unbind(list_heads[list_num], id);
                    list_heads.erase(list_heads.begin() + list_num);
                }
            }
        }
        else if (operation == "print") {
            std::cout << topology_container;
        }
        else if (operation == "pingall") {
            std::set<int> available_nodes;
            for (size_t i = 0; i < list_heads.size(); ++i) {
                int first_node_id = topology_container.get_first_id(i);
                //std::cout << first_node_id << std::endl;
                send_message(list_heads[i], std::to_string(first_node_id) + " pingall");

                std::string received_message = receive_message(list_heads[i]);
                std::istringstream reply(received_message);
                int node;
                while(reply >> node) {
                    available_nodes.insert(node);
                }
            }
            std::cout << "OK: ";
            if (available_nodes.empty()) {
                std::cout << "no available nodes" << std::endl;
            }
            else {
                for (auto v : available_nodes) {
                    std::cout << v << " ";
                }
                std::cout << std::endl;
            }
        }
        else if (operation == "ping") {
            int id;
            std::cin >> id;
            int list_num = topology_container.find(id);
            if (list_num == -1) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            send_message(list_heads[list_num], std::to_string(id) + " ping");

            std::string reply = receive_message(list_heads[list_num]);
            std::cout << reply << std::endl;
        }
        else if (operation == "exit") {
            for (size_t i = 0; i < list_heads.size(); ++i) {
                int first_node_id = topology_container.get_first_id(i);
                send_message(list_heads[i], std::to_string(first_node_id) + " kill");
                
                std::string reply = receive_message(list_heads[i]);
                if (reply != "OK") {
                    std::cout << reply << std::endl;
                }
                else {
                    unbind(list_heads[i], first_node_id);
                }
            }
            exit(0);
        }
        else {
            std::cout << "Incorrect operation" << std::endl;
        }
    }
}