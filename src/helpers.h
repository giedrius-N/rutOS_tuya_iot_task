#ifndef HELPERS_H
#define HELPERS_H

#define MAX_LUA_SCRIPTS 15

int daemonize();
int isJsonValid(const char* jsonString);
int getResultValue(char *jsonStr, char *responseString);
int checkAction(const char *input, const char *searchString);

#endif
