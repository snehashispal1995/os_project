#include "pastry_overlay.h"
#include <cmath>
#include <climits>
#include <cstring>
#include <vector>
#include <sstream>
#include <iostream>

using namespace std;
void swap(int &a,int &b) {
	int temp = a;
	a = b;
	b = temp;
}


extern Message_queue pastry_api_overlay_in, pastry_api_user_in;
extern Message_queue pastry_overlay_socket_in, pastry_overlay_api_in;
extern Message_queue pastry_socket_overlay_in;

void print_in_hex(int key,int width) {

	char format[10];
	sprintf(format,"%%0%dX ",width);
	printf(format,key);
}

int Pastry_overlay :: get_hex_at_pos(int key,int pos) {

	char hex[20],format[10];
	sprintf(format,"%%0%dX",max_rows);
	sprintf(hex,format,key);
	if (hex[pos] <= '9' && hex[pos] >= '0') return hex[pos] - '0';
	else if(hex[pos] >= 'A' && hex[pos] <= 'F') return 10 + hex[pos] - 'A';
}


void Pastry_overlay :: init(int current_node_id,Socket_layer *sl,int l_size,int m_size,int max_rows,int max_cols) {

	this->current_node_id = current_node_id;
	this->l_size = l_size;
	this->m_size = m_size;
	this->max_rows = max_rows;
	this->max_cols = max_cols;
	this->sock_layer = sl;
	
	leaf_set = new key_type[l_size];
	for(int i = 0; i < l_size/2; i++) 
		leaf_set[i] = 0;
	for(int i = l_size/2; i < l_size; i++)
		leaf_set[i] = INT_MAX;

	neighbour_set = new key_type[m_size]{INT_MAX};
	for(int i = 0; i < l_size; i++) neighbour_set[i] = INT_MAX;

	route_table = new key_type*[max_rows];
	for(int i = 0; i < max_rows; i++) {
		route_table[i] = new key_type[max_cols];
		for(int j=0; j < max_cols; j++) route_table[i][j] = INT_MAX;
	}
	
	thread *recv_socket_thread = new thread(&Pastry_overlay::recv_socket_thread,this);
	thread *recv_overlay_thread = new thread(&Pastry_overlay::recv_api_thread,this);
}

int Pastry_overlay :: longest_prefix(int key) {

	char cur_node[10];
	char match_key[10];
	char format[10];

	sprintf(format,"%%0%dX",max_rows);
	sprintf(cur_node,format,current_node_id);
	sprintf(match_key,format,key);
	int i=0;
	while(cur_node[i] != '\0' && cur_node[i] == match_key[i]) i++;
	return i;

}


void Pastry_overlay :: add_to_table(int key) {


	if(key > current_node_id && key < leaf_set[l_size -1]) {
		for(int i = l_size/2; i < l_size; i++) 
			if (leaf_set[i] > key) {
				route_mutex.lock();
				swap(leaf_set[i],key);
				route_mutex.unlock();
			}
	}
	else if(key < current_node_id && key > leaf_set[l_size/2-1]) {
		for(int i = 0; i < l_size/2; i++)
			if (leaf_set[i] < key) {
				route_mutex.lock();
				swap(leaf_set[i],key);
				route_mutex.unlock();
			}
	}
	if (key != 0 || key != INT_MAX) {

		int shl = longest_prefix(key);
		int pos = get_hex_at_pos(key,shl);
		route_mutex.lock();
		if((int)abs(key - current_node_id) < (int)abs(route_table[shl][pos] - current_node_id)) route_table[shl][pos] = key;
		route_mutex.unlock();
	}
}

void Pastry_overlay :: remove_from_table(int key) {

	if(key > current_node_id && key < leaf_set[l_size -1]) {
		for(int i = l_size/2; i < l_size; i++) 
			if (leaf_set[i] == key) {
				route_mutex.lock();
				leaf_set[i] = INT_MAX;
				sort(leaf_set + l_size/2,leaf_set + l_size);
				route_mutex.unlock();
			}

	}
	else if(key < current_node_id && key > leaf_set[l_size/2 -1]) {
		for(int i = 0; i < l_size/2; i++)
			if (leaf_set[i] == key) {
				route_mutex.lock();
				leaf_set[i] = 0;
				sort(leaf_set,leaf_set + l_size/2,greater<int>());
				route_mutex.unlock();
			}
	}
	else {

		int shl = longest_prefix(key);
		int pos = get_hex_at_pos(key,shl);
		route_mutex.lock();
		if(route_table[shl][pos] == key) route_table[shl][pos] = INT_MAX;
		route_mutex.unlock();
	}

}

key_type Pastry_overlay :: get_next_route(int key) {

	if(key <= leaf_set[l_size-1] && key >=leaf_set[l_size/2-1]) {

		int min_node = (int)abs(key - current_node_id),dest = current_node_id;

		route_mutex.lock();
		for(int i = 0; i < l_size; i++) {
			if (min_node > ((int)abs(key - leaf_set[i]))) {
				min_node = (int)abs(key - leaf_set[i]);
				dest = leaf_set[i];
			}
		}
		route_mutex.unlock();
		return dest;
	}
	else {

		int shl = longest_prefix(key);
		int pos = get_hex_at_pos(key,shl);

		route_mutex.lock();
		if( route_table[shl][pos] != INT_MAX) {
			route_mutex.unlock();
			return route_table[shl][pos];
		}
		else {

			int min_node = (int)abs(key - current_node_id),dest;

			for(int i = 0; i < l_size; i++) {
				if (min_node > ((int)abs(key - leaf_set[i]))) {
					min_node = (int)abs(key - leaf_set[i]);
					dest = leaf_set[i];
				}
		 	}
		 	for(int i = 0; i < max_rows; i++) {
		 		for(int j = 0; j < max_cols; j++) {
		 			if (min_node > ((int)abs(key - route_table[i][j]))) {
		 				min_node = (int)abs(key - route_table[i][j]);
		 				dest = route_table[i][j];
		 			}
		 		}
		 	}
		 	for(int i = 0; i < m_size; i++) {
		 		if (min_node > ((int)abs(key - neighbour_set[i]))) {
					min_node = (int)abs(key - neighbour_set[i]);
					dest = neighbour_set[i];
		 		}
		 	}
		 	route_mutex.unlock();
		 	return dest;
		}
	}
}

void Pastry_overlay :: recv_api_thread() {

	while(1) {

		message *mess;
		while((mess = pastry_api_overlay_in.get_from_queue()) != NULL) {

			if(mess -> type == PUT || mess -> type == GET) {
				route(mess);
				delete(mess);
			}
		}
	}
}

void Pastry_overlay :: route(message *mess) {

	int key;
	sscanf(mess->data.c_str(),"%d#",&key);
	key = key & ((1 << 17) - 1);
	int next_node = get_next_route(key);
	if(next_node == current_node_id) {
		//while(!pastry_overlay_api_in.add_to_queue(mess));
		if(mess -> type == INIT) {
			string msg = to_string((int)INIT_FINAL) + "#";
			sock_layer -> send_data(key,msg);
		}
		//printf("\nMessage received %s\n",mess->data.c_str());
	}
	else {

		string new_message = to_string((int)mess->type) + string("#") + mess -> data.c_str();
		printf("\nRerouted message %d %s",next_node,new_message.c_str());
		//printf("\nRerouted to %d \n",next_node);
		if(!sock_layer -> send_data(next_node,new_message)) {
			remove_from_table(next_node);
		}
	}
}

message *Pastry_overlay :: get_table_message() {

	message *mess = new message();
	mess -> type = RECV_TABLE;
	string sendstr = to_string(current_node_id) + string("#") + sock_layer -> cur_ip + string("#") + to_string(sock_layer -> cur_port) + "#";
	for(int i = 0; i < l_size; i++) sendstr = sendstr + to_string(leaf_set[i]) + "#";
	for(int i = 0; i < max_rows; i++) {
		for(int j = 0; j < max_cols; j++) 
			sendstr = sendstr + to_string(route_table[i][j]) + "#";
	}
	for(int i = 0; i < m_size; i++) sendstr = sendstr + to_string(neighbour_set[i]) + "#";

	mess -> data = sendstr;
	return mess;
}

void Pastry_overlay :: update_table_message(message *mess) {

	vector <string> tokens; 
    stringstream ss(mess -> data); 
    string word; 
    while(getline(ss, word, '#'))
        tokens.push_back(word); 
    add_to_table(atoi(tokens[0].c_str()));
    sock_layer -> add_ip_port(atoi(tokens[0].c_str()),tokens[1],atoi(tokens[2].c_str()));
    //printf("received data %s\n",mess -> data.c_str());

    //printf("leaf set");
    int tp = 3;
    for(int i = 0; i < l_size; i++) {
    	//cout << tokens[tp++] << " ";
    	add_to_table(atoi(tokens[tp++].c_str()));
    }
    for(int i = 0; i < max_rows; i++) {
    	for(int j = 0; j < max_cols; j++) {
    		//cout << tokens[tp++] << " ";
    		add_to_table(atoi(tokens[tp++].c_str()));
    	}
    	//cout << endl;
    }
    printf("Updated table \n");
    display_table();
}

void Pastry_overlay :: recv_socket_thread() {

	while(1) {

		message *mess;
		while((mess = pastry_socket_overlay_in.get_from_queue()) != NULL) {

			if(mess -> type == PUT || mess -> type == GET) {
				route(mess);
				delete(mess);
			}
			else if(mess -> type == ADD_NODE) {

				int nodeid;
				sscanf(mess->data.c_str(),"%d",&nodeid);
				add_to_table(nodeid);
				printf("Added %d to table\n",nodeid);
				display_table();
				delete(mess);

			}
			else if(mess -> type == SEND_TABLE || mess -> type == INIT) {

				if(check_init == false) {
					while(!pastry_socket_overlay_in.add_to_queue(mess));
					continue;
				}

				string ip;
				int nid, port;
				sscanf(mess -> data.c_str(),"%d#%[^#]#%d",&nid,ip.c_str(),&port);
				sock_layer -> add_ip_port(nid,ip,port);
				message *ret_mess = get_table_message();
				string msg = to_string((int)ret_mess -> type) + string("#") + ret_mess -> data;
				//printf("\nSending %s\n",ret_mess -> data.c_str());
				sock_layer -> send_data(nid,msg);
				delete(ret_mess);
				if (mess -> type == INIT) {
					msg = to_string(SEND_TABLE) + string("#") + to_string(current_node_id) + string("#") 
						+ sock_layer ->  cur_ip + string("#") + to_string(sock_layer -> cur_port);
					sock_layer -> send_data(nid,msg);
					route(mess);
				}
				delete(mess);
			}
			else if(mess -> type == RECV_TABLE) {
				update_table_message(mess);
				delete(mess);
			}
			else if(mess -> type == INIT_FINAL) {
				printf("Final initialization done\n");
				queue<message *> mess_queue;
				while(mess = pastry_socket_overlay_in.get_from_queue()) {
					if(mess -> type != RECV_TABLE) mess_queue.push(mess);
					else update_table_message(mess);
				}
				while(!mess_queue.empty()) {
					while(!pastry_socket_overlay_in.add_to_queue(mess_queue.front()));
					mess_queue.pop();
				}
				check_init = true;
				sock_layer -> remove_ip_port(0);
				printf("Node initialization done\n");
			}
			
		}

	}
}

void Pastry_overlay :: initialize_table(std::string ip,int port) {

	cout << ip << endl;
	cout << sock_layer -> cur_ip << endl;
	if(ip == sock_layer -> cur_ip && port == sock_layer -> cur_port) {
		printf("Node self initialized\n");
		check_init = true;
		return;
	}
	sock_layer -> add_ip_port(0,ip,port);
	string msg = to_string(INIT) + string("#") + to_string(current_node_id) + string("#") 
		+ sock_layer ->  cur_ip + string("#") + to_string(sock_layer -> cur_port);
	if(!sock_layer -> send_data(0,msg)) {
		printf("Failed to contact initialization node\n");
	}
	else printf("initialization started\n");
}

void Pastry_overlay :: display_table() {

	char format[10];
	sprintf(format,"%%0%dX ",max_rows);

	printf("\nCurrent Node ");
	printf(format,current_node_id);
	printf("\n");

	printf("Leaf Set\n");
	for(int i = 0; i < l_size; i++) {
		if(leaf_set[i] != INT_MAX || leaf_set[i] != 0)
			printf(format,leaf_set[i]);
		else printf(format,0);
 	}
 	printf("\nRoute Table\n");
 	for(int i = 0; i < max_rows; i++) {
 		for(int j = 0; j < max_cols; j++) {
 			if(route_table[i][j] != INT_MAX)
 				printf(format,route_table[i][j]);
 			else printf(format,0);
 		}
 		printf("\n");
 	}
 	printf("\nNeighbourhood Table\n");
 	for(int i = 0; i < m_size; i++) {
 		if(neighbour_set[i] != INT_MAX)
 			printf(format,neighbour_set[i]);
 		else printf(format,0);
 	}
 	printf("\n");
}