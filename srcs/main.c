#include <stdio.h>
#include <traceroute.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>


void print_usage()
{
    printf("%s", USAGE);
}

void parse_argv(int argc, char *argv[], char **interface, int *max_ttl, int *max_probes,
                char **destination, bool *debug, int *first_hop, bool *dns, bool *use_ipv6) {
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "?hi:m:q:df:4In6", long_options, NULL)) != -1) {
        switch (opt) {
            case '?':
            case 'h':
                fprintf(stderr, "Help:\n");
                print_usage();
                exit(0);
            case '4':
            case 'I':
                break;
            case '6':
                *use_ipv6 = true;
                break;
            case 'i':
                if (optarg) {
                    *interface = optarg;
                } else {
                    fprintf(stderr, "Option -i requires an argument.\n");
                    print_usage();
                    exit(1);
                }
                break;
            case 'm':
                if (optarg && isdigit(optarg[0])) {
                    *max_ttl = atoi(optarg);
                    if (*max_ttl > 255) {
                        fprintf(stderr, "ft_ping: invalid argument: '%d': out of range: 0 <= value <= 255\n", *max_ttl);
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "Option -m contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            case 'q':
                if (optarg && isdigit(optarg[0])) {
                    *max_probes = atoi(optarg);
                } else {
                    fprintf(stderr, "Option -q contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            case 'd':
                *debug = true;
                break;
            case 'f':
                if (optarg && isdigit(optarg[0])) {
                    *first_hop = atoi(optarg);
                } else {
                    fprintf(stderr, "Option -f contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            case 'n':
                *dns = false;
                break;
            default:
                fprintf(stderr, "Unknown option: %c\n", opt);
                print_usage();
                exit(1);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        print_usage();
        exit(1);
    }

    *destination = argv[optind];
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    char* interface = NULL;
    int first_hop = 1;
    int max_hops = 30;
    int pph = 3;
    char *destination = NULL;
    bool debug = false;
    bool dns = true;
    bool ipv6 = false;

    parse_argv(argc, argv, &interface, &max_hops, &pph, &destination, &debug, &first_hop, &dns, &ipv6);

    traceroute(destination, first_hop, max_hops, pph, debug, interface, dns, false);

    return 0;
}