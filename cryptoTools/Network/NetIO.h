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
	bool canceled = false;
	NetIO(const char * address, unsigned int port, bool quiet = false) {
		this->port = port & 0xFFFF;
		is_server = (address == nullptr);
		if (address == nullptr) {
			struct sockaddr_in dest;
			struct sockaddr_in serv;
			socklen_t socksize = sizeof(struct sockaddr_in);
			memset(&serv, 0, sizeof(serv));
			serv.sin_family = AF_INET;
			serv.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
			serv.sin_port = htons(port);           /* set the server port number */  
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
			consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
			close(mysocket);
		}
		else {
			addr = string(address);
			
			struct sockaddr_in dest;
			memset(&dest, 0, sizeof(dest));
			dest.sin_family = AF_INET;
			dest.sin_addr.s_addr = inet_addr(address);
			dest.sin_port = htons(port);

			while(1) {
				consocket = socket(AF_INET, SOCK_STREAM, 0);
				// std::cout << "Client: Waiting for connection..." << std::endl;
				
				if (connect(consocket, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == 0) {
					break;
				}
				
				close(consocket);
				usleep(1000);
			}
		}
		set_nodelay();
		stream = fdopen(consocket, "wb+");
		buffer = new char[NETWORK_BUFFER_SIZE];
		memset(buffer, 0, NETWORK_BUFFER_SIZE);
		setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);
		if(!quiet)
			std::cout << "connected\n";
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
		while(sent < len) {
			int res = fwrite(sent + (char*)data, 1, len - sent, stream);
			fflush(stream);
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
		while(sent < len) {
			fflush(stream);
			int res = fread(sent + (char*)data, 1, len - sent, stream);
			if (res >= 0)
				sent += res;
			else 
				fprintf(stderr,"error: net_send_data %d\n", res);
		}
		fflush(stream);
	}

	void send(const unsigned char* data, uint64_t size) {
		if (canceled)
			return;
		send_data(data, size);
	}

	void recv(unsigned char* data, uint64_t size) {
		if (canceled)
			return;
		recv_data(data, size);
	}

	void asyncCancel(std::function<void()> completionHandle) {
        canceled = true;
        completionHandle();
	}
};
}  // namespace emp
#endif  //NETWORK_IO_CHANNEL