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
	Mnemonic:	qp_xapp.cpp
	Abstract:   Simulates both, the QP Driver and QP xApp for testing the behavior
		of the TS xApp.

	Date:		20 May 2021
	Author:		Alexandre Huff
*/

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/schema.h>
#include <rapidjson/reader.h>
#include <string.h>
#include <thread>
#include <unimsg/net.h>
#include <unistd.h>

#define PORT 4580

using namespace rapidjson;
using namespace std;

static struct unimsg_sock *sock;

void prediction_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	cout << "[QP] Prediction Callback got a message, length=" << desc.size
	     << "\n";
	cout << "[QP] Payload is " << json << endl;

	Document document;
	document.Parse(json.c_str());

	const Value& uePred = document["UEPredictionSet"];
	if (uePred.Size() == 0)
		return;
	string ueid = uePred[0].GetString();
	/* We want to create:
		* {
		*	"ueid-user1": {
		*		"CID1": [10, 20],
		*		"CID2": [30, 40],
		*		"CID3": [50, 60]
		*	}
		* }";
		*/
	string body = "{\"" + ueid + "\": {";
	for (int i = 1; i <= 3; i++) {
		int down = rand() % 100;
		int up = rand() % 100;
		if (i != 3) {
			body += "\"CID" + to_string(i) + "\": ["
				+ to_string(down) + ", " + to_string(up)
				+ "], ";
		} else {
			body += "\"CID" + to_string(i) + "\": ["
				+ to_string(down) + ", " + to_string(up)
				+ "]}}";
		}
	}

	/* TODO: evaluate building payload in place */
	memcpy(desc.addr, body.c_str(), body.size());
	desc.size = body.size();

	cout << "[QP] Sending a message to TS, length=" << desc.size << "\n";
	cout << "[QP] Message body " << body << endl;

	int rc = unimsg_sock_send(sock, &desc);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;

	rc = unimsg_sock_create(&sock, UNIMSG_SOCK_CONNECTED, 0);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_sock_bind(sock, PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PORT,
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_sock_listen(sock);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}

	printf("Waiting for TS connection\n");

	struct unimsg_sock *tmp_sock;
	rc = unimsg_sock_accept(sock, &tmp_sock);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		exit(1);
	}

	printf("TS connected\n");

	unimsg_sock_close(sock);
	sock = tmp_sock;

	struct unimsg_shm_desc desc;
	rc = unimsg_sock_recv(sock, &desc);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	prediction_callback(desc);

	unimsg_sock_close(sock);

	return 0;
}
