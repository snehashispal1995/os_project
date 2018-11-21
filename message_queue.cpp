#include "message_queue.h"

using namespace std;
Message_queue pastry_api_overlay_in, pastry_api_user_in;
Message_queue pastry_overlay_socket_in, pastry_overlay_api_in;

int Message_queue ::  add_to_queue(message *m) {

	if(lock.try_lock()) {
		m_queue.push(m);
		lock.unlock();
		return 1;
	}
	return 0;

}

message *Message_queue :: get_from_queue() {

	if(lock.try_lock()) {
		if(m_queue.empty()) 
			return NULL;
		message *ret = m_queue.front();
		m_queue.pop();
		lock.unlock();
		return ret;
	}
	return NULL;
}

message *extract_message(string data) {

	message *new_message = new message;
	sscanf(data.c_str(),"%d#%s",&(new_message -> type),new_message -> data.c_str());
	return new_message;
}