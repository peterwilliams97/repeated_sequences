#include <fstream>
#include <iostream>
#include <limits>

#include "utils.h"
#include "timer.h"
#include "inverted_index.h"

using namespace std;

static
double
test_inverted_index(const vector<string>& path_list, int n_bad_allowed) {

    reset_elapsed_time();

    vector<RequiredRepeats> required_repeats_list = get_required_repeats(path_list);
    InvertedIndex *inverted_index = create_inverted_index(required_repeats_list, n_bad_allowed);
    show_inverted_index("initial", inverted_index);

    RepeatsResults repeats_results = get_all_repeats(inverted_index);

    bool converged = repeats_results._converged;
    vector<Term> exacts = repeats_results._exact;
    vector<Term> repeats = repeats_results._longest;

    cout << "--------------------------------------------------------------------------" << endl;
    cout << "converged = " << converged << ", repeats = " << repeats.size() << ", exacts = " << exacts.size() << endl;
    cout << "--------------------------------------------------------------------------" << endl;
    if (repeats.size() > 0) {
        cout << "Found " << repeats.size() << " repeated strings";
        if (repeats.size() > 0) {
            cout << " of length " << repeats.front().size();
        }
        cout << endl;
        print_term_vector("Repeated strings", repeats);
        for (int i = 0; i < (int)repeats.size(); i++) {
            cout << i << " : ";
            show_bytes(repeats[i]);
            cout << endl;
        }
    }

    cout << "--------------------------------------------------------------------------" << endl;
    if (exacts.size() > 0) {
        cout << "Found " << exacts.size() << " exactly repeated strings";
        if (exacts.size() > 0) {
            cout << " of length " << dec << exacts.front().size();
        }
        cout << endl;
        print_term_vector("Exactly repeated strings", exacts);
        for (int i = 0; i < (int)exacts.size(); i++) {
            cout << i << " : ";
            show_bytes(exacts[i]);
            cout << endl;
        }
    }

    delete_inverted_index(inverted_index);

    double duration = get_elapsed_time();
    cout << "duration = " << duration << endl;
    return duration;
}

void
show_stats(const vector<double>& d) {

    double min_d = numeric_limits<double>::max();
    double max_d = numeric_limits<double>::min();
    double total = 0.0;
    for (vector<double>::const_iterator it = d.begin(); it != d.end(); ++it) {
        min_d = min(min_d, *it);
        max_d = max(max_d, *it);
        total += *it;
    }
    size_t n = d.size();
    double ave = total / (double)n;
    double med = d[n / 2];
    cout << "min="<< min_d << ", max="<< max_d << ", ave=" << ave << ", med=" << med << endl;
}

void
multi_test(const string& path_list_path, int n, int n_bad_allowed) {
    vector<string> path_list = read_path_list(path_list_path);
    vector<double> durations;
    for (int i = 0; i < n; i++) {
        cout << "========================== test " << i << " of " << n << " ==============================" << endl;
        durations.push_back(test_inverted_index(path_list, n_bad_allowed));
        show_stats(durations);
    }
}

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " path_list_path" << endl;
        return 1;
    }

    string path_list_path(argv[1]);
    vector<string> path_list = read_path_list(path_list_path);
    if (path_list.size() == 0) {
        cerr << "No path_list in " << path_list_path << endl;
        return 1;
    }

    test_inverted_index(path_list, 1);
    return 0;
}
