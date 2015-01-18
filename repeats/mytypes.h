#ifndef MYTYPES_H
#define MYTYPES_H

#include <string>
#include <vector>
#if 1
#include <map>
#define Map map
#else
#include <unordered_map>
#define Map unordered_map
#endif

#define ALPHABET_SIZE 256

// !@#$ Need a better comment!
// We will always work at byte granularity as it is gives complete generality
typedef unsigned char byte;

// We encode offsets as 4 byte integers so that we get at most 4x increase in
//  size over the raw data
typedef unsigned int offset_t;


/*
 * A Term can be a string or sequence of bytes  !@#$
 */
#define TERM_IS_SEQUENCE 0

#if !TERM_IS_SEQUENCE
typedef std::string Term;

inline
Term
term_from_byte(byte b) {
    return std::string(1, (byte)b);
}

inline
std::string
term_to_string(const Term& term) {
    return term;
}

inline
Term
concat(const Term& a, const Term& b) {
    return a + b;
}

inline
Term
sub_term(const Term& term, int start) {
    return Term(term.begin() + start, term.end());
}

#else

typedef std::vector<int> Term;

inline
Term
term_from_byte(byte b) {
    std::vector<int> term;
    term.push_back(b);
    return term;
}

inline
std::string
term_to_string(const Term& term) {
    return std::string("!@#$");
}

inline
Term
concat(const Term& a, const Term& b) {
    Term r = a;
    return r; // !@#$
    //return r.insert(r.end(), b.begin(), b.end());
}

inline
Term
sub_term(const Term& term, int start) {
    return Term(term.begin() + start, term.end());
}

#endif

#define VERBOSITY 1

#endif // #ifndef MYTYPES_H
