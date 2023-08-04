#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#define TS_ADDR 0x7f000001
#define TS_PORT 4560
#define ITERATIONS 20
#define MAX_MSG_SIZE 4096

using namespace std;

void ts_callback(char *resp, int len)
{
	string json (resp, len);

	// cout << "[AD] TS Callback got a message, length=" << desc.size << "\n";
	// cout << "[AD] Payload is " << json << endl;
}

int main(int argc, char *argv[])
{
	struct timespec start, stop;
	unsigned long total = 0, latency;
	int sock;
	char resp[MAX_MSG_SIZE];

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(TS_ADDR);
	addr.sin_port = htons(TS_PORT);
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error connecting to server: %s\n",
			strerror(errno));
		exit(1);
	}

	cout << "[AD] Connected to TS\n";

	for (int i = 0; i < ITERATIONS; i++) {
		usleep(500000);

		if (clock_gettime(CLOCK_MONOTONIC, &start)) {
			cerr << "[ERROR] Cannot get time\n";
			exit(EXIT_FAILURE);
		}

		const char ad_msg[] = "[{"
			"\"du-id\": 1010,"
			"\"ue-id\": \"Train passenger 2\","
			"\"measTimeStampRf\": 1620835470108,"
			"\"Degradation\": \"RSRP RSSINR\""
		"}]";

		if (send(sock, ad_msg, sizeof(ad_msg) - 1, 0)
		    != sizeof(ad_msg) - 1) {
			fprintf(stderr, "Error sending message: %s\n",
				strerror(errno));
			exit(1);
		}

		int len = recv(sock, resp, MAX_MSG_SIZE, 0);
		if (len <= 0) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			exit(1);
		}

		ts_callback(resp, len);

		if (clock_gettime(CLOCK_MONOTONIC, &stop)) {
			cerr << "[ERROR] Cannot get time\n";
			exit(EXIT_FAILURE);
		}

		latency = (stop.tv_sec - start.tv_sec) * 1000000000
			  + stop.tv_nsec - start.tv_nsec;

		cout << "[AD] Control loop " << i << " took " << latency
		     << " ns\n";

		if (i > 0)
			total += latency;
	}

	close(sock);

	cout << "[AD] Average latency (excluding 1st loop) "
	     << (total / (ITERATIONS - 1)) << " ns\n";

	return 0;
}
