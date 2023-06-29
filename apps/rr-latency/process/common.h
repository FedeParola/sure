#include <stdint.h>

struct conn_id {
	uint32_t raddr;
	uint32_t laddr;
	uint16_t rport;
	uint16_t lport;
} __attribute__((packed));