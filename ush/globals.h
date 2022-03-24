
#pragma once
/* C preprocessor defines for definition and initialization */

#ifdef DEFINE_GLOBALS

#define GLOBAL_VAR(type, name, init)  extern type name ;  type name = init 

#else

#define GLOBAL_VAR(type, name, init) extern type name

#endif

#define WAIT 1
#define NOWAIT 2
#define NOEXPAND 3


/* Actual global variables */

GLOBAL_VAR(int, argc, 0);
GLOBAL_VAR(char, **argv, NULL);
GLOBAL_VAR(int, shift, 0);
GLOBAL_VAR(int, waitstat, 0);
