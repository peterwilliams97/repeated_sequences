#ifndef MYTYPES_H
#define MYTYPES_H

#include <string>
#include <vector>
//#include <iostream>
//#include <sstream>
//#include <iterator>

#if 0
#include <map>
#define Map map
#elif 0
#include <unordered_map>
#define Map unordered_map
#endif

#define ALPHABET_SIZE 256
#define MAX_SUBSTRING_LEN 100

// !@#$ Need a better comment!
// We will always work at byte granularity as it is gives complete generality
typedef unsigned char byte;

// We encode offsets as 4 byte integers so that we get at most 4x increase in
//  size over the raw data
typedef unsigned int offset_t;

/*
 * A Term can be a string or sequence of bytes  !@#$
 */
#define TERM_IS_SEQUENCE 1

typedef std::string TermStr;
typedef std::vector<int> TermSeq;


#if !TERM_IS_SEQUENCE
typedef TermStr Term;

inline
Term
byte_to_term(byte b) {
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
extend_term_byte(const Term& s, byte b) {
    return s + (char)b;
}

inline
Term
slice(const Term& term, int start) {
    return Term(term.begin() + start, term.end());
}

#else

typedef TermSeq Term;

inline
Term
byte_to_term(byte b) {
    std::vector<int> term;
    term.push_back(b);
    return term;
}

inline
std::string
term_to_string(const Term& term) {
    std::vector<char> char_vec;

    for (std::vector<int>::const_iterator it = term.begin(); it != term.end(); ++it) {
        char c = *it >= 0 ? (char)(byte)*it : '.';
        char_vec.push_back(c);
    }

    return std::string(char_vec.begin(), char_vec.end());
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
slice(const Term& term, int start) {
    return Term(term.begin() + start, term.end());
}

inline
int
num_wild(const Term& term) {
    int n_wild = 0;

    for (std::vector<int>::const_iterator it = term.begin(); it != term.end(); ++it) {
        if (*it < 0) {
            n_wild++;
        }
    }

    return n_wild;
}

#define B2I(b) ((int)(unsigned int)(b))

inline
Term
make_extension_term(int gap, byte b) {
    int x[MAX_SUBSTRING_LEN];
    for (int i = 0; i < gap; i++) {
        x[i] = -1;
    }
    x[gap] = (int)(unsigned int)b;
    return std::vector<int>(x, x + gap + 1);
}

inline
Term
extend_term_gap_byte(const Term& s, int gap, byte b) {
    Term s1 = std::vector<int>(s);
    for (int i = 0; i < gap; i++) {
        s1.push_back(-1);
    }
    s1.push_back((int)(unsigned int)b);
    return s1;
}



#endif

#define VERBOSITY 1

#endif // #ifndef MYTYPES_H
