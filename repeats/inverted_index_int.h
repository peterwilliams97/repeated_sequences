#ifndef INVERTED_INDEX_IN_H
#define INVERTED_INDEX_IN_H

#include <string>
#include <vector>
#include "utils.h"
#include "postings.h"

/*
 * Use an inverted index to find the longest term(s) that is repeated
 *  a specified number of times in a corpus of documents.
 *
const int MAX_SUBSTRING_LEN = 100;

/*
 * An InvertedIndex is a map of Postings of a set of terms across
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

    int _n_bad_allowed; // !@#$ Not exactly right. Mismatch must be same doc each time

    // `_postings_map[term]` is the Postings of Term `term`
    // !@#$ Separate byte Postings map
    std::map<Term, Postings> _byte_postings_map;

    // `_docs_map[i]` = path + min required repeats of document index i.
    //  The Postings in `_postings_map` index into this map
    std::map<int, RequiredRepeats> _docs_map;

    // `_allowed_terms` is all valid terms
    std::set<Term> _allowed_bytes;

private:
    InvertedIndex();

public:
    InvertedIndex(const std::vector<RequiredRepeats>& required_repeats_list, int n_bad_allowed);
    void add_doc(const RequiredRepeats& required_repeats, const std::map<Term, std::vector<offset_t>>& term_offsets);

};


#endif // #ifndef INVERTED_INDEX_IN_H
