/*
    Comet-c, a high performance Comet server.
    Copyright (C) 2008  Alejo Sanchez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <event.h>
#include <evhttp.h>
#include <json/json.h>
#include "requestHandler.hpp"
int debug = 0;
using namespace std;
void
generic_request_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = evbuffer_new();

	/*
       XXX add here code for managing non-subscription requests
	 */
	if (debug)
		fprintf(stderr, "Request for %s from %s\n", req->uri, req->remote_host);

	evbuffer_add_printf(evb, "Come again :D");
	evhttp_send_reply(req, HTTP_NOTFOUND, "Cannot help you", evb);
	evbuffer_free(evb);
	return;
}

void
create_tunnel_handler_cb(struct evhttp_request *req, void *arg)
{
	RequestHandler *rh = static_cast<RequestHandler*>(arg);
	Json::Value root;
	Json::Reader reader;

	bool parsedSuccess;

	struct evbuffer *evb = evbuffer_new();
	// Request
	struct evbuffer *requestBuffer;
	size_t requestLen;
	char *requestDataBuffer;
	char errorText[1024];


	// Process Request
	requestBuffer = evhttp_request_get_input_buffer(req);
	requestLen = evbuffer_get_length(requestBuffer);

	requestDataBuffer = (char *)malloc(sizeof(char) * requestLen);
	memset(requestDataBuffer, 0, requestLen);
	evbuffer_copyout(requestBuffer, requestDataBuffer, requestLen);
	cout << "Request: "<<requestDataBuffer<<endl;
	parsedSuccess = reader.parse(requestDataBuffer,
			root,
			false);

	Json::Value response = Json::Value();


	if(not parsedSuccess)
	{
		// Report failures and their locations
		// in the document.
		cout<<"Failed to parse JSON"<<endl
				<<reader.getFormatedErrorMessages()
				<<endl;
		evbuffer_add_printf(evb, "Error");
		evhttp_add_header(req->output_headers, "Content-Type", "application/json");
		evhttp_send_reply(req, HTTP_BADREQUEST, "Incorrect json structure", evb);
		return;
	}

	const Json::Value portInfo = root["port"];
	if(  portInfo.isObject() ){
		const Json::Value localPort = portInfo["local_port"];
		const Json::Value destinationCfg = portInfo["destination"];
		const Json::Value middleCfg = portInfo["middle"];
		if( localPort.isNull() ){
			cout << "No port" << endl;
			response["Error"].append("Local port is not defined");
		}else if( destinationCfg.isNull()){
			cout << "No destination" << endl;
			response["Error"].append("Destination is not defined");
		}else if( destinationCfg["port"].isNull()){
			cout << "No destination.port" << endl;
			response["Error"].append("Destination.port is not defined");
		}else if( destinationCfg["address"].isNull()){
			cout << "No destination.address" << endl;
			response["Error"].append("Destination.address is not defined");
		}else if( middleCfg["address"].isNull()){
			cout << "No middle.address" << endl;
			response["Error"].append("middle.address is not defined");
		}else if( middleCfg["port"].isNull()){
			cout << "No middle.port" << endl;
			response["Error"].append("middle.port is not defined");
		}else if( middleCfg["user"].isNull()){
			cout << "No middle.user" << endl;
			response["Error"].append("middle.user is not defined");
		}else if( middleCfg["password"].isNull()){
			cout << "No middle.password" << endl;
			response["Error"].append("middle.password is not defined");
		}else{
			cout << "Complete" << endl;


			Address destination(destinationCfg["address"].asString(), destinationCfg["port"].asInt());
			Address middle(middleCfg["address"].asString(),middleCfg["port"].asInt());
			User middleUser(middleCfg["user"].asString(), middleCfg["password"].asString());
			if( 0 == rh->getManager()->createTunnel(localPort.asInt(), middle, middleUser, destination) ){
				response["Success"] = "Tunnel created with success";
			}else
				response["Error"] = "Error creating the tunnel!!";

			//event_base_dispatch(mem->getEventBase());
		}
	}else{
		response["Error"].append("Port map is not passed");
	}
	printf("%s\n", evhttp_request_uri(req));

	if (debug)
		fprintf(stderr, "Request for %s from %s\n", req->uri, req->remote_host);
	/*
       XXX add here code for managing non-subscription requests
	 */
	//evbuffer_add_printf(evb, "{\"done\": 1}");
	cout << "My response will be: "<<response.toStyledString()<<endl;
	evbuffer_add_printf(evb, "%s", response.toStyledString().c_str());
	evhttp_add_header(req->output_headers, "Content-Type", "application/json");
	if( response["Error"].isNull())
		evhttp_send_reply(req, HTTP_OK, "Hello", evb);
	else
		evhttp_send_reply(req, HTTP_BADREQUEST, "Bad request", evb);
	evbuffer_free(evb);
	return;
}
void
close_tunnel_handler_cb(struct evhttp_request *req, void *arg)
{
	RequestHandler *rh = static_cast<RequestHandler*>(arg);
	Json::Value root;
	Json::Reader reader;

	bool parsedSuccess;

	struct evbuffer *evb = evbuffer_new();
	// Request
	struct evbuffer *requestBuffer;
	size_t requestLen;
	char *requestDataBuffer;
	char errorText[1024];


	// Process Request
	requestBuffer = evhttp_request_get_input_buffer(req);
	requestLen = evbuffer_get_length(requestBuffer);

	requestDataBuffer = (char *)malloc(sizeof(char) * requestLen);
	memset(requestDataBuffer, 0, requestLen);
	evbuffer_copyout(requestBuffer, requestDataBuffer, requestLen);
	cout << "Drop Request: "<<requestDataBuffer<<endl;
	parsedSuccess = reader.parse(requestDataBuffer,
			root,
			false);

	Json::Value response = Json::Value();


	if(not parsedSuccess)
	{
		// Report failures and their locations
		// in the document.
		cout<<"Failed to parse JSON"<<endl
				<<reader.getFormatedErrorMessages()
				<<endl;
		evbuffer_add_printf(evb, "Error");
		evhttp_add_header(req->output_headers, "Content-Type", "application/json");
		evhttp_send_reply(req, HTTP_BADREQUEST, "Incorrect json structure", evb);
		return;
	}

	const Json::Value portInfo = root["port"];
	const Json::Value forceClose = root["force"];
	if(  portInfo.isObject() ){
		if( portInfo.isNull() ){
			cout << "No port" << endl;
			response["Error"].append("Local port is not defined");
		}else{
			bool forceSSHClose = true;
			if(forceClose.isNull()){
				forceSSHClose = false;
			}
			if( 0 == rh->getManager()->closeTunnel(portInfo.asInt(), forceSSHClose) ){
				response["Success"] = "Tunnel closed";
			}else
				response["Error"] = "Error closing the tunnel!!";
		}
	}else{
		response["Error"].append("Port map is not passed");
	}
	printf("%s\n", evhttp_request_uri(req));

	if (debug)
		fprintf(stderr, "Request for %s from %s\n", req->uri, req->remote_host);
	/*
       XXX add here code for managing non-subscription requests
	 */
	//evbuffer_add_printf(evb, "{\"done\": 1}");
	cout << "My response will be: "<<response.toStyledString()<<endl;
	evbuffer_add_printf(evb, "%s", response.toStyledString().c_str());
	evhttp_add_header(req->output_headers, "Content-Type", "application/json");
	if( response["Error"].isNull())
		evhttp_send_reply(req, HTTP_OK, "Hello", evb);
	else
		evhttp_send_reply(req, HTTP_BADREQUEST, "Bad request", evb);
	evbuffer_free(evb);
	return;
}

thread RequestHandler::start_server(){
	return thread(&RequestHandler::create_server, *this);
}

int
RequestHandler::create_server(){
	struct evhttp *http_server = NULL;
	struct evhttp_bound_socket *handle;
	//event_init();

	//http_server = evhttp_start(ip_address.c_str(), port);
	http_server = evhttp_new(mem->getEventHTTPBase());
	if (http_server == NULL) {
		fprintf(stderr, "Error starting comet server on port %d\n",
				port);
		exit(1);
	}


	evhttp_set_cb(http_server, "/create", create_tunnel_handler_cb, this);
	evhttp_set_cb(http_server, "/drop", close_tunnel_handler_cb, this);

	/* XXX default handler */
	evhttp_set_gencb(http_server, generic_request_handler, this);
	handle = evhttp_bind_socket_with_handle(http_server, ip_address.c_str(), port);
	{
		/* Extract and display the address we're listening on. */
		struct sockaddr_storage ss;
		evutil_socket_t fd;
		ev_socklen_t socklen = sizeof(ss);
		char addrbuf[128];
		void *inaddr;
		const char *addr;
		int got_port = -1;
		fd = evhttp_bound_socket_get_fd(handle);
		memset(&ss, 0, sizeof(ss));
		if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
			perror("getsockname() failed");
			return 1;
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
			inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
		} else if (ss.ss_family == AF_INET6) {
			got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
			inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
		} else {
			fprintf(stderr, "Weird address family %d\n",
					ss.ss_family);
			return 1;
		}
		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
				sizeof(addrbuf));
		if (addr) {
			printf("Listening on %s:%d\n", addr, got_port);
			//evutil_snprintf(uri_root, sizeof(uri_root),
			//    "http://%s:%d",addr,got_port);
		} else {
			fprintf(stderr, "evutil_inet_ntop failed\n");
			return 1;
		}
	}
	event_base_dispatch(mem->getEventHTTPBase());
	//event_dispatch();  /* Brooom, brooom */
}
