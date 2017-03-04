/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "main_menu.hh"
#include "config.hh"
#include "exception.hh"
#include "input.hh"
#include "cgitrc.hh"
#include "utils.hh"
#include "repository_menu.hh"
#include "restore_ios.hh"
#include "key_menu.hh"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include <unistd.h>

//

namespace
{
    class accept_repository_name : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
            if (input.empty())
                return true;

            if (!accept_size(input.size(), config::repository_name_min_size, config::repository_name_max_size))
                return false;

            for (auto ptr =input.begin();
                 ptr != input.end();
                 ++ptr)
            {
                switch (*ptr) {
                case 'a' ... 'z':
                case 'A' ... 'Z':
                case '0' ... '9':
                case '.':
                case '/':
                case '_':
                case '-':
                    break;

                default:
                    std::cout << "invalid character(s), only a-z, A-Z, 0-9, '.', '-', '_' and '/' allowed\n";
                    return false;
                }
            }

            return true;
        }
    };

    // *****

    class accept_url : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
            if (input.empty())
                return true;

            if (!accept_size(input.size(), config::url_min_size, config::url_max_size))
                return false;

            for (auto ptr =input.begin();
                 ptr != input.end();
                 ++ptr)
            {
                switch (*ptr) {

                    // unreserved
                case 'a' ... 'z':
                case 'A' ... 'Z':
                case '0' ... '9':
                case '-':
                case '.':
                case '_':
                case '~':
                    break;

                    // reserved
                case ':': case '/': case '?': case '#': case '[':
                case ']': case '@':

                case '!': case '$': case '&': case '\'': case '(':
                case ')': case '*': case '+': case ',': case ';':
                case '=':
                    break;

                default:
                    std::cout << "\n"
                        "invalid character(s) found,\n"
                        "read <https://tools.ietf.org/html/rfc3986#section-2.3> for further information\n"
                        "\n";
                    return false;
                }
            }

            return true;
        }
    };
}

// *********************************************************

class main_menu::scanner : public git_dir_functor {
    main_menu &menu;
    const std::string &user;
public:
    scanner(main_menu &m, const std::string &u)
        : menu{m}, user{u} {}

    virtual void operator() (const std::string &path) const
    {
        try {
            const cgitrc rc =cgitrc::import_from_file(path + "/cgitrc");

            if (rc.get_owner() == user)
                menu.add_repository(path);
        }
        catch (import_exception) {
        }
    }
};

// *********************************************************

class main_menu::accept_functor : public accept_field_functor {
    const main_menu &menu;
public:
    accept_functor(const main_menu &m)
        : menu{m} {}

    virtual bool operator() (std::string &input) const
    {
        lowercase(input);

        if (input == "s"
            || input == "m"
            || input == "k"
            || input == "x"
            || input == "?")
        {
            return true;
        }

        //

        std::istringstream iss{input};

        int repository_number;

        if (iss >> repository_number
            && repository_number > 0
            && static_cast<unsigned>(repository_number) <= menu.repositories.size())
        {
            return true;
        }

        std::cout << "invalid selection\n";
        return false;
    }
};

// *********************************************************

bool main_menu::repository_compare(std::string left, std::string right)
{
    crop_name(left);
    crop_name(right);

    const bool dash_found_left  =(left.find('/') != std::string::npos);
    const bool dash_found_right =(right.find('/') != std::string::npos);

    if (dash_found_left != dash_found_right)
        return dash_found_right;

    return left < right;
}

void main_menu::share_local_repository(const std::string &user)
{
    // Ask name

    std::string new_name =read_field("repository name: ", false, accept_repository_name());

    filter_new_repository_name(new_name);

    if (new_name.empty())
        return;

    const std::string new_path =config::base_path + '/' + new_name;

    if (access(new_path.c_str(), F_OK) == 0) {
        std::cout << "repository named " << new_name << " seems to exists; aborting\n";

        read_field("\n(press enter)", true, accept_enter());
        return;
    }

    // "Are you sure?"

    {
        std::ostringstream question_oss;
        question_oss << "\n"
            "init a new repository '" << polish_name(new_name) << "' [yes/no] ? ";

        const std::string yes_or_no =read_field(question_oss.str(), false, accept_yes_or_no());

        std::cout << '\n';

        if (yes_or_no != "yes")
            return;
    }

    // get to work

    std::ostringstream command_oss;
    escape_bash escape{command_oss};

    command_oss << "mkdir -p ";
    escape << new_path;
    command_oss << " && git --git-dir=";
    escape << new_path;
    command_oss << " init";

    system(command_oss.str().c_str());

    // create cgitrc

    cgitrc rc =cgitrc::new_instance(user, cgitrc::repo_type::shared);
    rc.export_to_file(new_path + "/cgitrc");

    // instructions

    if (!config::clone_url_base.empty()) {
        escape_bash escape(std::cout);

        std::cout << "\n"
            "To make this repository your upstream:\n"
            "    git remote add origin ";
        escape << config::clone_url_base << new_name;
        std::cout << "\n"
            "    git push -u origin master\n";
    }

    //
    read_field("\n(press enter)", true, accept_enter());
}

void main_menu::mirror_remote_repository(const std::string &user)
{
    // Ask name

    std::string new_name =read_field("repository name (in here): ", false, accept_repository_name());

    filter_new_repository_name(new_name);

    if (new_name.empty())
        return;

    const std::string new_path =config::base_path + '/' + new_name;

    if (access(new_path.c_str(), F_OK) == 0) {
        std::cout << "repository named " << new_name << " seems to exists; aborting\n";

        read_field("\n(press enter)", true, accept_enter());
        return;
    }

    // Ask URL

    std::string url =read_field("origin url: ", false, accept_url());

    if (url.empty())
        return;

    // "Are you sure?"

    {
        std::ostringstream question_oss;
        question_oss << "\n"
            "start mirroring a repository from " << url << " ...\n"
            "... and name it as \"" << polish_name(new_name) << "\"  [yes/no] ? ";

        const std::string yes_or_no =read_field(question_oss.str(), false, accept_yes_or_no());

        if (yes_or_no != "yes") {
            std::cout << '\n';
            return;
        }
    }

    // get to work

    std::ostringstream command_oss;
    escape_bash escape{command_oss};

    command_oss << "git clone --mirror ";
    escape << url;
    command_oss << ' ';
    escape << new_path;

    system(command_oss.str().c_str());

    // create cgitrc

    cgitrc rc =cgitrc::new_instance(user, cgitrc::repo_type::mirrored);
    rc.export_to_file(new_path + "/cgitrc");

    //
    read_field("\n(press enter)", true, accept_enter());
}

void main_menu::add_repository(const std::string &r)
{
    repositories.push_back(r);
}

main_menu::main_menu(const std::string &user)
{
    for_each_git_dir(scanner(*this, user));

    std::sort(repositories.begin(), repositories.end(), repository_compare);
}

main_menu::accept_functor main_menu::get_accept_functor() const
{
    return accept_functor{*this};
}

bool main_menu::run(const std::string &user)
{
    main_menu menu{user};

    std::cout << menu;

    const std::string selection =read_field(": ", false, menu.get_accept_functor());

    //

    if (selection == "s")
    {
        share_local_repository(user);
        return true;
    }
    else if (selection == "m")
    {
        mirror_remote_repository(user);
        return true;
    }
    else if (selection == "k")
    {
        while (key_menu::run(user))
            ;
    }
    else if (selection == "x")
    {
        return false;
    }
    else if (selection == "?")
    {
        std::cout << "\n"
            "Help text .... still working on it ...\n";

        //
        read_field("\n(press enter)", true, accept_enter());
    }
    else {
        std::istringstream iss{selection};

        int repository_number;

        if (iss >> repository_number
            && repository_number > 0
            && (unsigned)repository_number <= menu.repositories.size())
        {
            while (repository_menu::run(user, menu.repositories[repository_number-1]))
                ;
        }
    }

    return true;
}

// *********************************************************

std::ostream &operator<< (std::ostream &out, const main_menu &menu)
{
    const restore_ios rios{out};

    out << std::setfill(' ')
        << std::left;

    out << "\n"
        "+=============================================================================|\n"
        "|\n";

    if (!menu.repositories.empty())
    {
        std::string last_section;

        for (unsigned int i =0;
             i < menu.repositories.size();
             ++i)
        {
            std::string entry =menu.repositories[i];

            crop_name(entry);

            const std::string::size_type dash =entry.find('/');
            const std::string section = (dash == std::string::npos) ? "" : entry.substr(0, dash);

            if (last_section != section) {
                out << "|       " << section << "\n";
                last_section =section;
            }

            const std::string remaining = (dash == std::string::npos) ? entry : entry.substr(dash+1);

            std::ostringstream id_string;
            id_string << (i+1) << ')';

            out << "| "
                << std::setw(8) << id_string.str()
                << remaining
                << "\n";
        }
    }
    else {
        out << "|   (no repositories)\n";
    }

    out << "| \n"
        "+--->\n"
        "\n"
        "S) share your local repository\n"
        "M) mirror a remote repository\n"
        "\n"
        "K) SSH key menu\n"
        "\n"
        "X) Exit\n"
        "\n"
        "//?) Help\n"
        "\n";

    return out;
}
