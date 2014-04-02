/*
 * Tunnel.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#include "Tunnel.hpp"
#include <unistd.h>
#include <string.h>
using namespace std;
int authenticate_kbdint(ssh_session session, const char *password);
Tunnel::Tunnel(struct event_base *base, int localPort, Address destination, Address middle, User middleUser):
	base(base),
	localPort(localPort),
	destination(destination),
	middle(middle),
	middleUser(middleUser){
	tunnel_session = ssh_new();

}

bool
Tunnel::start(){
	cout << "Starting tunnel"<< endl;
	struct sockaddr_in sin;
	//base = event_base_new();
	if (!base)
		return false; /*XXXerr*/

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(localPort);
#if !USE_LISTENER
	listener = socket(AF_INET, SOCK_STREAM, 0);

	evutil_make_socket_nonblocking(listener);

	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("bind");
		return false;
	}

	if (listen(listener, 16)<0) {
		perror("listen");
		return false;
	}

	//listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)this);
	/*XXX check it */
	//event_add(listener_event, NULL);
	/* We now have a listening socket, we create a read event to
	 * be notified when a client connects. */
	event_set(&listener_event, listener, EV_READ|EV_PERSIST, do_accept, (void *)this);
	event_base_set(base, &listener_event);
	event_add(&listener_event, NULL);
#else
	listener = evconnlistener_new_bind(base, accept_conn_cb, (void*)this,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
			(struct sockaddr*)&sin, sizeof(sin));
		if (!listener) {
				perror("Couldn't create listener");
				return false;
		}
		evconnlistener_set_error_cb(listener, accept_error_cb);
#endif
	tunnel_session = ssh_new();
	if( SSH_OK != connect_tunnel() ){
		 cout << "Error connecting....." << endl;
		 return false;
	 }

	forwarding_channel = ssh_channel_new(tunnel_session);
	if (forwarding_channel == NULL) {
	    return false;
	}
	int rc = ssh_channel_open_forward(forwarding_channel,
								destination.getHost().c_str(), destination.getPort(),
								"localhost", localPort);
	if (rc != SSH_OK)
	{
		ssh_channel_free(forwarding_channel);
		return false;
	}
	return true;
}

bool
Tunnel::stop(){

	return false;
}
#if !USE_LISTENER
void
Tunnel::do_accept(int errcode, short int addr, void *ptr) {
	cout << "Entered do_accept"<<endl;
    Tunnel *ctx = static_cast<Tunnel *>(ptr);

	struct sockaddr_storage ss;
	socklen_t slen = sizeof(ss);
	int fd = accept(ctx->listener, (struct sockaddr*)&ss, &slen);
	if (fd < 0) {
		perror("accept");
	} else if (fd > FD_SETSIZE) {
		close(fd);
	} else {
		struct bufferevent *bev;
		evutil_make_socket_nonblocking(fd);
		Connection *con = new Connection(ctx, fd);
		bev = bufferevent_socket_new(ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(bev, Connection::socket_to_ssh, NULL, Connection::errorcb, (void*)con);
		bufferevent_setwatermark(bev, EV_READ, 0, 2048);
		bufferevent_enable(bev, EV_READ|EV_WRITE);
	}

}
#else
void
Tunnel::accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ptr)
{
	Tunnel *ctx = static_cast<Tunnel *>(ptr);

	/* We got a new connection! Set up a bufferevent for it. */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
			ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
	Connection *con = new Connection(ctx, fd);
	bufferevent_setcb(bev, Connection::socket_to_ssh, NULL, Connection::errorcb, (void*)con);

	bufferevent_enable(bev, EV_READ|EV_WRITE);

}
void
Tunnel::accept_error_cb(struct evconnlistener *listener, void *ptr)
{
	Tunnel *ctx = static_cast<Tunnel *>(ptr);
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Got an error %d (%s) on the listener. "
                "Shutting down.\n", err, evutil_socket_error_to_string(err));

        event_base_loopexit(ctx->base, NULL);
}
#endif
Tunnel::~Tunnel() {
	// TODO Auto-generated destructor stub
}

int Tunnel::connect_tunnel(){
	cout << "Entered connect_tunnel"<<endl;
	  int rc = 0;
	  ssh_options_set(tunnel_session, SSH_OPTIONS_HOST, middle.getHost().c_str());
	  ssh_options_set(tunnel_session, SSH_OPTIONS_USER, middleUser.getUsername().c_str());
	  rc = ssh_connect(tunnel_session);
	  if (rc != SSH_OK){
		  cout <<"Error connecting to " <<  middle.getHost() << ": "<< ssh_get_error(tunnel_session)<<endl;
		  return 1;
	  }
	  cout <<"Trying to connect!!!!" << endl;
	  rc = ssh_userauth_none(tunnel_session, NULL);
	  if(rc == SSH_AUTH_SUCCESS){
		  cout << "Authenticated using method none" << endl;
	  } else {
		  cout << "Authentication console with password: '"<<middleUser.getPassword()<<"'"<<endl;
		  rc = authenticate_kbdint(tunnel_session, middleUser.getPassword().c_str());
		  if(rc != SSH_AUTH_SUCCESS){
			  cerr << "Authentication failed: " << ssh_get_error(tunnel_session) << endl;
			  return 1;
		  }
	  }
	  return SSH_OK;
}

int authenticate_kbdint(ssh_session session, const char *password) {
     int err;

     err = ssh_userauth_kbdint(session, NULL, NULL);
     while (err == SSH_AUTH_INFO) {
         const char *instruction;
         const char *name;
         char buffer[128];
         int i, n;

         name = ssh_userauth_kbdint_getname(session);
         instruction = ssh_userauth_kbdint_getinstruction(session);
         n = ssh_userauth_kbdint_getnprompts(session);

         if (name && strlen(name) > 0) {
             cout <<name << endl;
         }

         if (instruction && strlen(instruction) > 0) {
             cout <<instruction << endl;
         }

         for (i = 0; i < n; i++) {
             const char *answer;
             const char *prompt;
             char echo;

             prompt = ssh_userauth_kbdint_getprompt(session, i, &echo);
             if (prompt == NULL) {
                 break;
             }

             if (echo) {
                 char *p;

                 if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                     return SSH_AUTH_ERROR;
                 }

                 buffer[sizeof(buffer) - 1] = '\0';
                 if ((p = strchr(buffer, '\n'))) {
                     *p = '\0';
                 }

                 if (ssh_userauth_kbdint_setanswer(session, i, buffer) < 0) {
                     return SSH_AUTH_ERROR;
                 }

                 memset(buffer, 0, strlen(buffer));
             } else {
                 if (password && strstr(prompt, "Password:")) {
                     answer = password;
                 } else {
                     buffer[0] = '\0';

                     if (ssh_getpass(prompt, buffer, sizeof(buffer), 0, 0) < 0) {
                         return SSH_AUTH_ERROR;
                     }
                     answer = buffer;
                 }
                 err = ssh_userauth_kbdint_setanswer(session, i, answer);
                 memset(buffer, 0, sizeof(buffer));
                 if (err < 0) {
                     return SSH_AUTH_ERROR;
                 }
             }
         }
         err=ssh_userauth_kbdint(session,NULL,NULL);
     }

     return err;
}
int Connection::MAX_READ = 2048;
Connection::Connection(Tunnel* parent, int socket):
	parent(parent),
	socket(socket){
	cout << "Create connection!!"<<endl;
	// TODO Auto-generated constructor stub

}

Connection::~Connection() {
	// TODO Auto-generated destructor stub
}

void
Connection::socket_to_ssh(struct bufferevent *bev, void *ctx){
	cout << "Entered socket_to_ssh"<<endl;
	Connection * con = static_cast<Connection *>(ctx);
	char readBuff[MAX_READ];
	int readLen;
	struct evbuffer *input = bufferevent_get_input(bev);
	while ((readLen = evbuffer_remove(input, readBuff, sizeof(readBuff))) > 0) {
		readBuff[readLen] = 0;
		ssh_channel_write(con->parent->forwarding_channel, readBuff, readLen);
	}
}
void
Connection::ssh_to_socket(struct bufferevent *bev, void *ctx){
	cout << "Entered ssh_to_socket"<<endl;
	Connection * con = static_cast<Connection *>(ctx);
	char readBuff[MAX_READ];
	int r;
	int lus;
	while(con->parent->forwarding_channel && ssh_channel_is_open(con->parent->forwarding_channel) && (r = ssh_channel_poll(con->parent->forwarding_channel,0))!=0){
	        lus=ssh_channel_read(con->parent->forwarding_channel,readBuff,sizeof(readBuff) > r ? r : sizeof(readBuff),0);
	        if(lus==-1){
	            fprintf(stderr, "Error reading channel: %s\n",
	                    ssh_get_error(con->parent->tunnel_session));
	            return;
	        }
	        if(lus!=0){
	        	evbuffer_add_printf(bufferevent_get_output(bev), "%s", readBuff);
	        }
	}

}


void
Connection::errorcb(struct bufferevent *bev, short events, void *ctx)
{
	Connection * con = static_cast<Connection *>(ctx);
	int finished = 0;
    if (events & BEV_EVENT_EOF) {
        /* connection has been closed, do any clean up here */
    	cout << "Closing connection with:" << con->parent->destination.getHost()<<endl;
    	finished = 1;

    } else if (events & BEV_EVENT_ERROR) {
        /* check errno to see what error occurred */
    	cout << "Closing connection with:" << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())<<endl;
		finished = 1;
    } else if (events & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
    }

    if (finished) {
        free(ctx);
        bufferevent_free(bev);
    }
    bufferevent_free(bev);
}
