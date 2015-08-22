#pragma once

#include <stdio.h>

/*Invoke a program.
   - Synchronously passes control to the program and waits for it to finish.
   - Piped creates a pipe from the stdout of the program and returns it as a FILE.
  argv contains the program name, the arguments, and finally a null-terminator.*/
bool invokeSyncronously (char** argv);
FILE* invokePiped (char** argv);
