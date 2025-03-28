# forthquick

A dirty C89 implementation of a VM-based Forth.

## roadmap

- [x] (old) python3 version
- [x] port to c89
- [x] create words with `:`
- [x] comments with `(` and `((`
- [x] `if(` statement
- [x] simplify variables (they become simply words that return adresses)
- [x] floating point operations
- [x] turn on -Wpedantic and fix every issue that arises
- [x] get rid of stdbool.h (it's a C99 extension!)
- [x] test-compile with clang and some other C compiler
- [x] actually use return stack (I forgot that!!)
- [ ] loops / tail recursion
- [ ] very crappy malloc and free functions
- [ ] I/O primitives (probably a `read` and `write` which take both a fd, a buf and the max len )
- [ ] custom width variables?
- [ ] `)else(` on if statement
- [ ] how to handle nested if statements?
- [ ] fix bug: anything after >v VARNAME is ignored??
- [ ] lambdas (anonymous words)
- [ ] dynamic word calling (for lambdas)
- [ ] structs/tables/whatever (how would that work)
- [ ] C integration?? :))
- [ ] fibers? (how would that work? interpret code from each fiber alternatively?)
- [ ] else-if (how to do this)
