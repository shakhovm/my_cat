#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <vector>
namespace po = boost::program_options;
using namespace std;

int readbuffer(int fd, ssize_t size, char* buf, size_t& total_read) {

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

constexpr size_t buf_size = 512;
typedef struct stat stat_t;

int main(int argc, char** argv)
{
    char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    // Declare the supported options.
    po::options_description hidden("Allowed options");
    hidden.add_options()
        ("input-file", po::value< vector<string> >(), "input file")
        ("show-all,A", "hex code")
        ;
    po::options_description cmdline_options;
    cmdline_options.add(hidden);
    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(hidden).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("input-file"))
    {
//   //     cout << "Input files are: ";
        std::vector<string> v(vm["input-file"].as< vector<string> >());
//        for_each(v.begin(), v.end(), [](string& s){cout << s << " "; });
        std::vector<int> fds;
        int fd_file;
        for (const auto& file_name: v) {
            if ((fd_file = open(file_name.c_str(), O_RDONLY)) < 0) {

                return 1;
            }
            stat_t st;
            stat(file_name.c_str(), &st);
            if (S_ISDIR(st.st_mode)) {
                return 2;
            }

            fds.push_back(fd_file);
        }
        char buf[buf_size];
        for (const auto& fd: fds) {
            size_t x = -1;
            while (x >= buf_size)
            {
                readbuffer(fd, sizeof buf, buf, x);
                if (x == 0)
                    break;
                if (vm.count("show-all")) {

                    std::stringstream s;
                    std::string string;
                    for (size_t i = 0; i < x; ++i) {

                        if (isspace(buf[i]) || isprint(buf[i])) {
                            string += buf[i];
                        }

                        else {
                            string += "\\x";
                            string += hex_chars[ ( buf[i] & 0xF0 ) >> 4 ];
                            string += hex_chars[ ( buf[i] & 0xF ) >> 0 ];

                        }
                    }

                    writebuf(STDOUT_FILENO, string.c_str(), string.size());
                    continue;
                }

                writebuf(STDOUT_FILENO, buf, x);

            }
            close(fd);
        }


    }
    if (vm.count("show-all"))
    {

    }

}
