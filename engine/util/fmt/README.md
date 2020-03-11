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
* Release 6.0.0 from https://github.com/fmtlib/fmt/releases/tag/6.0.0
* rename .cc files to .cpp for simpler integration into our build systems.
* applied workaround for VS2019 bug: https://github.com/fmtlib/fmt/pull/1328/commits/35658bf42d6fc3eb2b3b5f78f85a0abd8f9b6817