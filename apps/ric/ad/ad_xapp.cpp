// vi: ts=4 sw=4 noet:
/*
==================================================================================
	Copyright (c) 2021 AT&T Intellectual Property.
	Copyright (c) 2021 Alexandre Huff.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
==================================================================================
*/

/*
	Mnemonic:	ad_xapp.cpp
	Abstract:   Simulates the AD xApp sending an anomaly dectection message to
		the TS xApp. It sends one message and exits.

	Date:		20 May 2021
	Author:		Alexandre Huff
*/

#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <unimsg/net.h>

#define TS_ADDR 0
#define TS_PORT 4560

// #include <rmr/RIC_message_types.h>
// #include "ricxfcpp/xapp.hpp"

using namespace std;
// using namespace xapp;

// unique_ptr<Xapp> xfw;

void ts_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	cout << "[AD] TS Callback got a message, length=" << desc.size << "\n";
	cout << "[AD] Payload is " << json << endl;

	unimsg_buffer_put(&desc);
}

// this thread just sends out one anomaly message to the TS xApp
void ad_loop() {
    // std::unique_ptr<Message> msg;
    // Msg_component payload;
    int size;
    int plen;

    cout << "[AD] In Anomaly Detection ad_loop()\n";
    sleep( 1 ); // just wait receiver thread starting up

    // msg = xfw->Alloc_msg(2048);
    // size = msg->Get_available_size();
    if ( size < 2048 ) {
	cout << "[ERROR] Message returned does not have enough size: " << size << " < 2048" << endl;
	exit(1);
    }

    // the message we are sending
    const char *ad_msg = "[{\"du-id\": 1010, \"ue-id\": \"Train passenger 2\", \"measTimeStampRf\": 1620835470108, \"Degradation\": \"RSRP RSSINR\"}]";

    // payload = msg->Get_payload();
    // snprintf( (char *) payload.get(), 2048, "%s", ad_msg );

    // plen = strlen( (char *) payload.get() );
    cout << "[AD] Sending a message to TS, length: " << plen << "\n";
    // cout << "[AD] Message body " << payload.get() << endl;

    // payload updated in place, nothing to copy from, so payload parm is nil
    // if ( ! msg->Send_msg( TS_ANOMALY_UPDATE, Message::NO_SUBID, plen, nullptr ) ) // msg type 30003
	// cout << "[ERROR] Unable to send a message to TS xApp, state: " << msg->Get_state() << endl;
}

int main(int argc, char *argv[])
{
	int rc;

	struct unimsg_sock *ts_sock;
	rc = unimsg_sock_create(&ts_sock, UNIMSG_SOCK_CONNECTED, 0);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_sock_connect(ts_sock, TS_ADDR, TS_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to TS: %s\n", strerror(-rc));
		exit(1);
	}

	printf("Connected to TS\n");

	struct unimsg_shm_desc desc;
	rc = unimsg_buffer_get(&desc); 
	if (rc) {
		fprintf(stderr, "Error getting buffer: %s\n", strerror(-rc));
		exit(1);
	}

	const char ad_msg[] = "[{"
		"\"du-id\": 1010,"
		"\"ue-id\": \"Train passenger 2\","
		"\"measTimeStampRf\": 1620835470108,"
		"\"Degradation\": \"RSRP RSSINR\""
	"}]";
	memcpy(desc.addr, ad_msg, sizeof(ad_msg));
	desc.size = sizeof(ad_msg);

	rc = unimsg_sock_send(ts_sock, &desc);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_sock_recv(ts_sock, &desc);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	ts_callback(desc);

	unimsg_sock_close(ts_sock);

	return 0;
}
