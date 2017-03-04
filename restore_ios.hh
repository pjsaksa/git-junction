/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_RESTORE_IOS_HEADER
#define GIT_JUNCTION_RESTORE_IOS_HEADER

#include <ios>

class restore_ios {
    std::ios &io;

    std::ios::fmtflags flags;
    std::ios::char_type fill;
    std::streamsize width;
    std::streamsize precision;

public:
    restore_ios(std::ios &_io)
        : io{_io},
          flags{io.flags()},
          fill{io.fill()},
          width{io.width()},
          precision{io.precision()}
    {
    }

    ~restore_ios(void)
    {
        io.flags(flags);
        io.fill(fill);
        io.width(width);
        io.precision(precision);
    }
};

#endif
