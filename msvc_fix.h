#ifdef _WIN32
#   include <winsock2.h>
#   include <windows.h>
/* Undef Windows API macros that conflict with FontFile.cpp */
#   ifdef GetCharWidth
#       undef GetCharWidth
#   endif
#endif
