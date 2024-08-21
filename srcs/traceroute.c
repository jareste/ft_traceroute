#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
    {
        sum += *buf++;
    }
    if (len == 1)
    {
        sum += *(unsigned char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void resolve_hostname(char *ip, char *hostname, size_t hostname_len)
{
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &sa.sin_addr);
    getnameinfo((struct sockaddr *)&sa, sizeof(sa), hostname, hostname_len, NULL, 0, 0);
}

void run_traceroute(const char *hostname)
{
    struct addrinfo hints, *res;
    struct sockaddr_in dest, recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    int err = getaddrinfo(hostname, NULL, &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return;
    }

    memcpy(&dest, res->ai_addr, sizeof(dest));
    freeaddrinfo(res);

    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, ipstr, sizeof(ipstr));
    printf("traceroute to %s (%s), 30 hops max\n", hostname, ipstr);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt timeout");
        close(sockfd);
        return;
    }

    for (int ttl = 1; ttl <= 30; ttl++) {
        if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("setsockopt TTL");
            close(sockfd);
            return;
        }

        int received_responses = 0;
        char hop_ip[INET_ADDRSTRLEN] = "";
        char hop_hostname[NI_MAXHOST] = "";

        for (int probe = 0; probe < 3; probe++)
        {
            struct icmp icmp_packet;
            memset(&icmp_packet, 0, sizeof(icmp_packet));
            icmp_packet.icmp_type = ICMP_ECHO;
            icmp_packet.icmp_code = 0;
            icmp_packet.icmp_cksum = 0;
            icmp_packet.icmp_id = getpid();
            icmp_packet.icmp_seq = ttl * 3 + probe;
            icmp_packet.icmp_cksum = checksum(&icmp_packet, sizeof(icmp_packet));

            struct timeval start, end;
            gettimeofday(&start, NULL);

            ssize_t sent_bytes = sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr*)&dest, sizeof(dest));
            if (sent_bytes <= 0)
            {
                perror("sendto");
                continue;
            }

            char recv_buffer[1500];

            ssize_t recv_bytes = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*)&recv_addr, &recv_addr_len);
            if (recv_bytes > 0)
            {
                gettimeofday(&end, NULL);
                double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

                inet_ntop(AF_INET, &recv_addr.sin_addr, hop_ip, sizeof(hop_ip));
                if (probe == 0)
                {
                    resolve_hostname(hop_ip, hop_hostname, sizeof(hop_hostname));
                    printf("%2d  %s (%s)  ", ttl, hop_hostname, hop_ip);
                }

                printf("%.3f ms  ", rtt);
                received_responses++;
            }
            else
            {
                if (probe == 0)
                {
                    printf("%2d  ", ttl);
                }
                printf("*  ");
            }
        }

        printf("\n");

        if (received_responses > 0 && recv_addr.sin_addr.s_addr == dest.sin_addr.s_addr) {
            break;
        }
    }

    close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    run_traceroute(argv[1]);

    return 0;
}
