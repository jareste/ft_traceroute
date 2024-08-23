#include <stdio.h>
#include <traceroute.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>


void print_usage()
{
    printf("USAGE: ft_traceroute [options] destination\n");
    printf("Options:\n");
    printf("  -h, -?            Display this help and exit.\n");
    printf("  -i <interface>    Specify the network interface to use.\n");
    printf("  -m <max_ttl>      Set the maximum TTL (hops), default is 30.\n");
    printf("  -q <max_probes>   Set the number of probes per hop, default is 3.\n");
    printf("\n");
    printf("Example:\n");
    printf("  ft_traceroute -m 64 -q 4 8.8.8.8\n");
}

void parse_argv(int argc, char *argv[], char **interface, int *max_ttl, int *max_probes, char **destination)
{
    int opt;

    while ((opt = getopt(argc, argv, "?hi:m:q:")) != -1)
    {
        switch (opt)
        {
            case '?':
            case 'h':
                print_usage();
                exit(0);
            case 'i':
                if (optarg)
                {
                    *interface = optarg;
                }
                else
                {
                    fprintf(stderr, "Option -i requires an argument.\n");
                    print_usage();
                    exit(1);
                }
                break;
            case 'm':
                if (optarg && isdigit(optarg[0]))
                {
                    *max_ttl = atoi(optarg);
                    if (*max_ttl > 255)
                    {
                        fprintf(stderr, "ft_ping: invalid argument: '%d': out of range: 0 <= value <= 255\n", *max_ttl);
                        exit (1);
                    }
                }
                else
                {
                    fprintf(stderr, "Option -m contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            case 'q':
                if (optarg && isdigit(optarg[0]))
                {
                    *max_probes = atoi(optarg);
                }
                else
                {
                    fprintf(stderr, "Option -q contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            default:
                print_usage();
                exit(1);
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Expected argument after options\n");
        print_usage();
        exit(1);
    }

    *destination = argv[optind];
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        print_usage();
        return 1;
    }

    char* interface = NULL;
    int max_hops = 30;
    int max_probes = 3;
    char *destination = NULL;

    parse_argv(argc, argv, &interface, &max_hops, &max_probes, &destination);


    run_traceroute(destination, interface, max_hops, max_probes);

    return 0;
}