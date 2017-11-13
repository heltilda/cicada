Cicada script is a simple, interpreted language that can be extended with the user's own C/C++ code.  For details, see the website:

http://heltilda.github.io/cicada

Building Cicada requires a machine with a command prompt, a C or C++ compiler and the 'make' tool.

If the C++ compiler is used:
* change the '.c' file suffix of all of Cicada's source files to '.cpp'
* delete the default 'Makefile', then rename 'Makefile_cpp' --> 'Makefile'

To compile:
1. Open the command prompt.
2. Navigate to the directory in which Cicada's source files are located.
3. Enter the command "make cicada".

To run, use a command prompt to navigate to Cicada and type "/.cicada" (UNIX) or just "cicada" (DOS).