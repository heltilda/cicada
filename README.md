Cicada is a lightweight scripting language that runs inside of C code.  For details, see the website:

http://heltilda.github.io/cicada<br><br>

<ins>Installation</ins>: From the command prompt go into the Cicada download directory, and type:

`> ./configure`

`> make`

`> make install`<br><br>

<ins>How to use</ins>: Include the Cicada header file in your C code:

`#include <cicada.h>`

and pass an `lcicada` option to the linker:

`gcc -lcicada -o myprogram ...`

The simplest way to run Cicada is to call:

`runCicada(NULL, NULL, true);`
