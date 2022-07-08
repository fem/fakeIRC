#include <iostream>

#include <cstring>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#include "ConnectionHandler.hpp"
#include "vprint.hpp"

void ConnectionHandler::start()
{
	run = true;
	int res;

	struct sockaddr_in client_sin4;
	struct sockaddr_in6 client_sin6;
	struct sockaddr* client_sin;
	socklen_t client_socklen;
	if (address_family == AF_INET) {
		client_sin = reinterpret_cast<struct sockaddr*>(&client_sin4);
		client_socklen = sizeof(client_sin4);
	} else if (address_family == AF_INET6) {
		client_sin = reinterpret_cast<struct sockaddr*>(&client_sin6);
		client_socklen = sizeof(client_sin6);
	} else {
		std::cerr << "Unexpected address family " << address_family
		          << " in ConnectionHandler\n";
		return;
	}

	int epollfd = epoll_create1(0);

	if (epollfd < 0) {
		std::cerr << "Failed setting up epollfd: " << strerror(errno) << '\n';
		return;
	}

	vprint("setting up epoll for sockfd\n");
	struct epoll_event sockfd_epe = {.events = EPOLLIN, .data = {.fd = sockfd}};
	res = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &sockfd_epe);
	if (res < 0) {
		std::cerr << "Failed setting up epoll for sockfd: " << strerror(errno)
		          << '\n';
		return;
	}
	vprint("setting up epoll for sigfd\n");
	struct epoll_event sigfd_epe = {.events = EPOLLIN, .data = {.fd = sigfd}};
	res = epoll_ctl(epollfd, EPOLL_CTL_ADD, sigfd, &sigfd_epe);
	if (res < 0) {
		std::cerr << "Failed setting up epoll for sigfd: " << strerror(errno)
		          << '\n';
	}

	struct epoll_event next_event;

	while (run) {
		res = epoll_wait(epollfd, &next_event, 1, -1);

		if (res == 0) {
			vprint("epoll_wait() returned without events\n");
			continue;
		} else if (res < 0 && errno == EINTR) {
			disconnect_clients();
			run = false;
			goto epoll_uninit;
		} else if (res < 0) {
			std::cerr << "epoll_wait() failed: " << strerror(errno) << '\n';
			goto epoll_uninit;
		}

		int event_fd = next_event.data.fd;
		vprint("epoll event for fd ", event_fd, "\n");

		if (event_fd == sockfd) {
			vprint("accepting new connection\n");
			res = accept(sockfd, client_sin, &client_socklen);

			if (res < 0) {
				std::cerr << "accept() failed with error: " << strerror(errno)
				          << '\n';
				continue;
			}

			std::cout << "New client!\n";
			int client_fd = res;
			auto peer = peers.emplace(std::make_pair(
			  client_fd,
			  ClientHandler{*this, client_fd, motd,
			                "user" + std::to_string(connections_accepted)}));
			peer.first->second.send_motd();
			peer.first->second.send_join_announcement();

			vprint("setting up epoll for new client ", client_fd, "\n");
			struct epoll_event client_epe = {.events = EPOLLIN,
			                                 .data = {.fd = client_fd}};
			res = epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &client_epe);
			if (res < 0) {
				std::cerr << "Failed setting up epoll for client fd: "
				          << strerror(errno) << '\n';
				goto socket_uninit;
			}

			++connections_accepted;
		} else if (event_fd == sigfd) {
			vprint("Received signal\n");
			disconnect_clients();
			run = false;
			goto epoll_uninit;
		} else {
			vprint("Client epoll event!\n");
			char buf;
			ssize_t peek_res = recv(event_fd, &buf, 1, MSG_PEEK);
			if (peek_res == 0) {
				vprint("Client disconnect\n");
				peers.at(event_fd).close();
				peers.erase(event_fd);
			} else {
				peers.at(event_fd).read_messages();
			}
		}
	}

socket_uninit:
	for (const auto& [fd, _] : peers) {
		close(fd);
	}

epoll_uninit:
	close(epollfd);
}

void ConnectionHandler::broadcast_message(const std::string& msg)
{
	std::string nl_msg = msg + '\n';
	for (const auto& [_, peer] : peers)
		peer.send_message(nl_msg);
}

void ConnectionHandler::disconnect_clients()
{
	std::cout << "Exiting...\n";
	broadcast_message("* this server is shutting down");

	for (auto& [fd, peer] : peers)
		peer.close();
	peers.clear();
}
