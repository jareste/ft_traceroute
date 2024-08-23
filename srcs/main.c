#include <stdio.h>
#include <traceroute.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    run_traceroute(argv[1]);

    return 0;
}