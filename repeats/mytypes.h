#ifndef MYTYPES_H
#define MYTYPES_H

#include <string>

// We will always work at byte granularity as it is gives complete generality
typedef unsigned char byte;

// We encode offsets as 4 byte integers so that we get at most 4x increase in
//  size over the raw data
typedef unsigned int offset_t;

/*
 * A `Term` is a string  !@#$
 */
typedef std::string Term;

#define VERBOSITY 1

#endif // #ifndef MYTYPES_H
