#include <iostream>
#include <memory>
#include <string.h>
#include <thread>
#include <unimsg/net.h>
#include <unistd.h>

#define TS_ADDR 0
#define TS_PORT 4560
#define ITERATIONS 20

using namespace std;

void ts_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	// cout << "[AD] TS Callback got a message, length=" << desc.size << "\n";
	// cout << "[AD] Payload is " << json << endl;

	unimsg_buffer_put(&desc);
}

int main(int argc, char *argv[])
{
	int rc;
	struct timespec start, stop;
	unsigned long total = 0, latency;

	struct unimsg_sock *sock;
	rc = unimsg_socket(&sock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_connect(sock, TS_ADDR, TS_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to TS: %s\n", strerror(-rc));
		exit(1);
	}

	cout << "[AD] Connected to TS\n";

	for (int i = 0; i < ITERATIONS; i++) {
		usleep(500000);

		if (clock_gettime(CLOCK_MONOTONIC, &start)) {
			cerr << "[ERROR] Cannot get time\n";
			exit(EXIT_FAILURE);
		}

		struct unimsg_shm_desc desc;
		rc = unimsg_buffer_get(&desc);
		if (rc) {
			fprintf(stderr, "Error getting buffer: %s\n",
				strerror(-rc));
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

		rc = unimsg_send(sock, &desc, 0);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		rc = unimsg_recv(sock, &desc, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		ts_callback(desc);

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

	unimsg_close(sock);

	cout << "[AD] Average latency (excluding 1st loop) "
	     << (total / (ITERATIONS - 1)) << " ns\n";

	return 0;
}
