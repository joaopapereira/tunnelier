/*
 * Tunnel.hpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#ifndef TUNNEL_HPP_
#define TUNNEL_HPP_
#include "Address.hpp"
#include "User.hpp"
#include <thread>
#include <event.h>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <vector>
#include <iostream>
#define USE_LISTENER 1
#if USE_LISTENER
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#endif
class Tunnel;
class Connection {
friend class Tunnel;
public:
	Connection( Tunnel* parent, int socket, ssh_channel fw_channel);
	virtual ~Connection();
	static void socket_to_ssh(struct bufferevent *bev, void *ctx);
	static void ssh_to_socket2(struct bufferevent *bev, void *ctx);
	static void ssh_to_socket(int fd, short event, void *arg);
	static void errorcb(struct bufferevent *bev, short error, void *ctx);
	static int MAX_READ;

	static int copy_chan_to_fd(ssh_session session,
	                                           ssh_channel channel,
	                                           void *data,
	                                           uint32_t len,
	                                           int is_stderr,
	                                           void *userdata);
private:
	int run_once();
	Tunnel * parent;
	int socket;
	ssh_channel fw_channel;
	struct event * ssh_event;
	struct bufferevent * socket_event;
	bool closed;
};

class Tunnel{
friend class Connection;
public:
	/**
	 * Class constructor
	 * @param localPort Port to bind locally
	 * @param destination Destination address of the tunnel
	 * @param middle Middle machine address
	 * @param middleUser User to connect
	 */
	Tunnel( struct event_base *base, int localPort, Address destination, Address middle, User middleUser );
	/**
	 * Start tunnel
	 */
	bool start();
	int run();
	/**
	 * Stop tunnel
	 */
	bool stop();
	/**
	 * Class destructor
	 */
	virtual ~Tunnel();
private:
	/**
	 * Local port
	 */
	int localPort;
	/**
	 * Destination host
	 */
	Address destination;
	/**
	 * Middle host
	 */
	Address middle;
	/**
	 * Username
	 */
	bool stillRunning;
	User middleUser;
	std::thread *thr;
	std::vector<Connection> tunnelConnections;
#if USE_LISTENER
	struct evconnlistener * listener;
#else
	//evutil_socket_t listener;
	int listener;
#endif
	struct event_base *base;
	struct event listener_event;
	ssh_session tunnel_session;
	ssh_channel forwarding_channel;
#if !USE_LISTENER
	static void do_accept(int errcode, short int addr, void *ptr);
#else
	static void accept_conn_cb(struct evconnlistener *listener,
	    evutil_socket_t fd, struct sockaddr *address, int socklen,
	    void *ptr);
	static void accept_error_cb(struct evconnlistener *listener, void *ptr);
#endif
	int connect_tunnel();
};

#endif /* TUNNEL_HPP_ */
