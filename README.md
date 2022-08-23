# Crelude

A personal C (C11) prelude / standard library.

For enhanced C syntax highlighting whilst using this library, consider using my
[`.vim/after/syntax/c.vim`](https://github.com/Demonstrandum/Dotfiles/blob/master/.vim/after/syntax/c.vim).

## Build and Install

```shell
make
PREFIX=/usr/local make install
```
or similar.

Generates `tests` executable and `libcrelude.so`.
Tested on GNU+Linux and macOS, on both x86_64 and aarch64 (ARM) architectures.

## TODO

- [ ] Completely implement the standard libc printf formatters.
- [ ] Cut down on GNU extensions.
