/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "input.hh"
#include "exception.hh"
#include "terminal_input.hh"
#include "utils.hh"

#include <iostream>
#include <sstream>

#include <unistd.h>

//

bool accept_field_functor::accept_size(std::string::size_type size,
                                       std::string::size_type min,
                                       std::string::size_type max)
{
    if (size < min) {
        std::cout << "too short, " << min << '-' << max << " characters\n";
        return false;
    }

    if (size > max) {
        std::cout << "too long, " << min << '-' << max << " characters\n";
        return false;
    }

    return true;
}

accept_field_functor::~accept_field_functor()
{
}

// *********************************************************

bool accept_yes_or_no::operator() (std::string &input) const
{
    if (input.size() < 2
        || input.size() > 3)
    {
        return false;
    }

    lowercase(input);

    return input == "yes"
        || input == "no";
}

// *********************************************************

static std::string read_line(const std::string &prompt, bool hide)
{
    terminal_input term_input{hide};

    std::cout << prompt << std::flush;

    std::ostringstream oss;
    char inp;

    while (true) {
        const int i =read(STDIN_FILENO, &inp, 1);

        if (i != 1) {
            std::cout << '\n';
            throw stdin_exception{};
        }

        if (inp == '\n' || inp == '\r')
            break;
        if (inp >= 0 && inp < 32)
            continue;

        oss << inp;
    }

    return oss.str();
}

// *********************************************************

std::string read_field(const std::string prompt, bool hide, const accept_field_functor &accept)
{
    while (true) {
        std::string input =read_line(prompt, hide);

        if (accept(input))  // accept() may modify input
            return input;
    }
}
