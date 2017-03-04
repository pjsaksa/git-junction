/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "config.hh"
#include "quote.h"
#include "cgitrc.hh"
#include "exception.hh"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <cstring>
#include <cerrno>

#include <unistd.h>

int main(int argc, char *argv[])
{
    const std::string gjuser{getenv("GJUSER")};

    if (gjuser.size() < config::user_min_size
        || gjuser.size() > config::user_max_size)
    {
        std::cerr << "missing/invalid environment variable GJUSER\n";
        return 1;
    }

    //

    const std::vector<std::string> args{argv + 1, argv + argc};

    if (args[0] != "-c") {
        std::cerr << "arguments don't seem to be shell commands\n";
        return 1;
    }

    std::istringstream iss(args[1]);

    std::string command;

    if (!(iss >> command >> std::ws
          && (command == "git-receive-pack"
              || command == "git-upload-pack"
              || command == "git-upload-archive")))
    {
        std::cerr << "only git commands are supported (" << command << ")\n";
        return 1;
    }

    // TODO: skip arguments starting with "--"

    std::string quoted_path;

    if (!getline(iss, quoted_path)
        || quoted_path.empty())
    {
        std::cerr << "failed to read the command argument\n";
        return 1;
    }

    char buffer[quoted_path.size() + 1];
    memcpy(buffer, quoted_path.data(), quoted_path.size());
    buffer[quoted_path.size()] = 0;

    const char *dequoted_path;

    if (!(dequoted_path =sq_dequote(buffer))) {
        std::cerr << "failing to dequote the project directory\n";
        return 1;
    }

    const std::string path =config::base_path + dequoted_path;

    //

    try {
        const cgitrc rc{cgitrc::import_from_file(path + "/cgitrc")};

        if (rc.get_owner() != gjuser) {
            std::cerr << "you don't seem to be the owner of this repository\n";
            return 1;
        }
    }
    catch (import_exception) {
        std::cerr << "failing to find a git repository in that directory (" << path << ")\n";
        return 1;
    }

    // quoting the modified path like the git-shell wants it quoted

    std::ostringstream oss;

    oss << command << " '";

    for (auto ptr = path.begin();
         ptr != path.end();
         ++ptr)
    {
        if (need_bs_quote(*ptr))
            oss << "̈́'\\" << *ptr << "'";
        else
            oss << *ptr;
    }
    oss << '\'';

    execl(config::git_shell_bin,
          config::git_shell_bin,
          "-c",
          oss.str().c_str(),
          static_cast<char*>(0));

    std::cerr << "exec: " << strerror(errno) << "\n";
    return 2;
}
