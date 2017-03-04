/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "key_menu.hh"
#include "config.hh"
#include "input.hh"
#include "utils.hh"
#include "restore_ios.hh"
#include "exception.hh"
#include "process_io.hh"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

#include <cctype>

#include <unistd.h>

//

namespace
{
    class accept_label : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
            if (input.empty())
                return true;

            if (!accept_size(input.size(), 0, config::key_label_max_size))
                return false;

            for (auto ptr =input.begin();
                 ptr != input.end();
                 ++ptr)
            {
                switch (*ptr) {
                case '0' ... '9':
                case 'A' ... 'Z':
                case 'a' ... 'z':
                case '-':
                    break;

                default:
                    std::cout << "invalid character(s), only a-z, A-Z, 0-9 and '-' allowed\n";
                    return false;
                }
            }

            return true;
        }
    };

    // *****

    class accept_key_data : public accept_field_functor {
    public:
        virtual bool operator() (std::string &input) const
        {
        start_over:
            if (input.empty())
                return true;

            // skip whitespace

            {
                auto skip_ws =input.begin();

                while (skip_ws != input.end()
                       && isspace(*skip_ws))
                {
                    ++skip_ws;
                }

                const unsigned int chars =std::distance(input.begin(), skip_ws);

                if (chars) {
                    input.erase(0, chars);
                    goto start_over;
                }
            }

            // skip key type

            if (input.compare(0, 7, "ssh-rsa") == 0) {
                input.erase(0, 7);
                goto start_over;
            }

            // done skipping

            std::string key_data;

            {
                std::istringstream iss(input);

                if (!(iss >> key_data
                      && config::key_data_sizes.find(key_data.size()) != config::key_data_sizes.end()))
                {
                    std::cout << "invalid key length\n";
                    return false;
                }

                for (auto ptr =key_data.begin();
                     ptr != key_data.end();
                     ++ptr)
                {
                    switch (*ptr) {
                    case '0' ... '9':   // base64 characters
                    case 'A' ... 'Z':
                    case 'a' ... 'z':
                    case '/':
                    case '+':
                    case '=':
                        break;

                    default:
                        std::cout << "invalid key content\n";
                        return false;
                    }
                }
            }

            input =key_data;
            return true;
        }
    };
}

// *********************************************************

class key_menu::accept_menu_command : public accept_field_functor {
    const key_menu &menu;
public:
    accept_menu_command(const key_menu &m)
        : menu{m} {}
    virtual bool operator() (std::string &input) const
    {
        lowercase(input);

        if (input == "r")
        {
            if (!menu.keys.empty())
                return true;

            std::cout << "no keys to remove\n";
            return false;
        }

        return input == "n"
            || input == "x"
            || input == "?";
    }
};

// *********************************************************

class key_menu::accept_key_index : public accept_field_functor {
    const key_menu &menu;
public:
    accept_key_index(const key_menu &m)
        : menu{m} {}
    virtual bool operator() (std::string &input) const
    {
        int key_index;

        return std::istringstream{input} >> key_index
            && key_index > 0
            && key_index <= static_cast<int>(menu.keys.size());
    }
};

// *********************************************************

key_menu::keys_t key_menu::load_keys(const std::string &user)
{
    typedef std::vector<std::string> filenames_t;
    //

    filenames_t filenames;

    {
        opendir_raii dir{config::keys_dir};
        struct dirent *dirent;

        while ((dirent =dir.readdir()))
        {
            if (dirent->d_name[0] == '.')
                continue;

            filenames.push_back(dirent->d_name);
        }
    }

    std::sort(filenames.begin(),
              filenames.end());

    keys_t keys;

    for (const std::string &fn : filenames) {
        ssh_key &key =ssh_key::import_from_storage(user, fn);
        keys.push_back(&key);
    }

    update_key_indices(keys);

    return keys;
}

void key_menu::update_key_indices(keys_t &keys)
{
    for (unsigned int key_index =0;
         key_index < keys.size();
         ++key_index)
    {
        keys[key_index]->update_index(key_index);
    }
}

void key_menu::install_keys(const std::string &user)
{
    std::ostringstream command_oss;
    escape_bash escape{command_oss};

    command_oss << "sudo -u git /home/git/bin/install-keys ";   //TODO: add this to config
    escape << user;
    command_oss << ' ';
    escape << getpid();

    system(command_oss.str().c_str());
}

key_menu::key_menu(const std::string &u)
    : user{u},
      keys{load_keys(user)}
{
}

void key_menu::ask_for_new_key()
{
    const std::string label =read_field("key label (0-30 characters): ", false, accept_label());
    const std::string data  =read_field("public key data (base64 encoded): ", false, accept_key_data());

    if (data.empty())
        return;

    ssh_key &key =*new ssh_key(label, data);

    const unsigned int key_index =keys.size();
    keys.push_back(&key);

    key.update_index(key_index);
    key.export_to_storage(user);
}

void key_menu::remove_key()
{
    const std::string index_string =read_field("key index: ", false, accept_key_index(*this));

    int ui_index;

    if (std::istringstream{index_string} >> ui_index
        && ui_index > 0
        && ui_index <= static_cast<int>(keys.size()))
    {
        const unsigned int real_index =ui_index - 1;

        ssh_key &old_key =*keys[real_index];

        old_key.remove();

        delete &old_key;
        keys.erase(keys.begin() + real_index);

        update_key_indices(keys);
    }
}

key_menu::~key_menu()
{
    try {
        for (ssh_key *key : keys)
            key->update_filename();
    }
    catch (stdlib_exception &e) {
        std::cerr << "file rename error (stdlib): " << e << "\n";
    }

    //

    delete_vector(keys);

    //

    try {
        install_keys(user);
    }
    catch (...) {
        std::cerr << "install_keys() exception\n";
    }
}

bool key_menu::run(const std::string &user)
{
    key_menu menu{user};

    std::cout << menu;

    const std::string selection =read_field(": ", false, accept_menu_command(menu));

    if (selection == "n")
    {
        menu.ask_for_new_key();
        return true;
    }
    else if (selection == "r")
    {
        menu.remove_key();
        return true;
    }
    else if (selection == "x")
    {
        return false;
    }

    return true;
}

// *********************************************************

std::ostream &operator<< (std::ostream &out, const key_menu &menu)
{
    out << "\n"
        "+-----------------------------------------------------------------------------|\n"
        "|\n";

    if (!menu.keys.empty())
    {
        for (const ssh_key *key : menu.keys)
        {
            key->print_to_menu(out);
            out << "|\n";
        }
    }
    else {
        out << "|  (no keys)\n"
            << "|\n";
    }

    out << "+--->\n"
        "\n"

        // print buttons

        "N) import new SSH key\n"
        "R) remove SSH key permanently\n"
        "\n"
        "X) back\n"
        "\n"
        "//?) Help\n"
        "\n";

    return out;
}
