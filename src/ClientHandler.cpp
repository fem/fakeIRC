#include <array>
#include <cstring>
#include <iostream>
#include <string_view>

#include <unistd.h>

#include "ClientHandler.hpp"
#include "ConnectionHandler.hpp"
#include "vprint.hpp"

namespace
{
constexpr size_t READ_BUF_SIZE{4096};

constexpr std::array<const char*, 55> RICKROLL{
  "We're no strangers to love",
  "You know the rules and so do I",
  "A full commitment's what I'm thinking of",
  "You wouldn't get this from any other guy",
  "I just wanna tell you how I'm feeling",
  "Gotta make you understand",
  "Never gonna give you up",
  "Never gonna let you down",
  "Never gonna run around and desert you",
  "Never gonna make you cry",
  "Never gonna say goodbye",
  "Never gonna tell a lie and hurt you",
  "We've known each other for so long",
  "Your heart's been aching but you're too shy to say it",
  "Inside we both know what's been going on",
  "We know the game and we're gonna play it",
  "And if you ask me how I'm feeling",
  "Don't tell me you're too blind to see",
  "Never gonna give you up",
  "Never gonna let you down",
  "Never gonna run around and desert you",
  "Never gonna make you cry",
  "Never gonna say goodbye",
  "Never gonna tell a lie and hurt you",
  "Never gonna give you up",
  "Never gonna let you down",
  "Never gonna run around and desert you",
  "Never gonna make you cry",
  "Never gonna say goodbye",
  "Never gonna tell a lie and hurt you",
  "Never gonna give, never gonna give",
  "(Give you up)",
  "We've known each other for so long",
  "Your heart's been aching but you're too shy to say it",
  "Inside we both know what's been going on",
  "We know the game and we're gonna play it",
  "I just wanna tell you how I'm feeling",
  "Gotta make you understand",
  "Never gonna give you up",
  "Never gonna let you down",
  "Never gonna run around and desert you",
  "Never gonna make you cry",
  "Never gonna say goodbye",
  "Never gonna tell a lie and hurt you",
  "Never gonna give you up",
  "Never gonna let you down",
  "Never gonna run around and desert you",
  "Never gonna make you cry",
  "Never gonna say goodbye",
  "Never gonna tell a lie and hurt you",
  "Never gonna give you up",
  "Never gonna let you down",
  "Never gonna run around and desert you",
  "Never gonna make you cry",
  "Never gonna say goodbye"};

bool is_command(std::string_view cmd, std::string_view cmdline)
{
	return cmdline.substr(0, cmd.length()) == cmd;
}
} // namespace

void ClientHandler::send_motd() const
{
	vprint("Sending MOTD\n");
	send_message(motd);
}

void ClientHandler::send_join_announcement() const
{
	conn_hdl.broadcast_message("* " + nick + " entered the chat");
}

void ClientHandler::send_message(const std::string& message) const
{
	vprint("Sending message:\n", message);
	ssize_t res = write(sockfd, message.c_str(), message.length());
	size_t ures = static_cast<size_t>(res);

	if (res < 0) {
		std::clog << "Write error: " << strerror(errno) << '\n';
	}

	if (ures < message.length()) {
		res = write(sockfd, message.c_str() + res, message.length() - res);
		ures = static_cast<size_t>(res);
		if (res < 0) {
			std::clog << "Write error: " << strerror(errno) << '\n';
		} else if (ures < message.length() - res) {
			std::cout << "Partial write!\n";
		}
	}
}

void ClientHandler::read_messages()
{
	vprint("reading\n");
	char read_buf[READ_BUF_SIZE];

	ssize_t res = read(sockfd, read_buf, READ_BUF_SIZE);

	if (res < 0) {
		std::clog << "Read error: " << strerror(errno) << '\n';
		return;
	}

	for (ssize_t i = 0; i < res; ++i) {
		if (read_buf[i] == '\n') {
			input_line = input_buffer.str();
			input_buffer.str("");
			input_buffer.clear();
			process_input_line();
		} else {
			input_buffer << read_buf[i];
		}
	}
}

void ClientHandler::close() const
{
	conn_hdl.broadcast_message("* " + nick + " disconnected");
	::close(sockfd);
}

void ClientHandler::process_input_line()
{
	vprint("Processing input line...\n");
	if (input_line.length() == 0)
		return;
	if (input_line.at(0) == '/' && input_line.length() > 1) {
		std::string_view cmdline = std::string_view{input_line}.substr(1);
		vprint("Received command: ", cmdline, "\n");

		if (is_command("nick ", cmdline)) {
			std::string old_nick = std::move(nick);
			nick = cmdline.substr(std::string_view{"nick "}.length());
			conn_hdl.broadcast_message("* " + old_nick + " is now known as "
			                           + nick);
		} else if (is_command("rick", cmdline)) {
			for (const char* msg : RICKROLL)
				conn_hdl.broadcast_message(std::string{"* "} + msg);
		} else {
			send_message("Invalid command line: " + std::string{cmdline}
			             + "\n");
		}
	} else {
		vprint("Received message: ", input_line, "\n");
		conn_hdl.broadcast_message("<" + nick + "> " + input_line);
	}
}
