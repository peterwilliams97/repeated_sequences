#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include <string>
#include <vector>
#include "utils.h"

/*
 * Use an inverted index to find the longest substring(s) that is repeated
 *  a specified number of times in a corpus of documents.
 *
 * (The fact that the implementation uses an inverted index is not visiblen
 *  in this file.)
 *
 * Expected usage
 * ---------------
 *  // Create an inverted index from a list of files in filename that have
 *  // their number of repeats encoded like "repeats=5.txt"
 *  InvertedIndex *inverted_index = create_inverted_index(filenames);
 *
 *  // Optionally show the contents of the inverted index
 *  show_inverted_index("initial", inverted_index);
 *
 *  // Compute the longest substrings that are repeated the specified
 *  // number of times
 *  vector<string> repeats = get_all_repeats(inverted_index);
 *
 *  // Free up all the resources in the InvertedIndex
 *  delete_inverted_index(inverted_index);
 */

// !@#$ Should be a settable parameter
#define MAX_SUBSTRING_LEN 100

struct RepeatsResults {
    const bool _converged;                  // Did search converge?
    const std::vector<Term> _valid;         // Longest terms that matched at least the required number of times per doc
    const std::vector<Term> _exact;         // Longest Terms that matched the exact number of times per doc

    RepeatsResults(bool converged, const std::vector<Term> valid, const std::vector<Term> exact):
        _converged(converged), _valid(valid), _exact(exact) {}
};

struct InvertedIndex;

// Create an inverted index from a list of files in filename that have
// their number of repeats encoded like "repeats=5.txt"
InvertedIndex *create_inverted_index(const std::vector<RequiredRepeats>& required_repeats_list, int n_bad_allowed);

// Free up all the resources in the InvertedIndex
void delete_inverted_index(InvertedIndex *inverted_index);

// Return the longest substrings that are repeated the specified
// number of times
RepeatsResults get_all_repeats(InvertedIndex *inverted_index, size_t max_substring_len=MAX_SUBSTRING_LEN);

#endif // #ifndef INVERTED_INDEX_H
