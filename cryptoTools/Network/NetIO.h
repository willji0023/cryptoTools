#ifndef EMP_NETWORK_IO_CHANNEL
#define EMP_NETWORK_IO_CHANNEL

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "emp-tool/io/io_channel.h"
using std::string;

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace emp {

class NetIO: public IOChannel<NetIO> { public:
	bool is_server;
	int mysocket = -1;
	int consocket = -1;
	FILE * stream = nullptr;
	char * buffer = nullptr;
	bool has_sent = false;
	string addr;
	unsigned int port;
	NetIO(const char * address, unsigned int port, bool quiet = false) {
    	std::cout << "Netio Object Creation" << std::endl;// TODO: remove
		this->port = port & 0xFFFF;
		is_server = (address == nullptr);
		if (address == nullptr) {
			std::cout << "Server: Creation" << std::endl;// TODO: remove
			struct sockaddr_in dest;
			struct sockaddr_in serv;
			socklen_t socksize = sizeof(struct sockaddr_in);
			memset(&serv, 0, sizeof(serv));
			serv.sin_family = AF_INET;
			serv.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
			std::cout << "Server: Set to any address: " << serv.sin_addr.s_addr << std::endl;
			serv.sin_port = htons(port);           /* set the server port number */  
            std::cout << "Server: Set to server port " << serv.sin_port<< std::endl;// TODO: remove  
			mysocket = socket(AF_INET, SOCK_STREAM, 0);
			int reuse = 1;
			setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
			if(bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) < 0) {
				perror("error: bind");
				exit(1);
			}
			if(listen(mysocket, 1) < 0) {
				perror("error: listen");
				exit(1);
			}
			std::cout << "Server: Socket made" << std::endl;// TODO: remove  
			consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
			std::cout << "Server: Socket connection made with client." << std::endl;// TODO: remove  
			close(mysocket);
		}
		else {
            std::cout << "Client: Creation" << std::endl;// TODO: remove
			addr = string(address);
			
			struct sockaddr_in dest;
			memset(&dest, 0, sizeof(dest));
			dest.sin_family = AF_INET;
			dest.sin_addr.s_addr = inet_addr(address);
	        std::cout << "Client: Set to address " << dest.sin_addr.s_addr << std::endl;// TODO: remove
			dest.sin_port = htons(port);
	        std::cout << "Client: Set to port " << dest.sin_port << std::endl;// TODO: remove

			while(1) {
				consocket = socket(AF_INET, SOCK_STREAM, 0);
				// std::cout << "Client: Waiting for connection..." << std::endl;
				
				if (connect(consocket, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == 0) {
					std::cout << "Socket connection made with server" << std::endl;// TODO: remove  
					break;
				}
				
				close(consocket);
				usleep(1000);
			}
		}
		set_nodelay();
		std::cout << "No delay set" << std::endl;// TODO: remove  
		std::cout << "Consocket: " << consocket << std::endl; // TODO: remove
		stream = fdopen(consocket, "wb+");
		buffer = new char[NETWORK_BUFFER_SIZE];
		std::cout << "Buffer initialized" << std::endl;// TODO: remove  
		memset(buffer, 0, NETWORK_BUFFER_SIZE);
		setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);
		if(!quiet)
			std::cout << "connected\n";
	}

	void sync() {
		int tmp = 0;
		if(is_server) {
			send_data_internal(&tmp, 1);
			recv_data_internal(&tmp, 1);
		} else {
			recv_data_internal(&tmp, 1);
			send_data_internal(&tmp, 1);
			flush();
		}
	}

	~NetIO() {
		flush();
		fclose(stream);
		delete[] buffer;
	}

	void set_nodelay() {
		const int one=1;
		setsockopt(consocket,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one));
	}

	void set_delay() {
		const int zero = 0;
		setsockopt(consocket,IPPROTO_TCP,TCP_NODELAY,&zero,sizeof(zero));
	}

	void flush() {
		fflush(stream);
	}

	void send_data_internal(const void * data, int len) {
		int sent = 0;
		std::cout << "Called send_data_internal"  << std::endl;
		while(sent < len) {
			int res = fwrite(sent + (char*)data, 1, len - sent, stream);
			if (res >= 0)
				sent+=res;
			else
				fprintf(stderr,"error: net_send_data %d\n", res);
		}
		has_sent = true;
	}

	void recv_data_internal(void  * data, int len) {
		if(has_sent)
			fflush(stream);
		has_sent = false;
		int sent = 0;
		std::cout << "sent: " << sent << " len: " << len << std::endl;
		std::cout << "data: " << data << std::endl;
		while(sent < len) {
			int res = fread(sent + (char*)data, 1, len - sent, stream);
			std::cout << "sent: " << sent << " len: " << len << " res: " << res << std::endl; // TODO: DEBU	G
			if (res >= 0)
				sent += res;
			else 
				fprintf(stderr,"error: net_send_data %d\n", res);
		}
	}

	void send(const unsigned char* data, uint64_t size) {
		send_data(data, size);
	}

	void recv(unsigned char* data, uint64_t size) {
		if (is_server) {
			std::cout << "Netio server recv called" << std::endl; // TODO: remove
		} else {
			std::cout << "Netio client recv called" << std::endl; // TODO: remove
		}
		recv_data(data, size);
	}

	void asyncCancel(std::function<void()> completionHandle) {}
};
}  // namespace emp
#endif  //NETWORK_IO_CHANNEL