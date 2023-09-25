#include <getopt.h>
#include <iostream>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unimsg/net.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define PORT 4560
#define QP_ADDR 2
#define QP_PORT 4580
#define RC_ADDR 3
#define RC_PORT 5000

static unsigned opt_iterations = 0;
static int downlink_threshold = 0;  // A1 policy type 20008 (in percentage)
static struct unimsg_sock *ad_sock;
static struct unimsg_sock *qp_sock;
static unsigned it = 0;
static unsigned long total_latency = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{0, 0, 0, 0}
};

using namespace rapidjson;
using namespace std;

using Namespace = std::string;
using Key = std::string;
using Data = std::vector<uint8_t>;
using DataMap = std::map<Key, Data>;
using Keys = std::set<Key>;

struct PredictionHandler : public BaseReaderHandler<UTF8<>, PredictionHandler> {
	unordered_map<string, int> cell_pred_down;
	unordered_map<string, int> cell_pred_up;
	std::string ue_id;
	bool ue_id_found = false;
	string curr_key = "";
	string curr_value = "";
	string serving_cell_id;
	bool down_val = true;
	bool Null() {  return true; }
	bool Bool(bool b) {  return true; }
	bool Int(int i) {  return true; }
	bool Uint(unsigned u)
	{
		/* Currently, we assume the first cell in the prediction message
		 * is the serving cell
		 */
		if ( serving_cell_id.empty() ) {
			serving_cell_id = curr_key;
		}

		if (down_val) {
			cell_pred_down[curr_key] = u;
			down_val = false;
		} else {
			cell_pred_up[curr_key] = u;
			down_val = true;
		}

		return true;
	}
	bool Int64(int64_t i) {  return true; }
	bool Uint64(uint64_t u) {  return true; }
	bool Double(double d) {  return true; }
	bool String(const char* str, SizeType length, bool copy)
	{
		return true;
	}
	bool StartObject() {  return true; }
	bool Key(const char* str, SizeType length, bool copy)
	{
		if (!ue_id_found) {
			ue_id = str;
			ue_id_found = true;
		} else {
			curr_key = str;
		}

		return true;
	}
	bool EndObject(SizeType memberCount) {  return true; }
	bool StartArray() {  return true; }
	bool EndArray(SizeType elementCount) {  return true; }
};

struct AnomalyHandler : public BaseReaderHandler<UTF8<>, AnomalyHandler> {
	/* Assuming we receive the following payload from AD
	 * [{
	 *	"du-id": 1010,
	 *	"ue-id": "Train passenger 2",
	 *	"measTimeStampRf": 1620835470108,
	 *	"Degradation": "RSRP RSSINR"
	 * }]
	 */
	vector<string> prediction_ues;
	string curr_key = "";

	bool Key(const Ch* str, SizeType len, bool copy)
	{
		curr_key = str;
		return true;
	}

	bool String(const Ch* str, SizeType len, bool copy)
	{
		/* We are only interested in the "ue-id" */
		if ( curr_key.compare( "ue-id") == 0 )
			prediction_ues.push_back( str );

		return true;
	}
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of control loop iterations\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
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

struct rest_resp {
	unsigned status_code;
	char *body;
	unimsg_shm_desc desc;
};

static char post_template[] = "POST %s HTTP/1.1\r\n" /* url */
			      "Host: %u:%u\r\n" /* addr:port */
			      "Accept: application/json\r\n"
			      "Content-Type: application/json\r\n"
			      "Content-Length: %u\r\n" /* body length */
			      "\r\n"
			      "%s"; /* body */

static struct rest_resp do_post(__u32 addr, __u16 port, string url, string body)
{
	struct unimsg_sock *rc_sock;
	struct unimsg_shm_desc desc;
	int rc;

	rc = unimsg_socket(&rc_sock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_connect(rc_sock, addr, port);
	if (rc) {
		fprintf(stderr, "Error connecting to RC: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_buffer_get(&desc, 1);
	if (rc) {
		fprintf(stderr, "Error getting buffer: %s\n", strerror(-rc));
		exit(1);
	}

	sprintf((char *)desc.addr, post_template, url.c_str(), addr, port,
		body.size(), body.c_str());
	desc.size = strlen((char *)desc.addr);

	rc = unimsg_send(rc_sock, &desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned ndescs = 1;
	rc = unimsg_recv(rc_sock, &desc, &ndescs, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	unimsg_close(rc_sock);

	struct rest_resp resp;
	if (sscanf((char *)desc.addr, "HTTP/1.1 %u OK", &resp.status_code)
	    == EOF) {
		fprintf(stderr, "Received unexpected response: %s\n",
			(char *)desc.addr);
		exit(1);
	}

	/* Locate body */
	resp.body = strstr((char *)desc.addr, "\r\n\r\n");
	if (!resp.body) {
		fprintf(stderr, "Received response without payload:\n%s",
			(char *)desc.addr);
		exit(1);
	}
	resp.body += 4; /* Skip newlines */
	resp.desc = desc;

	return resp;
}

/* Sends a handover message through REST */
void send_rest_control_request(string ue_id, string serving_cell_id,
			       string target_cell_id)
{
	time_t now;
	string str_now;
	/* Static counter, not thread-safe */
	static unsigned int seq_number = 0;

	/* Building a handoff control message */
	now = time(nullptr);
	str_now = ctime(&now);
	str_now.pop_back(); /* removing the \n character */

	seq_number++; /* Static counter, not thread-safe */

	rapidjson::StringBuffer s;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("command");
	writer.String("HandOff");
	writer.Key("seqNo");
	writer.Int(seq_number);
	writer.Key("ue");
	writer.String(ue_id.c_str());
	writer.Key("fromCell");
	writer.String(serving_cell_id.c_str());
	writer.Key("toCell");
	writer.String(target_cell_id.c_str());
	writer.Key("timestamp");
	writer.String(str_now.c_str());
	writer.Key("reason");
	writer.String("HandOff Control Request from TS xApp");
	writer.Key("ttl");
	writer.Int(10);
	writer.EndObject();
	/* Creates a message like
	 * {
	 * 	"command": "HandOff",
	 * 	"seqNo": 1,
	 * 	"ue": "ueid-here",
	 * 	"fromCell": "CID1",
	 * 	"toCell": "CID3",
	 * 	"timestamp": "Sat May 22 10:35:33 2021",
	 * 	"reason": "HandOff Control Request from TS xApp",
	 * 	"ttl": 10
	 * }
	 */

	string msg = s.GetString();

	// cout << "[INFO] Sending a HandOff CONTROL message\n";
	// cout << "[INFO] HandOff request is " << msg << endl;

	// struct timespec start, stop;
	// if (clock_gettime(CLOCK_MONOTONIC, &start)) {
	// 	cerr << "[ERROR] Cannot get time\n";
	// 	exit(EXIT_FAILURE);
	// }

	struct rest_resp resp = do_post(RC_ADDR, RC_PORT, "/api/echo", msg);

	// if (clock_gettime(CLOCK_MONOTONIC, &stop)) {
	// 	cerr << "[ERROR] Cannot get time\n";
	// 	exit(EXIT_FAILURE);
	// }
	// unsigned long latency = (stop.tv_sec - start.tv_sec) * 1000000000
	// 			+ stop.tv_nsec - start.tv_nsec;

	// cout << "[TS] POST " << it << " took " << latency << " ns\n";

	// if (it++ > 0)
	// 	total_latency += latency;

	if (resp.status_code == 200) {
		/* ============= DO SOMETHING USEFUL HERE =============
		 * Currently, we only print out the HandOff reply
		 */
		rapidjson::Document document;
		document.Parse(resp.body);
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
		document.Accept(writer);
		// cout << "[INFO] HandOff reply is " << s.GetString() << endl;

	} else {
		cout << "[ERROR] Unexpected HTTP code " << resp.status_code
		     << "\n[ERROR] HTTP payload is " << resp.body << endl;
	}

	unimsg_buffer_put(&resp.desc, 1);
}

void prediction_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	// cout << "[INFO] Prediction Callback got a message, length=" << desc.size
	//      << "\n";
	// 	cout << "[INFO] Payload is " << json << endl;

	unimsg_buffer_put(&desc, 1);

	PredictionHandler handler;
	try {
		Reader reader;
		StringStream ss(json.c_str());
		reader.Parse(ss,handler);
	} catch (...) {
		cout << "[ERROR] Got an exception on stringstream read parse\n";
	}

	/* We are only considering download throughput */
	unordered_map<string, int> throughput_map = handler.cell_pred_down;

	/* Decision about CONTROL message
	 * (1) Identify UE Id in Prediction message
	 * (2) Iterate through Prediction message.
	 *     If one of the cells has a higher throughput prediction than
	 *     serving cell, send a CONTROL request. We assume the first cell in
	 *     the prediction message is the serving cell
	 */

	int serving_cell_throughput = 0;
	int highest_throughput = 0;
	string highest_throughput_cell_id;

	// Getting the current serving cell throughput prediction
	auto cell = throughput_map.find( handler.serving_cell_id );
	serving_cell_throughput = cell->second;

	// Iterating to identify the highest throughput prediction
	for (auto map_iter = throughput_map.begin();
	     map_iter != throughput_map.end(); map_iter++) {
		string curr_cellid = map_iter->first;
		int curr_throughput = map_iter->second;

		if ( highest_throughput < curr_throughput ) {
			highest_throughput = curr_throughput;
			highest_throughput_cell_id = curr_cellid;
		}
	}

	/* We also take into account the threshold in A1 policy type 20008 */
	float thresh = 0;
	if (downlink_threshold > 0)
		thresh = serving_cell_throughput * (downlink_threshold / 100.0);

	if (highest_throughput > (serving_cell_throughput + thresh)) {
		/* Sending a control request message */
		send_rest_control_request(handler.ue_id,
					  handler.serving_cell_id,
					  highest_throughput_cell_id);
	} else {
		/* We always send a control request for testing purposes */
		send_rest_control_request(handler.ue_id,
					  handler.serving_cell_id,
					  highest_throughput_cell_id);
	}
}

void send_prediction_request(vector<string> ues_to_predict)
{
	struct unimsg_shm_desc desc;
	int rc;

	rc = unimsg_buffer_get(&desc, 1);
	if (rc) {
		fprintf(stderr, "Error getting buffer: %s\n", strerror(-rc));
		exit(1);
	}

	string ues_list = "[";
	for (unsigned long i = 0; i < ues_to_predict.size(); i++) {
		if (i == ues_to_predict.size() - 1) {
			ues_list = ues_list + "\"" + ues_to_predict.at(i)
				   + "\"]";
		} else {
			ues_list = ues_list + "\"" + ues_to_predict.at(i) + "\""
				   + ",";
		}
	}

	string message_body = "{\"UEPredictionSet\": " + ues_list + "}";

	/* TODO: evaluate building payload in place */
	/* TODO: handle message spanning multiple descs */
	memcpy(desc.addr, message_body.c_str(), message_body.size());
	desc.size = message_body.size();

	// cout << "[INFO] Prediction Request length=" << desc.size << ", payload="
	//      << message_body << endl;

	rc = unimsg_send(qp_sock, &desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned ndescs = 1;
	rc = unimsg_recv(qp_sock, &desc, &ndescs, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	prediction_callback(desc);
}

/* This function works with Anomaly Detection(AD) xApp. It is invoked when
 * anomalous UEs are send by AD xApp. It parses the payload received from AD
 * xApp, sends an ACK with same UEID as payload to AD xApp, and sends a
 * prediction request to the QP Driver xApp.
 */
void ad_callback(struct unimsg_shm_desc *descs, unsigned ndescs)
{
	/* TODO: handle message spread across multiple descs */
	string json ((char *)descs[0].addr, descs[0].size);

	// cout << "[INFO] AD Callback got a message, length=" << desc.size
	//      << "\n";
	// cout << "[INFO] Payload is " << json << "\n";

	AnomalyHandler handler;
	Reader reader;
	StringStream ss(json.c_str());
	reader.Parse(ss,handler);

	send_prediction_request(handler.prediction_ues);

	/* Send the same message back as ACK */
	int rc = unimsg_send(ad_sock, descs, ndescs, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	struct unimsg_sock *lsock;
	int rc;

	parse_command_line(argc, argv);

	rc = unimsg_socket(&lsock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_bind(lsock, PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PORT,
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_listen(lsock);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}

	cout << "[INFO] Waiting for AD connection\n";

	rc = unimsg_accept(lsock, &ad_sock, 0);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		exit(1);
	}

	unimsg_close(lsock);

	cout << "[INFO] AD connected\n";

	rc = unimsg_socket(&qp_sock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_connect(qp_sock, QP_ADDR, QP_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to QP: %s\n", strerror(-rc));
		exit(1);
	}

	cout << "[INFO] Connection to QP established\n";

	for (unsigned i = 0; i < opt_iterations; i++) {
		struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
		unsigned ndescs = 1;
		rc = unimsg_recv(ad_sock, descs, &ndescs, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		ad_callback(descs, ndescs);
	}

	unimsg_close(qp_sock);
	unimsg_close(ad_sock);

	// cout << "[TS] Average POST latency (excluding 1st iteration) "
	//      << (total_latency / (opt_iterations - 1)) << " ns\n";

	return 0;
}
