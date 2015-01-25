#include "inverted_index.h"
#include "inverted_index_int.h"

#if !TERM_IS_SEQUENCE

using namespace std;

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
        /*
         * Walk through s_offsets and b_offsets keeping them aligned as follows
         *  b offset == end of s offset => save s offset as it is an s + b offset
         *  b offset < end of s offset  => advance b offset
         *  b offset > end of s offset  => advance s offset
         */
        while (ib != b_end && is != s_end) {
            offset_t s_m = *is + m;  // offset of end of s
            if (*ib == s_m) {
                sb_offsets.push_back(*is);
                ++is;
            } else if (*ib < s_m) {
                while (ib != b_end && *ib < s_m) {
                    ++ib;
                }
            } else {
                offset_t b_m = *ib - m;
                while (is != s_end && *is < b_m) {
                    ++is;
                }
            }
        }
    } else {
        /*
         * Walk through s_offsets and b_offsets keeping them aligned as follows
         *  b offset == end of s offset => save s offset as it is an s + b offset
         *  b offset < end of s offset  => advance b offset binary searching regions of step_size_b
         *  b offset > end of s offset  => advance s offset
         */
        size_t step_size_b = next_power2(ratio);
        while (ib != b_end && is != s_end) {
            offset_t s_m = *is + m;
            if (*ib == s_m) {
                sb_offsets.push_back(*is);
                ++is;
            } else if (*ib < s_m) {
                 ib = get_gteq2(ib, b_end, s_m, step_size_b);
            } else {
                offset_t b_m =  *ib - m;
                while (is != s_end && *is < b_m) {
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
 * Return Postings for term s + b if s + b is repeated a sufficient number of times in each document
 *  otherwise an empty Postings
 *  Caller must guarantee that s and b are valid (repeated a sufficient number of times in each document)
 *
 *  Params:
 *      inverted_index: The InvertedIndex
 *      term_postings_map: All Posting of current term length !@#$ Could get this from InvertedIndex
 *      s: A valid length m term
 *      b: A vaild length 1 term
 *  Returns:
 *      Offsets of all s + b Terms in the document
 */
inline
Postings
get_sb_postings(const InvertedIndex *inverted_index,
                const map<Term, Postings>& term_postings_map,
                const Term& s, byte b) {

    offset_t m = (offset_t)s.size();
    const Postings& s_postings = term_postings_map.at(s);
    const Postings& b_postings = inverted_index->_byte_postings_map.at(b);
    Postings sb_postings;

    const map<int, RequiredRepeats>& docs_map = inverted_index->_docs_map;
    int n_bad = 0;
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
            n_bad++;
            if (n_bad > inverted_index->_n_bad_allowed) {
                // Empty map signals no match
                return Postings();
            }
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
get_valid_string_counts(const map<string, Postings>& term_postings_map) {
    map<string, map<int, offset_t>> string_counts;
    for (map<string, Postings>::const_iterator it = term_postings_map.begin(); it != term_postings_map.end(); ++it) {
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
    return true;
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
                  const map<Term, Postings>& term_postings_map) {
    vector<Term> exact_matches;

     for (map<Term, Postings>::const_iterator it = term_postings_map.begin(); it != term_postings_map.end(); ++it) {
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
 *  `term_postings_map` contains valid length m terms
 *      "valid" means occurring sufficient numbers of times in all docs
 *      theoretic worst case size = 4 x size of all docs (1 int offset per byte)
 *
 *  In each inner loop over valid_bytes
 *      term_postings_map[s] (length m terms) is replaced by term_postings_map[s + b] (length m + 1 terms)
 *          where b is a byte
 *      In going from length m terms to length m + 1 terms:
 *          number of terms cannot grow by more than a factor of ALPHABET_SIZE
 *          total size (all offsets of all terms) cannot grow because all s + b terms start with an s term
 *      Terms that do not occur often enough in all docs are filtered out so the total size
 *      will start decreasing when m is large enough
 *
 */
RepeatsResults
get_all_repeats(InvertedIndex *inverted_index, size_t max_term_len) {

    // Postings Map of terms of length 1
    const map<byte, Postings>& byte_postings_map = inverted_index->_byte_postings_map;

    // Postings Map of terms of length m + 1 is constructed from from terms of length m
    map<Term, Postings> term_postings_map = copy_map_byte_term(byte_postings_map);

#if VERBOSITY >= 1
    cout << "get_all_repeats: valid_bytes=" << byte_postings_map.size()
         << ",repeated_strings=" << term_postings_map.size()
         << ",max_term_len=" << max_term_len
         << endl;
#endif
    const vector<byte> valid_bytes = get_keys_vector(byte_postings_map);
    vector<Term> valid_terms = get_keys_vector(term_postings_map);

    // Track the last case of exact matches
    vector<Term> exact_matches;

    // Set converged to true if loop below converges
    bool converged = false;

    bool show_exact_matches = false;

    // Each pass through this for loop builds offsets of substrings of length m + 1 from
    // offsets of substrings of length m
    for (offset_t m = 1; m <= max_term_len; m++) {

        {   // Keep track of exact matches
            // We may need to backtrack to the longest exact match term
            const vector<Term>& em = get_exact_matches(inverted_index->_docs_map, term_postings_map);
            if (em.size() >= 3) {
                show_exact_matches = true;
            }
            if (show_exact_matches && em.size() > 0) {
                print_term_vector(" *** exact matches", em, 3);
                exact_matches = em;
            }
        }

#if VERBOSITY >= 1
        // Report progress to stdout
        cout << "--------------------------------------------------------------------------" << endl;
        cout << "get_all_repeats: len=" << m << ", num valid terms=" << valid_terms.size()
             << ", time= " << get_elapsed_time() << endl;
#endif
#if VERBOSITY >= 2
         for (int i = 0; i < min(3, (int)valid_terms.size()); i++) {
            const Term& s = valid_terms[i];
            //print_vector(term_to_string(s), term_postings_map[s].counts_per_doc());
        }
        print_vector("valid_terms", valid_terms, 10);
#endif
        /*
         * Construct all possible length m + 1 terms from existing length m terms in valid_s_b
         * and filter out length m + 1 term that don't end with an existing length m term
         */

        /*
         * valid_s_b[s][b] is later converted to s + b: s is length m, b is length 1
         * valid_s_b[s][b] contains only s, b such that (s + b)[:-1] and (s + b)[1:]
         * are elements of valid_terms
         */
        map<Term, vector<byte>> valid_s_b;
        for (vector<Term>::const_iterator is = valid_terms.begin(); is != valid_terms.end(); ++is) {
            const Term& s = *is;
            vector<byte> extension_bytes;
            for (vector<byte>::const_iterator ib = valid_bytes.begin(); ib != valid_bytes.end(); ++ib) {
                byte b = *ib;
                if (binary_search(valid_terms.begin(), valid_terms.end(), slice(extend_term_byte(s, b), 1))) {
                    extension_bytes.push_back(b);
                }
            }
            if (extension_bytes.size() > 0) {
                valid_s_b[s] = extension_bytes;
            }
        }

        // Postings of length m + 1 terms
        map<Term, Postings> term_m1_postings_map;

        // Replace term_postings_map[s] with term_m1_postings_map[s + b] for all b in bytes that
        // have survived the valid_s_b filtering above
        // This cannot increase total number of offsets as each s + b starts with s
        for (map<Term, vector<byte>>::const_iterator iv = valid_s_b.begin(); iv != valid_s_b.end(); ++iv) {

            const Term& s = iv->first;
            const vector<byte>& bytes = iv->second;
            for (vector<byte>::const_iterator ib = bytes.begin(); ib != bytes.end(); ++ib) {
                byte b = *ib;
                const Postings postings = get_sb_postings(inverted_index, term_postings_map, s, b);
                if (postings.empty()) {
                    continue;
                }
                const Term s_b = extend_term_byte(s, b);

                // Hand tuning!!
                if (!is_allowed_for_printer(s_b)) {
                   continue;
                }

                term_m1_postings_map[s_b] = postings;
            }
        }

#if VERBOSITY >= 1
        //print_vector("   valid_s_b", get_keys_vector(valid_s_b));
        cout << valid_terms.size() << " terms * "
             << valid_bytes.size() << " bytes = "
             << valid_terms.size() * valid_bytes.size() << " ("
             << get_map_vector_size(valid_s_b) << " valid) = "
             << term_m1_postings_map.size() << " filtered"
             << endl;
#endif

        // If there are no matches then we were done in the last pass
        if (term_m1_postings_map.size() == 0) {
            converged = true;
            break;
        }

        term_postings_map = term_m1_postings_map;
        valid_terms = get_keys_vector(term_postings_map);
    }

    return RepeatsResults(converged, valid_terms, exact_matches);
}

#endif // #if !TERM_IS_SEQUENCE
