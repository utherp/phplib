#pragma once

#ifdef _DEBUG_
    #ifdef USE_FASTCGI
        #include <fcgi_stdio.h>
    #else 
        #include <stdio.h>
    #endif
    #include <unistd.h>

    #ifdef _DEBUG_FILENAME_
        #define _DEBUG_MAIN_
        #define _DEBUG_HANDLE_ NULL
        #define _DEBUG_INIT_ LOG_OUTPUT = fopen(_DEBUG_FILENAME_, "a");
    #endif

    #ifdef _DEBUG_MAIN_
        #ifndef _DEBUG_HANDLE_
            #define _DEBUG_HANDLE_ stderr
        #endif
        #ifndef _DEBUG_INIT_
            #define _DEBUG_INIT_ LOG_OUTPUT = _DEBUG_HANDLE_;
        #endif
        FILE *LOG_OUTPUT;
    #else
        extern FILE *LOG_OUTPUT;
    #endif

    #define _debug(format, ...) \
        fprintf(LOG_OUTPUT, "[%d] DEBUG: (" __FILE__ ")[%u]: " format "\n", \
                getpid(), \
                __LINE__ , \
                __VA_ARGS__ \
        ); \
        fflush(LOG_OUTPUT)
#endif

#ifndef _DEBUG_INIT_
    #define _DEBUG_INIT_
#endif

#ifndef _debug
    #define _debug(format, ...) 
#endif
