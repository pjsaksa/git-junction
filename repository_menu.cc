/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "repository_menu.hh"
#include "input.hh"
#include "process_io.hh"
#include "utils.hh"
#include "exception.hh"
#include "restore_ios.hh"

#include <iostream>
#include <sstream>
#include <iomanip>

//

namespace
{
    class accept_description : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
            return accept_size(input.size(), 0, config::description_max_size);
        }
    };
}

// *********************************************************

class repository_menu::accept_functor : public accept_field_functor {
    const repository_menu &menu;
public:
    accept_functor(const repository_menu &m)
        : menu{m} {}
    virtual bool operator() (std::string &input) const
    {
        lowercase(input);

        if (input == "n"
            || input == "d"
            || input == "x"
            || input == "?")
        {
            return true;
        }

        switch (menu.rc.get_type()) {
        case cgitrc::repo_type::shared:
            if (input == "p")
                return true;
            break;

        default:
            break;
        }

        if (input.size() == 1 + safe_width)
        {
            std::istringstream iss{input};
            char sel;
            unsigned int safe;

            if (iss >> sel >> safe
                && ((sel == 'a'
                     && safe == menu.abandon_safe)
                    || (sel == 'r'
                        && safe == menu.remove_safe)))
            {
                return true;
            }
        }

        std::cout << "invalid selection\n";
        return false;
    }
};

// *********************************************************

bool repository_menu::read_publicity(const std::string &path)
{
    std::ostringstream command_oss;
    escape_bash escape{command_oss};

    command_oss << "git --git-dir=";
    escape << path;
    command_oss << " config daemon.receivepack";

    process_io git_config(command_oss.str());
    std::string line;

    return git_config.read() >> line
        && line == "true";
}

repository_menu::repository_menu(const std::string &user, const std::string &p)
    : path{p},
      rc{cgitrc::import_from_file(path + "/cgitrc")},
      publicity{rc.get_type() == cgitrc::repo_type::shared ? read_publicity(path) : false},
      abandon_safe{static_cast<unsigned int>(random() % safe_max)},
      remove_safe{static_cast<unsigned int>(random() % safe_max)}
{
    if (rc.get_owner() != user)
        throw import_exception{};
}

repository_menu::accept_functor repository_menu::get_accept_functor() const
{
    return accept_functor{*this};
}

void repository_menu::change_description()
{
    cgitrc new_rc =rc;

    new_rc.set_desc(read_field("new description: ", false, accept_description()));
    new_rc.export_to_file(path + "/cgitrc");
}

void repository_menu::toggle_publicity()
{
    std::ostringstream command_oss;

    if (publicity)
        command_oss << "git '--git-dir=" << path << "' config --unset daemon.receivepack";
    else
        command_oss << "git '--git-dir=" << path << "' config daemon.receivepack true";

    system(command_oss.str().c_str());
}

bool repository_menu::run(const std::string &user, const std::string &path)
{
    try {
        repository_menu menu{user, path};

        std::cout << menu;

        const std::string selection =read_field(": ", false, menu.get_accept_functor());

        if (selection == "d")
        {
            menu.change_description();
            return true;
        }
        else if (selection == "x")
        {
            return false;
        }

        switch (menu.rc.get_type()) {
        case cgitrc::repo_type::shared:
            if (selection == "p")
            {
                menu.toggle_publicity();
                return true;
            }
            break;

        default:
            break;
        }

        return true;
    }
    catch (import_exception) {
        return false;
    }
}

// *********************************************************

std::ostream &operator<< (std::ostream &out, const repository_menu &menu)
{
    using repo_type =cgitrc::repo_type;
    //

    const restore_ios rios{out};

    out << std::setfill('0');

    // print info screen

    out << "\n"
        "+-----------------------------------------------------------------------------|\n"
        "|\n"
        "|    " << polish_name(menu.path) << "\n"
        "|\n"
        "| description: " << menu.rc.get_desc() << "\n";

    switch (menu.rc.get_type()) {
    case repo_type::shared:
        out << "| publicity:   " << (menu.publicity?"public":"private") << "\n";
        break;

    case repo_type::mirrored:
        {
            std::ostringstream command_oss;
            escape_bash escape{command_oss};

            command_oss << "git --git-dir=";
            escape << menu.path;
            command_oss << " config remote.origin.url";

            //

            out << "| origin: " << std::flush;

            process_io origin_process(command_oss.str().c_str());
            std::string origin;

            if (getline(origin_process.read(), origin))
                out << origin << '\n';
            else
                out << "(unknown)\n";
        }
        break;

    case repo_type::unknown:
        break;
    }

    out << "|\n"
        "+--->\n"
        "\n"

    // print buttons

        "//N) rename / move repository\n"
        "D) change description\n";

    switch (menu.rc.get_type()) {
    case repo_type::shared:
        out << "P) toggle publicity\n";
        break;

    case repo_type::mirrored:
        break;

    case repo_type::unknown:
        break;
    }

    out << "\n"
        "//A" << std::setw(repository_menu::safe_width) << menu.abandon_safe << ") abandon the repository\n"
        "//R" << std::setw(repository_menu::safe_width) << menu.remove_safe << ") remove the repository permanently\n"
        "\n"
        "X) back\n"
        "\n"
        "//?) Help\n"
        "\n";

    return out;
}
