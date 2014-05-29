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
using namespace jpCppLibs;
static string loggerModule("REQ");
static struct evhttp *http_server = nullptr;
static struct evhttp_bound_socket *handle;
void
generic_request_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = evbuffer_new();

	/*
       XXX add here code for managing non-subscription requests
	 */
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_DBG,"Request for %s from %s",req->uri, req->remote_host);
	/**if (debug)
		fprintf(stderr, "Request for %s from %s\n", req->uri, req->remote_host);*/

	evbuffer_add_printf(evb, "Come again :D");
	evhttp_send_reply(req, HTTP_NOTFOUND, "Cannot help you", evb);
	evbuffer_free(evb);
	return;
}

void
create_tunnel_handler_cb(struct evhttp_request *req, void *arg)
{
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"Going to try to create a tunnel!!");
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
	OneInstanceLogger::instance().log(loggerModule,M_LOG_LOW, M_LOG_DBG,"Create Request RAW: %s", requestDataBuffer);
	parsedSuccess = reader.parse(requestDataBuffer,
			root,
			false);

	Json::Value response = Json::Value();


	if(not parsedSuccess)
	{
		// Report failures and their locations
		// in the document.
		OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Failed parsing JSON %s", reader.getFormatedErrorMessages().c_str());
		evbuffer_add_printf(evb, "Error");
		evhttp_add_header(req->output_headers, "Content-Type", "application/json");
		evhttp_send_reply(req, HTTP_BADREQUEST, "Incorrect json structure", evb);
		return;
	}
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"JSON Parsed correctly");
	const Json::Value portInfo = root["port"];
	if(  portInfo.isObject() ){
		const Json::Value localPort = portInfo["local_port"];
		const Json::Value destinationCfg = portInfo["destination"];
		const Json::Value middleCfg = portInfo["middle"];
		if( localPort.isNull() ){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Port not present in the JSON request");
			response["Error"].append("Local port is not defined");
		}else if( destinationCfg.isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Destination not present in the JSON request");
			response["Error"].append("Destination is not defined");
		}else if( destinationCfg["port"].isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"'destination.port' not present in the JSON request");
			response["Error"].append("Destination.port is not defined");
		}else if( destinationCfg["address"].isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"'destination.address' not present in the JSON request");
			response["Error"].append("Destination.address is not defined");
		}else if( middleCfg["address"].isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"'middle.address' not present in the JSON request");
			response["Error"].append("middle.address is not defined");
		}else if( middleCfg["port"].isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"'middle.port' not present in the JSON request");
			response["Error"].append("middle.port is not defined");
		}else if( middleCfg["user"].isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"'middle.user' not present in the JSON request");
			response["Error"].append("middle.user is not defined");
		}else if( middleCfg["password"].isNull()){
			OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"'middle.password' not present in the JSON request");
			response["Error"].append("middle.password is not defined");
		}else{
			OneInstanceLogger::instance().log(loggerModule,M_LOG_HGH, M_LOG_DBG,"All parameters present!");


			Address destination(destinationCfg["address"].asString(), destinationCfg["port"].asInt());
			Address middle(middleCfg["address"].asString(),middleCfg["port"].asInt());
			User middleUser(middleCfg["user"].asString(), middleCfg["password"].asString());
			if( 0 == rh->getManager()->createTunnel(localPort.asInt(), middle, middleUser, destination) ){
				OneInstanceLogger::instance().log(loggerModule,M_LOG_HGH, M_LOG_INF,"Tunnel created with success!");
				response["Success"] = "Tunnel created with success";
			}else{
				OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Error creating tunnel!");
				response["Error"] = "Error creating the tunnel!!";
			}

		}
	}else{
		OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Port map is not present!");
		response["Error"].append("Port map is not passed");
	}


	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"The response to the request will be: %s",response.toStyledString().c_str());
	evbuffer_add_printf(evb, "%s", response.toStyledString().c_str());
	evhttp_add_header(req->output_headers, "Content-Type", "application/json");
	if( response["Error"].isNull())
		evhttp_send_reply(req, HTTP_OK, "Hello", evb);
	else
		evhttp_send_reply(req, HTTP_BADREQUEST, "Bad request", evb);
	evbuffer_free(evb);
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"Tunnel creation ended");
	return;
}
void
close_tunnel_handler_cb(struct evhttp_request *req, void *arg)
{
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"Request to drop tunnel");
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
	OneInstanceLogger::instance().log(loggerModule,M_LOG_LOW, M_LOG_DBG,"Drop Request RAW: %s", requestDataBuffer);
	parsedSuccess = reader.parse(requestDataBuffer,
			root,
			false);

	Json::Value response = Json::Value();


	if(not parsedSuccess)
	{
		OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Failed parsing JSON %s", reader.getFormatedErrorMessages().c_str());
		// Report failures and their locations
		// in the document.
		evbuffer_add_printf(evb, "Error");
		evhttp_add_header(req->output_headers, "Content-Type", "application/json");
		evhttp_send_reply(req, HTTP_BADREQUEST, "Incorrect json structure", evb);
		return;
	}

	const Json::Value portInfo = root["port"];
	const Json::Value forceClose = root["force"];
	if(  !portInfo.isNull() ){

			bool forceSSHClose = true;
			if(forceClose.isNull()){
				forceSSHClose = false;
			}
			if( 0 == rh->getManager()->closeTunnel(portInfo.asInt(), forceSSHClose) ){
				OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Local socket closed successfully");
				response["Success"] = "Tunnel closed";
			}else{
				OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Error closing the tunnel!");
				response["Error"] = "Error closing the tunnel!!";
			}

	}else{
		OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"Port map not present");
		response["Error"].append("Port map is not passed");
	}


	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_ERR,"The response to the request will be: %s",response.toStyledString().c_str());

	evbuffer_add_printf(evb, "%s", response.toStyledString().c_str());
	evhttp_add_header(req->output_headers, "Content-Type", "application/json");
	if( response["Error"].isNull())
		evhttp_send_reply(req, HTTP_OK, "Hello", evb);
	else
		evhttp_send_reply(req, HTTP_BADREQUEST, "Bad request", evb);
	evbuffer_free(evb);
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"Ended request to drop tunnel");
	return;
}
void
stop_server_handler_cb(struct evhttp_request *req, void *arg)
{
	RequestHandler *rh = static_cast<RequestHandler*>(arg);
	OneInstanceLogger::instance().log(loggerModule,M_LOG_HGH, M_LOG_INF,"Going to stop server!");
	struct evbuffer *evb = evbuffer_new();
	Json::Value response = Json::Value();
	response["Success"] = "Server will be stopped!";
	evbuffer_add_printf(evb, "%s", response.toStyledString().c_str());
	evhttp_add_header(req->output_headers, "Content-Type", "application/json");
	evhttp_send_reply(req, HTTP_OK, "Hello", evb);
	evbuffer_free(evb);
	event_base_loopexit(rh->getHttpBase(), nullptr);
	event_base_loopbreak(rh->getHttpBase());
	evhttp_del_accept_socket(http_server, handle);
	OneInstanceLogger::instance().log(loggerModule,M_LOG_HGH, M_LOG_DBG,"Exited from loop");
}
thread RequestHandler::start_server(){
	return thread(&RequestHandler::create_server, *this);
}

int
RequestHandler::create_server(){
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"Creating HTTP Server");

	//event_init();

	//http_server = evhttp_start(ip_address.c_str(), port);
	http_server = evhttp_new(mem->getEventHTTPBase());
	if (http_server == nullptr) {
		OneInstanceLogger::instance().log(loggerModule,M_LOG_MAX, M_LOG_ERR, "Error creating HTTP Server in Port %d", port);
		exit(1);
	}


	OneInstanceLogger::instance().log(loggerModule,M_LOG_LOW, M_LOG_TRC,"Going to set callbacks!");
	evhttp_set_cb(http_server, "/create", create_tunnel_handler_cb, this);
	evhttp_set_cb(http_server, "/drop", close_tunnel_handler_cb, this);
	evhttp_set_cb(http_server, "/stopserver", stop_server_handler_cb, this);

	/* XXX default handler */
	evhttp_set_gencb(http_server, generic_request_handler, this);
	OneInstanceLogger::instance().log(loggerModule,M_LOG_LOW, M_LOG_TRC,"REST callbacks set!");
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_DBG) << "Binding on: " << ip_address.c_str() << ":" << port<< std::endl;
	handle = evhttp_bind_socket_with_handle(http_server, ip_address.c_str(), port);
	OneInstanceLogger::instance().log(loggerModule,M_LOG_LOW, M_LOG_TRC,"Port and address bound!");
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
			OneInstanceLogger::instance().log(loggerModule,M_LOG_MAX, M_LOG_ERR, "Error getting socket name");
			return 1;
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
			inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
		} else if (ss.ss_family == AF_INET6) {
			got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
			inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
		} else {
			OneInstanceLogger::instance().log(loggerModule,M_LOG_MAX, M_LOG_ERR, "Weird address family %d", ss.ss_family);
			return 1;
		}
		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
				sizeof(addrbuf));
		if (addr) {
			OneInstanceLogger::instance().log(loggerModule,M_LOG_MAX, M_LOG_INF, "Listening at %s port %d", addr, got_port);
		} else {
			OneInstanceLogger::instance().log(loggerModule,M_LOG_MAX, M_LOG_ERR, "Failed to create HTTP server at %s port %d", addr, got_port);
			return 1;
		}
	}
	OneInstanceLogger::instance().log(loggerModule,M_LOG_HGH, M_LOG_INF, "Ready to start processing HTTP Events");
	event_base_dispatch(mem->getEventHTTPBase());
	OneInstanceLogger::instance().log(loggerModule,M_LOG_NRM, M_LOG_TRC,"HTTP Server stopped");

}
