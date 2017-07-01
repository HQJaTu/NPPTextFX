#pragma once
#include <tchar.h>

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

EXTERNC unsigned meminvertcase(TCHAR *str, unsigned destlen);
EXTERNC unsigned memsentencecase(TCHAR *str, unsigned destlen);
EXTERNC unsigned mempropercase(TCHAR *str, unsigned destlen);
EXTERNC unsigned memuppercase(TCHAR *str, unsigned destlen);
EXTERNC unsigned memlowercase(TCHAR *str, unsigned destlen);
