#ifndef POSTINGS_H
#define POSTINGS_H

#include <string>
#include <vector>
#include "utils.h"

// !@#$ Move this
std::vector<int> get_counts_per_doc(const std::map<int, std::vector<offset_t>>& offsets_map);

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
    std::vector<int> _doc_indexes;

    // _offsets_map[i] = offsets of term in document with index i
    //  Each _offsets_map[i] is sorted smallest to largest
    std::map<int, std::vector<offset_t>> _offsets_map;

    // Optional
    // ends[i] = offset of end of term in document with index i
    //map<int, vector<offset_t>> _ends_map;

    // All fields are zero'd on construction
    // (The containers do this by default)
    Postings() : _total_terms(0) {}

    // Add `offsets` which contains all offsets for document with index `doc_offset` to this Postings
    // i.e. _offsets_map[doc_index] <- offsets
    void add_offsets(int doc_index, const std::vector<offset_t>& offsets) {
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

    std::vector<int> counts_per_doc() const {
        return get_counts_per_doc(_offsets_map);
    }
};

#endif // #ifndef POSTINGS_H
