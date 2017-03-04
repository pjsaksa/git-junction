/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_PROCESS_IO_HEADER
#define GIT_JUNCTION_PROCESS_IO_HEADER

#include <string>
#include <istream>
#include <ostream>

#include <ext/stdio_filebuf.h>

class process_io {
    const std::pair<int, int> fds;
    __gnu_cxx::stdio_filebuf<char> input_buf;
    __gnu_cxx::stdio_filebuf<char> output_buf;
    std::istream input_stream;
    std::ostream output_stream;

public:
    process_io(const std::string &argv0, bool use_shell =true);

    std::istream &read() { return input_stream; }
    std::ostream &write() { return output_stream; }

    void close_read();
    void close_write();
};

#endif
