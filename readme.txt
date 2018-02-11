CMPH - C Minimal Perfect Hashing Library

This code is not mine. Just compiled with MSVC 6.0

See http://cmph.sourceforge.net/ for detailes.

Quote from cmph.sourceforge.net:

> A perfect hash function maps a static set of n keys into a set of m integer
> numbers without collisions, where m is greater than or equal to n. If m is
> equal to n, the function is called minimal.
> 
> Minimal perfect hash functions are widely used for memory efficient
> storage and fast retrieval of items from static sets, such as words in
> natural languages, reserved words in programming languages or interactive
> systems, universal resource locations (URLs) in Web search engines, or item
> sets in data mining techniques. Therefore, there are applications for
> minimal perfect hash functions in information retrieval systems, database
> systems, language translation systems, electronic commerce systems,
> compilers, operating systems, among others.
>
> The use of minimal perfect hash functions is, until now, restricted to
> scenarios where the set of keys being hashed is small, because of the
> limitations of current algorithms. But in many cases, to deal with huge set of
> keys is crucial. So, this project gives to the free software community an API
> that will work with sets in the order of billion of keys.
>
> Probably, the most interesting application for minimal perfect hash functions
> is its use as an indexing structure for databases. The most popular data
> structure used as an indexing structure in databases is the B+ tree. In fact,
> the B+ tree is very used for dynamic applications with frequent insertions and
> deletions of records. However, for applications with sporadic modifications
> and a huge number of queries the B+ tree is not the best option, because
> practical deployments of this structure are extremely complex, and perform
> poorly with very large sets of keys such as those required for the new
> frontiers database applications.
>
> For example, in the information retrieval field, the work with huge
> collections is a daily task. The simple assignment of ids to web pages of a
> collection can be a challenging task. While traditional databases simply
> cannot handle more traffic once the working set of web page urls does not fit
> in main memory anymore, minimal perfect hash functions can easily scale to
> hundred of millions of entries, using stock hardware.
>
> As there are lots of applications for minimal perfect hash functions, it is
> important to implement memory and time efficient algorithms for constructing
> such functions. The lack of similar libraries in the free software world has
> been the main motivation to create the C Minimal Perfect Hashing Library
> (gperf is a bit different, since it was conceived to create very fast perfect
> hash functions for small sets of keys and CMPH Library was conceived to create
> minimal perfect hash functions for very large sets of keys). C Minimal Perfect
> Hashing Library is a portable LGPLed library to generate and to work with very
> efficient minimal perfect hash functions.
