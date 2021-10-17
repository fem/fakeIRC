#pragma once

#include <sstream>
#include <string>

#include <netinet/in.h>

class ConnectionHandler;

class ClientHandler
{
public:
	ClientHandler(ConnectionHandler& conn_hdl, int sockfd, std::string& motd,
	              std::string_view initial_nick)
	    : conn_hdl(conn_hdl), sockfd(sockfd), motd(motd), nick(initial_nick)
	{}
	ClientHandler(const ClientHandler& other) = delete;
	ClientHandler(ClientHandler&&) = default;
	~ClientHandler() = default;

	void send_motd() const;
	void send_join_announcement() const;

	void read_messages();

	void send_message(const std::string& message) const;

	void close() const;

private:
	void process_input_line();

	ConnectionHandler& conn_hdl;
	int sockfd;
	std::string& motd;

	std::stringstream input_buffer;
	std::string input_line;

	std::string nick;
};
