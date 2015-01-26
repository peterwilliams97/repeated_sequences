#include <fstream>
#include <iostream>
#include <iomanip>
#include <assert.h>
#include <sys/stat.h>
#include <regex>
#include "mytypes.h"
#include "utils.h"

using namespace std;

int
string_to_int(const string& s) {
    int x;
    return from_string(s, x);
}

size_t
get_file_size(const string& path) {
    struct stat filestatus;
    int ret = stat(path.c_str(), &filestatus);
    if (ret != 0) {
        cerr << "Can't stat '" << path << ", errno=" << errno << endl;
    }
    assert(ret == 0);
    return filestatus.st_size;
}

byte *
read_file(const string& path) {
    size_t size = get_file_size(path);
    byte *data = new byte[size];
    if (data == NULL) {
        cerr << "could not allocate " << size << " bytes" << endl;
        return 0;
    }

    ifstream f;
    f.open(path, ios::in | ios::binary);
    if (!f.is_open()) {
        cerr << "could not open " << path << endl;
        delete[] data;
        return 0;
    }

    f.read((char *)data, size);

    if (f.gcount() < (streamsize)size) {
        cerr << "could not read " << path << endl;
        delete[] data;
        data = NULL;
    }

    f.close();
    return data;
}

void
show_bytes(const Term& term) {
    cout << "[";
    for (Term::const_iterator it = term.begin(); it != term.end(); ++it) {
        cout << "0x" << hex << setfill('0') << setw(2) << (int)(byte)*it << ", ";
    }
    cout << "]";
    cout << dec;
}

void
print_term_vector(const string& name, const vector<Term>& lst_in, size_t n) {
    vector<Term>& lst = vector<Term>(lst_in.begin(), lst_in.end());
    sort(lst.begin(), lst.end());

    cout << name << ": " << lst.size() << " [";
    vector<Term>::const_iterator end = lst.begin() + min(n, lst.size());
    for (vector<Term>::const_iterator it = lst.begin(); it != end; ++it) {
        cout << "\"" << term_to_string(*it) << "\" (";
        const Term& term = *it;
        for (Term::const_iterator jt = term.begin(); jt != term.end(); ++jt) {
            cout << "0x" << hex << (int)(byte)*jt << ", ";
        }
         cout << "), ";
    }
    cout << "] " << lst.size() <<  endl;
    cout << dec;
}

/*
 * A liine in paths file contains a code and or a comment
 */
struct CodeComment {
    string _code;
    string _comment;

    CodeComment(const string& line) {
        stringstream ss(line);
        string item;
        if (getline(ss, item, '#')) {
            _code = trim(item);
        }
        if (getline(ss, item)) {
            _comment = trim(item);
        }
    }
};

#if 0
CodeComment
get_code_comment(const string& line) {
    stringstream ss(line);
    string item;
    CodeComment code_comment;
    if (getline(ss, item, '#')) {
        code_comment._code = trim(item);
    }
    if (getline(ss, item)) {
        code_comment._comment = trim(item);
    }
    return code_comment;
}
#endif

/*
 *  Params:
 *      path_list_path: Text file containing one path per line
 *  Returns:
 *      Vector of paths
 */
vector<string>
read_path_list(const string& path_list_path) {
    ifstream f(path_list_path);
    if (!f.is_open()) {
        cerr << "Unable to open '" << path_list_path << "'" << endl;
        return vector<string>();
    }

    vector<string> path_list;
    while (f.good()) {
        string line;
        getline(f, line);
        CodeComment code_comment(line);
        if (code_comment._code.size()) {
            path_list.push_back(code_comment._code);
        }
        if (code_comment._comment.size()) {
            cout << "# " << code_comment._comment << endl;
        }
    }
    f.close();

    return path_list;
}

#if 0
/*
 * A RequiredRepeats
 *  - describes a document and
 *  - specifies the number of times a term (substring) must occur in the document
 */
struct RequiredRepeats {
    std::string _doc_name;      // Document name
    size_t _size;               // Size of document in bytes

    unsigned int _num;          // Number of times term in is repeated in document

    // Average size of each repeat
    // This is an indicator of how much time it will take to search for a repeat
    // in a document, all other things being equal.
    double repeat_size() const { return (double)_size / (double)_num; }

    RequiredRepeats() : _doc_name(""), _num(0), _size(0) {}

    RequiredRepeats(const std::string doc_name, unsigned int num, size_t size) :
        _doc_name(doc_name), _num(num), _size(size) {}
};
#endif

// How number of repeats is encoded in document names
static const string PATTERN_REPEATS = "pages=?(\\d+)";
//static const string PATTERN_REPEATS = "(\\d+)page";

/*
 * Comparison function to sort documents in order of average size of repeat
 */
static
bool
comp_reqrep(const RequiredRepeats& rr1, const RequiredRepeats& rr2) {
     return rr1.repeat_size() < rr2.repeat_size();
}

/*
 * Given a vector of path_list with PATTERN_REPEATS name encoding, return the
 *  corresponding vector of RequiredRepeats
 * Vector is sorted in order of increasing repeat size as smaller repeat
 *  sizes are more selective
 */
vector<RequiredRepeats>
get_required_repeats(const vector<string>& path_list) {

#if VERBOSITY >= 1
    cout << "get_required_repeats: " << path_list.size() << " files" << endl;
#endif
    vector<RequiredRepeats> required_repeats;
    regex re_repeats(PATTERN_REPEATS);

    for (vector<string>::const_iterator it = path_list.begin(); it < path_list.end(); ++it) {
        const string& path = *it;
        cmatch matches;
        if (regex_search(path.c_str(), matches, re_repeats)) {
            required_repeats.push_back(RequiredRepeats(path, string_to_int(matches[1]), get_file_size(path)));
        } else {
            cerr << "path='" << path << "' does not match pattern " << PATTERN_REPEATS << endl;
        }
    }

    sort(required_repeats.begin(), required_repeats.end(), comp_reqrep);
#if VERBOSITY >= 1
    for (vector<RequiredRepeats>::const_iterator it = required_repeats.begin(); it < required_repeats.end(); ++it) {
        cout << it - required_repeats.begin() << ": " << it->_doc_name << ", " << it->_num << ", " << it->_size << endl;
    }
#endif
    return required_repeats;
}
