#include <getopt.h>
#include <iostream>
#include <string.h>
#include <thread>
#include <unimsg/net.h>
#include <unistd.h>
#include <uk/plat/time.h>

#define UNIMSG_BUFFER_AVAILABLE (UNIMSG_BUFFER_SIZE - UNIMSG_BUFFER_HEADROOM)
#define TS_ADDR 1
#define TS_PORT 4560

using namespace std;

static unsigned opt_iterations = 0;
static unsigned opt_ue_records = 0;
static double opt_anomaly_rate = -1;
static unsigned opt_prediciton_time = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"ue-records", required_argument, 0, 'r'},
	{"anomaly-rate", required_argument, 0, 'a'},
	{"prediction-time", required_argument, 0, 'p'},
	{0, 0, 0, 0}
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations		Number of control loop iterations\n"
		"  -r, --ue-records		Number of ue record read at each iteration\n"
		"  -a, --anomaly-rate		Fraction of ue records represening an anomaly\n"
		"  -p, --prediction-time	Emulated prediciton time (ms) (default 0)\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:r:a:p:", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
			break;
		case 'r':
			opt_ue_records = atoi(optarg);
			break;
		case 'a':
			opt_anomaly_rate = strtod(optarg, NULL);
			break;
		case 'p':
			opt_prediciton_time = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (!opt_iterations) {
		fprintf(stderr, "Iterations option must be > 0\n");
		usage(argv[0]);
	}

	if (!opt_ue_records) {
		fprintf(stderr, "Missing required parameter ue-records\n");
		usage(argv[0]);
	}

	if (opt_anomaly_rate < 0 || opt_anomaly_rate > 1) {
		fprintf(stderr, "Anomaly rate must be in range [0, 1]\n");
		usage(argv[0]);
	}
}

void ts_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	// cout << "[AD] TS Callback got a message, length=" << desc.size << "\n";
	// cout << "[AD] Payload is " << json << endl;

	unimsg_buffer_put(&desc, 1);
}

int main(int argc, char *argv[])
{
	int rc;
	unsigned long start;
	unsigned long total = 0, latency;

	parse_command_line(argc, argv);

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

	for (unsigned i = 0; i < opt_iterations; i++) {
		usleep(500000);

		start = ukplat_monotonic_clock();

		/* Burn CPU cycles to emulate prediciton */
		unsigned long now;
		do
			now = ukplat_monotonic_clock();
		while (now - start < opt_prediciton_time * 1000000);

		// printf("Started at %lu, stopped ad %lu\n", start, now);

		std::string ad_msg = "[";
		unsigned anomaly_records = opt_ue_records * opt_anomaly_rate
					   + 0.5;
		for (unsigned i = 0; i < anomaly_records; i++) {
			/* The content doesn't matter */
			ad_msg += "{";
			ad_msg += "\"du-id\": 1010,";
			ad_msg += "\"ue-id\": \"Train passenger 2\",";
			ad_msg += "\"measTimeStampRf\": 1620835470108,";
			ad_msg += "\"Degradation\": \"RSRP RSSINR\"";
			ad_msg += "}";
			if (i < anomaly_records - 1)
				ad_msg += ",";
		}
		ad_msg += "]";

		if (ad_msg.size() > UNIMSG_BUFFER_AVAILABLE) {
			cerr << "Message is too big\n";
			exit(1);
		}

		struct unimsg_shm_desc desc;
		rc = unimsg_buffer_get(&desc, 1);
		if (rc) {
			fprintf(stderr, "Error getting buffer: %s\n",
				strerror(-rc));
			exit(1);
		}

		memcpy(desc.addr, ad_msg.c_str(), ad_msg.size());
		desc.size = ad_msg.size();

		rc = unimsg_send(sock, &desc, 1, 0);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		unsigned ndescs = 1;
		rc = unimsg_recv(sock, &desc, &ndescs, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		ts_callback(desc);

		latency = ukplat_monotonic_clock() - start;

		cout << "[AD] Control loop " << i << " took " << latency
		     << " ns\n";

		if (i > 0)
			total += latency;
	}

	unimsg_close(sock);

	cout << "[AD] Average latency (excluding 1st loop) "
	     << (total / (opt_iterations - 1)) << " ns\n";

	return 0;
}
