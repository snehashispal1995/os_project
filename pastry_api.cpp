#include "pastry_api.h"	
#define print(A) cout<<A<<endl;


void Pastry_api :: init(){
	recvOverlayThread = new thread(&Pastry_api :: recv_overlay_thread,this);
}

void Pastry_api :: recv_overlay_thread(){
	while(1){
			message *msg=pastry_overlay_api_in.get_from_queue();
			if(msg){
				if(msg->type==PUT){
					//ADD into dht
					//data format key#value
					std::vector<string> keyValue=parse(msg->data,'#');
					// printf("In PUT %s %s\n", keyValue[0].c_str(), keyValue[1].c_str() );
					add_key_value_pair(atoi(keyValue[0].c_str()),keyValue[1]);
					message *msg=new message();
					msg->type=RESPONSE;
					msg->data="successfully put";
					while(!pastry_api_overlay_in.add_to_queue(msg));
				}
				else if(msg->type==GET){
					//get value from my lookup
					//data will be sourcenodeid#key
					std::vector<string> nodeIdKey=parse(msg->data,'#');
					// printf("In GET %s %s\n", nodeIdKey[0].c_str(), nodeIdKey[1].c_str() );
					string value=look_up(atoi(nodeIdKey[1].c_str()));
					message *msg=new message();
					msg->type=RESPONSE;
					msg->data=nodeIdKey[0]+"#"+value;
					while(!pastry_api_overlay_in.add_to_queue(msg));
				}
			}
	}
}

void Pastry_api:: add_key_value_pair(int key, string value){
	dht[key]=value;
}

string Pastry_api ::  look_up(int key){
	return dht[key];
}

vector<string> parse(string s, char delim){
	vector <string> tokens; 
    stringstream ss(s); 
    string word; 
    while(getline(ss, word, delim))
        tokens.push_back(word); 
    return tokens;
}


void cliAddToQueue(enum msg_type mType, string data){
	//add to overlay queue
	message *msg=new message();
	msg->type=mType;
	msg->data=data;
	pastry_api_overlay_in.add_to_queue(msg);
}

long createNode(int port,char host[]){
	char ip_port[20];
    char myport[5];
	char temp[41];
    unsigned char key[21];
    fetch_myIp_address(host);
    memset(ip_port,0,sizeof(20));
    int len = snprintf(myport,5,"%d\n",port);
    string mysha;
    strcat(ip_port,host);
    strcat(ip_port,myport);

    // printf("%s length=%d\n", ip_port,strlen(ip_port));
    SHA1((unsigned char *)ip_port,strlen(ip_port),key);
    for (int i = 0; i < 20; i++){
    	// printf("%02x", key[i]);
		snprintf(temp+i*2,4,"%02x", key[i]);
    }
   	// cout<<endl;
    mysha=temp;
    // cout<<stol(mysha.substr(0,4), nullptr, 16)<<"\n"; 
    return stol(mysha.substr(0,4), nullptr, 16);
}

void fetch_myIp_address(char host[NI_MAXHOST]){
    struct ifaddrs *ifaddr, *ifa;
    int family,s;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            if(!(strcmp(ifa->ifa_name,"e")))
                break;
        }
    }
}


void Pastry_api:: recv_user_thread(){
	/
	string s;
	while(1){
		getline(cin,s);
		vector<string> cli=parse(s,' ');
		int totalWords=cli.size();
		if(totalWords>0){
			string opcode=cli[0];
			if(opcode=="port"){
				print("port code");
				if(totalWords>1)
					// cliAddToQueue(PORT,cli[1]);
					port=atoi(cli[1].c_str());
					if(!port)
						print("please provide valid port");
			}
			else if(opcode=="create"){
				print("create code");
				cliAddToQueue(CREATE,"");
				if(port){
					nodeId=createNode(port,host);
					cout<<nodeId;
					sockets.init(nodeId,port);
				}
			}
			else if(opcode=="join"){
				print("join code");
				if(totalWords>2)
					cliAddToQueue(JOIN,cli[1]+"#"+cli[2]);
				pastry_api_overlay_in.printQueue();
			}
			else if(opcode=="put"){
				print("put code");
				if(totalWords>2)
					cliAddToQueue(PUT,cli[1]+"#"+cli[2]);
			}
			else if(opcode=="get"){
				print("get code");
				if(totalWords>1)
					cliAddToQueue(GET,cli[1]);
			}
			else if(opcode=="lset"){
				print("lset code");
				cliAddToQueue(LSET,"");
			}
			else if(opcode=="nset"){
				print("nset code");
				cliAddToQueue(NSET,"");
			}
			else if(opcode=="routetable"){
				print("routetable code");
				cliAddToQueue(ROUTETABLE,"");	
			}
			else if(opcode=="quit"){
				print("quit code");
			}
			else if(opcode=="shutdown"){
				print("shutdown code");
			}
			else{
				print("Invalid command");
			}
		}
		else
			print("Please provide some command")

	}
	
}