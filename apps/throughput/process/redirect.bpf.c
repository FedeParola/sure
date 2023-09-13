#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include "common.h"

struct {
	__uint(type, BPF_MAP_TYPE_SOCKHASH);
	__uint(key_size, sizeof(struct conn_id));
	__uint(value_size, sizeof(int));
	__uint(max_entries, 1024);
} sockmap SEC(".maps");

SEC("sk_msg")
int prog_msg_verdict(struct sk_msg_md *msg)
{
	struct conn_id cid = {
		.raddr = msg->remote_ip4,
		.laddr = msg->local_ip4,
		.rport = msg->remote_port >> 16,
		.lport = msg->local_port,
	};

	return bpf_msg_redirect_hash(msg, &sockmap, &cid, BPF_F_INGRESS);
}

char _license[] SEC("license") = "GPL";