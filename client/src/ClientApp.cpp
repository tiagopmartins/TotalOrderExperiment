#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "Client.h"

int main(int argc, char *argv[]) {
    Client *client = new Client();

    int msgN = 0;
    std::string command;
    // User input reading
    while (true) {
        std::cin >> command;

        if (command.compare("begin") == 0) {
            std::cout << "Number of messages to exchange: ";
            std::cin >> msgN;
            std::cout << std::endl;

            if (msgN <= 0) {
                std::cerr << "Invalid number of messages specified.\n" << std::endl;
                continue;
            }

            client->begin(msgN);
        }

        else if (command.compare("fetch") == 0) {
            client->fetchLog();
        }

        else {
            std::cerr << "Invalid command specified.\n" << std::endl;
        }
    }

    return 0;
}