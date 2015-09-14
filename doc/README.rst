$project documentation
---------------------

## Doxygen
Creates automatic source code documentation in the following format
* html: good as a API reference
* XML: used for Sphinx/Breathe

## Sphinx
A rst -> html documentation generator.
Parses all manuall .rst files inside the doc/ folder

## Breathe
A Sphinx extension which allows combining doxygen XML output to be imported into Sphinx.
- Not yet working, memory problems

## Problems so far:
* babel module used by sphinx has problem on python3/windows10: pip install git+https://github.com/mitsuhiko/babel.git@2.0
* breathe runs into MemoryError when just importing a simple SimC class from doxygen (at least with 32bit python).