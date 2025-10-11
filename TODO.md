# TODO

Some things that I know I have to do someday:


# Someday

- Make it so we can change the current directory when using bazel run to the one where bazel is running.
    - Should not do it for tests though.
- Replace Optional someday https://github.com/Sedeniono/tiny-optional
- Rename file.h to io.h
- Move ArenaPush* functions to return std::span rather than a pointer.
- Have a function that removes a lot of the preamble from std::source_location when the path is of the project
    - Our stack trace already does this, so should be easy.
