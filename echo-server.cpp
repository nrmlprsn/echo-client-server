#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>

#define endl '\n'
#define BUFSIZ 4096
using namespace std;

vector<int> clients;
mutex client_mutex;

void usage(){
	cerr << "syntax : echo-server <port> [-e[-b]]" << endl;
	cerr << "sample : echo-server 1234 -e -b" << endl;
}

void add_client(int s){
	lock_guard<mutex> lock(client_mutex);
	clients.push_back(s);
}

void remove_client(int s){
	lock_guard<mutex> lock(client_mutex);
	clients.erase(remove(clients.begin(), clients.end(), s), clients.end());
}

void broadcast(const char* buf, ssize_t len){
	lock_guard<mutex> lock(client_mutex);
	for(int& client: clients) send(client, buf, len, 0);
}

void handle_client(int s, bool echo, bool broadcast_mode){
	add_client(s);

	char buf[BUFSIZ];
	while(true){
		auto len = recv(s, buf, sizeof(buf)-1, 0);
		if(len <= 0) break;
		buf[len] = '\0';
		cout << buf << flush;

		if(broadcast_mode) broadcast(buf, len);
		else if(echo) send(s, buf, len, 0);
	}

	remove_client(s);
	close(s);
}

int main(int argc, char* argv[]){
	if(argc < 2 || argc > 4){
		usage();
		return 1;
	}

	bool echo = false;
	bool broadcast_mode = false;

	for(int i=2;i<argc;i++){
		string opt = argv[i];
		if(opt == "-e") echo = true;
		else if(opt == "-b") broadcast_mode = true;
		else{
			usage();
			return 1;
		}
	}

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1){
		perror("socket");
		return 1;
	}

	int opt = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // reuse addr after restart
	
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(stoi(argv[1]));

	if(bind(s, (sockaddr*)&addr, sizeof(addr)) == -1){
		perror("bind");
		close(s);
		return 1;
	}

	if(listen(s, 5) == -1){
		perror("listen");
		close(s);
		return 1;
	}

	while(true){
		int client_s = accept(s, nullptr, nullptr);
		if(client_s == -1){
			perror("accept");
			continue;
		}
		thread(handle_client, client_s, echo, broadcast_mode).detach();
	}

	close(s);
	return 0;
}

