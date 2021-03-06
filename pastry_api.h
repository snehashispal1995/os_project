#ifndef PASTRY_API
#define PASTRY_API
#include<iostream>
#include<string>
#include <sstream>
#include "pastry_overlay.h"
#include "socket_layer.h"
#include "message_queue.h"

#include <fstream>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <cstdio>
#include <math.h>

#define MAX_ROWS 4
using namespace std;


vector<string> parse(string s, char delim);
void addToQueue(enum msg_type mType, string data);
long createNode(int port,char host[]);
void fetch_myIp_address(char host[NI_MAXHOST]);

class Pastry_api {

	std::thread *recvOverlayThread;	
	Pastry_overlay overlay;
	Socket_layer sockets;

	std::map<int,std::string> dht;
	int status = 1;

	public:
	int port=0;	
	char host[NI_MAXHOST];

	string ip;
	int nodeId;
	void init();
	std::string look_up(int key);
	void add_key_value_pair(int key,std::string value);
	void delete_key_value_pair(int key);
	void recv_overlay_thread();
	int recv_user_thread();

	void putOperation(string key,string value);
	void getOperation(string key);
	void printDHT();
	void replicate(int key, string value);
	void replicateOperation();

};

#endif