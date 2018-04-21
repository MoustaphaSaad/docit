# Docit
Docit is a C++ documentation extraction tool.

## Example
Check out the [example.cpp](example.cpp) file and see the generated [example.md](example.md) document.

## Rules

1. Anything preceded with the following style of comments

   1. ```C++
      /**
       * This is by default a documentation comment
       */
      ```


2. We support these list of tags in the comment
   1. @brief
   2. @param[in]
   3. @param[out]
   4. @param
   5. @tparam
   6. @return


3. You can use any markdown syntax in the comments

4. You can insert any markdown comments anywhere in the document using this style

   1. ```C++
      /**
       * [[markdown]]
       * by adding this tag anywhere in the comment
       */
      ```

5. Any variable/function/type prefixed with an _ will be considered private and won't be included in the documentation even if it's documented

6. Any undocumented variable/function/type will not be included in the documentation.

## Usage

```
docit.exe example.cpp -std=c++14 > example.md
```

You can use docit directly from the command line the output will be printed out in the standard output so if you want to save it to a file you can simply direct it to a file as it's demonstrated above.

## Build

```
cd docit
premake5 vs2017
```

**Note:** On Windows you have to have a `LLVM_DIR` environment variable pointing to llvm installation directory

This is only tested on windows but I don't see why it won't work for Linux but make sure you have libclang installed first


## License
**docit** is released under the GNU Lesser General Public License version 3.

