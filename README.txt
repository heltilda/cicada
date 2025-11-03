Cicada is a minimal scripting language that talks to C.  For details, see the website:

http://heltilda.github.io/cicada

Install Cicada from the command prompt in the usual way, using the following commands:

`> ./configure`
`> make`
`> make install`

To link against the Cicada library, include the header file in your C code:

`#include <cicada.h>`

and pass an 'lcicada' option to the linker:

`gcc -lcicada -o myprogram ...`
