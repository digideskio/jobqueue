# JobQueue Prototype for Learning

I needed a job queue for a project. While I still plan on either getting one off
the shelf or otherwise doing something smarter. I figured I could prototype one
in an afternoon to see how it might work. This is just that. The code is a bit
messy, the word prototype is used in the title of this readme for a reason, etc.

## Building

My `Makefile` is specifically for `clang` because I was being pretty lazy.
Theoretically any C++11 compiler could compile this, and most of my switches are
just to generate warnings to help me catch errors. Symlink an implementation
into `main.cpp` such as `ln -s array-swap.cpp main.cpp` then run `make` and it
will build and run that implementation.

## The Implementations

I've written a few implementations so far as I've been learning and trying
things. These aren't really meant to be benchmarked against eachother as they
have more tradoffs than just their raw performance. If I'm feeling ambitious,
I'll see about writing a test suite that tests them all with the same input,
which could be used to extrapolate some performance info if you are so inclined.

### pointer.cpp

This implementation uses a sparse array of pointers to structs to hold the jobs.
This was done for simplicity sake. No need to manage multiple arrays, do any
moving, sorting, etc. This was to nail down the core concept that I'd use for
futher implementations. As I wrote this one, I knew I'd need to switch to array
of structs for performance reasons. Those reasons being array of pointers means
the random memory access instead of sequential (defeats the CPU cache) and
memory fragmentation since they are being allocated where ever.

### sorted-insert.cpp

Also know as "wraithan does battle with memset/memmove". I had many `SIGABRT`
and `SIGSEGV` while writing this. Mostly from cases of swapping the source and
destinations on accident. I seem to be more used to `source, dest` APIs instead
of `dest, source` APIs. Aside from that, this was a super fun implementation to
make. I had to really think about each function but once I had them all working
it was pretty neat.

This one has a much more expensive insert due to the copy, unless you are adding
a job that comes after the currently queued ones. If so then inserts are O(1).
Otherwise inserts are O(n). Memory is all in one array so I know I'm getting
cache line advantages, but with all the extra copying around, I'm sure I'm
consuming a fair amount of that advantage with extra CPU instructions.

Over fun to build, but the complexity makes me hesitant to use something like
this in real life. Would need a test suite and deeper code review before I'd
feel safe.

### array-swap.cpp

This implementation was not my idea. A coworker mentioned this because the
arrays should be adjacent (and thinking about it, I could actually force that by
allocating double the size then grabbing a pointer to the middle for the second
queue). It has a pontential issue if there are adds while update is running
where jobs could be dropped. I am going to think more about this and see if I
can't do something better around that.

Overall this implementation feels pretty great. Much less complex than the
`sorted-insert.cpp` and probably just as fast updates and all inserts are O(1).

### Future

* I might try building one with a dynamically sized array instead of using fixed
  size arrays.
* Would like to write a test suite for this so I can ensure things are called in
  the correct order.
* Rust implementations would be fun, both safe and unsafe ones.
* Any ideas you have!

## License

ISC but keep in mind, these are prototypes that are untested other than a little
visual inspection of their logging.
