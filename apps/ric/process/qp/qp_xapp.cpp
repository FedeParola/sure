#include <iostream>
#include <netinet/in.h>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string.h>
#include <unistd.h>

#define PORT 4580
#define ITERATIONS 20
#define MAX_MSG_SIZE 4096

using namespace rapidjson;
using namespace std;

static int sock;

void prediction_callback(char *msg, ssize_t size)
{
	string json (msg, size);

	// cout << "[QP] Prediction Callback got a message, length=" << size
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

	// cout << "[QP] Sending a message to TS, length=" << size << "\n";
	// cout << "[QP] Message body " << body << endl;

	if (send(sock, body.c_str(), body.size(), 0) != body.size()) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	char msg[MAX_MSG_SIZE];

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0x7f000001);
	addr.sin_port = htons(PORT);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error binding: %s\n", strerror(errno));
		exit(1);
	}

	if (listen(sock, 10)) {
		fprintf(stderr, "Error listening: %s\n", strerror(errno));
		exit(1);
	}

	cout << "[QP] Waiting for TS connection\n";

	int cs;
	cs = accept(sock, NULL, NULL);
	if (cs < 0) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(errno));
		exit(1);
	}

	cout << "[QP] TS connected\n";

	close(sock);
	sock = cs;

	for (int i = 0; i < ITERATIONS; i++) {
		ssize_t size = recv(sock, msg, sizeof(msg), 0);
		if (size <= 0) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			exit(1);
		}

		prediction_callback(msg, size);
	}

	close(sock);

	return 0;
}
