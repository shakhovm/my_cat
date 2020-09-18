#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <vector>
#include <algorithm>
namespace po = boost::program_options;

int readbuffer(int fd, ssize_t size, char* buf, ssize_t& total_read) {

    ssize_t x = 0;
    while (x < size) {
        ssize_t add_size = read(fd, buf + x, size - x);

        if (add_size == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        else if (add_size == 0) {
            break;
        }
        x += add_size;
    }
    total_read = x;
    return 0;
}

int writebuf(int fd, const char* buf, ssize_t s) {
    ssize_t add_b = 0;
    while (add_b < s) {
        ssize_t now = write(fd, buf + add_b, s - add_b);
        if (now == -1) {
            if (errno != EINTR)
            {
                return -1;
            }

        }
        else {
            add_b += now;
        }
    }
    return 0;
}

inline void close_all_files(const std::vector<int>& fds) {
    std::for_each(fds.begin(), fds.end(), close);
}


typedef struct stat stat_t;
constexpr ssize_t buf_size = 512;

int simple_cat(const std::vector<int>& fds) {
    char buf[buf_size];
    for (const auto& fd: fds) {
        ssize_t read_size = buf_size;
        while (read_size >= buf_size)
        {
            if (readbuffer(fd, sizeof(buf), buf, read_size) < 0) {
                std::string error_message("mycat: failed to read\n");
                writebuf(2, error_message.c_str(), error_message.size());
                close_all_files(fds);
                return 1;
            }
            if (read_size == 0)
                break;

            if (writebuf(STDOUT_FILENO, buf, read_size) < 0) {
                std::string error_message("mycat: failed to wright\n");
                writebuf(2, error_message.c_str(), error_message.size());
                close_all_files(fds);
                return 5;
            }
        }
        close(fd);
    }
    return 0;
}

int hex_cat(const std::vector<int>& fds) {
    char buf[buf_size];
    char const hex_chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    for (const auto& fd: fds) {
        ssize_t read_size = buf_size;
        while (read_size >= buf_size)
        {
            if (readbuffer(fd, sizeof(buf), buf, read_size) < 0) {
                std::string error_message("mycat: failed to read\n");
                writebuf(2, error_message.c_str(), error_message.size());
                close_all_files(fds);
                return 1;
            }
            if (read_size == 0)
                break;
            std::string s;
            for (size_t i = 0; i < read_size; ++i) {
                if (isspace(buf[i]) || isprint(buf[i])) {
                    s += buf[i];
                }
                else {
                    s.append("\\x");
                    s.push_back(hex_chars[ ( buf[i] & 0xF0 ) >> 4 ]);
                    s.push_back(hex_chars[ ( buf[i] & 0xF ) >> 0 ]);
                }
            }
            if (writebuf(STDOUT_FILENO, s.c_str(), s.size()) < 0) {
                std::string error_message("mycat: failed to wright\n");
                writebuf(2, error_message.c_str(), error_message.size());
                close_all_files(fds);
                return 3;
            }
        }
        close(fd);
    }
    return 0;
}



int main(int argc, char** argv)
{
    po::options_description hidden("Allowed options");
    hidden.add_options()
        ("input-file", po::value< std::vector<std::string> >(), "input file")
        ("show-all,A", "hex code")
        ("help,h", "Show help")
        ;
    po::options_description cmdline_options;
    cmdline_options.add(hidden);
    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(hidden).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << hidden << "\n";
        return 1;
    }
    if (!vm.count("input-file"))
    {
        char error_message[] = "mycat: no input files!\n";
        writebuf(2, error_message, sizeof(error_message));
        return 3;
    }
    std::vector<std::string> v(vm["input-file"].as<std::vector<std::string>>());
    std::vector<int> fds;
    int fd_file;
    for (const auto& file_name: v) {
        if ((fd_file = open(file_name.c_str(), O_RDONLY)) < 0) {
            std::string error_message("mycat: failed to open ");
            error_message.append(file_name + '\n');
            writebuf(2, error_message.c_str(), error_message.size());
            close_all_files(fds);
            return 1;
        }
        stat_t st;
        stat(file_name.c_str(), &st);
        if (S_ISDIR(st.st_mode)) {
            std::string error_message("mycat: ");
            error_message.append(file_name);
            error_message.append(" is directory!\n");
            writebuf(2, error_message.c_str(), error_message.size());
            close_all_files(fds);
            return 2;
        }

        fds.push_back(fd_file);
    }
    if (vm.count("show-all")) {
      return hex_cat(fds);
    }
    return simple_cat(fds);
}
