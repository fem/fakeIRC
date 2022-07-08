#include <iostream>
#include <stdexcept>

#include <cstdlib>
#include <cstring>

#include <netinet/ip.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#include "ConnectionHandler.hpp"
#include "vprint.hpp"

namespace
{
constexpr const char* OPTSTRING{"hp:m:v46"};
constexpr int LISTEN_CONNLIMIT{1024};

std::string motd{""};
uint16_t port{1337};

int sock_domain{-1};
sa_family_t sa_family;

int create_signal_fd();
void print_usage();
bool assert_args();
} // namespace

int main(int argc, char** argv)
{
	signal(SIGPIPE, SIG_IGN);

	int optchar;
	while ((optchar = getopt(argc, argv, OPTSTRING)) > 0) {
		switch (static_cast<char>(optchar)) {
		case 'h':
			print_usage();
			return 0;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'm':
			motd = optarg;
			motd += '\n';
			break;
		case 'v':
			vprint_verbose = true;
			break;
		case '4':
			sock_domain = AF_INET;
			sa_family = AF_INET;
			break;
		case '6':
			sock_domain = AF_INET6;
			sa_family = AF_INET6;
			break;

		case '?':
			print_usage();
			return 1;
			break;
		}
	}

	if (!assert_args())
		return 1;

	int res;

	vprint("fakeirc starting up\n");
	vprint("setting up socket\n");

	res = socket(sock_domain, SOCK_STREAM, 0);
	if (res < 0) {
		std::clog << "Failed setting up socket: " << strerror(errno) << '\n';
		return 1;
	}

	int sockfd = res;

	struct sockaddr_in sin4;
	struct sockaddr_in6 sin6;

	vprint("binding socket\n");
	if (sa_family == AF_INET) {
		sin4 = {.sin_family = sa_family,
		        .sin_port = htons(port),
		        .sin_addr = {INADDR_ANY}};

		res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&sin4),
		           sizeof(sockaddr_in));
	} else if (sa_family == AF_INET6) {
		sin6 = {.sin6_family = sa_family,
		        .sin6_port = htons(port),
		        .sin6_flowinfo = 0,
		        .sin6_addr = IN6ADDR_ANY_INIT,
		        .sin6_scope_id = 0};

		res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&sin6),
		           sizeof(sockaddr_in6));
	} else {
		std::cerr << "Unexpected error: Invalid sa_family\n";
		return 1;
	}

	if (res < 0) {
		std::clog << "Failed binding socket: " << strerror(errno) << '\n';
		return 1;
	}

	vprint("listening on socket\n");
	res = listen(sockfd, LISTEN_CONNLIMIT);

	if (res < 0) {
		std::clog << "Failed listening on socket: " << strerror(errno) << '\n';
		return 1;
	}

	int sigfd = create_signal_fd();
	vprint("Set up sigfd ", sigfd, "\n");

	ConnectionHandler conn_handler{sigfd, sockfd, sa_family, motd};
	conn_handler.start();

	return 0;
}

namespace
{
int create_signal_fd()
{
	int res;
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGQUIT);

	res = sigprocmask(SIG_BLOCK, &mask, NULL);
	if (res == -1) {
		throw std::runtime_error(std::string{"Failed to set sigprocmask: "}
		                         + strerror(errno));
	}

	int sigfd = signalfd(-1, &mask, 0);
	if (sigfd == -1) {
		throw std::runtime_error(std::string{"Failed to set sigfd: "}
		                         + strerror(errno));
	}

	return sigfd;
}

void print_usage()
{
	std::cout << "fakeirc\tCopyright (C) 2021 Adrian Schollmeyer\n\n"
	          << "Usage: fakeirc [Options]\n\n"
	          << "Options:\n"
	          << "\t-h        \tPrint this help and exit\n"
	          << "\t-v        \tEnable verbose output\n"
	          << "\t-p [Port] \tListen on this port\n"
	          << "\t-m [MOTD] \tSet message of the day\n"
	          << "\t-4        \tEnable a legacy version of IP\n"
	          << "\t-6        \tEnable the current version of IP\n";
}

bool assert_args()
{
	if (sock_domain == -1) {
		std::cerr << "Missing IP version!\n";
		return false;
	}

	return true;
}
} // namespace
