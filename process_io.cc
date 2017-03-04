/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "process_io.hh"
#include "config.hh"
#include "exception.hh"

#include <sstream>
#include <unistd.h>

//

namespace
{
    static void move_fd(int oldfd, int newfd)
    {
        if (oldfd < 0 || newfd < 0)
            throw generic_exception{"move_fd given invalid fds"};

        if (oldfd == newfd)
            return;

        if (dup2(oldfd, newfd) < 0) {
            std::ostringstream oss_err_msg;
            oss_err_msg << "dup2(" << oldfd << ", " << newfd << ")";

            throw stdlib_exception{oss_err_msg.str(), errno};
        }

        close(oldfd);
    }

    static std::pair<int, int> pipe_fork_exec(const std::string &argv0, bool use_shell)
    {
        int fds_read[2]  {-1, -1};
        int fds_write[2] {-1, -1};

        // create pipes

        if (pipe(fds_read) != 0
            || pipe(fds_write) != 0)
        {
            throw stdlib_exception{"pipe", errno};
        }

        // spawn process

        int pid =-1;

        if ((pid =fork()) < 0) {
            throw stdlib_exception{"fork", errno};
        }
        else if (pid == 0) // child
        {
            close(fds_read[0]);
            close(fds_write[1]);

            move_fd(fds_write[0], STDIN_FILENO);
            move_fd(fds_read[1], STDOUT_FILENO);

            if (use_shell) {
                execl(config::bash_bin,
                      config::bash_bin,
                      "-c",
                      argv0.c_str(),
                      (char *)0);
            }
            else {
                execl(argv0.c_str(),
                      argv0.c_str(),
                      (char *)0);
            }
            perror("execl");
            _exit(-1);
        }

        // parent

        close(fds_read[1]);
        close(fds_write[0]);

        return std::make_pair(fds_read[0], fds_write[1]);
    }
}

// *********************************************************

process_io::process_io(const std::string &argv0, bool use_shell)
    : fds{pipe_fork_exec(argv0, use_shell)},
      input_buf{fds.first, std::_S_in},
      output_buf{fds.second, std::_S_out},
      input_stream{&input_buf},
      output_stream{&output_buf}
{
}

void process_io::close_read()
{
    input_buf.close();
}

void process_io::close_write()
{
    output_stream.flush();
    output_buf.close();
}
