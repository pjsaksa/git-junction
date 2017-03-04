/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_SHELL_QUOTE_HEADER
#define GIT_JUNCTION_SHELL_QUOTE_HEADER

#ifdef __cplusplus
extern "C" {
#endif

    int need_bs_quote(char c);
    char *sq_dequote(char *arg);

#ifdef __cplusplus
}
#endif

#endif
