#include <getopt.h>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string.h>
#include <unimsg/net.h>
#include <unistd.h>
#include <uk/plat/time.h>

#define UNIMSG_BUFFER_AVAILABLE (UNIMSG_BUFFER_SIZE - UNIMSG_BUFFER_HEADROOM)
#define PORT 4580
#define DEFAULT_CELLS 3

using namespace rapidjson;
using namespace std;

static struct unimsg_sock *sock;
static unsigned opt_iterations = 0;
static unsigned opt_prediciton_time = 0;
static unsigned opt_cells = DEFAULT_CELLS;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"prediction-time", required_argument, 0, 'p'},
	{"cells", required_argument, 0, 'c'},
	{0, 0, 0, 0}
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations		Number of control loop iterations\n"
		"  -p, --prediction-time	Emulated per-UE, per-cell prediciton time (ms) (default 0)\n"
		"  -c, --cells			Number of cells assocaited to queried for each UE (default %d)\n",
		prog, DEFAULT_CELLS);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:p:c:", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
			break;
		case 'p':
			opt_prediciton_time = atoi(optarg);
			break;
		case 'c':
			opt_cells = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (!opt_iterations) {
		fprintf(stderr, "Iterations option must be > 0\n");
		usage(argv[0]);
	}
}

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

	unsigned long tot_pred_time = uePred.Size() * opt_cells
				      * opt_prediciton_time * 1000000;

	unsigned long start = ukplat_monotonic_clock();
	unsigned long now;
	do
		now = ukplat_monotonic_clock();
	while (now - start < tot_pred_time);

	printf("Started at %lu, stopped ad %lu\n", start, now);

	/* We want to create:
	 * {
	 *	"ueid-user1": {
	 *		"CID1": [10, 20],
	 *		"CID2": [30, 40],
	 *		"CID3": [50, 60]
	 *	}
	 * }";
	 */
	string body = "[";
	for (unsigned i = 0; i < uePred.Size(); i++) {
		body += "{\"";
		body += uePred[i].GetString();
		body += "\": {";
		for (unsigned j = 0; j < opt_cells; j++) {
			body += "\"CID";
			body += to_string(j);
			body += "\": [0, 0]";
			if (j < opt_cells - 1)
				body += ",";
		}
		body += "}";
		if (i < uePred.Size() - 1)
			body += ",";
	}
	body += "]";

	if (body.size() > UNIMSG_BUFFER_AVAILABLE) {
		cerr << "Message is too big\n";
		exit(1);
	}

	/* TODO: evaluate building payload in place */
	memcpy(desc.addr, body.c_str(), body.size());
	desc.size = body.size();

	// cout << "[QP] Sending a message to TS, length=" << desc.size << "\n";
	// cout << "[QP] Message body " << body << endl;

	int rc = unimsg_send(sock, &desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;

	parse_command_line(argc, argv);

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

	for (unsigned i = 0; i < opt_iterations; i++) {
		struct unimsg_shm_desc desc;
		unsigned descs = 1;
		rc = unimsg_recv(sock, &desc, &descs, 0);
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
