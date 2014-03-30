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

evbuffer_add_printf(evb, "And i am the generic");
evhttp_send_reply(req, HTTP_OK, "Hello", evb);
evbuffer_free(evb);
return;
}

/*
 * Bayeux /meta handler
 */

/*void
bayeux_meta_handler_cb(struct evhttp_request *req, void *arg)
{
struct evbuffer *evb = evbuffer_new();

if (debug)
   fprintf(stderr, "Request for %s from %s\n", req->uri, req->remote_host);

       XXX add here code for managing non-subscription requests

evbuffer_add_printf(evb, "I handle meta data");
evhttp_send_reply(req, HTTP_OK, "Hello", evb);
evbuffer_free(evb);
return;
}*/
void
create_tunnel_handler_cb(struct evhttp_request *req, void *arg)
{
	SharedMemory *mem = (SharedMemory*)arg;
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
         evhttp_send_reply(req, HTTP_BADREQUEST, "Hello", evb);
         return;
       }

       const Json::Value portInfo = root["port"];
       if(  portInfo.isObject() ){
    	   const Json::Value localPort = portInfo["local_port"];
    	   const Json::Value destination = portInfo["destination"];
    	   if( localPort.isNull() ){
    		   cout << "No port" << endl;
    		   response["Error"].append("Local port is not defined");
    	   }else if( destination.isNull()){
    		   cout << "No destination" << endl;
    		   response["Error"].append("Destination is not defined");
    	   }else if( destination["port"].isNull()){
    		   cout << "No destination.port" << endl;
    		   response["Error"].append("Destination.port is not defined");
    	   }else if( destination["address"].isNull()){
    		   cout << "No destination.address" << endl;
    		   response["Error"].append("Destination.address is not defined");
    	   }else{
    		   cout << "Complete" << endl;
    		   response["Success"] = "Tunnel created with success";
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

/*void
usage(const char *progname)
{
fprintf(stderr,
   "%s: [-B] [-d] [-p port] [-l addr]\n"
   "\t -B enable Bayeux support (on)\n"
   "\t -d enable debug (off)\n"
   "\t -l local address to bind comet server on (127.0.0.1)\n"
   "\t -p port port number to create comet server on (8080)\n"
   "\t (C) Alejo Sanchez - AGPL)\n",
   progname);
}*/

/*int
main(int argc, char **argv)
{
extern char   *optarg;
extern int     optind;
short          http_port = 8080;
const char          *http_addr = "127.0.0.1";
struct evhttp *http_server = NULL;
int            c;
int            bayeux = 1;

while ((c = getopt(argc, argv, "Bd:p:l:")) != -1)
   switch(c) {
   case 'B':
       bayeux++;
       break;
   case 'd':
       debug++;
       break;
   case 'p':
       http_port = atoi(optarg);
       if (http_port == 0) {
           usage(argv[0]);
           exit(1);
       }
       break;
   case 'l':
       http_addr = optarg;
       break;
   default:
       usage(argv[0]);
       exit(1);
   }
argc -= optind;
argv += optind;

 init libevent
event_init();

http_server = evhttp_start(http_addr, http_port);
if (http_server == NULL) {
   fprintf(stderr, "Error starting comet server on port %d\n",
       http_port);
   exit(1);
}

 XXX bayeux /meta handler
if (bayeux)
   evhttp_set_cb(http_server, "/meta", bayeux_meta_handler_cb, NULL);
evhttp_set_cb(http_server, "/create", create_tunnel_handler_cb, NULL);

 XXX default handler
evhttp_set_gencb(http_server, generic_request_handler, NULL);

fprintf(stderr, "Comet server started on port %d\n", http_port);
event_dispatch();   Brooom, brooom

exit(0);  UNREACHED ?
}*/

thread RequestHandler::start_server(){
	return thread(&RequestHandler::create_server, *this);
}

int
RequestHandler::create_server(){
	struct evhttp *http_server = NULL;
	event_init();

	http_server = evhttp_start(ip_address.c_str(), port);
	if (http_server == NULL) {
	   fprintf(stderr, "Error starting comet server on port %d\n",
	       port);
	   exit(1);
	}


	evhttp_set_cb(http_server, "/create", create_tunnel_handler_cb, this);

	/* XXX default handler */
	evhttp_set_gencb(http_server, generic_request_handler, this);

	fprintf(stderr, "Comet server started on port %d\n", port);
	event_dispatch();  /* Brooom, brooom */
}
