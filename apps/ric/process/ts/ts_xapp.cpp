#include <iostream>
#include <map>
#include <netinet/in.h>
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
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define PORT 4560
#define QP_ADDR 1
#define QP_PORT 4580
#define RC_ADDR 0x7f000001 /* 127.0.0.1 */
#define RC_PORT 5000
#define ITERATIONS 20
#define MAX_MSG_SIZE 4096

static int downlink_threshold = 0;  // A1 policy type 20008 (in percentage)
static int ad_sock;
static int qp_sock;
static unsigned it = 0;
static unsigned long total_latency = 0;

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

struct rest_resp {
	unsigned status_code;
	char *body;
};

static char post_template[] = "POST %s HTTP/1.1\r\n" /* url */
			      "Host: %u:%u\r\n" /* addr:port */
			      "Accept: application/json\r\n"
			      "Content-Type: application/json\r\n"
			      "Content-Length: %u\r\n" /* body length */
			      "\r\n"
			      "%s"; /* body */

static struct rest_resp do_post(uint32_t addr, uint16_t port, string url,
				string body)
{
	int rc_sock;
	char msg[MAX_MSG_SIZE];
	int rc;

	rc_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (rc_sock < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(addr);
	saddr.sin_port = htons(port);
	if (connect(rc_sock, (struct sockaddr *)&saddr, sizeof(saddr))) {
		fprintf(stderr, "Error connecting to RC: %s\n",
			strerror(errno));
		exit(1);
	}

	sprintf(msg, post_template, url.c_str(), addr, port, body.size(),
		body.c_str());

	if (send(rc_sock, msg, strlen(msg), 0) != strlen(msg)) {
		fprintf(stderr, "Error sending message: %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	rc = recv(rc_sock, msg, sizeof(msg), 0);
	if (rc <= 0) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(errno));
		exit(1);
	}
	msg[rc] = 0;

	close(rc_sock);

	struct rest_resp resp;
	if (sscanf(msg, "HTTP/1.1 %u OK", &resp.status_code) == EOF) {
		fprintf(stderr, "Received unexpected response: %s\n", msg);
		exit(1);
	}

	/* Locate body */
	resp.body = strstr(msg, "\r\n\r\n");
	if (!resp.body) {
		fprintf(stderr, "Received response without payload:\n%s", msg);
		exit(1);
	}
	resp.body += 4; /* Skip newlines */

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
}

void prediction_callback(char *msg, ssize_t size)
{
	string json (msg, size);

	// cout << "[INFO] Prediction Callback got a message, length=" << size
	//      << "\n";
	// cout << "[INFO] Payload is " << json << endl;

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

	// cout << "[INFO] Prediction Request length=" << message_body.size()
	//      << ", payload=" << message_body << endl;

	if (send(qp_sock, message_body.c_str(), message_body.size(), 0)
	    != message_body.size()) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		exit(1);
	}

	char msg[MAX_MSG_SIZE];
	ssize_t size = recv(qp_sock, msg, sizeof(msg), 0);
	if (size <= 0) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		exit(1);
	}
	msg[size] = 0;

	prediction_callback(msg, size);
}

/* This function works with Anomaly Detection(AD) xApp. It is invoked when
 * anomalous UEs are send by AD xApp. It parses the payload received from AD
 * xApp, sends an ACK with same UEID as payload to AD xApp, and sends a
 * prediction request to the QP Driver xApp.
 */
void ad_callback(char *msg, ssize_t size)
{
	string json (msg, size);

	// cout << "[INFO] AD Callback got a message, length=" << msg << "\n";
	// cout << "[INFO] Payload is " << json << "\n";

	AnomalyHandler handler;
	Reader reader;
	StringStream ss(json.c_str());
	reader.Parse(ss,handler);

	send_prediction_request(handler.prediction_ues);

	/* Send the same message back as ACK */
	if (send(ad_sock, msg, size, 0) != size) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int lsock;
	int rc;
	char msg[MAX_MSG_SIZE];

	lsock = socket(AF_INET, SOCK_STREAM, 0);
	if (lsock < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0x7f000001);
	addr.sin_port = htons(PORT);
	if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error binding: %s\n", strerror(errno));
		exit(1);
	}

	if (listen(lsock, 10)) {
		fprintf(stderr, "Error listening: %s\n", strerror(errno));
		exit(1);
	}

	cout << "[INFO] Waiting for AD connection\n";

	ad_sock = accept(lsock, NULL, NULL);
	if (ad_sock < 0) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(errno));
		exit(1);
	}

	close(lsock);

	cout << "[INFO] AD connected\n";

	qp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (qp_sock < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0x7f000001);
	addr.sin_port = htons(QP_PORT);
	if (connect(qp_sock, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error connecting to QP: %s\n",
			strerror(errno));
		exit(1);
	}

	cout << "[INFO] Connection to QP established\n";

	for (int i = 0; i < ITERATIONS; i++) {
		ssize_t size = recv(ad_sock, msg, sizeof(msg), 0);
		if (size <= 0) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			exit(1);
		}
		msg[size] = 0;

		ad_callback(msg, size);
	}

	close(qp_sock);
	close(ad_sock);

	// cout << "[TS] Average POST latency (excluding 1st iteration) "
	//      << (total_latency / (ITERATIONS - 1)) << " ns\n";

	return 0;
}
