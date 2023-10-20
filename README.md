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
- [x] get rid of stdbool.h (it's a C99 extension!)
- [ ] test-compile with clang and some other C compiler
- [ ] loops / tail recursion
- [ ] turn on -Wpedantic and fix every issue that arises
- [ ] custom width variables?
- [ ] `)else(` on if statement
- [ ] I/O primitives (probably a `read` and `write` which take both a
  fd, a buf and the max len )
- [ ] fix bug: anything after >v VARNAME is ignored??
- [ ] lambdas (anonymous words)
- [ ] dynamic word calling (for lambdas)
- [ ] structs/tables/whatever (how would that work)
- [ ] C integration?? :))
- [ ] fibers? (how would that work? interpret code from each fiber alternatively?)
