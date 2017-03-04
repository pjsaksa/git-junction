/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "terminal_input.hh"

#include <iostream>
#include <cstring>
#include <unistd.h>

//

namespace
{
    static void empty_signal_handler(int)
    {
    }
}

// *********************************************************

terminal_input::terminal_input(bool h)
    : hide{h}
{
    tcflush(STDIN_FILENO, TCIFLUSH);    // flush input

    if (hide) {
        struct termios tio;

        tcgetattr(STDIN_FILENO, &old_tio);
        tio          = old_tio;
        tio.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);  // echo off
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);
    }

    struct sigaction signal_handler;

    memset(&signal_handler, 0, sizeof(signal_handler));

    signal_handler.sa_handler =empty_signal_handler;
    sigaction(SIGINT, &signal_handler, &old_signal_handler);
}

terminal_input::~terminal_input()
{
    tcflush(STDIN_FILENO, TCIFLUSH);    // flush again, just because

    sigaction(SIGINT, &old_signal_handler, 0);

    if (hide) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

        std::cout << '\n' << std::flush;
    }
}
