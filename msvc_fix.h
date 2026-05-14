#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#   include <winsock2.h>
#   include <windows.h>
/* Undef Windows API macros that conflict with FontFile.cpp */
#   ifdef GetCharWidth
#       undef GetCharWidth
#   endif
#endif
