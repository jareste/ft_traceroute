#include <stdio.h>
#include <traceroute.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    char* interface = NULL;
    int max_hops = 30;

    run_traceroute(argv[1], interface, max_hops);

    return 0;
}