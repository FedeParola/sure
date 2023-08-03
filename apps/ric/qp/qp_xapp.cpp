#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string.h>
#include <thread>
#include <unimsg/net.h>
#include <unistd.h>

#define PORT 4580
#define ITERATIONS 20

using namespace rapidjson;
using namespace std;

static struct unimsg_sock *sock;

void prediction_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	// cout << "[QP] Prediction Callback got a message, length=" << desc.size
	//      << "\n";
	// cout << "[QP] Payload is " << json << endl;

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

	// cout << "[QP] Sending a message to TS, length=" << desc.size << "\n";
	// cout << "[QP] Message body " << body << endl;

	int rc = unimsg_send(sock, &desc, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;

	rc = unimsg_socket(&sock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_bind(sock, PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PORT,
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_listen(sock);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}

	cout << "[QP] Waiting for TS connection\n";

	struct unimsg_sock *tmp_sock;
	rc = unimsg_accept(sock, &tmp_sock, 0);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		exit(1);
	}

	cout << "[QP] TS connected\n";

	unimsg_close(sock);
	sock = tmp_sock;

	for (int i = 0; i < ITERATIONS; i++) {
		struct unimsg_shm_desc desc;
		rc = unimsg_recv(sock, &desc, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		prediction_callback(desc);
	}

	unimsg_close(sock);

	return 0;
}
