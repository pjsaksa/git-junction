/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "main_menu.hh"
#include "config.hh"
#include "exception.hh"
#include "input.hh"
#include "utils.hh"

#include <string>
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <sys/stat.h>

//

namespace
{
    class accept_user : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
            if (!accept_size(input.size(), config::user_min_size, config::user_max_size))
                return false;

            lowercase(input);

            for (auto ptr =input.begin();
                 ptr != input.end();
                 ++ptr)
            {
                switch (*ptr) {
                case 'a' ... 'z':
                case '0' ... '9':
                    break;

                default:
                    std::cout << "invalid character, only a-z and 0-9 allowed\n";
                    return false;
                }
            }

            return true;
        }
    };

    // *****

    class accept_password : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
            if (!accept_size(input.size(), config::password_min_size, config::password_max_size))
                return false;

            for (auto ptr =input.begin();
                 ptr != input.end();
                 ++ptr)
            {
                const unsigned char ch =*ptr;

                if (ch < 32 || ch > 127) {
                    std::cout << "invalid character(s) found, only printable ASCII-characters accepted\n";
                    return false;
                }
            }

            return true;
        }
    };

    // *********************************************************

    static void create_user(const std::string &directory)
    {
        if (mkdir(directory.c_str(), 0700) != 0)
            throw stdlib_exception{"mkdir(" + directory + ")", errno};

        const std::string keys_dir =directory + "/keys";

        if (mkdir(keys_dir.c_str(), 0700) != 0)
            throw stdlib_exception{"mkdir(" + keys_dir + ")", errno};
    }

    // *********************************************************

    static std::string do_login()
    {
    start_over:
        const std::string user =read_field("u: ", false, accept_user());

        const bool user_exists =scan_for_user(user);

        if (!user_exists) {
            std::cout << "user " << user << " doesn't seem to exist\n";

            std::ostringstream question_oss;
            question_oss << "create a new user '" << user << "' [yes/no] ? ";

            const std::string yes_or_no =read_field(question_oss.str(), false, accept_yes_or_no());

            if (yes_or_no != "yes") {
                std::cout << '\n';
                goto start_over;
            }
        }

        const std::string password  =read_field("p: ", true, accept_password());
        const std::string directory =calc_directory_name(user, password);

        if (!user_exists)
            create_user(directory);

        if (chdir(directory.c_str()) != 0) {
            std::cout << "invalid password\n"
                "\n";

            sleep(3);
            goto start_over;
        }

        return user;
    }
}

// *********************************************************

int main()
{
    enum {
        return_ok =0,
        return_stdin_error,
        return_generic_error,
        return_stdlib_error,
    };

    umask(0002);
    srandom(time(0));

    std::cout << "\n"
        "git junction console\n"
        "\n";

    try {
        const std::string user =do_login();

        while (main_menu::run(user))
            ;
    }
    catch (stdin_exception) {
        return return_stdin_error;
    }
    catch (generic_exception &e) {
        std::cerr << "something happened that is not expected to happen, contact the administration\n"
            "message (generic): " << e << "\n";

        read_field("\n(press enter)", true, accept_enter());

        return return_generic_error;
    }
    catch (stdlib_exception &e) {
        std::cerr << "something happened that is not expected to happen, contact the administration\n"
            "message (stdlib): " << e << "\n";

        read_field("\n(press enter)", true, accept_enter());

        return return_stdlib_error;
    }

    return return_ok;
}
