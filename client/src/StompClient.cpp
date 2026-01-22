#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include "ConnectionHandler.h"
#include "StompProtocol.h"

int main() {
	while (true) {
		std::string input;
		while (std::getline(std::cin, input)) {
			if (input.substr(0, 5) == "login") {
				break; //got login command
            }
            std::cout << "Please login first" << std::endl;
		}

		//if reached here, client is trying to login
		std::istringstream stream(input);
        std::string command, host_port, username, password;
        stream >> command >> host_port >> username >> password;

		size_t colon_pos = host_port.find(':');
        std::string host = host_port.substr(0, colon_pos);
        short port = std::stoi(host_port.substr(colon_pos + 1));

		ConnectionHandler* handler = new ConnectionHandler(host, port);
        if (!handler->connect()) {
            std::cout << "Could not connect to server" << std::endl;
            delete handler;
            continue;  //back to login loop
        }

		//if reached here, client is logged-in
		StompProtocol protocol;
        protocol.setHandler(handler);
		protocol.processKeyboardCommand(input);

		std::thread keyboardThread([&protocol]() {
            while (!protocol.shouldTerminate()) {
                std::string keyboard_input;
                if (std::getline(std::cin, keyboard_input)) {
                    protocol.processKeyboardCommand(keyboard_input);
                }
            }
        });

		std::thread socketThread([&protocol]() {
            while (!protocol.shouldTerminate()) {
                std::string frame;
                if (protocol.getHandler()->getFrameAscii(frame, '\0')) {
                    protocol.processServerFrame(frame);
                } else {
                    break; //connection closed
                }
            }
        });

		keyboardThread.join();
        socketThread.join();
		delete handler;
	}

	return 0;
}