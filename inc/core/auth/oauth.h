#ifndef OAUTH_H
#define OAUTH_H

#include <request.h>

res_t getCcToken();
void parseToken(res_t tkn);
void readToken();

#endif /* oauth.h */