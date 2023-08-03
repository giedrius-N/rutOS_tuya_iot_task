#ifndef ARG_PARSER_H
#define ARG_PARSER_H
#include <argp.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

error_t parse_opt(int key, char *arg, struct argp_state *state);

#endif