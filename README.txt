Cicada script is a simple, interpreted language that can be extended with the user's own C/C++ code.  For details, see the website:

http://heltilda.github.io/cicada

Building Cicada requires a machine with a command prompt, a C or C++ compiler and the 'make' tool.

To compile using the Make tool:
1. Open the command prompt.
2. Navigate to the directory in which Cicada's source files are located.
3. Compile using the command "make cicada", if using the C++ compiler.
  * If using the C compiler (i.e. if we're embedding our own C code), use the command "make cicada CC=gcc" instead.

To run, use a command prompt to navigate to Cicada and type "/.cicada" (UNIX) or just "cicada" (DOS).
