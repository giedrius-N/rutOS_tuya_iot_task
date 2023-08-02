#ifndef ARG_PARSER_H
#define ARG_PARSER_H
#include <argp.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

struct argp_option options[];
char args_doc[];
struct argp argp;

struct arguments {
    char prodId[50];
    char devId[50];
    char devSec[50];
    bool daemonize;
};

error_t parse_opt(int key, char *arg, struct argp_state *state);

#endif