/* Global variables definition
 *
 * 4/29/20, JohnHenry Ward
 */


#pragma once

/* Preprocessor defines for definition and initialization */

#ifdef DEFINE_GLOBALS

#define GLOBAL_VAR(type, name, init) extern type name ; type name = init

#else

#define GLOBAL_VAR(type, name, init) extern type name

#endif


/* Global Variables */

GLOBAL_VAR (int, mainargc, 0);

GLOBAL_VAR (char **, mainargv, 0);

GLOBAL_VAR (int, currindex, 2);

GLOBAL_VAR (int, og_argc, 0);

GLOBAL_VAR (int, exitVal, 0);
