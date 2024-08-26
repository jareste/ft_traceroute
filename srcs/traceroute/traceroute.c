#include <traceroute.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int memdiff(void *a, void *b, size_t len)
{
	return memcmp(a, b, len) != 0;
}

unsigned short checksum(void *buf, size_t len)
{
    unsigned short* addr = (unsigned short*)buf;
	unsigned long sum = 0;
	for (; len > sizeof(char); len -= sizeof(short))
		sum += *addr++;
	if (len == sizeof(char))
		sum += *(unsigned char *)addr;
	unsigned char bits = sizeof(short) * 8;
	while (sum >> bits)
		sum = (sum & ((1 << bits) - 1)) + (sum >> bits);
	return (~sum);
}

void	craft_traceroute_packet(struct icmphdr *buf)
{
	buf->type = ICMP_ECHO;
	buf->code = 0;
	buf->checksum = 0;
	buf->un.echo.id = SWAP_ENDIANESS_16(getpid());
	buf->un.echo.sequence = SWAP_ENDIANESS_16(1);

	for (uint64_t i = 0; i < PCK_DATA_SIZE; ++i)
    {
		((char *)buf)[sizeof(struct icmphdr) + i] = i % 3 + 1;
	}

	buf->checksum = checksum(buf, PCK_SIZE);
}

void traceroute(char* host, uint64_t first_hop, uint64_t max_hops, uint64_t probes_per_hop, bool debug)
{
	struct addrinfo	*addr;

	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;

	if (getaddrinfo(host, NULL, &hints, &addr) < 0)
    {
		fprintf(stderr, "traceroute: cannot resolve %s: Unknown host\n", host);
		exit(1);
	}

	int	sock = socket(addr->ai_family, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0)
    {
		perror("traceroute: could not create socket");
		exit(1);
	}

	struct timeval timeout = {1, 0};
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
		perror("traceroute: setsockopt SO_RCVTIMEO");
		exit(1);
	}

	if (debug)
    {
		if (setsockopt(sock, SOL_SOCKET, SO_DEBUG, &debug, sizeof(debug)) < 0)
        {
			perror("traceroute: setsockopt SO_DEBUG");
			exit(1);
		}
	}

	uint64_t	hops = first_hop;

	char	hostip_s[INET6_ADDRSTRLEN];
	inet_ntop(addr->ai_family, &((struct sockaddr_in *)addr->ai_addr)->sin_addr, hostip_s, INET6_ADDRSTRLEN);

	printf("traceroute to %s (%s), %ld hops max, %ld byte packets\n",
		host, hostip_s,
		max_hops, PCK_SIZE + sizeof(struct ip));

	/** Exec */

	while (hops <= max_hops)
    {
		if (setsockopt(sock, IPPROTO_IP, IP_TTL, &hops, sizeof(hops)) < 0)
        {
			perror("traceroute: setsockopt IP_TTL");
			exit(1);
		}

		printf("%2ld", hops);
		int	reached = 1;
		struct sockaddr_in	prev_addr = {};

		for (uint64_t i = 0; i < probes_per_hop; ++i)
        {
			char	buf[PCK_SIZE];
			craft_traceroute_packet((struct icmphdr *)buf);

			struct timeval	start;
            gettimeofday(&start, NULL);

			if (sendto(sock, buf, PCK_SIZE, 0, addr->ai_addr, addr->ai_addrlen) < 0)
            {
				perror("traceroute: sendto");
				exit(1);
			}

			char				recvbuf[RECV_BUFSIZE];
			struct sockaddr_in	r_addr;
			uint				addr_len = sizeof(r_addr);

			if (recvfrom(sock, &recvbuf, RECV_BUFSIZE, 0, (struct sockaddr*)&r_addr, &addr_len) < 0)
            {
				printf("  *");
				reached = 0;
				continue ;
			}

			struct timeval end;
            gettimeofday(&end, NULL);
            
			double	time = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000;

			char	ipbuf[INET6_ADDRSTRLEN];
			inet_ntop(r_addr.sin_family, &r_addr.sin_addr, ipbuf, sizeof(ipbuf));

			if (i == 0 || memdiff(&prev_addr, &r_addr, sizeof(r_addr)))
            {
				printf("  %s", ipbuf);
			}
			printf("  %.3f ms", time);

			prev_addr = r_addr;

			struct icmphdr	*icmp_res = (void *)recvbuf + sizeof(struct ip);

			if (icmp_res->type == ICMP_TIME_EXCEEDED)
            {
				reached = 0;
			}
		}

		printf("\n");

		if (reached)
        {
			break ;
		}

		++hops;
	}

}