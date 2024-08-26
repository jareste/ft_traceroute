#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PCK_SIZE 40
#define PCK_DATA_SIZE (PCK_SIZE - sizeof(struct icmphdr))

#define RECV_BUFSIZE 1024

#define SWAP_ENDIANESS_16(n) ((n & 0xff) << 8 | (n >> 8))

int memdiff(void *a, void *b, size_t len);

void traceroute(char* host, uint64_t first_hop, uint64_t max_hops, uint64_t probes_per_hop, bool debug);

#endif