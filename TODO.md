TODO
----
* Binary search on strings, linear search on bytes
* Raw binary search
* Multi-thread
* Try http://dlib.net/
* Test for substrings that are repeated more times that required
* Construct worst case of the maximum number of unique _valid_ substrings
* Add backtracking to C++
* Handle and describe worst case of document containing all blanks
* Document make_repeats.py
* Read-file interface
* Reduce copying
* python glob wrapper
* make naming consitent between README, cpp and py

* Partial match. ie Match m of n documents 1 <= m <= n
* map -> unordered_map ?
* set -> unordered_set
* suffix array
* neighborhoods
* parametrize: show_exact_matches, epsilon


Sequence Search
---------------
    Term(term, gap, b) = term followd by gap wildcards followed by byte b

    for m = 0 to MAX_TERM_LEN:
        extend so that
            longest terms: |term| == m + 1
            fraction wild cards <= 1 - epsilon
            NOTE: This determines shortest terms to keep
                w(term) := # wildcards in term
                #wildcards at len m + 1 = w(term) + gap
                                        = w(term) + m - |term|
                Allowed #wildcards = (1 - epsilon) x (m + 1)
                Limit: w(term) + m - |term| <= (1 - epsilon) x (m + 1)

        extendable_terms = terms that satisfy Limit
        m_1_candidates = []
        for term in extendable_terms:
            for b in alphabet:
                m_1_candidates.append(Term(term, m - |term|, b))

        m_1_terms = m_1_candidates that satisfy doc count requirements
