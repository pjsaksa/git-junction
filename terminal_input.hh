/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_TERMINAL_INPUT_HEADER
#define GIT_JUNCTION_TERMINAL_INPUT_HEADER

#include <termios.h>
#include <signal.h>

class terminal_input {
    bool hide;
    struct termios old_tio;
    struct sigaction old_signal_handler;

public:
    terminal_input(bool h);
    ~terminal_input();
};

#endif
