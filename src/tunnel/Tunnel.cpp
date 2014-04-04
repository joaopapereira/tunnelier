/*
 * Tunnel.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#include "Tunnel.hpp"
#include <unistd.h>
#include <string.h>
#include <thread>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

using namespace std;
#define NIPQUAD(addr) \
         ((unsigned char *)&addr)[0], \
         ((unsigned char *)&addr)[1], \
         ((unsigned char *)&addr)[2], \
         ((unsigned char *)&addr)[3]
int authenticate_kbdint(ssh_session session, const char *password);

Tunnel::Tunnel(struct event_base *base, int localPort, Address destination, Address middle, User middleUser):
	base(base),
	localPort(localPort),
	destination(destination),
	middle(middle),
	middleUser(middleUser),
	thr(nullptr),
	tunnel_session(ssh_new()){
}
bool
Tunnel::start(){
	stillRunning = true;
	thread *thr = new thread(&Tunnel::run, *this);
	return true;
}

int
Tunnel::run(){
	struct sockaddr_in sin;
	base = event_base_new();
	if (!base)
		return false; /*XXXerr*/

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(localPort);
#if !USE_LISTENER
	listener = socket(AF_INET, SOCK_STREAM, 0);

	//evutil_make_socket_nonblocking(listener);
	if (fcntl(listener, F_SETFL, O_NDELAY) < 0) {
		perror("Can't set socket to non-blocking");
		exit(0);
	}

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
	//event_set(&listener_event, listener, EV_READ|EV_PERSIST, do_accept, (void *)this);
	//event_base_set(base, &listener_event);
	//event_add(&listener_event, NULL);
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

	event_base_dispatch(base);
	return true;
}

bool
Tunnel::stop(){
	stillRunning = false;
	return true;
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
	ssh_channel forwarding_channel;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 10;
	forwarding_channel = ssh_channel_new(ctx->tunnel_session);
	ssh_set_log_level( SSH_LOG_TRACE );
	if (forwarding_channel == NULL) {
		return;
	}
	int rc = ssh_channel_open_forward(forwarding_channel,
									ctx->destination.getHost().c_str(), ctx->destination.getPort(),
									"localhost", ctx->localPort);
	if (rc != SSH_OK)
	{
		ssh_channel_free(forwarding_channel);
		return;
	}


	/* We got a new connection! Set up a bufferevent for it. */
	struct event_base *base = evconnlistener_get_base(listener);
	Connection *con = new Connection(ctx, fd, forwarding_channel);
	con->socket_event = bufferevent_socket_new(
			ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(con->socket_event, Connection::socket_to_ssh, NULL, Connection::errorcb, (void*)con);
	//bufferevent_setcb(ssh_get_fd(ctx->tunnel_session), Connection::socket_to_ssh, NULL, Connection::errorcb, (void*)con);

	bufferevent_enable(con->socket_event, EV_READ|EV_WRITE);
	con->ssh_event = evtimer_new(base, Connection::ssh_to_socket, (void*)con);
	evtimer_add(con->ssh_event, static_cast<const timeval*>(&tv));
	//con->closed = false;

	struct ssh_channel_callbacks_struct *channel_cb = new struct ssh_channel_callbacks_struct();

	channel_cb->channel_data_function = &Connection::copy_chan_to_fd;
	channel_cb->userdata = con;
	ssh_callbacks_init(channel_cb);
	ssh_set_channel_callbacks(con->fw_channel, channel_cb);


	//con->ssh_event = event_new(base, -1, EV_PERSIST, Connection::ssh_to_socket, (void*)con);
	//event_add(con->ssh_event, static_cast<const timeval*>(&tv));

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
Connection::Connection(Tunnel* parent, int socket, ssh_channel forwarding_channel):
	parent(parent),
	socket(socket),
	fw_channel(forwarding_channel){
	cout << "Create connection!!"<<endl;
	// TODO Auto-generated constructor stub

}
int signal_delayed=0;
static struct termios terminal;
static void sigwindowchanged(int i){
  (void) i;
  signal_delayed=1;
}

static void setsignal(void){
    signal(SIGWINCH, sigwindowchanged);
    signal_delayed=0;
}

static void sizechanged(ssh_channel chan){
    struct winsize win = { 0, 0, 0, 0 };
    ioctl(1, TIOCGWINSZ, &win);
    ssh_channel_change_pty_size(chan,win.ws_col, win.ws_row);
    setsignal();
}

int read_from_socket(int socketfd, char * readBuff, int max_size, struct timeval timeout){
    fd_set read_flags,write_flags;
    int res;
    FD_ZERO(&read_flags);
    FD_ZERO(&write_flags);
    FD_SET(socketfd, &read_flags);

    res = select( socketfd+1, &read_flags,NULL,NULL,&timeout);
    if( res < 0 )
        return -1;
    else if( FD_ISSET(socketfd , &read_flags )){
        return  read(socketfd, readBuff, max_size-1);
    }
    return -2;
}


int
Connection::run_once(){
	 int listenfd = 0, connfd = 0, read_size = 0, ret = 0;
	 struct sockaddr_in serv_addr;
	 ssh_session client_session;
	 ssh_channel forwarding_channel = parent->forwarding_channel;

	 char sendBuff[1025];
	 time_t ticks;
	 int lus,r,rc;
	 ssh_channel channels[2], outchannels[2];
	 int eof=0;
	 int maxfd;
	 fd_set fds;
	 struct timeval timeout;

		 printf("Reading form socket\n");
		 fflush(stdout);
		 timeout.tv_sec=0;
		 timeout.tv_usec=1;
		 do{
			 read_size = read_from_socket( socket, sendBuff, sizeof(sendBuff), timeout );
			 if(read_size > 0 ){
				 printf("Just readed %s\n", sendBuff);
				 fflush(stdout);
				 sendBuff[read_size] = 0;
				 printf("RC from COnnect is: %d", rc);
				 ssh_channel_write(parent->forwarding_channel, sendBuff, read_size);
			 }
		 }while( read_size > 0 );
		 printf("Readed from socket going to channel!!\n");

		 channels[0]=parent->forwarding_channel; // set the first channel we want to read from
		 channels[1]=NULL;
		 ret=ssh_select(channels,outchannels,maxfd,&fds,&timeout);
		 if(signal_delayed)
			 sizechanged(parent->forwarding_channel);
		 if(ret==EINTR)
			 return 0;

		 printf("Reading from channel\n");
		 fflush(stdout);
		 while(parent->forwarding_channel && ssh_channel_is_open(parent->forwarding_channel) && (r = ssh_channel_poll(parent->forwarding_channel,0))!=0){
				 lus=ssh_channel_read(parent->forwarding_channel,sendBuff,sizeof(sendBuff) > r ? r : sizeof(sendBuff),0);
				 if(lus==-1){
					 fprintf(stderr, "Error reading channel: %s\n",
							 ssh_get_error(client_session));
					 return -1;
				 }
				 if(lus==0){
					 cerr << "channel closing...."<<endl;
					 ssh_channel_free(parent->forwarding_channel);
					 parent->forwarding_channel=channels[0]=NULL;
				 } else

				   if (write(socket, sendBuff,lus) < 0) {
					 fprintf(stderr, "Error writing to buffer\n");
					 return -1;
			   }
		 }


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
		ssh_channel_write(con->fw_channel, readBuff, readLen);
	}


	cout << "Exit socket_to_ssh"<<endl;
}
void
Connection::ssh_to_socket2(struct bufferevent *bev, void *ctx){
	cout << "Entered ssh_to_socket"<<endl;
	Connection * con = static_cast<Connection *>(ctx);
	char readBuff[MAX_READ];
	int r;
	int lus;
	while(con->fw_channel && ssh_channel_is_open(con->fw_channel) && (r = ssh_channel_poll(con->fw_channel,0))!=0){
	        lus=ssh_channel_read(con->fw_channel,readBuff,sizeof(readBuff) > r ? r : sizeof(readBuff),0);
	        if(lus==-1){
	            fprintf(stderr, "Error reading channel: %s\n",
	                    ssh_get_error(con->fw_channel));
	            return;
	        }
	        if(lus!=0){
	        	evbuffer_add_printf(bufferevent_get_output(bev), "%s", readBuff);
	        }
	}
	cout << "Exit ssh_to_socket"<<endl;
}
void
Connection::ssh_to_socket(int fd, short event, void *arg){
	//cout << "Entered ssh_to_socket"<<endl;
	Connection * con = static_cast<Connection *>(arg);
	/*char readBuff[MAX_READ];
	int r;
	int lus;
	while(con->fw_channel && ssh_channel_is_open(con->fw_channel) && (r = ssh_channel_poll(con->fw_channel,0))!=0){
	        lus=ssh_channel_read(con->fw_channel,readBuff,sizeof(readBuff) > r ? r : sizeof(readBuff),0);
	        if(lus==-1){
	            fprintf(stderr, "Error reading channel: %s\n",
	                    ssh_get_error(con->fw_channel));
	            goto RETRIGGER_TIMER;
	        }
	        if(lus!=0){
	        	//evbuffer_add_printf(bufferevent_get_output(con->socket_event), "%s", readBuff);
	        	write(con->socket, readBuff, lus);
	        }
	}
	cout << "Exit ssh_to_socket"<<endl;
RETRIGGER_TIMER:*/
	ssh_channel_poll(con->fw_channel,0);
	if( con->closed )
		return;
	evtimer_del(con->ssh_event);
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 10;
	con->ssh_event = evtimer_new(con->parent->base, Connection::ssh_to_socket, (void*)con);
	evtimer_add(con->ssh_event, static_cast<const timeval*>(&tv));
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
    	evtimer_del(con->ssh_event);

    } else if (events & BEV_EVENT_ERROR) {
        /* check errno to see what error occurred */
    	cout << "Closing connection with:" << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())<<endl;
		finished = 1;
    } else if (events & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
    	evtimer_del(con->ssh_event);
    }

    if (finished) {
        free(ctx);
        bufferevent_free(bev);
    }
    bufferevent_free(bev);
}

int
Connection::copy_chan_to_fd(ssh_session session,
                                           ssh_channel channel,
                                           void *data,
                                           uint32_t len,
                                           int is_stderr,
                                           void *userdata) {
    Connection* con = static_cast<Connection *>(userdata);
    int sz;
    (void)session;
    (void)channel;
    (void)is_stderr;

    sz = write(con->socket, data, len);
    cout << "Copy_chan called"<<endl;
    return sz;
}

