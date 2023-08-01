// vi: ts=4 sw=4 noet:
/*
==================================================================================
	Copyright (c) 2020 AT&T Intellectual Property.

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
	Mnemonic:	ts_xapp.cpp
	Abstract:	Traffic Steering xApp
		   1. Receives A1 Policy
			       2. Receives anomaly detection
			       3. Requests prediction for UE throughput on current and neighbor cells
			       4. Receives prediction
			       5. Optionally exercises Traffic Steering action over E2

	Date:     22 April 2020
	Author:		Ron Shacham

  Modified: 21 May 2021 (Alexandre Huff)
	    Update for traffic steering use case in release D.
	    07 Dec 2021 (Alexandre Huff)
	    Update for traffic steering use case in release E.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <thread>
#include <iostream>
#include <memory>
#include <algorithm>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <deque>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/schema.h>
#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#include <unimsg/net.h>

#include <sstream>

#define PORT 4560
#define QP_ADDR 1
#define QP_PORT 4580
#define RC_ADDR 2
#define RC_PORT 5000

static int downlink_threshold = 0;  // A1 policy type 20008 (in percentage)
static struct unimsg_sock *ad_sock;
static struct unimsg_sock *qp_sock;

using namespace rapidjson;
using namespace std;

using Namespace = std::string;
using Key = std::string;
using Data = std::vector<uint8_t>;
using DataMap = std::map<Key, Data>;
using Keys = std::set<Key>;

// scoped enum to identify which API is used to send control messages
enum class TsControlApi { REST, gRPC };
TsControlApi ts_control_api;  // api to send control messages
string ts_control_ep;         // api target endpoint

typedef struct nodeb {
  string ran_name;
  struct {
    string plmn_id;
    string nb_id;
  } global_nb_id;
} nodeb_t;

unordered_map<string, shared_ptr<nodeb_t>> cell_map; // maps each cell to its nodeb

/* struct UEData {
  string serving_cell;
  int serving_cell_rsrp;
}; */

struct PolicyHandler : public BaseReaderHandler<UTF8<>, PolicyHandler> {
  /*
    Assuming we receive the following payload from A1 Mediator
    {"operation": "CREATE", "policy_type_id": 20008, "policy_instance_id": "tsapolicy145", "payload": {"threshold": 5}}
  */
  unordered_map<string, string> cell_pred;
  std::string ue_id;
  bool ue_id_found = false;
  string curr_key = "";
  string curr_value = "";
  int policy_type_id;
  int policy_instance_id;
  int threshold;
  std::string operation;
  bool found_threshold = false;

  bool Null() { return true; }
  bool Bool(bool b) { return true; }
  bool Int(int i) {

    if (curr_key.compare("policy_type_id") == 0) {
      policy_type_id = i;
    } else if (curr_key.compare("policy_instance_id") == 0) {
      policy_instance_id = i;
    } else if (curr_key.compare("threshold") == 0) {
      found_threshold = true;
      threshold = i;
    }

    return true;
  }
  bool Uint(unsigned u) {

    if (curr_key.compare("policy_type_id") == 0) {
      policy_type_id = u;
    } else if (curr_key.compare("policy_instance_id") == 0) {
      policy_instance_id = u;
    } else if (curr_key.compare("threshold") == 0) {
      found_threshold = true;
      threshold = u;
    }

    return true;
  }
  bool Int64(int64_t i) {  return true; }
  bool Uint64(uint64_t u) {  return true; }
  bool Double(double d) {  return true; }
  bool String(const char* str, SizeType length, bool copy) {

    if (curr_key.compare("operation") != 0) {
      operation = str;
    }

    return true;
  }
  bool StartObject() {

    return true;
  }
  bool Key(const char* str, SizeType length, bool copy) {

    curr_key = str;

    return true;
  }
  bool EndObject(SizeType memberCount) {  return true; }
  bool StartArray() {  return true; }
  bool EndArray(SizeType elementCount) {  return true; }
};

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
  bool Uint(unsigned u) {
    // Currently, we assume the first cell in the prediction message is the serving cell
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
  bool String(const char* str, SizeType length, bool copy) {

    return true;
  }
  bool StartObject() {  return true; }
  bool Key(const char* str, SizeType length, bool copy) {
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

// void policy_callback( Message& mbuf, int mtype, int subid, int len, Msg_component payload,  void* data ) {
//   string arg ((const char*)payload.get(), len); // RMR payload might not have a nil terminanted char

//   cout << "[INFO] Policy Callback got a message, type=" << mtype << ", length=" << len << "\n";
//   cout << "[INFO] Payload is " << arg << endl;

//   PolicyHandler handler;
//   Reader reader;
//   StringStream ss(arg.c_str());
//   reader.Parse(ss,handler);

//   //Set the threshold value
//   if (handler.found_threshold) {
//     cout << "[INFO] Setting Threshold for A1-P value: " << handler.threshold << "%\n";
//     downlink_threshold = handler.threshold;
//   }

// }

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

	rc = unimsg_buffer_get(&desc);
	if (rc) {
		fprintf(stderr, "Error getting buffer: %s\n", strerror(-rc));
		exit(1);
	}

	sprintf((char *)desc.addr, post_template, url.c_str(), addr, port,
		body.size(), body.c_str());
	desc.size = strlen((char *)desc.addr);

	rc = unimsg_send(rc_sock, &desc, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_recv(rc_sock, &desc, 0);
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

	cout << "[INFO] Sending a HandOff CONTROL message\n";
	cout << "[INFO] HandOff request is " << msg << endl;

	struct rest_resp resp = do_post(RC_ADDR, RC_PORT, "/api/echo", msg);

	if (resp.status_code == 200) {
		/* ============= DO SOMETHING USEFUL HERE =============
		 * Currently, we only print out the HandOff reply
		 */
		rapidjson::Document document;
		document.Parse(resp.body);
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
		document.Accept(writer);
		cout << "[INFO] HandOff reply is " << s.GetString() << endl;

	} else {
		cout << "[ERROR] Unexpected HTTP code " << resp.status_code
		     << "\n[ERROR] HTTP payload is " << resp.body << endl;
	}

	unimsg_buffer_put(&resp.desc);
}

void prediction_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	cout << "[INFO] Prediction Callback got a message, length=" << desc.size
	     << "\n";
	cout << "[INFO] Payload is " << json << endl;

	unimsg_buffer_put(&desc);

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
		cout << "[INFO] The current serving cell \""
		     << handler.serving_cell_id << "\" is the best one" << endl;
	}
}

void send_prediction_request(vector<string> ues_to_predict)
{
	struct unimsg_shm_desc desc;

	int rc = unimsg_socket(&qp_sock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_connect(qp_sock, QP_ADDR, QP_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to QP: %s\n", strerror(-rc));
		exit(1);
	}

	unimsg_buffer_get(&desc); 
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
	memcpy(desc.addr, message_body.c_str(), message_body.size());
	desc.size = message_body.size();

	cout << "[INFO] Prediction Request length=" << desc.size << ", payload="
	     << message_body << endl;

	rc = unimsg_send(qp_sock, &desc, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_recv(qp_sock, &desc, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	unimsg_close(qp_sock);

	prediction_callback(desc);
}

/* This function works with Anomaly Detection(AD) xApp. It is invoked when
 * anomalous UEs are send by AD xApp. It parses the payload received from AD
 * xApp, sends an ACK with same UEID as payload to AD xApp, and sends a
 * prediction request to the QP Driver xApp.
 */
void ad_callback(struct unimsg_shm_desc desc)
{
	string json ((char *)desc.addr, desc.size);

	cout << "[INFO] AD Callback got a message, length=" << desc.size
	     << "\n";
	cout << "[INFO] Payload is " << json << "\n";

	AnomalyHandler handler;
	Reader reader;
	StringStream ss(json.c_str());
	reader.Parse(ss,handler);

	/* Send an empty resposne, it can only mean ACK */
	desc.size = 0;
	int rc = unimsg_send(ad_sock, &desc, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	send_prediction_request(handler.prediction_ues);
}

int main(int argc, char *argv[])
{
	int rc;

	rc = unimsg_socket(&ad_sock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_bind(ad_sock, PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PORT,
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_listen(ad_sock);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}

	printf("Waiting for AD connection\n");

	struct unimsg_sock *tmp_sock;
	rc = unimsg_accept(ad_sock, &tmp_sock, 0);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		exit(1);
	}

	printf("AD connected\n");

	unimsg_close(ad_sock);
	ad_sock = tmp_sock;

	struct unimsg_shm_desc desc;
	rc = unimsg_recv(ad_sock, &desc, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	ad_callback(desc);

	unimsg_close(ad_sock);

	return 0;
}
