---
title: Coding Guidelines
---


# Coding Style
## Basic Rules
* Use two spaces for indentation. Do not use tabs.
* Use [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) date-time formatting.

  SimC is used and developed around the world, and there has been enough confusion between American and European date formats in the past.

# Guidelines
* **Readability**

  Because SimulationCraft is a open-source project mainly developed by hobby programmers, we try to focus strongly on code readability. Always try to make your code as easy to read as possible and don't hold back on white-space.

* **Comments**

  Try to add comments for unintuitive or complex code. Not only does it help other/new developers to understand the meaning and intention of a code block, but also your future self.

  Try to document non-intuitive game mechanics and bugs with information on who tested/implemented it and add a date to it.

* **Hardcoded numbers**

  Avoid hard coded numbers if possible. If they are absolutely necessary, document them and include a date.

# Clang format
To easily format the code you write, you can run clang-format on files you are working on. The current ruleset can be found in the `.clang-format` file in the root directory of simc. 

To use this in VSCode you can follow these steps:
1. Install the [CPP extension](https://code.visualstudio.com/docs/languages/cpp) for VSCode
2. Install [LLVM](https://releases.llvm.org/download.html) to your machine (make sure to Add LLVM to the system PATH for all users when installing)
3. Format Document inside of VSCode now applies the clang-format rules

Please note that not all files in the project are currently formatted with this rulset. In particular some class modules have their own way of formatting. 
Please do not mix actual code changes with large automated reformatting. If you intend to reformat a whole file/module you are working on, create a separate commit containing only the format changes.
