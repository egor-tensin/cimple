# Hopefully putting the NULL reference to a separate library should force the
# sigsegv binary to crash. If I put it in main.c, surprisingly, Clang (with
# -O2) produces a binary that doesn't crash: https://godbolt.org/z/joeqEEs3Y.
# To force it to crash, the -fno-delete-null-pointer-checks flag is required:
# https://godbolt.org/z/o3xEoeG3z. This is a bit hacky though; putting the
# reference in a separate library should also do the trick.

add_library(null lib.c)

add_executable(sigsegv main.c)
target_link_libraries(sigsegv PRIVATE null)
