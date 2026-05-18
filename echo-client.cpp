#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#define endl '\n'
#define BUFSIZ 4096
using namespace std;

void usage(){
	cerr << "syntax : echo-client <ip> <port>" << endl;
	cerr << "sample : echo-client 192.168.10.2 1234" << endl;
}

void recv_thr(int s){
	char buf[BUFSIZ];
	while(true){
		auto len = recv(s, buf, sizeof(buf)-1, 0);
		if(len <= 0) break;
		buf[len] = '\0';
		cout << buf << flush;
	}
}

int main(int argc, char* argv[]){
	if(argc != 3){
		usage();
		return 1;
	}

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1){
		perror("socket");
		return 1;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(stoi(argv[2]));
	addr.sin_addr.s_addr = inet_addr(argv[1]);

	if(connect(s, (sockaddr*)&addr, sizeof(addr)) == -1){
		perror("connect");
		close(s);
		return 1;
	}

	thread t(recv_thr, s);

	string msg;
	while(getline(cin, msg)){
		msg += '\n';
		send(s, msg.c_str(), msg.size(), 0);
	}

	shutdown(s, SHUT_WR);
	t.join();
	close(s);

	return 0;
}
