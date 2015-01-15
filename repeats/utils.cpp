#include <fstream>
#include <iostream>
#include <iomanip>
#include <assert.h>
#include <sys/stat.h>
#include "mytypes.h"
#include "utils.h"

using namespace std;

int
string_to_int(const string s) {
    int x;
    return from_string(s, x);
}

size_t
get_file_size(const string filename) {
    struct stat filestatus;
    int ret = stat(filename.c_str(), &filestatus);
    assert(ret == 0);
    return filestatus.st_size;
}

byte *
read_file(const string filename) {
    size_t size = get_file_size(filename);
    byte *data = new byte[size];
    if (data == NULL) {
        cerr << "could not allocate " << size << " bytes" << endl;
        return 0;
    }

    ifstream f;
    f.open(filename, ios::in | ios::binary);
    if (!f.is_open()) {
        cerr << "could not open " << filename << endl;
        delete[] data;
        return 0;
    }

    f.read((char *)data, size);

    if (f.gcount() < (streamsize)size) {
        cerr << "could not read " << filename << endl;
        delete[] data;
        data = NULL;
    }

    f.close();
    return data;
}

void
show_bytes(const string str) {
    cout << "[";
    for (string::const_iterator s = str.begin(); s != str.end(); s++) {
        cout << "0x" << hex << setfill('0') << setw(2) << (int)(byte)*s << ", ";
    }
    cout << "]";
    cout << dec;
}

void
print_term_vector(const std::string& name, const std::vector<Term>& lst, size_t n) {
    cout << name << ": " << lst.size() << " [";
    std::vector<Term>::const_iterator end = lst.begin() + min(n, lst.size());
    for (std::vector<Term>::const_iterator it = lst.begin(); it != end; ++it) {
        cout << "\"" << *it << "\" (";
        const Term& term = *it;
        for (Term::const_iterator jt = term.begin(); jt != term.end(); ++jt) {
            cout << "0x" << hex << (int)(unsigned char)*jt << ", ";
        }
         cout << "), ";
    }
    cout << "] " << lst.size() << std:: endl;
    cout << dec;
}