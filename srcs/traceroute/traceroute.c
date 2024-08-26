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
#include <netinet/icmp6.h>


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

void craft_traceroute_packet_v6(struct icmp6_hdr *buf)
{
    buf->icmp6_type = ICMP6_ECHO_REQUEST;
    buf->icmp6_code = 0;
    buf->icmp6_cksum = 0;
    buf->icmp6_id = SWAP_ENDIANESS_16(getpid());
    buf->icmp6_seq = SWAP_ENDIANESS_16(1);

    for (uint64_t i = 0; i < PCK_DATA_SIZE; ++i)
    {
        ((char *)buf)[sizeof(struct icmp6_hdr) + i] = i % 3 + 1;
    }

    buf->icmp6_cksum = checksum(buf, PCK_SIZE); // IPv6 pseudo-header checksum is typically handled by the kernel
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

void resolve_hostname(char *ip, char *hostname, size_t hostname_len, int family)
{
    struct sockaddr_in6 sa6;
    struct sockaddr_in sa;

    if (family == AF_INET6)
    {
        inet_pton(AF_INET6, ip, &sa6.sin6_addr);
        getnameinfo((struct sockaddr *)&sa6, sizeof(sa6), hostname, hostname_len, NULL, 0, 0);
    }
    else
    {
        sa.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &sa.sin_addr);
        getnameinfo((struct sockaddr *)&sa, sizeof(sa), hostname, hostname_len, NULL, 0, 0);
    }
}


#include <netinet/ip6.h>   // For struct ip6_hdr
#include <netinet/icmp6.h> // For struct icmp6_hdr

void traceroute(char* host, uint64_t first_hop, uint64_t max_hops, uint64_t probes_per_hop, bool debug, char *interface, bool dns, bool use_ipv6)
{
    struct addrinfo *addr;
    struct addrinfo hints = {0};

    hints.ai_family = use_ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = use_ipv6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP;

    if (getaddrinfo(host, NULL, &hints, &addr) < 0)
    {
        fprintf(stderr, "traceroute: cannot resolve %s: Unknown host\n", host);
        exit(1);
    }

    int sock = socket(addr->ai_family, SOCK_RAW, use_ipv6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP);
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

    if (interface != NULL)
    {
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0)
        {
            perror("setsockopt (SO_BINDTODEVICE)");
            close(sock);
            return;
        }
    }

    if (debug)
    {
        int debug_int = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_DEBUG, &debug_int, sizeof(debug_int)) < 0)
        {
            perror("traceroute: setsockopt SO_DEBUG");
            exit(1);
        }
    }

    uint64_t hops = first_hop;

    char hostip_s[INET6_ADDRSTRLEN];
    char hostname[NI_MAXHOST] = "";

    inet_ntop(addr->ai_family,
              addr->ai_family == AF_INET 
                ? (void *)&((struct sockaddr_in *)addr->ai_addr)->sin_addr 
                : (void *)&((struct sockaddr_in6 *)addr->ai_addr)->sin6_addr, 
              hostip_s, 
              INET6_ADDRSTRLEN);

    printf("traceroute to %s (%s), %ld hops max, %ld byte packets\n",
           host, hostip_s, max_hops, PCK_SIZE + (use_ipv6 ? sizeof(struct ip6_hdr) : sizeof(struct ip)));

    while (hops <= max_hops)
    {
        if (setsockopt(sock, use_ipv6 ? IPPROTO_IPV6 : IPPROTO_IP, use_ipv6 ? IPV6_UNICAST_HOPS : IP_TTL, &hops, sizeof(hops)) < 0)
        {
            perror("traceroute: setsockopt TTL");
            exit(1);
        }

        printf("%2ld", hops);
        int reached = 1;
        struct sockaddr_in prev_addr = {};
        struct sockaddr_in6 prev_addr6 = {};

        for (uint64_t i = 0; i < probes_per_hop; ++i)
        {
            char buf[PCK_SIZE];
            if (use_ipv6)
                craft_traceroute_packet_v6((struct icmp6_hdr *)buf);
            else
                craft_traceroute_packet((struct icmphdr *)buf);

            struct timeval start;
            gettimeofday(&start, NULL);

            if (sendto(sock, buf, PCK_SIZE, 0, addr->ai_addr, addr->ai_addrlen) < 0)
            {
                perror("traceroute: sendto");
                exit(1);
            }

            char recvbuf[RECV_BUFSIZE];
            struct sockaddr_storage r_addr;
            socklen_t addr_len = sizeof(r_addr);

            if (recvfrom(sock, &recvbuf, RECV_BUFSIZE, 0, (struct sockaddr *)&r_addr, &addr_len) < 0)
            {
                printf("  *");
                reached = 0;
                continue;
            }

            struct timeval end;
            gettimeofday(&end, NULL);

            double time = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000;

            char ipbuf[INET6_ADDRSTRLEN];
            inet_ntop(r_addr.ss_family, r_addr.ss_family == AF_INET 
                      ? (void *)&((struct sockaddr_in *)&r_addr)->sin_addr 
                      : (void *)&((struct sockaddr_in6 *)&r_addr)->sin6_addr, 
                      ipbuf, sizeof(ipbuf));

            if (i == 0 || (r_addr.ss_family == AF_INET && memdiff(&prev_addr, &r_addr, sizeof(struct sockaddr_in))) || (r_addr.ss_family == AF_INET6 && memdiff(&prev_addr6, &r_addr, sizeof(struct sockaddr_in6))))
            {
                resolve_hostname(ipbuf, hostname, NI_MAXHOST, r_addr.ss_family);
                if (dns)
                {
                    printf("  %s (%s)", hostname, ipbuf);
                }
                else
                {
                    printf("  %s", ipbuf);
                }
            }
            printf("  %.3f ms", time);

            if (r_addr.ss_family == AF_INET)
                prev_addr = *(struct sockaddr_in *)&r_addr;
            else
                prev_addr6 = *(struct sockaddr_in6 *)&r_addr;

            struct icmp6_hdr *icmp_res = use_ipv6 
                ? (void *)recvbuf + sizeof(struct ip6_hdr) 
                : (void *)recvbuf + sizeof(struct ip);

            if ((use_ipv6 && icmp_res->icmp6_type == ICMP6_TIME_EXCEEDED) || (!use_ipv6 && ((struct icmphdr *)icmp_res)->type == ICMP_TIME_EXCEEDED))
			{
                reached = 0;
            }
        }

        printf("\n");

        if (reached)
        {
            break;
        }

        ++hops;
    }

    freeaddrinfo(addr);
}

