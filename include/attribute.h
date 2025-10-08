#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

#define __attribute_maybe_unused__ __attribute__((__unused__))

// Optimization attributes

#define __attribute_no_vectorize__ __attribute__((optimize("no-tree-slp-vectorize")))

#endif //__ATTRIBUTE_H__