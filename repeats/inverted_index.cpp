/*
 * Use an inverted index to find the longest substring(s) that is repeated
 *  a specified number of times in a corpus of documents.
 *
 * Documentation in https://github.com/peterwilliams97/repeated_sequences
 */

#include <assert.h>
#include <iostream>
#include "mytypes.h"
#include "utils.h"
#include "timer.h"
#include "inverted_index.h"
#include "inverted_index_int.h"

using namespace std;

/*
 *  Params:
 *      offsets_map: {doc index: offsets in doc}
 *  Returns:
 *      vector of #offsets in each doc
 */
vector<int>
get_counts_per_doc(const map<int, vector<offset_t>>& offsets_map) {
    vector<int> counts;
    for (map<int, vector<offset_t>>::const_iterator it = offsets_map.begin(); it != offsets_map.end(); ++it) {
        counts.push_back((int)it->second.size());
    }
    return counts;
}

// Number of bytes to ignore at start of all files
// !@#$% Should be a tunable param
#define HEADER_SIZE 484
//#define HEADER_SIZE 0

/*
 * Read file named `path` into a map of {byte: all offsets of byte in a document}
 *  and return the map
 * Only read in offsets of bytes in allowed_bytes that occur >= min_repeats
 *  times
 */
static
map<byte, vector<offset_t>>
get_doc_offsets_map(const string& path, set<byte>& allowed_bytes, int min_repeats) {

    int counts[ALPHABET_SIZE] = {0};

    size_t length = get_file_size(path);
    byte *in_data = read_file(path);
    byte *end = in_data + length;

    byte *data = in_data + HEADER_SIZE;

    // Pass through the document once to get counts of all bytes
    for (byte *p = data; p < end; p++) {
        counts[*p]++;
    }

    // valid_bytes are those with sufficient counts
    set<byte> valid_bytes;
    for (int b = 0; b < ALPHABET_SIZE; b++) {
        if (counts[b] >= min_repeats) {
            valid_bytes.insert(b);
        }
    }

    // We use only the bytes that are valid for all documents so far
    allowed_bytes = get_intersection(allowed_bytes, valid_bytes);

    // We have counts so we can pre-allocate data structures
    // !@#$ CLEAN UP!!
    map<byte, vector<offset_t>> offsets_map;
    vector<offset_t>::iterator offsets_ptr[ALPHABET_SIZE];
    bool byte_lut[ALPHABET_SIZE] = {0};

    for (set<byte>::const_iterator it = allowed_bytes.begin(); it != allowed_bytes.end(); ++it) {
        byte b = *it;
        offsets_map[b] = vector<offset_t>(counts[b]);
        offsets_ptr[b] = offsets_map[b].begin();
        byte_lut[b] = true;
    }

    // Scan the document a second time and read in the bytes
    for (byte *p = data; p < end; p++) {
        if (byte_lut[*p]) {
            *(offsets_ptr[*p]++) = offset_t(p - data);
        }
    }

    delete[] in_data;

    // Report what was read to stdout
#if VERBOSITY >= 2
    cout << "get_doc_offsets_map(" << path << ") " << offsets_map.size() << " {";
    for (map<string, vector<offset_t>>::iterator it = offsets_map.begin(); it != offsets_map.end(); ++it) {
        cout << it->first << ":" << it->second.size() << ", ";
        //check_sorted(it->second);
    }
    cout << "}" << endl;
#endif
    return offsets_map;
}

InvertedIndex::InvertedIndex() :
    _n_bad_allowed(0) {
    // Start `_allowed_terms` as all single bytes
    for (int b = 0; b < ALPHABET_SIZE; b++) {
        _allowed_bytes.insert(b);
    }
}

InvertedIndex::InvertedIndex(const vector<RequiredRepeats>& required_repeats_list, int n_bad_allowed) :
     InvertedIndex() {
    _n_bad_allowed = n_bad_allowed;
    for (vector<RequiredRepeats>::const_iterator it = required_repeats_list.begin(); it != required_repeats_list.end(); ++it) {
        const RequiredRepeats& rr = *it;
        const map<byte, vector<offset_t>> offsets_map = get_doc_offsets_map(rr._doc_name, _allowed_bytes, rr._num);
        if (offsets_map.size() > 0) {
            add_doc(rr, offsets_map);
        }

#if VERBOSITY >= 1
        cout << " Added " << rr._doc_name << " to inverted index" << endl;
#endif
    }
}

/*
 * Add byte offsets from a document to the inverted index
 *  Trim `_postings_map` keys that are not in `term_offsets`
 */
void
InvertedIndex::add_doc(const RequiredRepeats& required_repeats, const map<byte, vector<offset_t>>& byte_offsets) {
    // Remove keys in _byte_postings_map that are not keys of s_offsets
    set<byte> common_bytes = get_intersection(_allowed_bytes, get_keys_set(byte_offsets));
    trim_keys(_byte_postings_map, common_bytes);

    int doc_index = (int)_docs_map.size();
    _docs_map[doc_index] = required_repeats;

    for (set<byte>::const_iterator it = common_bytes.begin(); it != common_bytes.end(); ++it) {
        byte b = *it;
        _byte_postings_map[b].add_offsets(doc_index, byte_offsets.at(b));
    }

    _allowed_bytes = get_intersection(_allowed_bytes, get_keys_set(_byte_postings_map));
}

#if 0
/*
 * Return index of document named doc_name if it is in inverted_index,
 *  otherwise -1
 * !@#$ Clean this up with a reverse Map
 */
static
int
get_doc_index(InvertedIndex *inverted_index, const string& doc_name) {
    map<int, RequiredRepeats>& docs_map = inverted_index->_docs_map;
    for (int i = 0; i < (int)docs_map.size(); i++) {
        string& doc = docs_map[(const int)i]._doc_name;
        if (doc == doc_name) {
            return i;
        }
    }
    return -1;
}
#endif

#if 0
static
bool
check_sorted(const vector<offset_t>& offsets) {
    for (unsigned int i = 1; i < offsets.size(); i++) {
        assert(offsets[i] > offsets[i-1]);
        if (offsets[i] <= offsets[i-1]) {
            return false;
        }
    }
    return true;
}
#endif

/*
 * Create the InvertedIndex corresponding to `required_repeats`
 */
InvertedIndex *
create_inverted_index(const vector<RequiredRepeats>& required_repeats_list, int n_bad_allowed) {
    return new InvertedIndex(required_repeats_list, n_bad_allowed);
}

void
delete_inverted_index(InvertedIndex *inverted_index) {
    delete inverted_index;
}

static
struct VersionInfo {
    VersionInfo() {
        cout << "TERM_IS_SEQUENCE = " << TERM_IS_SEQUENCE << endl;
        cout << "INNER_LOOP = " << INNER_LOOP << endl;
        cout << "TRACK_EXACT_MATCHES = " << TRACK_EXACT_MATCHES << endl;
        cout << "Sizes of main types" << endl;
        cout << "offset_t size = " << sizeof(offset_t) << " bytes" << endl;
        cout << "Postings size = " << sizeof(Postings) << " bytes" << endl;
        string s;
        cout << "string size = " << sizeof(s) << " bytes" << endl;
        Term t;
        cout << "Term size = " << sizeof(t) << " bytes" << endl;
    };
} _version_info;

