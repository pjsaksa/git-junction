/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "ssh_key.hh"
#include "config.hh"
#include "exception.hh"
#include "process_io.hh"
#include "restore_ios.hh"

#include <sstream>
#include <iomanip>
#include <set>

#include <ctime>

#include <sys/stat.h>
#include <unistd.h>


#include <iostream> // debug


namespace
{
    typedef std::pair<std::string, int> fingerprint_and_size_t;

    static bool fingerprint_valid(const fingerprint_and_size_t &fp)
    {
        return fp.second <= config::key_max_size
            && fp.second >= config::key_min_size
            && ((fp.first.compare(0, 7, "SHA256:") == 0
                 && fp.first.size() == 50)
                || ((fp.first.compare(0, 4, "MD5:") == 0
                     && fp.first.size() == 51)));
    } 

    static fingerprint_and_size_t parse_fingerprint(std::istream &in)
    {
        // example input:
        // 2048 SHA256:KlCtE6AkJFbMBuEYsqxKYxxp4+O99seWfJLYUvbQolY pate@osaka (RSA)
        // 2048 MD5:6e:7c:19:86:95:eb:5e:95:54:28:be:87:77:27:b3:5e pate@osaka (RSA)

        int key_size;
        std::string fingerprint;

        if (in >> key_size >> fingerprint)
        {
            const fingerprint_and_size_t fp{fingerprint, key_size};

            if (fingerprint_valid(fp))
                return fp;
        }

        return fingerprint_and_size_t{};
    }

    static bool sanity_check_input_time(struct tm *tm)
    {
        return tm->tm_year >= 2016
            && tm->tm_year <= 2099
            && tm->tm_mon  >=  1
            && tm->tm_mon  <= 12
            && tm->tm_mday >=  0
            && tm->tm_mday <= 31
            && tm->tm_hour >=  0
            && tm->tm_hour <= 23
            && tm->tm_min  >=  0
            && tm->tm_min  <= 59
            && tm->tm_sec  >=  0
            && tm->tm_sec  <= 59;
    }

    static bool sanity_check_key_characters(const std::string &token)
    {
        for (auto ptr =token.begin();
             ptr != token.end();
             ++ptr)
        {
            switch (*ptr) {
            case '0' ... '9':
            case 'A' ... 'Z':
            case 'a' ... 'z':
            case '+':
            case '/':
            case '=':
                break;

            default:
                return false;
            }
        }

        return true;
    }
}

// *********************************************************

ssh_key::ssh_key()
    : index(-1),
      timestamp{},
      flags{}
{
}

ssh_key::ssh_key(const std::string &l,
                 const std::string &d)
    : index(-1),
      timestamp{time(0)},
      label{l},
      flags{},
      data{d}
{
}

ssh_key::~ssh_key()
{
}

void ssh_key::set_flag(flags_t flag)
{
    flags |= (1 << flag);
}

void ssh_key::clear_flag(flags_t flag)
{
    flags &= ~(1 << flag);
}

bool ssh_key::flag(flags_t flag) const
{
    return flags & (1 << flag);
}

std::string ssh_key::compile_filename() const
{
    if (index < 0)
        throw generic_exception{"ssh_key::compile_filename: key index not known"};

    struct tm tm;
    if (!localtime_r(&timestamp, &tm))
        throw stdlib_exception{"localtime_r", 0};

    // "01_cheesecake_2016-04-21_01-25-10.key"

    std::ostringstream oss;

    oss << std::setfill('0')
        //
        << std::setw(2) << index+1
        << '_' << label
        << '_' << (tm.tm_year+1900) << '-' << (tm.tm_mon+1) << '-' << tm.tm_mday
        << '_' << tm.tm_hour << '-' << tm.tm_min << '-' << tm.tm_sec
        << '.' << (flag(f_disabled) ? "disabled" : "key");

    return oss.str();
}

void ssh_key::update_index(int i)
{
    index =i;
}

void ssh_key::update_filename()
{
    if (original_filename.empty())
        return;

    const std::string new_filename =compile_filename();

    if (original_filename != new_filename)
    {
        std::cout << "renaming '" << original_filename << "' -> '" << new_filename << "'\n";

        if (rename((config::keys_dir + '/' + original_filename).c_str(),
                   (config::keys_dir + '/' + new_filename).c_str()) != 0)
        {
            throw stdlib_exception("rename", errno);
        }

        original_filename =new_filename;
    }
}

void ssh_key::remove()
{
    if (original_filename.empty())
        return;

    unlink((config::keys_dir + '/' + original_filename).c_str());
    original_filename.clear();

    //

    index =-1;
    timestamp =0;
    label.clear();
    flags =0;
    data.clear();
}

void ssh_key::print_to_menu(std::ostream &out) const
{
    struct tm tm;
    if (!localtime_r(&timestamp, &tm))
        throw stdlib_exception{"localtime_r", 0};

    const std::string filename =config::keys_dir + '/' + original_filename;

    {
        // check filename

        if (filename.empty())
            throw generic_exception{"print_to_menu: failing to conjure filename for ssh_key"};
        if (access(filename.c_str(), R_OK) != 0)
            throw generic_exception{"print_to_menu: failing to access ssh_key ("+filename+")"};

        struct stat st;

        if (stat(filename.c_str(), &st) != 0)
            throw generic_exception{"print_to_menu: failing to stat ssh_key"};
        if (!S_ISREG(st.st_mode))
            throw generic_exception{"print_to_menu: ssh_key doesn't seem to be a regular file"};
    }

    const fingerprint_and_size_t sha256_print =parse_fingerprint( process_io{"ssh-keygen -l -E sha256 -f " + filename}.read() );
    const fingerprint_and_size_t md5_print    =parse_fingerprint( process_io{"ssh-keygen -l -E md5 -f "    + filename}.read() );

    if (!sha256_print.second
        || !md5_print.second)
    {
        throw generic_exception{"print_to_menu: unable to parse ssh_key fingerprints"};
    }

    // header line

    const restore_ios rios(out);

    out << "|     ";

    if (!label.empty())
        out << label << " / ";

    out << tm.tm_mday << '.' << (tm.tm_mon+1) << '.' << (tm.tm_year+1900)
        << ' ' << tm.tm_hour << ':' << tm.tm_min << ':' << tm.tm_sec;

    if (flag(f_disabled))
        out << " / DISABLED";

    out << '\n';

    // fingerprints

    out << "| "
        << std::setfill(' ') << std::right
        << std::setw(2) << (index+1) << ") "
        << sha256_print.second << ' ' << sha256_print.first << '\n'

        << "|     "
        << md5_print.second << ' ' << md5_print.first << '\n';
}

void ssh_key::export_to_storage(const std::string &user)
{
    if (!original_filename.empty())
        return;
    original_filename =compile_filename();

    //

    std::ofstream ofs{config::keys_dir + '/' + original_filename};

    if (!ofs)
        throw generic_exception{"export_to_storage: can't open file"};

    if (!(ofs << "ssh-rsa " << data
          << ' ' << user << '@' << label << '/' << timestamp << '\n'))
    {
        throw generic_exception{"export_to_storage: can't write key details to file"};
    }
}

ssh_key &ssh_key::import_from_storage(const std::string &user,
                                      const std::string &storage_filename)
{
    const std::string filename =config::keys_dir + '/' + storage_filename;

    std::ifstream ifs{filename};

    if (!ifs)
        throw generic_exception{"import_from_storage: can't open file"};

    //

    ssh_key &key =*new ssh_key();

    // parsing filename

    char delim;
    int index;
    std::string extension;
    struct tm tm;

    std::istringstream iss{storage_filename};

    if (!(iss >> index >> delim
          && delim == '_'
          && getline(iss, key.label, '_')
          && iss >> tm.tm_year >> delim && delim == '-'
          && iss >> tm.tm_mon  >> delim && delim == '-'
          && iss >> tm.tm_mday >> delim && delim == '_'
          && iss >> tm.tm_hour >> delim && delim == '-'
          && iss >> tm.tm_min  >> delim && delim == '-'
          && iss >> tm.tm_sec  >> delim && delim == '.'
          && sanity_check_input_time(&tm)
          && iss >> extension
          && (extension == "key"
              || extension == "disabled")))
    {
        throw generic_exception{"import_from_storage: parsing filename failed"};
    }

    // timestamp

    tm.tm_year -=1900;
    --tm.tm_mon;
    tm.tm_isdst =-1;

    key.timestamp =mktime(&tm);

    if (key.timestamp < 0)
        throw generic_exception{"import_from_storage: parsing timestamp from filename failed"};

    // flags

    if (extension == "disabled")
        key.set_flag(f_disabled);

    // parsing file content

    std::string algo;
    std::string comment;

    if (!(ifs >> algo >> key.data
          && getline(ifs, comment)
          && algo == "ssh-rsa"
          && config::key_data_sizes.find(key.data.size()) != config::key_data_sizes.end()
          && sanity_check_key_characters(key.data)))
    {
        throw generic_exception{"import_from_storage: failing to import key details"};
    }

    std::istringstream iss_comment{comment};

    std::string verify_user;
    std::string verify_label;
    time_t verify_timestamp;

    if (!(iss_comment >> std::ws
          && getline(iss_comment, verify_user, '@')
          && getline(iss_comment, verify_label, '/')
          && iss_comment >> verify_timestamp
          && (iss_comment.tellg() == -1
              || iss_comment.tellg() == static_cast<ssize_t>(comment.size()))))
    {
        throw generic_exception{"import_from_storage: failing to parse comment"};
    }

    if (!(user == verify_user
          && key.label == verify_label
          && key.timestamp == verify_timestamp))
    {
        throw generic_exception{"import_from_storage: failing to verify SSH-key details"};
    }

    key.original_filename =storage_filename;

    return key;
}
