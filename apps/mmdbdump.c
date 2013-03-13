#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "MMDB.h"
#include "MMDB_Helper.h"
#include "getopt.h"
#include <assert.h>
#include <netdb.h>

static int is_ipv4(MMDB_s * mmdb)
{
    return mmdb->depth == 32;
}

int main(int argc, char *const argv[])
{
    int verbose = 0;
    int character;
    char *fname = NULL;

    while ((character = getopt(argc, argv, "vf:")) != -1) {
        switch (character) {
        case 'v':
            verbose = 1;
            break;
        case 'f':
            fname = strdup(optarg);
            break;
        default:
        case '?':
            usage(argv[0]);
        }
    }
    argc -= optind;
    argv += optind;

    if (!fname) {
        fname = strdup("/usr/local/share/GeoIP2/city-v6.db");
    }

    assert(fname != NULL);

    MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);
    //MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_STANDARD);

    if (!mmdb) {
        fprintf(stderr, "Can't open %s\n", fname);
        exit(1);
    }

    free(fname);

    char *ipstr = argv[0];
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } ip;

    int ai_family = is_ipv4(mmdb) ? AF_INET : AF_INET6;
    int ai_flags = AI_V4MAPPED; // accept everything

    if (ipstr == NULL || 0 != MMDB_lookupaddressX(ipstr, ai_family, ai_flags,
                                                  &ip)) {
        fprintf(stderr, "Invalid IP\n");
        exit(1);
    }

    if (verbose) {
        dump_meta(mmdb);
    }

    int status;
    MMDB_root_entry_s root = {.entry.mmdb = mmdb };
    if (is_ipv4(mmdb)) {
        uint32_t ipnum = htonl(ip.v4.s_addr);
        status = MMDB_lookup_by_ipnum(ipnum, &root);
    } else {
        status = MMDB_lookup_by_ipnum_128(ip.v6, &root);

    }
    if (status == MMDB_SUCCESS) {
        if (root.entry.offset > 0) {
            MMDB_return_s res_location;
            MMDB_decode_all_s *decode_all =
                calloc(1, sizeof(MMDB_decode_all_s));
            int err = MMDB_get_tree(&root.entry, &decode_all);
            if (decode_all != NULL)
                MMDB_dump(decode_all, 0);
            free(decode_all);

        } else {
            puts("Sorry, nothing found");       // not found
        }
    }
    return (0);
}