#pragma once

#include <map>
#include <string>

#include <netinet/in.h>

#include "ClientHandler.hpp"

class ConnectionHandler
{
public:
	ConnectionHandler(int sockfd, int address_family, std::string& motd) : sockfd(sockfd), address_family(address_family), motd(motd)
	{}
	ConnectionHandler(const ConnectionHandler&) = delete;
	~ConnectionHandler() = default;

	void start();

	void broadcast_message(const std::string& msg);

private:
	int sockfd;
	int address_family;
	volatile bool run{false};

	std::map<int, ClientHandler> peers;
	std::string& motd;

	size_t connections_accepted{0};
};
