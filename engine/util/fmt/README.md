{fmt}
=====

{fmt} is an open-source formatting library for C++. It can be used as a safe and fast alternative to (s)printf and IOStreams.

Source: https://github.com/fmtlib/fmt

How to use:
-----------
```
std::string s = fmt::format("{0}{1}{0}", "abra", "cad");
// s == "abracadabra"
```

SimC Integration:
-----------------
* Release 5.2.1 from https://github.com/fmtlib/fmt/releases/tag/5.2.1
* rename .cc files to .cpp for simpler integration into our build systems.
