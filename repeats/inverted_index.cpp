/*
 * Use an inverted index to find the longest substring(s) that is repeated
 *  a specified number of times in a corpus of documents.
 *
 * Documentation in https://github.com/peterwilliams97/repeated_sequences
 */
#define INNER_LOOP 4

#include <assert.h>
#include <iostream>
#include "mytypes.h"
#include "utils.h"
#include "timer.h"
#include "inverted_index.h"

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

/*
 * A Postings is a list of lists of offsets of a particular term (substring)
 *  in all documents in a corpus.
 *
 *  _offsets_map[i] stores the offsets in document i
 *
 * http://en.wikipedia.org/wiki/Inverted_index
 */
struct Postings {
    // Total # occurrences of term in all documents
    size_t _total_terms;

    // Indexes of documents that term occurs in
    vector<int> _doc_indexes;

    // _offsets_map[i] = offsets of term in document with index i
    //  Each _offsets_map[i] is sorted smallest to largest
    map<int, vector<offset_t>> _offsets_map;

    // Optional
    // ends[i] = offset of end of term in document with index i
    //map<int, vector<offset_t>> _ends_map;

    // All fields are zero'd on construction
    // (The containers do this by default)
    Postings() : _total_terms(0) {}

    // Add `offsets` which contains all offsets for document with index `doc_offset` to this Postings
    // i.e. _offsets_map[doc_index] <- offsets
    void add_offsets(int doc_index, const vector<offset_t>& offsets) {
        _doc_indexes.push_back(doc_index);
        _offsets_map[doc_index] = offsets;
#if 0
        vector<offset_t>& offs = _offsets_map[doc_index];
        cout << " offs: "<< offs.size() << " -- " << offs.capacity() << endl;
#endif
        _total_terms += offsets.size();
    }

    // Return number of documents whose offsets are stored in Posting
    unsigned int num_docs() const {
        return (unsigned int)_offsets_map.size();
    }

    // Return total number of offsets stored in _offsets_map
    size_t size() const {
        return get_map_vector_size(_offsets_map);
    }

    // Return true if no documents are encoding in Posting
    bool empty() const {
        return num_docs() == 0;
    }

    vector<int> counts_per_doc() const {
        return get_counts_per_doc(_offsets_map);
    }
};

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
map<Term, vector<offset_t>>
get_doc_offsets_map(const string& path, set<Term>& allowed_terms, unsigned int min_repeats) {

    offset_t counts[ALPHABET_SIZE] = {0};

    size_t length = get_file_size(path);
    byte *in_data = read_file(path);
    byte *end = in_data + length;

    byte *data = in_data + HEADER_SIZE;

    // Pass through the document once to get counts of all bytes
    for (byte *p = data; p < end; p++) {
        counts[*p]++;
    }

    // valid_bytes are those with sufficient counts
    set<Term> valid_bytes;
    for (int b = 0; b < ALPHABET_SIZE; b++) {
        if (counts[b] >= min_repeats) {
            valid_bytes.insert(byte_to_term(b));
        }
    }

    // We use only the bytes that are valid for all documents so far
    allowed_terms = get_intersection(allowed_terms, valid_bytes);

    // We have counts so we can pre-allocate data structures
    map<Term, vector<offset_t>> offsets_map;
    bool allowed_bytes[ALPHABET_SIZE];
    vector<offset_t>::iterator offsets_ptr[ALPHABET_SIZE];
    for (int b = 0; b < ALPHABET_SIZE; b++) {
        Term s = byte_to_term(b);
        bool allowed = allowed_terms.find(s) != allowed_terms.end();
        allowed_bytes[b] = allowed;
        if (allowed) {
            offsets_map[s] = vector<offset_t>(counts[b]);
            offsets_ptr[b] = offsets_map[s].begin();
        }
    }

    // Scan the document a second time and read in the bytes
    offset_t ofs = 0;
    for (byte *p = data; p < end; p++) {
        if (allowed_bytes[*p]) {
            *(offsets_ptr[*p]) = ofs;
            ofs++;
            offsets_ptr[*p]++;
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

/*
 * An InvertedIndex is a Map of Postings of a set of terms across
 *  all documents in a corpus.
 *
 *  _postings_map[term] stores all the offsets of term in all the
 *      documents in the corpus
 *
 * Typical usage is to construct an initial InvertedIndex whose terms
 *  are all bytes that occur in the corpus then to replace these
 *  with each string that occurs in the corpus. This is done
 *  bottom-up, replacing _postings_map[s] with _postings_map[s + b]
 *  for all bytes b to get from terms of length m to terms of
 *  length m + 1
 */
struct InvertedIndex {
    // `_postings_map[term]` is the Postings of Term `term`
    map<Term, Postings> _postings_map;

    // `_docs_map[i]` = path + min required repeats of document index i.
    //  The Postings in `_postings_map` index into this map
    map<int, RequiredRepeats> _docs_map;

    // `_allowed_terms` is all valid terms
    set<Term> _allowed_terms;

private:
    InvertedIndex() {
        // Start `_allowed_terms` as all single bytes
        for (int b = 0; b < ALPHABET_SIZE; b++) {
            _allowed_terms.insert(byte_to_term(b));
        }
    }

public:
    InvertedIndex(const vector<RequiredRepeats>& required_repeats_list) : InvertedIndex() {
        for (vector<RequiredRepeats>::const_iterator it = required_repeats_list.begin(); it != required_repeats_list.end(); ++it) {
            const RequiredRepeats& rr = *it;
            const map<Term, vector<offset_t>> offsets_map = get_doc_offsets_map(rr._doc_name, _allowed_terms, rr._num);
            if (offsets_map.size() > 0) {
                add_doc(rr, offsets_map);
            }

#if VERBOSITY >= 1
            cout << " Added " << rr._doc_name << " to inverted index" << endl;
#endif
#if VERBOSITY >= 2
            inverted_index->show(rr._doc_name);
#endif
        }

        }

    /*
     * Add term offsets from a document to the inverted index
     *  Trim `_postings_map` keys that are not in `term_offsets`
     */
    void add_doc(const RequiredRepeats& required_repeats, const map<Term, vector<offset_t>>& term_offsets) {
        // Remove keys in _postings_map that are not keys of s_offsets
        set<Term> common_terms = get_intersection(_allowed_terms, get_keys_set(term_offsets));
        trim_keys(_postings_map, common_terms);

        int doc_index = (int)_docs_map.size();
        _docs_map[doc_index] = required_repeats;

        for (set<Term>::const_iterator it = common_terms.begin(); it != common_terms.end(); ++it) {
            const Term& term = *it;
            _postings_map[term].add_offsets(doc_index, term_offsets.at(term));
        }
    }

    size_t size() const {
        size_t sz = 0;
        for (map<Term, Postings>::const_iterator it = _postings_map.begin(); it != _postings_map.end(); ++it) {
            sz += it->second.size();
        }
        return sz;
    }

    void show(const string& title) const {
#if VERBOSITY >= 2
        cout << " InvertedIndex ===== " << title << endl;
        print_vector(" _postings_map", get_keys_vector(_postings_map));
        print_vector(" _docs_map", get_keys_vector(_docs_map));
        print_set(" _allowed_terms", _allowed_terms);
#endif
   }
};

/*
 * Write contents of inverted_index to stdout
 */
void
show_inverted_index(const string& title, const InvertedIndex *inverted_index) {
    inverted_index->show(title);
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
create_inverted_index(const vector<RequiredRepeats>& required_repeats_list) {

    return new InvertedIndex(required_repeats_list);
}

void
delete_inverted_index(InvertedIndex *inverted_index) {
    delete inverted_index;
}

/*
 * Return ordered vector of offsets of terms s + b in document where
 *      `s_offsets` is ordered vector of offsets of Term s in document
 *      `b_offsets` is ordered vector of offsets of strings b in document
 *      `m` is length of s
 *
 * THIS IS THE INNER LOOP
 *
 *  Params:
 *      s_offsets: All offsets of Term s in a document
 *      m: length of Term s
 *      b_offsets: All offsets of Term b in a document
 *  Returns:
 *      Offsets of all s + b Terms in the document
 *
 * Basic idea is to keep 2 pointers and move the one behind and record matches of
 *  *is + m == *ib
 */
inline
const vector<offset_t>
get_sb_offsets(const vector<offset_t>& s_offsets, offset_t m, const vector<offset_t>& b_offsets) {
    vector<offset_t> sb_offsets;
    vector<offset_t>::const_iterator is = s_offsets.begin();
    vector<offset_t>::const_iterator ib = b_offsets.begin();


#if INNER_LOOP == 1
    vector<offset_t>::const_iterator b_end = bytes.end();
    vector<offset_t>::const_iterator s_end = strings.end();

    while (ib < b_end && is < s_end) {
        offset_t is_m = *is + m;
        if (*ib == is_m) {
            sb_offsets.push_back(*is);
            ++is;
        } else if (*ib < is_m) {
            while (ib < b_end && *ib < is_m) {
                ++ib;
            }
        } else {
            offset_t ib_m =  *ib - m;
            while (is < s_end && *is < ib_m) {
                ++is;
            }
        }
    }

#elif INNER_LOOP == 2

    while (ib < bytes.end() && is < strings.end()) {
        if (*ib == *is + m) {
            sb_offsets.push_back(*is);
            ++is;
        } else if (*ib < *is + m) {
            ib = get_gteq(ib, bytes.end(), *is + m);
        } else {
            is = get_gteq(is, strings.end(), *ib - m);
        }
    }

#elif INNER_LOOP == 3

    // Performance about same for 256, 512 when num chars is low
    // !@#$ Need to optimize this
    // The next power 2 calculation slows 2 MB test 35 sec => 42 sec!
    //size_t step_size_b = next_power2((double)(bytes.back() - bytes.front())/(double)bytes.size());
    //size_t step_size_s = next_power2((double)(strings.back() - strings.front())/(double)strings.size());

    size_t step_size_b = 512;
    size_t step_size_s = 512;

    while (ib < bytes.end() && is < strings.end()) {

        if (*ib == *is + m) {
            sb_offsets.push_back(*is);
            ++is;
        } else if (*ib < *is + m) {
            ib = get_gteq2(ib, bytes.end(), *is + m, step_size_b);
        } else {
            is = get_gteq2(is, strings.end(), *ib - m, step_size_s);
        }
    }

#elif INNER_LOOP == 4
    vector<offset_t>::const_iterator s_end = s_offsets.end();
    vector<offset_t>::const_iterator b_end = b_offsets.end();

    double ratio = (double)b_offsets.size() / (double)s_offsets.size();

    if (ratio < 8.0) {
        while (ib < b_end && is < s_end) {
            offset_t is_m = *is + m;
            if (*ib == is_m) {
                sb_offsets.push_back(*is);
                ++is;
            } else if (*ib < is_m) {
                while (ib < b_end && *ib < is_m) {
                    ++ib;
                }
            } else {
                offset_t ib_m = *ib - m;
                while (is < s_end && *is < ib_m) {
                    ++is;
                }
            }
        }
    } else {
        size_t step_size_b = next_power2(ratio);
        while (ib < b_end && is < s_end) {
            offset_t is_m = *is + m;
            if (*ib == is_m) {
                sb_offsets.push_back(*is);
                ++is;
            } else if (*ib < is_m) {
                 ib = get_gteq2(ib, b_end, *is + m, step_size_b);
            } else {
                offset_t ib_m =  *ib - m;
                while (is < s_end && *is < ib_m) {
                    ++is;
                }
            }
        }
    }
#endif

    return sb_offsets;
}

#if 0
inline vector<offset_t>
get_non_overlapping_strings(const vector<offset_t>& offsets, size_t m) {
    if (offsets.size() < 2) {
        return offsets;
    }
    vector<offset_t> non_overlapping;
    vector<offset_t>::const_iterator it0 = offsets.begin();
    vector<offset_t>::const_iterator it1 = it0 + 1;
    vector<offset_t>::const_iterator end = offsets.end();

    non_overlapping.push_back(*it0);
    while (it1 < end) {
        if (*it1 >= *it0 + m) {
            non_overlapping.push_back(*it1);
            it0++;
            it1++;
        } else {
            while (it1 < end && *it1 < *it0 + m) {
                it1++;
            }
        }
    }
    return non_overlapping;
}
#endif

// This is a bit slow. Should calculate and store this value when creating strings linst
size_t
get_non_overlapping_count(const vector<offset_t>& offsets, size_t m) {
    if (offsets.size() < 2) {
        return offsets.size();
    }

    vector<offset_t>::const_iterator it0 = offsets.begin();
    vector<offset_t>::const_iterator it1 = it0 + 1;
    vector<offset_t>::const_iterator end = offsets.end();
    size_t count = 1;

    while (it1 < end) {
        if (*it1 >= *it0 + m) {
            count++;
            it0++;
            it1++;
        } else {
            while (it1 < end && *it1 < *it0 + m) {
                it1++;
            }
        }
    }
    return count;
}

/*
 * Return Postings for Term s + b if s + b is repeated a sufficient number of times in each document
 *  otherwise an empty Postings
 *  Caller must guarantee that s and b are in all RequiredRepeats (and therefore
 *  are repeated a sufficient number of times in each document)
 *
 *  Params:
 *      inverted_index: The InvertedIndex
 *      term_postings_map: All Posting of current term length !@#$ Could get this from InvertedIndex
 *      b_offsets: All offsets of Term b in a document
 *  Returns:
 *      Offsets of all s + b Terms in the document
 */
inline
Postings
get_sb_postings(const InvertedIndex *inverted_index,
                const map<Term, Postings>& term_postings_map,
                const Term& s, const Term& b) {

    unsigned int m = (unsigned int)s.size();
    const Postings& s_postings = term_postings_map.at(s);
    const Postings& b_postings = inverted_index->_postings_map.at(b);
    Postings sb_postings;

    const map<int, RequiredRepeats>& docs_map = inverted_index->_docs_map;
    for (map<int, RequiredRepeats>::const_iterator it = docs_map.begin(); it != docs_map.end(); ++it) {
        int doc_index = it->first;
        const vector<offset_t>& s_offsets = s_postings._offsets_map.at(doc_index);
        const vector<offset_t>& b_offsets = b_postings._offsets_map.at(doc_index);

        vector<offset_t> sb_offsets = get_sb_offsets(s_offsets, m, b_offsets);

        /*
         * Only count non-overlapping offsets when checking validity.
         *
         * We can do this because any non-overlapping length m + 1 substring must
         *  start with a non-overlapping length m substring.
         *
         * We CANNOT remove non-overlapping substrings of length m because
         *  valid substrings of length m + 1 may start with length m
         *  substrings may be overlapped byt other valid length m
         *  substrings
         *  e.g. looking for longest substring that appears twice in "aabcabcaa"
         *           Non-overlapping     Overlapping
         *      m=1: a:5, b:2, c:2       a:5, b:2, c:2
         *      m=2: aa:2, bc:2, ca:2    aa:2, ab:2, bc:2, ca:2
         *      m=3: none                abc:2
         */
        //sb_offsets = get_non_overlapping_strings(sb_offsets, m+1);

        if (get_non_overlapping_count(sb_offsets, m + 1) < it->second._num) {
            // Empty map signals no match
            return Postings();
        }

        sb_postings.add_offsets(doc_index, sb_offsets);
    }

#if VERBOSITY >= 3
    cout << " matched '" << s + b + "' for " << sb_postings.size() << " docs" << endl;
#endif
    return sb_postings;
}

#if 0

inline bool
is_exact_match(map<int, RequiredRepeats>& docs_map,
               const map<int, offset_t>& doc_counts) {
    for (map<int, offset_t>::const_iterator it = doc_counts.begin(); it != doc_counts.end(); ++it) {
        int key = it->first;
        const RequiredRepeats& rr = docs_map[key];
        if (rr._num != it->second) {
            return false;
        }
    }
    return true;
}

map<string, map<int, offset_t>>
get_valid_string_counts(const map<string, Postings>& term_m_postings_map) {
    map<string, map<int, offset_t>> string_counts;
    for (map<string, Postings>::const_iterator it = term_m_postings_map.begin(); it != term_m_postings_map.end(); ++it) {
        const string& s = it->first;
        const map<int, vector<offset_t>>& offsets_map = it->second._offsets_map;
        map<int, offset_t> doc_counts;
        for (map<int, vector<offset_t>>::const_iterator jt = offsets_map.begin(); jt != offsets_map.end(); jt++) {
            doc_counts[jt->first] = jt->second.size();
        }
        string_counts[s] = doc_counts;
    }
    return string_counts;
}

inline
vector<string>
get_exact_matches(map<int, RequiredRepeats>& docs_map,
                  const map<string, map<int, offset_t>>& string_counts) {
    vector<string> exact_matches;
    for (map<string, map<int, offset_t>>::const_iterator it = string_counts.begin(); it != string_counts.end(); ++it) {
        if (is_exact_match(docs_map, it->second)) {
            exact_matches.push_back(it->first);
        }
    }
    return exact_matches;
}

/*
 * Backtrack through all the vectors of strings with >= required required_repeats
 *  to find the longest with == RequiredRepeats
 */
vector<string>
get_longest_exact_matches(map<int, RequiredRepeats>& docs_map,
                          const vector<map<string, map<int, offset_t>>>& valid_string_counts) {
    for (vector<map<string, map<int, offset_t>>>::const_reverse_iterator it = valid_string_counts.rbegin(); it != valid_string_counts.rend(); ++it) {
        const map<string, map<int, offset_t>>& string_counts = *it;
        vector<string> exact_matches = get_exact_matches(docs_map, string_counts);

        if (exact_matches.size() > 0) {
           //  cout << "Backtracking: !!!" << endl;
            return exact_matches;
        }
    }
    return vector<string>();

}
#endif

// !@#$%

const byte CDCA[] = {0xcd, 0xca, 0x10, 0x00, 0x00, 0x18, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const byte PATTERN2[] = {0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const byte PATTERN3[] = {0x81, 0x22, 0x81, 0x22};

static
bool
is_part_of_pattern(const Term& str, const byte pattern[], size_t pattern_size) {
#if !TERM_IS_SEQUENCE
    size_t n = str.size();
    if (n > pattern_size) {
        return false;
    }
    const byte *data = (const byte *)str.c_str();
    for (size_t i = 0; i <= pattern_size - n; i++) {
        if (memcmp(data, &pattern[i], n) == 0) {
            return true;
        }
    }
    return false;
#else
    return false; // !@#$
#endif
}

static
bool
is_part_of_cdca(const Term& str) {
    return is_part_of_pattern(str, CDCA, sizeof(CDCA))
        || is_part_of_pattern(str, PATTERN2, sizeof(PATTERN2))
        || is_part_of_pattern(str, PATTERN3, sizeof(PATTERN3))
        ;
}

#define MIN_STR_SIZE 4

static
bool
is_allowed_for_printer(const Term& str) {
#if 1
    if (is_part_of_cdca(str)) {
        return false;
    }
#endif
    size_t n = str.size();
    if (n < MIN_STR_SIZE) {
        return true;
    }

#if !TERM_IS_SEQUENCE // !@#$
    const byte *data = (const byte *)str.c_str();
    for (size_t i = 0; i < n; i++) {
        if (data[i] != 0) {
            return true;
        }
    }
#endif

    return false;
}

inline
const vector<Term>
get_exact_matches(const map<int, RequiredRepeats>& docs_map,
                  const map<Term, Postings>& term_m_postings_map) {
    vector<Term> exact_matches;

     for (map<Term, Postings>::const_iterator it = term_m_postings_map.begin(); it != term_m_postings_map.end(); ++it) {
        const Term& s = it->first;
        const map<int, vector<offset_t>>& offsets_map = it->second._offsets_map;
        bool is_match = true;
        for (map<int, vector<offset_t>>::const_iterator jt = offsets_map.begin(); jt != offsets_map.end(); ++jt) {
            int d = jt->first;
            const RequiredRepeats& rr = docs_map.at(d);
            // !@#$ Strictly, non-overlapping count, not size()
            offset_t count = (offset_t)jt->second.size();
            if (rr._num != count) {
                is_match = false;
                break;
           }
        }

        // !@#$%
        //if (is_part_of_cdca(s)) {
        //    is_match = false;
        //}


        if (is_match) {
            exact_matches.push_back(s);
        }
    }
    return exact_matches;
}

/*
 * Return the list of terms that are repeated a sufficient numbers of times in all documents
 *
 * THIS IS THE MAIN FUNCTION
 *
 * Basic idea
 *  `term_m_postings_map` contains repeated strings (worst case 4 x size of all docs)
 *  in each inner loop over repeated_bytes
 *      byte_postings_map[s] is replaced by <= ALPHABET_SIZE x byte_postings_map[s + b]
 *      total size cannot grow because all s + b strings are contained in byte_postings_map[s]
 *      (NOTE Could be smarter and use byte_postings_map for strings of length 1)
 *      strings that do not occur often enough in all docs are filtered out
 *
 */
RepeatsResults
get_all_repeats(InvertedIndex *inverted_index, size_t max_substring_len) {

    // Postings Map of terms of length 1
    const map<Term, Postings>& byte_postings_map = inverted_index->_postings_map;

    // Postings Map of terms of length m + 1 is constructed from from terms of length m
    map<Term, Postings> term_m_postings_map = copy_map(byte_postings_map);

#if VERBOSITY >= 1
    cout << "get_all_repeats: repeated_bytes=" << byte_postings_map.size()
         << ",repeated_strings=" << term_m_postings_map.size()
         << ",max_substring_len=" << max_substring_len
         << endl;
#endif
    vector<Term> repeated_bytes = get_keys_vector(byte_postings_map);
    vector<Term> repeated_terms = get_keys_vector(term_m_postings_map);

    // Track the last case of exact matches
    vector<Term> exact_matches;

    // Set converged to true if loop below converges
    bool converged = false;

    size_t most_repeats = 0;
    offset_t most_repeats_m = 0;

    bool show_exact_matches = false;

    // Each pass through this for loop builds offsets of substrings of length m + 1 from
    // offsets of substrings of length m
    for (offset_t m = 1; m <= max_substring_len; m++) {

        {   // Keep track of exact matches
            // We may need to backtrack to the longest exact match term
            const vector<Term>& em = get_exact_matches(inverted_index->_docs_map, term_m_postings_map);
            if (em.size() >= 3) {
                show_exact_matches = true;
            }
            if (show_exact_matches && em.size() > 0) {
                print_term_vector(" *** exact matches", em, 3);
                exact_matches = em;
            }
        }

#if 0      // !@#$%
        {
           vector<string> keys = get_keys_vector(term_m_postings_map);
            for (vector<string>::iterator it = keys.begin(); it != keys.end(); ++it) {
                if (it->size() > 3) {
                    const unsigned char *data = (const unsigned char *)it->c_str();
                    if (data[0] == 0xcd
                            && data[1] == 0xca
                            && data[2] == 0x10) {
                        term_m_postings_map.erase(*it);
                    }
                }
            }

        }
#endif        // !@#$%

        if (repeated_terms.size() > most_repeats) {
            most_repeats = repeated_terms.size();
            most_repeats_m = m;
        }

#if VERBOSITY >= 1
        // Report progress to stdout
        cout << "--------------------------------------------------------------------------" << endl;
        cout << "get_all_repeats: num repeated strings=" << repeated_terms.size()
             << ", len= " << m
             << ", time= " << get_elapsed_time() << endl;
        for (int i = 0; i < min(3, (int)repeated_terms.size()); i++) {
            Term& s = repeated_terms[i];
            print_vector(term_to_string(s), term_m_postings_map[s].counts_per_doc());
        }
#endif
#if VERBOSITY >= 2
        print_vector("repeated_strings", repeated_strings, 10);
#endif
        /*
         * Construct all possible length n + 1 terms from existing length n terms in valid_s_b
         * and filter out length n + 1 term that don't end with an existing length n term
         * extension_bytes[s][b] is later converted to s + b: s is length n, b is length 1
         */
        map<Term, vector<Term>> valid_s_b;
        for (vector<Term>::const_iterator is = repeated_terms.begin(); is != repeated_terms.end(); ++is) {
            const Term& s = *is;
            vector<Term> extension_bytes;
            for (vector<Term>::const_iterator ib = repeated_bytes.begin(); ib != repeated_bytes.end(); ++ib) {
                const Term& b = *ib;
                if (binary_search(repeated_terms.begin(), repeated_terms.end(), slice(concat(s, b), 1))) {
                    extension_bytes.push_back(b);
                }
            }
            if (extension_bytes.size() > 0) {
                valid_s_b[s] = extension_bytes;
            }
        }

#if VERBOSITY >= 1
        //print_vector("   valid_s_b", get_keys_vector(valid_s_b));
        cout << repeated_terms.size() << " strings * "
             << repeated_bytes.size() << " bytes = "
             << repeated_terms.size() * repeated_bytes.size() << " ("
             << get_map_vector_size(valid_s_b) << " valid), "
             << inverted_index->size() << " total offsets"
             << endl;
#endif
        // Remove from `term_m_postings_map` the offsets of all length n strings that won't be
        // used to construct length n + 1 strings below
        for (vector<Term>::const_iterator is = repeated_terms.begin(); is != repeated_terms.end(); ++is) {
            if (valid_s_b.find(*is) == valid_s_b.end()) {
                term_m_postings_map.erase(*is);
            }
        }

        // Replace term_m_postings_map[s] with term_m_postings_map[s + b] for all b in bytes that
        // have survived the valid_s_b filtering above
        // This cannot increase total number of offsets as each s + b starts with s
        for (map<Term, vector<Term>>::const_iterator iv = valid_s_b.begin(); iv != valid_s_b.end(); ++iv) {

            const Term s = iv->first;
            const vector<Term> bytes = iv->second;
            for (vector<Term>::const_iterator ib = bytes.begin(); ib != bytes.end(); ++ib) {
                const Term b = *ib;
                const Postings postings = get_sb_postings(inverted_index, term_m_postings_map, s, b);
                if (!postings.empty()) {
                    // !@#$%
#if 1
                    if (is_allowed_for_printer(concat(s, b))) {
                       term_m_postings_map[concat(s, b)] = postings;
                    }
#endif
                }
            }
            term_m_postings_map.erase(s);
        }

        // If there are no matches then we were done in the last pass
        if (term_m_postings_map.size() == 0) {
            converged = true;
            break;
        }

        repeated_terms = get_keys_vector(term_m_postings_map);
    }

#if VERBOSITY >= 1
    cout << "most_repeats = " << most_repeats << " for m = " << most_repeats_m << endl;
#endif

    RepeatsResults repeats_result;
    repeats_result._converged = converged;
    repeats_result._longest = repeated_terms;
    repeats_result._exact = exact_matches;
    return repeats_result;
}

static
struct VersionInfo {
    VersionInfo() {
        cout << "INNER_LOOP = " << INNER_LOOP << endl;
        cout << "offset_t size = " << sizeof(offset_t) << " bytes" << endl;
        cout << "Postings size = " << sizeof(Postings) << " bytes" << endl;
        string s("1");
        cout << "string size = " << sizeof(s) << " bytes" << endl;
    };
} _version_info;

#ifdef NOT_DEFINED
#endif // #ifdef NOT_DEFINED
