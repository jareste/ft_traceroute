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

void traceroute(char* host, uint64_t first_hop, uint64_t max_hops, uint64_t probes_per_hop, bool debug, char *interface, bool dns, bool use_ipv6);

#define USAGE "Usage: ft_traceroute [options] <destination>\n" \
              "Options:\n" \
              "  -h, --help        Display this help message\n" \
              "  -?                Display this help message\n" \
              "  -4                Use IPv4\n" \
              "  -6                Use IPv6\n"\
              "  -I                Use ICMP ECHO for tracerouting\n" \
              "  -i <interface>    Specify network interface\n" \
              "  -d                Enable socket-level debugging\n" \
              "  -m <max_hops>     Set the max number of hops\n" \
              "  -q <nqueries>     Set the number of probes per hop\n" \
              "  -f <first_hop>    Set the initial hop distance\n"

#endif