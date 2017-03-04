/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_INPUT_HEADER
#define GIT_JUNCTION_INPUT_HEADER

#include <string>

class accept_field_functor {
protected:
    static bool accept_size(std::string::size_type size,
                            std::string::size_type min,
                            std::string::size_type max);

public:
    virtual ~accept_field_functor();
    virtual bool operator() (std::string &) const =0;
};

// *****

class accept_yes_or_no : public accept_field_functor {
public:
    virtual bool operator() (std::string &) const;
};

// *****

class accept_enter : public accept_field_functor {
public:
    virtual bool operator() (std::string &) const
    {
        return true;
    }
};

// *********************************************************

std::string read_field(const std::string prompt, bool hide, const accept_field_functor &accept);

#endif
