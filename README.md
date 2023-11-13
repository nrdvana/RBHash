## Introduction

This is an implementation of a "Red/Black Hash Table"; a hash table where collisions are
organized using Red/Black trees.  It uses about as much memory as popular hash table
implementations, while giving worst-case performance of O(log N).  The structure is allocated
in a flat integer array and requires very minimal memory management.  The structure only deals
with "Node IDs" and does not contain any pointers.

If you want to see it in action, see the [live demo](https://nrdvana.net/rbhash/demo)

### TL;DR

Q: What is it?

A: It's a HashMap of TreeMaps, optimized for low-level code.

Q: Why would someone want a HashMap of TreeMaps?

A: HashMaps are faster than TreeMaps, but TreeMaps are immune to collision attacks.

Q: Why are HashMaps of TreeMaps uncommon?

A: Most implementations of TreeMaps use way too much memory for this purpose.  C++'s `std::map`
uses 32 bytes per node.  This one starts at 2 bytes per node (up to 0x7F nodes), and scales up
as you add items to it.

Q: What are the ideal use cases for this implementation?

A: Low level code that wants to quickly and conveniently create a set or map from a small-ish
array without a lot of setup effort, such as worrying about the quality of your hash function
or hash randomization to protect against collision attacks.  It would be especially useful for
embedded systems or device drivers where dynamic memory allocations are awkward.  It is also
extremely efficient for small sets (N < 128), like JSON objects.  I see it as "the quicksort
of hash tables".

Q: What are the limitations of this algorithm?

A: Everything is allocated in a single block of memory, so collections that are large and
growing on demand will eventually perform badly during re-allocations.  However, this could be
combined with another algorithm like a hash-trie for allocation efficiency at large scale.

### How it Works

For a user-supplied array allocated to a capacity of `N` elements, allocate `N` red/black tree
nodes in a flat array and `B` hashtable buckets on the end of that same flat array, and store
hash table collisions as a tree composed of the nodes corresponding to the colliding array
elements.  In other words, for nodes `i_1`, `i_2`, `i_3` that all hashed to value `X (mod B)`
`bucket[X]` acts as a pointer to the head of a tree composed of `node[i_1], node[i_2], node[i_3]`.
(In this implementation, Node ID 0 is reserved, so there's a +1 when going from an array index
to a Node ID)

All references to user array elements (and by extension, tree nodes) use the smallest integer
that can refer to the full range of `0 .. N*2`.  Additionally, all red/black tree nodes are
reduced to two such integers (2 bytes per node up to 127 elements, 4 bytes per node up to 32K
elements, and so on) resulting in memory usage comparable to most hash table implementations.

The choice of `B` (number of buckets) is arbitrary and can be used for performance tuning.
Choosing `B = 1` allows the algorithm to act as a TreeMap instead of a HashMap.

Whenever the user-supplied array grows beyond `N`, throw away the entire RBHash block of bytes
and build a new one using a new `N` and `B`.  (just like a typical hash table) For bonus points,
allocate these extra bytes at the same time as you reallocate the array of user elements:

    X *array= (X*) malloc(
      N * sizeof(X)
      + RBHASH_SIZEOF(N, B) // should also be aligned, hopefully sizeof(X) did that for you
    );
    bzero(array + N, RBHASH_SIZEOF(N, B));
    ...
    // Add an element to your array
    array[i]= some_value;
    rbhash_insert(
      array + N,                 // pointer to the extra bytes
      N,                         // Describe the dimensions
      i + 1,                     // Node ID which will be inserted
      calc_hash(some_value) % B, // bucket idx for this new node
      compare_fn,                // Callback that compares nodes
      compare_data               // Data parameter for the callback
    );

The insertion above just changes a few integers in the extra data on the end of the array.
It never allocates things.  It runs somewhere between `O(1)` and `O(log N)` time depending on
how lucky you got with your hash function.
You can then use a similar function to perform `O(1) .. O(log N)` time lookup to see if the
value already exists in the array, or delete the reference to the element.  It's important to
note that many poplar hash table implementations *cannot* efficiently delete elements.

If you re-allocate the array, you need to allocate a new block of RBHash bytes, initialize them
to zero, and repeat all the inserts.

Benefits:
  * Best-case performance of `O(1)` for insert/delete/find, like a hash table
  * Worst case performance of `O(log N)`; no need to randomize the hash function vs. attackers
  * You choose the size of the hash table and hash function to suit your needs.
    Larger table gets you closer to `O(1)` on average and smaller closer to `O(log N)`.
  * Doubles as a library for Red/Black trees by choosing `B = 1`, giving you sorted elements
  * Can use a fast hash function instead of a "good" one.
  * All the benefits of an array of elements (like preserving original order)
  * No memory management overhead vs. what you already do for a vector of elements
  * Constructor is nothing more than zeroing the memory
  * No destructor
  * Doesn't need to be part of the user data array; any contiguous block of memory works
  * Build temporary ones on the stack, for small arrays passed to you
  * Can be statically allocated in low-memory microcontrollers for a global map/set of things
  * Perfect for small maps like typical JSON objects

Drawbacks:
  * No way to break the array into smaller chunks, or re-use past allocations
  * Reallocations cost `O(N) .. O(N log(N))` time (same drawback as a hash table)
    and you have to code them yourself, since the RBHash knows nothing about the user array.
  * Loses efficiency appeal when dynamically growing beyond 32K elements, when R/B nodes grow
    to 8 bytes each and reallocation costs start to add up.
    (but probably still smaller than any structure using 64-bit pointers)
  * Changing a Node ID would cost `O(log(N))` each, like if you want to shift a range of array
    elements (same drawback as deleting from an array of elements).  But, you are in control
    of the array and can replace it with any equivalent data structure, like a ring buffer, or
    [array with tombstones](https://blog.toit.io/hash-maps-that-dont-hate-you-1a96150b492a)
    so long as you keep track of which element goes with which Node IDs.

## Building

The library is written as a [cpppp template](/nrdvana/RBHash/blob/main/rbhash.cpppp).
Use the [cpppp tool](https://metacpan.org/pod/CodeGen::Cpppp) to convert that
to `rbhash.h` and `rbhash.c` with the parameters of your choosing:

    cpppp rbhash.cpppp \
     -p min_bits=8 \
     -p max_bits=32 \
     --section-out public=rbhash.h \
     -o rbhash.c

(coming soon) You will also be able to create type-safe wrappers with the tree
comparison function inlined, such as would be done by `std::map`.

### Parameters

  * namespace:           This selects the prefix added to all symbols in the
                         library.  The default is 'rbhash'.
  * min_bits / max_bits: The library dynamically chooses integers to accommodate
                         the `capacity` of your container.  If you know your
                         collections will seldom have fewer than 127 elements or
                         never exceed 2^31 elements, you can choose a narrower
                         range for min/max to reduce the number of conditional
                         tests generated.
  * feature_print:       Whether to include the `rbhash_print` function, which
                         prints the structure of a rbhash to a `FILE*` stream.
  * feature_demo:        Whether to generate a main() function that gives an
                         interactive commandline demo of the algorithm.

### Output

The headers are written to a section named `public` and the implementation is
written to the section `private`.  You can direct those out to be their own
files, or specify that they be spliced into existing files.

  * -o SECTION=FILENAME:        Direct SECTION into FILENAME
  * -o SECTION=MARKER@FILENAME  Insert SECTION into FILENAME between
                                "BEGIN MARKER" and "END MARKER"
  * -o FILENAME:                All (remaining) output goes to FILENAME

