This is a toy GC I built in a couple afternoons.  The main thing I wanted to
experiment with here was not the GC algorithm (which happens to be Cheney,
because that's what I knew how to write) but to implement a handle class which
wraps pointers into the managed heap.

That turns out to be easy, and the structure is dead-simple.  The handle
constructor tells the GC about a stack-allocated pointer, and passes it the
address of the pointer on the stack such that the GC can overwrite it at
collection time.  When the handle goes out of scope, the destructor
un-registers that pointer with the GC, because it's no longer reachable from
the stack.  Apparently this is a very common pattern in commercial GCs.

The remaining problem to be solved is related to the GC itself.  How do you
heap allocate a list of heterogeneous elements (array of discriminated union
values) and, during collection, traverse any pointers the list might contain?


This problem has solutions.


1) The dynamic solution:

   For each element, allocate a header that encodes the offset of each pointer
   in the value.  Then the collector can loop over them, move the children, and
   update the pointer in the original element.  Requires some (probably) basic
   metaprogramming to compute offsets for arbitrary structs.  Process is recursive.

   Pro: Easy to understand; the implementation (outside the metapgrogramming) is ~20 lines.

   Con: Inefficient. Lists have memory overhead per-element, and without
        clever offset encoding would have a max number of pointer members.


2) A static solution:

   Use metaprogramming to generate code that reads the type tag of each element
   in the list, then traverses the pointers.

   Pro: Code would be straight-forward to read and debug.

   Pro: Efficient. List allocations only have a header per list, and the
        element traversal is as fast as it could be.

   Con: Generating that code would be difficult from C++ templates, so I'd
        have to do the metaprogramming from poof.  Would be a bit of work to
        port the current C++ code back to something it would understand.

   Con? Pro?: I'd have to actually get poof into a more-finished state.

   Con: The actual GC loop would probably have to be generated, which might get
        confusing.  Could start with a middle-ground of generate the
        traversal/collection code, but manually write the loop that calls it.
        This would make adding new containers manual, but very easy.  Once it
        started to take shape generating the loop would be easy.


3) A static solution:

   Hand-write code to collect only specifc types of structures.

   Pro: Simplest in terms of toolchain.

   Pro: Efficient on memory and CPU.

   Con: Boring; error prone. Manually writing code we could generate statically.

   Con: We'd be limited to collecting containers we've implemented collection for.


4) Something I haven't thought of yet.. ?
