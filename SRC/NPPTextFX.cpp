//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//http://www.gnu.org/copyleft/gpl.html
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Notepad ++ http://notepad-plus.sourceforge.net/uk/site.htm
// Scintilla: http://www.scintilla.org/
//
// To install this plugin, copy the DLL and any required files to the
// plugins folder in the Notepad++ install folder.
//
// NPPTextFX (C) 2005-2007 by Chris Severance

// What can you do to help?
//  Look for bugs in existing functions.
//  Expand routines for more functionality and more specific programming language support.
//  Think of new esoteric things that are hard or impossible with Copy-Paste or regex Search-Replace
//    you want done in an editor and add it. If you're lucky, you can reuse lots of existing code.
//  Come up with better titles for menu items.
//  Arrange the menus to make more sense.
//  Hassle Don Ho until he adds the features we need to make better plugins.
//  Hassle Don Ho to put most of these functions into his editor. Many of these things shouldn't be plugins.
//  Adapt functions for UNICODE.
//  Improve the help texts.
//  Make it run on more compilers.
//  Make the examples in the demo text file better
//  Beg other Notepad writers to make a plugin system and adapt this code to it.
//  Take functions that now use clipboard text because it's all I can get to and write proper dialog inputs for them.
// "new feature" is found throughout the code where new features have been thought of but not implemented

// Users wanting to run MS Visual C++ 2003 Free must download both the toolkit and the SDK
// Install the SDK base option only and move include & lib over to the include&lib in the Visual Toolkit folder.
// http://msdn.microsoft.com/visualc/vctoolkit2003/
// http://www.microsoft.com/msdownload/platformsdk/sdkupdate/ (I prefer the ISO install)
// http://www.microsoft.com/whdc/devtools/debugging/installx86.mspx

// The project compiles easiest when placed in the Notepad ++ plugins folder

// Quite a few goto's are used in this code. I strive to remove gotos
// but I use them when they are more clear and concise than the alternatives.
// Modern compilers seem to optimize around them just fine.
// Donald Knuth's "Structured Programming with go to Statements"
// http://www.stevemcconnell.com/ccgoto.htm

/* This was designed as a C file but only MinGW and Digital Mars handle variable declarations within a function for .c sources. */
/* Switching to .cpp opened up the rest of the compilers */

/* Indent Lines Sticky Margin was my first routine that could go low/high performace so I had it GetTickCount() with NPPTextFX.CPP==178K on a P3-500.
   High,Low,Size,Time Performace // few optimizations turned on
  1800,2600,108K,10s Borland-C (NPPDEBUG=0) // merge duplicate strings
  1500,4300,108K,10s Borland-C (NPPDEBUG=1) // merge duplicate strings
  1600,1700,110K, 4s DMC       (NPPDEBUG=0) // This is pretty impressive for a work-in-progress compiler
  2200,4800,110K, 4s DMC       (NPPDEBUG=1)
  3200,5800,110K,25s MSVC      (NPPDEBUG=1)
  5800,8100, 52K,37s MinGW     (NPPDEBUG=1) // optimizations turned off
  5600,9500,106K,14s OpenWatcom (NPPDEBUG=1)
  Borland-C & DMC may have a larger runtime size but they are blowing the other compilers away for runtime speed and compile times.
  Borland-C has the most useful warnings.
  DMC will be used for releases from now on.
  Looks like switching to DMC will give me much greater performance gains than converting any more functions to dual buffers high performance.
*/

// Exclude rarely-used stuff from Windows headers
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h> /* DMC: Needs to have it's includes and libs updated to get shlwapi.h */
#include <shlobj.h>  /* MSVC: needs shlwapi.h copied from MS Visual Studio to work */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#ifndef UNICODE
#error "Need Unicode!"
#endif

#include "PluginInterface.h"
#include "../minilzo/minilzo.h"
#include "enable.h"
#include "Transforms.h"
#include "MicroXML/MicroXML.h"


#if defined(__POCC__)
#pragma warn(disable:2216 2209 2030) /* Return value from function is never used; Unreachable code removed; = used in a conditional expression */
#endif

#if defined(__POCC__) /* Pelles C */ || defined(_MSC_VER)
#define itoa _itoa
#define memicmp _memicmp
#endif

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

#define NPPTextFX_DELAYUNSAFE
#include "NPPTextFX.h"
#define PLUGIN_VERSION "0.25"
// This is the name which will be displayed in Plugins Menu
#define PLUGIN_NAME_MENU NPPTEXT("TextFX")
//This is used through the application
#define PLUGIN_NAME "TextFX"
//#define SUPPORT_PATH "NPPTextFX" /* thanks to smprintfpath we don't need to put a slash on the end of this */
#define TEXTFX_TOP_MENU 1	 // set to 1 for TextFX to be separate, 0 for it to be in Plugins

HINSTANCE g_hInstance;
BOOL g_fOnNT=FALSE;
HANDLE g_fLoadonce=NULL;	// Singleton
UINT g_cfColumnSelect;
struct NppData g_nppData;
//char *g_pszPluginpath=NULL;
#define HighPerformance 0 /* define this to disable adaptive code; remove define to enable adaptive code */
#ifndef HighPerformance
BOOL HighPerformance;
#define HighPerformanceon 0 /* an if() can use HighPerformanceon if it wants to be on when HighPerformance is not adaptive */
#else
#define HighPerformanceon 1
#endif

#define LaunchURL(lurl) ShellExecute(g_nppData._nppHandle, _T("open"), lurl, NULL, _T("."), SW_SHOW)

//----------------------------------------------------------//
// Here're the definition of functions for our functionalities
//----------------------------------------------------------//

// SECTION: Beginning of library functions

/* realloc() is incorrectly designed. It leaves the original block intact if a block of the new size can't be allocated.
  This is never useful therefore it's a bad design which we will fix. We will free the original block if a
  new block can't be allocated. */
EXTERNC void *ReallocFree(void *memblock, size_t size) {
  void *rv=realloc(memblock,size);
  if (memblock && size && !rv)
	  free(memblock);
  return rv;
}

#if NPPDEBUG
/* We control all malloc/realloc/free buffers. We add 3 bytes to the buffer size and place
   1 check code at the beginning and 2 at the end. If any of those are modifed then we know
   that the caller has gone beyond the bounds. */
struct _MALLOC_STACK {
  void *buf;
  unsigned len;
  const TCHAR *title,*dummy; /* dummy makes it 16 bytes */
} g_mallocstack[100];

#define SAFEBEGIN sizeof(int) /* resulting buffers must be DWORD aligned if the originals are */
#define SAFEEND sizeof(int)
// wants the fake buf and fake ct
// returns the real buf
EXTERNC void *freesafebounds(void *bf, unsigned ct, const TCHAR *title) {
  if (bf) {
    unsigned i;
    unsigned char *bft=(unsigned char *)bf;
    bft -= SAFEBEGIN;
    for (i=0; i < SAFEBEGIN; i++)
		if (bft[i] != 255-(unsigned char)i) {
			MessageBox(0, _T("Buffer write beyond begin"), title, MB_OK|MB_ICONSTOP);
			break;
		}
    //if (bft[0] != 255)  MessageBox(0,"Buffer went beyond buffer begin",title, MB_OK|MB_ICONSTOP);
    for (i=0; i < SAFEEND; i++)
		if (bft[ct + SAFEBEGIN + (unsigned char)i] != 255-(unsigned char)i) {
			MessageBox(0, _T("Buffer write beyond end"), title, MB_OK|MB_ICONSTOP);
			break;
		}
    //if (bft[ct+1]!=255) MessageBox(0,"Buffer went beyond wrote 1+ buffer end",title, MB_OK|MB_ICONSTOP);
    //if (bft[ct+2]!=254 )  MessageBox(0,"Buffer went beyond wrote 2+ buffer end",title, MB_OK|MB_ICONSTOP);
    bf = (void *)bft;
  }
  return(bf);
}

// wants the real buf and fake ct
// returns the fake buf
EXTERNC void *reallocsafebounds(void *bf,size_t ct) {
  void *bf2;
  bf2=ReallocFree(bf,ct+(SAFEBEGIN+SAFEEND));
  if (bf2) {
    unsigned char *bft2=(unsigned char *)bf2;
    unsigned i;
	for(i=0; i<SAFEBEGIN; i++)
		bft2[i]=255-i;
    //bft2[0]   =255;
    for(i=0; i<SAFEEND; i++)
		bft2[ct+SAFEBEGIN+i]=255-i;
    //bft2[ct+1]=255;
    //bft2[ct+2]=254;
    bft2+=SAFEBEGIN;
    bf2 = (void *)bft2;
    //if (!bf) memset(bf2,'@',ct);
  }
  return(bf2);
}

EXTERNC void freesafe(void *bf, const TCHAR *title) {
  if (bf) {
    unsigned i;
	for(i=0; i<NELEM(g_mallocstack); i++)
		if (g_mallocstack[i].buf==bf) {
			free(freesafebounds(g_mallocstack[i].buf,g_mallocstack[i].len, title));
			//g_mallocstack[i].buf=NULL;
			memset(g_mallocstack+i,0,sizeof(g_mallocstack[0]));
			return;
		}
    MessageBox(0, _T("free() buffer never allocated"), title, MB_OK|MB_ICONSTOP);
    return;
  }

  MessageBox(0, _T("free() NULL buffer"), title, MB_OK|MB_ICONSTOP);
}

EXTERNC void *reallocsafeX(void *bf, unsigned ct, TCHAR *title, int allownull) {
  void *bf2=NULL;
  if (ct) {
    if (bf) {
      unsigned i;
	  for(i=0; i<NELEM(g_mallocstack); i++)
		  if (g_mallocstack[i].buf==bf) {
			  if (ct==g_mallocstack[i].len)
				  MessageBox(0, _T("Buffer realloc() to same size"), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
			  bf2=freesafebounds(g_mallocstack[i].buf,g_mallocstack[i].len,title);
			  goto fnd;
		  }
      MessageBox(0, _T("Buffer realloc() not originally realloc()"), title, MB_OK|MB_ICONSTOP);
      return(NULL);
fnd: ;
    } else if (!allownull)
		MessageBox(0, _T("Attempt to realloc NULL buf"), title, MB_OK|MB_ICONSTOP);
    bf2=reallocsafebounds(bf2,ct);
    if (bf2) {
      unsigned i;
      for(i=0; i<NELEM(g_mallocstack); i++)
		  if (g_mallocstack[i].buf==bf) {
			  g_mallocstack[i].buf=bf2; g_mallocstack[i].len=bf2?ct:0;
			  if (!g_mallocstack[i].title)
				  g_mallocstack[i].title = bf2 ? title : NULL;
			  goto fnd2;
		  }
      MessageBox(0, _T("no more space in malloc stack"), title, MB_OK|MB_ICONSTOP);
    }
  } else if (bf)
	  freesafe(bf,title); // this is different than the spec of realloc()

fnd2:
  return(bf2);
}

#define mallocsafeinit() memset(g_mallocstack,0,sizeof(g_mallocstack))

EXTERNC void mallocsafedone(void) {
  unsigned badct;
  unsigned i;
  for(badct=0, i=0; i<NELEM(g_mallocstack); i++)
	  if (g_mallocstack[i].buf) {
		  badct++;
		  MessageBox(0, g_mallocstack[i].title, _T("mallocsafedone"), MB_OK|MB_ICONSTOP);
		  free(freesafebounds(g_mallocstack[i].buf, g_mallocstack[i].len, _T("mallocsafedone")));
	  }
  if (badct) {
    TCHAR debug[256];
	snprintfX(debug, 256, _T("%u malloc() buffers not free'd\n"), badct);
	MessageBox(0, debug, _T("mallocsafedone"), MB_OK|MB_ICONSTOP);
  }
  mallocsafeinit();
}

#define mallocsafe(ct,ti) reallocsafeX(NULL,ct,ti,1)
#define reallocsafe(bf,ct,ti) reallocsafeX(bf,ct,ti,0)
#define reallocsafeNULL(bf,ct,ti) reallocsafeX(bf,ct,ti,1)
#define malloc malloc_unsafe
#define realloc realloc_unsafe
#define calloc calloc_unsafe /* I never use calloc() but just in case someone tries */
#define free free_unsafe
#define strdup strdup_unsafe

EXTERNC TCHAR *strdupsafe(const TCHAR *source, TCHAR *title) {
  size_t ls=wcslen(source) + 1;
  TCHAR *rv=(TCHAR *)mallocsafe(ls, title);
  if (rv)
	  memcpy(rv,source,ls);

  return(rv);
}

#if 1
#define testmallocsafe()
#else
EXTERNC void testmallocsafe(void) {
  char *test,*test2;
  mallocsafeinit();
  if (test=(char *)mallocsafe(0,"test")) MessageBox(0,"Improper return #1",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  freesafe(test,"test"); // free NULL buf
  if (!(test=(char *)mallocsafe(10,"test"))) MessageBox(0,"Improper return #2",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  memset(test,'@',10);
  if (!(test=(char *)reallocsafe(test,10,"test"))) MessageBox(0,"Improper return #3",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  test[-1]='x'; // buffer beyond bounds
  test2=(char *)reallocsafe(NULL,11,"test2"); // realloc NULL buff
  test2[11]='x';
  freesafe(test,"test"); // no error
  freesafe(test,"test"); // free never allocated
  freesafe(NULL,"test");
  mallocsafedone();
}
#endif

#else
#define mallocsafe(ct,ti) malloc(ct)
#define reallocsafe(bf,ct,ti) ReallocFree(bf,ct)
#define reallocsafeNULL(bf,ct,ti) ReallocFree(bf,ct)
#define freesafe(bf,ti) free(bf)
#if defined(__POCC__) /* Pelles C */ || defined(_MSC_VER)
#define strdupsafe(bf,ti) _wcsdup(bf)
#else
#define strdupsafe(bf,ti) strdup(bf)
#endif
#endif

#if NPPDEBUG
EXTERNC TCHAR *memdupsafe(const TCHAR *source, unsigned ls, TCHAR *title)
#else
#define memdupsafe(bf,ln,ti) memdup(bf,ln)
EXTERNC TCHAR *memdup(const TCHAR *source, unsigned ls)
#endif
{
  TCHAR *rv=(TCHAR *)mallocsafe(ls, title);
  if (rv)
	  memcpy(rv, source, ls);

  return(rv);
}

#if NPPDEBUG
EXTERNC TCHAR *memdupzsafe(const TCHAR *source, unsigned ls, TCHAR *title)
#else
#define memdupzsafe(bf,ln,ti) memdupz(bf,ln)
EXTERNC TCHAR *memdupz(const TCHAR *source, unsigned ls)
#endif
{
  TCHAR *rv=(TCHAR *)mallocsafe(ls + 1, title);
  if (rv) {
    memcpy(rv, source, ls);
    rv[ls]='\0';
  }
  return(rv);
}

EXTERNC size_t roundtonextpower(size_t numo) { /* round to next power */
  size_t pow,num;
  num = numo;
  if (num >= 32768) {
	  pow = 15;
	  num >>= 15;
  }
  else if (num>1024) {
	  pow=10;
	  num >>=10;
  }
  else if (num>128) {
	  pow=5;
	  num>>=5;
  }
  else pow=0;
  while (num > 1) {
	  pow++;
	  num >>= 1;
  }
  num = (size_t)1 << pow;
  if (num < numo)
	  num <<= 1;

  return(num);
}

// returns the number of bytes *dest was changed by so the caller can update their pointers too, including when *dest becomes NULL by accident.
// If you are certain that *dest started non null, you can be sure that it hasn't become NULL if the return value is 0
/* Constant realloc()'s are abusive to memory because realloc() must copy the data to a new malloc()'d area.
   On the other hand, slack space on buffers that will never use it is wasteful.
   Choose strategy to minimize realloc()'s while making good use of memory */
#define ARMSTRATEGY_INCREASE 0 //increase buffer with slack space when necessary; good for a constantly expanding buffer
#define ARMSTRATEGY_MAINTAIN 1 //increase buffer only the minimum amount, buffer will not be reduced if too large; good for a buffer that will be reused with some large and some small allocations
#define ARMSTRATEGY_REDUCE 2   //increase buffer only the minimum amount, reduce buffer if too large; good for buffers of known size or that will only be used once
// If choosing ARMSTRATEGY_REDUCE for a new buffer it is usually better to just use malloc() yourself and not call armrealloc()
// clear=1, memset new space to 0
//size_t g_armrealloc_cutoff=16384; /* sizeof(buf)<=, next size doubled, >, added */
// new feature: ARMSTRATEGY_REDUCE should only reduce when half can be reclaimed, much like ARMSTRATEGY_INCREASE only doubles when necessary
EXTERNC int armrealloc(TCHAR **dest, size_t *destsz, size_t newsize, int strategy, int clear
#if NPPDEBUG
,TCHAR *title
#define armreallocsafe armrealloc
#define THETITLE title
#else
#define armreallocsafe(dt,ds,ns,st,cl,ti) armrealloc(dt,ds,ns,st,cl)
#define THETITLE "armrealloc" /* the macros discards this */
#endif
) {
  unsigned rv=0, oldsz= (*dest) ? (*destsz) : 0;
  TCHAR *dest1;

  if (strategy==ARMSTRATEGY_REDUCE && *destsz != newsize) {
	  *destsz = newsize;
	  goto doit; // without this goto, the conditionals were so convoluted that they never worked
  }
  if (strategy==ARMSTRATEGY_INCREASE){
    if (newsize < 64)
		newsize=64;
    if (!*dest && newsize < *destsz)
		newsize=*destsz;
  }
  if (!*dest || *destsz <  newsize) {
    *destsz = (strategy==ARMSTRATEGY_INCREASE) ? roundtonextpower(newsize) : newsize;
doit:
    dest1=*dest;
    dest1= (dest1) ? (TCHAR *)reallocsafe(dest1, *destsz, THETITLE) : (TCHAR *)mallocsafe(*destsz, THETITLE);
      //if (dest1) strcpy((dest1+*destsz-1),"#$"); // $ is beyond string
	rv = dest1 - *dest;
    if (dest1 && clear && *destsz>oldsz)
		memset((char*)dest1+oldsz,0,*destsz-oldsz);
    *dest = dest1;
  }

  return(rv);
}
#undef THETITLE

/* strcpyarm() append-realloc-malloc; This function is designed to
   continually add onto the end of a string. It copies source to *dest+*destlen
   and *destlen += strlen(source). If the new data including \0 would exceed *destsz, the buffer is realloc()'d
   larger and *destsz will be updated with the new size. One of the handy side affects of the arm routines
   is that buffers are no longer created on the stack where they can endanger other stack variables.
   Hackers will find it difficult to overrun a buffer and when they do, all they will trash is the heap.
   returns the number of bytes *dest was changed by during a realloc()
   if source==NULL then no action is taken and a 0 is returned
   *dest must be NULL or a malloc()'d pointer. *dest must not be a stack or data pointer
   if *dest==NULL, a new buffer will be malloc()'d to hold source, *destlen will start at 0
     you can force the new buffer to be larger than source by setting *destsz to the desired starting size
     *destsz should be set to 0 if you do not wish to specify a size for the initial buffer
   if *dest is not NULL, *destsz is always the actual size of the
     malloc()'d buffer so should never be altered while in use unless you free() and reallocate *dest yourself.
   destlen==NULL disables append
     source will always be placed at *dest
     the buffer will always be allocated to it's minimum possible size
     when destlen!=NULL, the buffer will usually have slack space on the end for more things to be added
   dest,destsz cannot be NULL
   On rare occasion, a realloc() will fail and a once valid *dest may be sent back as NULL,
     when this happens *destsz,*destlen will be left at their most recent values though
     they are no longer valid. The next call will start *destlen at 0 and will attempt to allocate *destsz or strlen(source) bytes of memory depending on destlen
   maxlen is the maximum number of characters to copy from source, or (unsigned)-1 to copy all characters. */
// someday we may need a strncpyarmfree() which frees source after it is used
EXTERNC int strncpyarm(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *source, size_t maxlen
#if NPPDEBUG
, TCHAR *title
#define strncpyarmsafe strncpyarm
#define strcpyarmsafe(buf,bufsz,bufl,scsrc,ti) strncpyarm(buf,bufsz,bufl,scsrc,(unsigned)-1,ti)
#define THETITLE title
#else
#define strncpyarmsafe(dt,ds,dl,st,ml,ti) strncpyarm(dt,ds,dl,st,ml)
#define strcpyarmsafe(buf,bufsz,bufl,scsrc,ti) strncpyarm(buf,bufsz,bufl,scsrc,(unsigned)-1)
#define THETITLE "strncpyarm" /* the macros discards this */
#endif
) {
  size_t destlen1,slen;
  int rv=0;
  if (source) { // maxlen==0 is perfectly valid
    slen = wcslen(source);
    if (slen > maxlen)
		slen = maxlen;
    destlen1 = (*dest && destlen) ? *destlen : 0;
    if (*dest && !destlen1 && *destsz!=slen+1) {
      freesafe(*dest, THETITLE);
      *dest=NULL;
      *destsz=slen+1;
    }
	rv = armreallocsafe(dest, destsz, CHARSIZE(destlen1 + slen + 1), destlen ? ARMSTRATEGY_INCREASE : ARMSTRATEGY_REDUCE, 0, THETITLE);
    if (*dest) {
      if (slen) {
        memcpy(*dest+destlen1, source, CHARSIZE(slen));
        destlen1 += slen;
      }
      *(*dest+destlen1)='\0';
      if (destlen)
		  *destlen=destlen1;
    }
  }
  return(rv);
}
#undef THETITLE

EXTERNC int memcpyarm(void **dest, size_t *destsz, size_t *destlen, const TCHAR *source, size_t slen
#if NPPDEBUG
, TCHAR *title
#define memcpyarmsafe memcpyarm
#define THETITLE title
#else
#define memcpyarmsafe(dt,ds,dl,st,ml,ti) memcpyarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
) {
  size_t destlen1;
  int rv=0;
  if (slen == (unsigned)-1) slen= wcslen(source);
  if (source) { // slen=0 must at least allocate memory
    destlen1=(*dest && destlen)?*destlen:0;
    if (*dest && !destlen1 && *destsz!=slen) {
      freesafe(*dest, THETITLE);
      *dest=NULL;
      *destsz=slen;
    }
    rv=armreallocsafe((TCHAR**)dest, destsz, destlen1+slen, destlen ? ARMSTRATEGY_INCREASE : ARMSTRATEGY_REDUCE, 0, THETITLE);
    if (*dest) {
      if (slen) {
        memcpy((char*)*dest+destlen1,source,slen);
        destlen1 += slen;
      }
      if (destlen) *destlen=destlen1;
    }
  }
  return(rv);
}
#undef THETITLE

EXTERNC int memsetarm(TCHAR **dest,size_t *destsz,size_t *destlen,int chr,size_t slen
#if NPPDEBUG
, TCHAR *title
#define memsetarmsafe memsetarm
#define THETITLE title
#else
#define memsetarmsafe(dt,ds,dl,st,ml,ti) memsetarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
) {
  size_t destlen1;
  destlen1=(*dest && destlen)?*destlen:0;
  if (*dest && !destlen1 && *destsz!=slen) {
    freesafe(*dest, THETITLE);
    *dest=NULL;
    *destsz=slen;
  }
  int rv=armreallocsafe(dest, destsz, CHARSIZE(destlen1 + slen), destlen ? ARMSTRATEGY_INCREASE : ARMSTRATEGY_REDUCE, 0, THETITLE);
  if (*dest) {
    if (slen) {
      memset(*dest+destlen1,chr,slen);
      destlen1 += slen;
    }
    if (destlen) *destlen=destlen1;
  }
  return(rv);
}
#undef THETITLE

/* snprintfX() attempts to perfectly duplicate what sprintf() does
   without the unsafeness or undesirable behaviour of sprintf()
   or the bugs and problems of various snprintf()'s
   Call: snprintfX(buf,sizeof(buf),"format string",...);
   Returns: number of non null characters written.
   Unlike the official snprintf():
     * snprintfX() writes no more than sizeof(buf)-1 non null characters plus a guaranteed \0 ending.
     * snprintfX() always returns the exact number of non null characters written
     * snprintfX() does not return a -1 if an error occurs or more buffer space is needed
     * snprintfX() does not return the total number bytes that would be
         needed to store the entire string when there is not enough space
   If you need any of that functionality, you must call the real functions and deal with the bugs as vsarmprintf() does */
EXTERNC size_t snprintfX(TCHAR *buffer, size_t buffer_chars, const TCHAR *format,...) {
  size_t rv=0;
  va_list ap;
  if (buffer && buffer_chars) {
    va_start(ap, format);
    buffer[0]='\0';
    rv = _vsnwprintf(buffer,buffer_chars,format,ap);
    buffer[buffer_chars-1]='\0';
    if (rv==(unsigned)-1)
		rv = wcslen(buffer);
    if (rv>=buffer_chars) rv=buffer_chars-1;
    va_end(ap);
  }
  return(rv);
}

/* sprintf() using the same buffer system as strncpyarm()
   see strncpyarm() for parameter meanings
   http://libslack.org/manpages/snprintf.3.html
   smprintf() handles the broken vsnprintf() implementations that return -1 when there isn't enough space
     or ones that ensure or don't ensure a \0 at the end
   The v...() routines are rarely used directly. They are made so the non v routine wrappers and custom wrappers can be built.*/
EXTERNC size_t vsarmprintf(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *format, va_list ap2) {
  int vs = 0;
  size_t destcnt1, destsz1 = *destsz;
  TCHAR *dest1=*dest;
  va_list ap;

  destcnt1 = destlen ? *destlen : 0;
  if (!dest1) {
    destcnt1=0;
    if (destsz1 < 8)
		destsz1 = 8;
    dest1=(TCHAR *)mallocsafe(CHARSIZE(destsz1), _T("vsarmprintf"));
  }
  if (dest1 && destsz1 < 8 + destcnt1) { //guarantee at least 8 characters into which _vsnwprintf() can write
    destsz1 = 8 + destcnt1;
    dest1=(TCHAR *)reallocsafe(dest1, CHARSIZE(destsz1), _T("vsarmprintf"));
  }

  goto bottest;
  while (dest1 && destsz1 &&
	  !(dest1[destsz1-1]='\0') &&
	  ((vs = _vsnwprintf(dest1 + destcnt1, destsz1 - destcnt1, format, ap)) > (destsz1 - destcnt1) || dest1[destsz1 - 1])
	  ) {
	  if (vs == -1)
		  destsz1=roundtonextpower(destsz1*2);
	  else
		  destsz1=destlen ? roundtonextpower(destcnt1 + vs + 1) : destcnt1+vs+1;
#if NPPWINDOWS /* no recursive calls please */
    //char debug[256]; snprintfX(debug,sizeof(debug),"vsnwprintf() returns %u; Trying length %u",vs,destsz1); MessageBox(g_nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif
	  if (!destcnt1) {
		  freesafe(dest1, _T("vsarmprintf"));
		  dest1=(TCHAR *)mallocsafe(CHARSIZE(destsz1), _T("vsarmprintf"));
	  } else
		  dest1=(TCHAR *)reallocsafe(dest1, CHARSIZE(destsz1), _T("vsarmprintf"));
bottest:
	  ap=ap2;
    //memset(dest1+destcnt1,'@',destsz1-destcnt1); dest1[destsz1]='?'; // beyond string
  }
  if (dest1) {
#if NPPDEBUG
    if (vs != wcslen(dest1+destcnt1)) { /* no recursive calls please */
      TCHAR debug[256];
	  snprintfX(debug, 256, _T("vs=%u strlen(dest1)=%u"), vs, wcslen(dest1));
	  MessageBox(0, debug, _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
      vs = wcslen(dest1+destcnt1);
    }
    if (destsz1<=vs+destcnt1) {
		TCHAR debug[256];
		snprintfX(debug, 256, _T("Buffer too short, destcnt1=%u vs=%u"), destsz1, vs);
		MessageBox(0,debug, _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
    }
#endif
    if (!destlen && destsz1>vs+1+destcnt1) {
      destsz1 = vs + 1 + destcnt1;
      dest1=(TCHAR *)reallocsafe(dest1, CHARSIZE(destsz1), _T("vsarmprintf"));
#if NPPWINDOWS /* no recursive calls please */
      //char debug[256]; snprintfX(debug,sizeof(debug),"Final length (including \\0) was %u",vs+1); MessageBox(g_nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif
    }
  }
  if (!(*dest=dest1))
	  vs=0;
  *destsz=destsz1;
  if (destlen)
	  *destlen=destcnt1+vs;

  return(vs);
}

EXTERNC size_t sarmprintf(TCHAR **dest,size_t *destsz,size_t *destlen,const TCHAR *format,...) {
  size_t rv;
  va_list ap;
  va_start(ap,format);
  rv=vsarmprintf(dest,destsz,destlen,format,ap);
  va_end(ap);
  return(rv);
}

/* snprintf() falls short of the mark.
   What we really need is smprintf(...) AKA mprintf(...).
   Like strdup() it malloc()'s sufficient space and places the sprintf()
   results in it. The caller will get the entire result without needing to
   overguess and waste space. The caller will free the pointer. The caller
   may create a special routine that uses and free's the pointer all at once. */
EXTERNC TCHAR *smprintf(const TCHAR *format,...) {
  TCHAR *rv=NULL;
#if NPPDEBUG
  size_t rvsz=0;
#else
  size_t rvsz=256;
#endif
  va_list ap;
  va_start(ap,format);
  vsarmprintf(&rv,&rvsz,NULL,format,ap);
  va_end(ap);

  return(rv);
}

/* vsmprintfpath() gets much reduced functionality from smprintf()
   but it gains a couple of features needed for path construction
   and it probably runs a lot faster than the official smprintf()
   so it's better for simple format strings. Simple format strings
   without any of the special functions are interchangable with
   sprintf() variants.
   If you need full sprintf() functionality, use smprintf() or snprintfX() above. */
/* Control characters:
   Supported: exactly as shown; no modifiers allowed!
     %%=%
     %c=character
     %s=string
     %d,%i=integer
     %o=octal
     %u=unsigned
     %x,%X hex
   New and Improved Functionality:
//new feature: %bn# print number in any base, n can be *
//new feature: optional characters also cancel next character if the same
//new feature: %S a string that needs to be free()'d after it is used
//new feature: %s prints (null) when NULL is passed in
//new feature: %#n(...) repeat ... n times
//new feature: %D hex dump of string
//new feature: %*s type binary string where *=length
     %?_, optional character _ immediately after ? is only inserted if the previous character is not a _
     %C, optional character next in va_args is only inserted if the previous character is not the same
       special note: if the character is a space then it is only inserted if the previous character is not whitespace
     %#n* ; %#n#_ where n is a number or * and _ is a character;
       inserts n characters _ of your choice: example: %#57#? inserts 57 question marks. n may be a * to extract the number from the arglist
       if the second # is a * then the character is drawn from the arglist
       %#** (both from arglist) or %#57* (char from arglist)
   Not Supported:
     Any internal modifiers
     float types
   Example: smprintfpath("%c%?:%s%?\\%s%?\\%s",'C',"\\WINDOWS\","TEMP","TEMPFILE.TXT")
     -> C:\WINDOWS\TEMP\TEMPFILE.TXT
*/
EXTERNC size_t vsarmprintfpath(TCHAR **dest,size_t *destsz, size_t *destlen,const TCHAR *format,va_list ap2) {
	size_t tlen=0,llen;
	TCHAR *cx;
	TCHAR *argst, *dest1;
	TCHAR lastch, checkch;
	TCHAR cmd[3];
	TCHAR tempspace[128];
	unsigned pass,argu;
	size_t destlen1=(*dest && destlen)?*destlen:0;

	for (pass=0; pass<2; pass++) {
		va_list ap;
		ap=ap2;
		dest1=*dest+destlen1; // not used on the first pass, prevents compiler warnings
		for(lastch='\0', cx=(TCHAR *)format; *cx; cx++) {
			if (*cx!='%') {
pctch:
	        lastch=*cx;
		    goto dochar;
			} else
				switch(*++cx) {
					// better idea: insert the % symbol and go on to the next character
				case '\0': cx--; break; /* feed the \0 back to kill the for loop */
				case '%': lastch  = '%';
					goto dochar;
				case 'c': lastch  = (TCHAR)(int)va_arg(ap,unsigned);
					goto dochar;
				case 'C': checkch = (TCHAR)(int)va_arg(ap,unsigned);
					goto checkchar;
				case '?':
					cx++;
					checkch=*cx;
checkchar:
					if (!((checkch==' ' && isspace(lastch)) || lastch==checkch)) {
						lastch=checkch;
dochar:
						if (!pass) {
							tlen++;
						} else {
							*dest1=lastch;
							dest1++;
							*dest1='\0';
						}
					}
					break;
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
					argu = va_arg(ap,unsigned);
					memcpy(cmd,"%c",3);
					cmd[1]=*cx;
					snprintfX(tempspace, 128, cmd, argu);
					argst=tempspace;
					goto dostring;  /* this saves code but wastes a strlen on a known short string */
				case 's':
					argst = va_arg(ap, TCHAR *);
dostring:
					if (!pass) {
						if ((llen=wcslen(argst))) {
							lastch=argst[llen-1];
							tlen+=llen;
						}
					} else {
						while((*dest1=*argst)) {
							dest1++;
							argst++;
						} // stpcpy() isn't always available
          //memcpy(dest,argst,strlen(argst)+1);  %S support
						if (dest1>*dest)
							lastch=*(dest1-1);
					}
					break;
				case '#':
					cx++;
					if (!*cx)
						continue;
					else if (*cx=='*') {
						llen=va_arg(ap,unsigned);
						cx++;
						if (!*cx)
							continue;
					} else
						llen = wcstol(cx, &cx, 0);
					if (llen) {
						if (*cx=='*') {
							lastch=(TCHAR)va_arg(ap, int);
						} else {
							cx++;
							lastch=*cx;
						}
						if (!pass) {
							tlen+=llen;
						} else {
							memset(dest1,lastch,llen);
							dest1 += llen;
							*dest1='\0';
						}
					} else
						cx++;
					break;
				default:
					goto pctch;
				}
		}
		if (!pass) {
			armreallocsafe(dest, destsz, CHARSIZE(destlen1 + tlen + 1), destlen ? ARMSTRATEGY_INCREASE : ARMSTRATEGY_REDUCE, 0, _T("vsarmprintfpath"));
			if (!*dest)
				break;
		}
  }
  if (destlen)
	  *destlen += tlen;

  return(tlen);
}

EXTERNC size_t sarmprintfpath(TCHAR **dest,size_t *destsz,size_t *destlen,const TCHAR *format,...) {
  size_t rv;
  va_list ap;
  va_start(ap,format);
  rv=vsarmprintfpath(dest,destsz,destlen,format,ap);
  va_end(ap);
  return(rv);
}

EXTERNC TCHAR *smprintfpath(const TCHAR *format,...) {
  TCHAR *rv=NULL;
#if NPPDEBUG
  size_t rvsz=0;
#else
  size_t rvsz=256;
#endif
  va_list ap;
  va_start(ap,format);
  vsarmprintfpath(&rv,&rvsz,NULL,format,ap);
  va_end(ap);
  return(rv);
}

/* Just like MessageBox() except that lpText must be NULL or a malloc()'d pointer that will be free()'d after display.
   This allows you to display message text of unknown size without creating and freeing a pointer.
   If lpText is NULL meaning the malloc() failed, a nondescript default error message is printed instead.
   Ideally lpText will be the result of a strdup(), smprintf(), smprintfpath(), or other malloc() function.
   Example: MessageBoxFree(hWnd,smprintf("Message of unknown length:\r\n%s",str),"Caption",MB_OK); */
EXTERNC int MessageBoxFree(HWND hWnd,TCHAR *lpText,LPCTSTR lpCaption,UINT uType) {
  int rv=MessageBox(hWnd,lpText?lpText : _T("a memory problem occured"),lpCaption,uType);
  if (lpText)
	  freesafe(lpText, _T("MessageBoxFree"));

  return(rv);
}

#if NPPDEBUG
EXTERNC void testfree(int tofree, TCHAR *res, TCHAR *realstring, size_t rv) {
  if (res) {
    if (wcscmp(res, realstring))
		MessageBox(0,res, _T("String Mismatch"), MB_OK|MB_ICONSTOP);
    if (tofree)
		freesafe(res, _T("testfree"));
  } else
	  MessageBox(0, _T("malloc() failed"), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
  if (rv != (unsigned)-1 && rv != wcslen(realstring))
	  MessageBoxFree(0, smprintf(_T("Mismatch: rv=%u strlen()=%u"), rv, wcslen(realstring)), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
}

EXTERNC void testsprintfuncs(void) {
	//char *res;
	size_t rv;
	TCHAR buf[8];
	TCHAR *zbuf,*zbuf2;
	size_t zbufsz, zbufsz2;
	unsigned temp,strategy,domalloc,want;
	int dx;

	for (temp=4; temp<=4096; temp<<=1)
		for (dx=-1; dx!=2; dx++)
			for (strategy=0; strategy<=2; strategy++)
				for (domalloc=0; domalloc<=1; domalloc++) {
					zbufsz=temp+dx;
					zbuf=domalloc?(TCHAR *)mallocsafe(zbufsz, _T("testsprintfuncs")):NULL;
					armreallocsafe(&zbuf,&zbufsz,temp+dx,strategy,0, _T("testsprintfuncs"));
					want=strategy==ARMSTRATEGY_INCREASE?roundtonextpower(temp+dx):temp+dx;
					if (domalloc)
						want=temp+dx;
					if (strategy==ARMSTRATEGY_INCREASE && want<64)
						want=64;
					if (zbufsz != want)
						MessageBoxFree(0, smprintf(_T("Mismatch: strategy:%u temp+dx=%u want=%u zbufsz=%u zbufwas=%s"), strategy, temp+dx, want, zbufsz, domalloc ? _T("..."): _T("NULL")), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
					freesafe(zbuf, _T("testsprintfuncs"));
				}

  for (temp=1; temp<130; temp++) {
    zbuf=NULL;
	zbufsz=0;
    sarmprintf(&zbuf,&zbufsz,NULL, _T("%*s"), temp, "X");
    zbuf2=NULL;
	zbufsz2=0;
    strcpyarmsafe(&zbuf2,&zbufsz2,NULL,zbuf, _T("testsprintfuncs"));
    freesafe(zbuf2, _T("testsprintfuncs"));
    freesafe(zbuf, _T("testsprintfuncs"));
    zbuf=NULL;
	zbufsz=0;
    sarmprintf(&zbuf,&zbufsz,NULL, _T("%*s"), 131-temp, _T("X"));
    zbuf2=NULL;
	zbufsz2=0;
    strcpyarmsafe(&zbuf2,&zbufsz2,NULL,zbuf, _T("testsprintfuncs"));
    freesafe(zbuf2, _T("testsprintfuncs"));
    freesafe(zbuf, _T("testsprintfuncs"));
  }

  //test with smprintf() initial buffer size of 8
  testfree(1, smprintf(_T("Howdy %d partner"), 57), _T("Howdy 57 partner"), (unsigned)-1); // too long
  testfree(1, smprintf(_T("123456")), _T("123456"), (unsigned)-1); // too short
  testfree(1, smprintf(_T("x2%s"), _T("ex")), _T("x2ex"), (unsigned)-1); // too short
  testfree(1, smprintf(_T("1234567")), _T("1234567"), (unsigned)-1); // just right
  testfree(1, smprintfpath(_T("%s%?\\%s%?\\%s"), _T("A"), _T("B\\"), _T("C")), _T("A\\B\\C"), (unsigned)-1);
  rv=snprintfX(buf, 8, _T("123456789ABCDEF"));
  testfree(0, buf, _T("1234567"),rv);
  rv=snprintfX(buf, 8, _T("123"));
  testfree(0, buf, _T("123"),rv);
  memset(buf,0,sizeof(buf));
  // Borland,DMC  : 7:7,'\0' 8:8,'8'  9:9,'9'
  // MSVC,MinGW   : 7:7,'\0' 8:8,'8' 9:-1,'9'
  // Watcom,Pelles: 7:7,'\0' 8:8,'\0' 9:9,'\0' (Pelles with MSVCRT.LIB is same as MinGW)
  // Conclusion: None are safe since they can return counts larger than what was written to the string
  // Conclusion: Watcom&Pelles are the safest for general use being always \0 terminated but are the least useful for vsarmprintf()
  //temp=snprintf(buf,sizeof(buf),"7777777"); MessageBoxFree(0,smprintf("%c:%u buf[7]:%d",buf[0],temp,buf[7]),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  //temp=snprintf(buf,sizeof(buf),"88888888"); MessageBoxFree(0,smprintf("%c:%u buf[7]:%d",buf[0],temp,buf[7]),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  //temp=snprintf(buf,sizeof(buf),"999999999"); MessageBoxFree(0,smprintf("%c:%u buf[7]:%d",buf[0],temp,buf[7]),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
}
#endif

// When enabled, this will ensure that string lengths are being maintained properly by the highly detailed
// string transforms. When they are known to be working, turning off NPPDEBUG will disable all this
// slow code.
#if NPPDEBUG && 1 /* { binary safe functions no longer permit this testing */
EXTERNC void *memmovetest(TCHAR *dest, TCHAR *src, size_t n) {
	size_t m;
	m = wcslen(src);
	if (m + 1 != n) {
#if NPPWINDOWS /* { */
		MessageBoxFree(g_nppData._nppHandle, smprintf("memmovetest wrong SLN strlen(+1):%u != SLN:%u", strlen((const char *)src) + 1, n), PLUGIN_NAME, MB_OK | MB_ICONSTOP);
#else /* } else { */ /* Borland C for DOS */
		printf("Wrong SLN:%u != strlen(+1):%d\n", n, m + 1);
#endif /* } */
		n = m;
	}
	return(wmemmove(dest, src, n));
}
#else /* } else { */
#define memmovetest(dest, src, n) wmemmove((dest), (src), n)
#endif /* } */

// unlike other arm routines, destlen is required here because memmovearm() is not interested in appending anything.
// returns the number of bytes *dest was changed by during a realloc() so the caller can adjust their pointers
// memmovearm always moves one extra byte assuming the caller is trying to maintain a \0 terminated C-string
EXTERNC int memmovearm(void **dest, size_t *destsz, size_t *destlen, TCHAR *destp, TCHAR *sourcep
#if NPPDEBUG
	, int notest
#endif
) {
	unsigned rv = 0;
	if (destp != sourcep) {
		if ((rv = armreallocsafe((TCHAR**)dest, destsz, *destlen + 1 + destp - sourcep, ARMSTRATEGY_INCREASE, 0, _T("memmovearm")))) {
			destp += rv;
			sourcep += rv;
		}
		if (*dest) {
			//memset(*dest+*destlen,'@',*destsz-*destlen); *(*dest+*destlen)='%'; // % is \0
#if NPPDEBUG
			if (notest)
				wmemmove(destp, sourcep, (*destlen) - (sourcep - *dest) + 1);
			else
#endif
				memmovetest(destp, sourcep, (*destlen) - (sourcep - *dest) + 1);
			*destlen += destp - sourcep;
		}
	}
	return(rv);
}
#if NPPDEBUG
#define memmovearmtest memmovearm
#else
#define memmovearmtest(dt,ds,dl,dp,sp,nt) memmovearm(dt,ds,dl,dp,sp)
#endif

// because so many c string functions must call strlen() before operating, these mem...
// functions are more efficient on huge buffers. If you wish to provide a \0 terminated C-string
// calculate the strlen() yourself as most c string functions do every time they
// are called.
EXTERNC void memcqspnstart(const TCHAR *find, unsigned findl, unsigned *quick) {
	unsigned q;
	for (q = 0; q < 256 / (sizeof(unsigned) * 8); q++)
		quick[q] = 0; // how many bits can we store in unsigned?
	while (findl) {
		quick[(unsigned)*(unsigned char *)find / (sizeof(unsigned) * 8)] |= 1 << (unsigned)*(unsigned char *)find % (sizeof(unsigned) * 8);
		find++;
		findl--;
	}
}

// strcqspn quick version for large find strings used multiple times
// end=buf+buflen
// for C strings this is the position of the \0
// for buffers, this is 1 character beyond the end (same place)
// returns end when the search fails
EXTERNC TCHAR *memcqspn(const TCHAR *buf, const TCHAR *end, const unsigned *quick) {
	if (buf < end) for (; buf < end; buf++) {
		if (quick[(unsigned)*(unsigned char *)buf / (sizeof(unsigned) * 8)] & 1 << ((unsigned)*(unsigned char *)buf % (sizeof(unsigned) * 8)))
			return((TCHAR*)buf);
	}
	else if (buf > end) for (; buf > end; buf--) {
		if (quick[(unsigned)*(unsigned char *)buf / (sizeof(unsigned) * 8)] & 1 << ((unsigned)*(unsigned char *)buf % (sizeof(unsigned) * 8)))
			return((TCHAR*)buf);
	}
	return((TCHAR*)end);
}

EXTERNC TCHAR *memqspn(const TCHAR *buf, const TCHAR *end, const unsigned *quick) {
	if (buf < end) for (; buf < end; buf++) {
		if (!(quick[(unsigned)*(unsigned char *)buf / (sizeof(unsigned) * 8)] & 1 << ((unsigned)*(unsigned char *)buf % (sizeof(unsigned) * 8))))
			return((TCHAR*)buf);
	}
	else if (buf > end) for (; buf > end; buf--) {
		if (!(quick[(unsigned)*(unsigned char *)buf / (sizeof(unsigned) * 8)] & 1 << ((unsigned)*(unsigned char *)buf % (sizeof(unsigned) * 8))))
			return((TCHAR*)buf);
	}
	return((TCHAR*)end);
}

// strcspn: returns the first location in buf of any character in find
// extra performance derived from pam_mysql.c
EXTERNC TCHAR *memcspn(const TCHAR *ibuf, const TCHAR *iend, const TCHAR *ifind, unsigned findl) {
	const TCHAR *find = ifind;
	TCHAR *buf = (TCHAR *)ibuf, *end = (TCHAR *)iend;
	unsigned char c1, c2;

	switch (findl) {
	case 0:
		break;
	case 1:
		if (buf < end) {
			TCHAR *rv;
			if ((rv = (TCHAR *)wmemchr(buf, *find, end - buf)))
				return(rv);
		}
		break;
	case 2:
		c1 = find[0];
		c2 = find[1];
		for (; buf < end; buf++)
			if (*buf == c1 || *buf == c2)
				return(buf);
		break;
	default:
		if (buf < end) {
			const TCHAR *findt, *finde = find + CHARSIZE(findl);
			unsigned char min = (unsigned char)-1, max = 0, and_mask = (unsigned char)~0, or_mask = 0;
			for (findt = find; findt < finde; findt++) {
				if (max < *findt)
					max = *findt;
				if (min > *findt)
					min = *findt;
				and_mask &= *findt;
				or_mask |= *findt;
			}
			for (; buf < end; buf++) {
				if (*buf >= min && *buf <= max && (*buf & and_mask) == and_mask && (*buf | or_mask) && wmemchr(find, *buf, findl))
					return((TCHAR *)buf);
			}
		} break;
	}

	return(end);
}

EXTERNC char *memcspn_chr(const char *ibuf, const char *iend, const char *ifind, unsigned findl) {
	const char *find = ifind;
	char *buf = (char *)ibuf, *end = (char *)iend;
	unsigned char c1, c2;

	switch (findl) {
	case 0:
		break;
	case 1:
		if (buf < end) {
			char *rv;
			if ((rv = (char *)memchr(buf, *find, end - buf)))
				return(rv);
		}
		break;
	case 2:
		c1 = find[0];
		c2 = find[1];
		for (; buf < end; buf++)
			if (*buf == c1 || *buf == c2)
				return(buf);
		break;
	default:
		if (buf < end) {
			const char *findt, *finde = find + CHARSIZE(findl);
			unsigned char min = (unsigned char)-1, max = 0, and_mask = (unsigned char)~0, or_mask = 0;
			for (findt = find; findt < finde; findt++) {
				if (max < *findt)
					max = *findt;
				if (min > *findt)
					min = *findt;
				and_mask &= *findt;
				or_mask |= *findt;
			}
			for (; buf < end; buf++) {
				if (*buf >= min && *buf <= max && (*buf & and_mask) == and_mask && (*buf | or_mask) && memchr(find, *buf, findl))
					return((char *)buf);
			}
		} break;
	}

	return(end);
}

// http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
// http://www.students.iscte.pt/lamb/doxygen-glibc/html/sysdeps_2i386_2bits_2string_8h-source.html
// http://gcc.gnu.org/onlinedocs/gcc-3.3.1/gcc/Extended-Asm.html#Extended%20Asm
// like memcspn & strcspn, returns the first character that isn't __c, or __e if nothing found
// if __e<__s then the search proceeds backwards, __s points to the first character to be searched, __e points one below the beginning of the string
EXTERNC /*inline*/ TCHAR *memchrspn (const TCHAR *__s, const TCHAR *__e, unsigned __c) {
  if (__s<__e) while(__s<__e && *(TCHAR *)__s==(TCHAR)__c) __s++;
  else if (__s>__e) while(__s>__e && *(TCHAR *)__s==(TCHAR)__c) __s--;
  return (TCHAR *)__s;
}

// strspn: returns the first location in buf of any character not in find
// extra performance derived from pam_mysql.c
EXTERNC TCHAR *memspn(const TCHAR *ibuf,const TCHAR *iend,const TCHAR *ifind,unsigned findl) {
  TCHAR *buf=(TCHAR *)ibuf, *end=(TCHAR *)iend;
  const TCHAR *find=(const TCHAR *)ifind;
  switch(findl) {
  case 0: return((TCHAR *)buf);
  case 1: return memchrspn(ibuf,iend,*find);
  case 2: {
      unsigned char c1=find[0],c2=find[1];
      while(buf<end && (*buf==c1 || *buf==c2)) buf++;
    }
    return((TCHAR *)buf);
  default: if (buf<end) {
      const TCHAR *findt,*finde=find+findl;
      unsigned char min=(unsigned char)-1,max=0,and_mask=(unsigned char)~0,or_mask=0;
      for(findt=find; findt<finde; findt++) {
        if (max<*findt) max=*findt;
        if (min>*findt) min=*findt;
        and_mask &= *findt;
        or_mask |= *findt;
      }
      for(; buf<end; buf++) {
        if (*buf<min || *buf>max || (*buf & and_mask) != and_mask || !(*buf | or_mask) || !memchr(find,*buf,findl)) return((TCHAR *)buf);
      }
    } break;
  }
  return((TCHAR *)end);
}

// correcting a little incompetence in the original definition of memchr() allows us to do bidirectional searching memrchr
// __e points to the character after the end of the buffer, typically the \0 byte for a C string
// if __e<__s then the search proceeds backwards
// returns __e if the search fails to find character __c
EXTERNC /*inline*/ TCHAR *memchrX(const TCHAR *__s, const TCHAR *__e, unsigned __c) {
  if (__s > __e) {
	  // Reverse search:
	  for(; __s>__e; __s--)
		  if ((TCHAR)*__s==(TCHAR)__c)
			  return((TCHAR *)__s);
  } else if (__s < __e) {
	  // Regular search:
	  TCHAR *rv;
	  if (rv=(TCHAR *)wmemchr(__s,__c,__e-__s))
		  return rv;
  }

  // Not found. Return end of string.
  return (TCHAR *)__e;
}

#if NPPDEBUG && 0
void testmemcspn(void) {
    char test[]="AABBB\0\0\0\0";
    MessageBox(0,memchrX(test+strlen(test)-1,test,'A'),"???",MB_OK);
    MessageBox(0,memchrX(test+strlen(test)-1,test+1,'A'),"???",MB_OK);
    MessageBox(0,memchrX(test+strlen(test)-1,test+2,'A'),"???",MB_OK);
    MessageBox(0,memchrX(test+strlen(test)-1,test+4,'A'),"???",MB_OK);
}
#endif

// if buf>end then the search goes backwards
// full boyer-moore sounds nice but I suspect it can't beat REP SCASB which is also much simpler
EXTERNC TCHAR *memstr(const TCHAR *buf, const TCHAR *end, const TCHAR *find, unsigned findl) {
	if (findl > 1) {
		if (buf <= end - findl) {
			const TCHAR *end2 = end - findl;
			findl--;
			//#error end2 should be used instead of end2, see library for more efficient method
			while ((buf = (TCHAR *)memchrX(buf, end, (unsigned char)*find)) <= end2) {
				if (buf[findl] == find[findl] && (findl == 1 || !memcmp(buf + 1, find + 1, findl - 1)))
					return((TCHAR *)buf);
				buf++;
			}
		} else if (buf >= end + findl) {
			findl--;
			buf -= findl;
			//#error end2 should be used instead of end2
			while ((buf = (TCHAR *)memchrX(buf, end, (TCHAR)*find)) > end) {
				if (buf[findl] == find[findl] && (findl == 1 || !memcmp(buf + 1, find + 1, findl - 1)))
					return((TCHAR *)buf);
				buf--;
			}
		}
	} else if (findl == 1)
		return memchrX(buf, end, (TCHAR)*find);

	return((TCHAR *)end);
}

// copies a buf:len to a limited size szDest buffer
// szDest is always zero terminated in addition to any \0 that may already be in sSource
// returns the end of szDest or NULL if szDest or uDestSz is improperly specified
// if uSourceLen==-1 then sSource will be treated as a C string which makes this a complete
// replacement for the bugged strncpy()
// strncpy() is a dangerous function. Per spec, it is not required to place a \0 byte
// at the end of a string. This is an unfortunate definition because though this
// is a valuable use for strncpy(), such a feature is almost never used intentionally.
// #define strncpy(dest,src,len) strncpymem(dest,len,src,(unsigned)-1)
#define strncpy strncpy_unsafe
EXTERNC TCHAR *strncpymem(TCHAR *szDest,size_t uDestSz,const TCHAR *sSource,unsigned uSourceLen) {
  if (szDest && uDestSz) {
    if (sSource) {
      if (uSourceLen == (unsigned)-1) uSourceLen=wcslen(sSource);
      if (uSourceLen>=uDestSz) uSourceLen=uDestSz-1;
      if (uSourceLen>1) memcpy(szDest,sSource,uSourceLen);
      else *szDest=*sSource;
    } else uSourceLen=0;
    szDest[uSourceLen]='\0';
    return szDest+uSourceLen;
  }
  return NULL;
}

/* strtok() is defective in some compilers and the bad calling sequence makes it difficult to be thread safe.
   We'll redo it and fix all of the problems.
   szStr always points at the start of the string
   Start uPos at 0 and pass the modified value back each time you want a new string extracted
   uPos is maintained separately from szStr so the original string can be optimized and/or relocated in the caller's code
   *uPos can be NULL if you only need a single decode */
TCHAR *strtokX(TCHAR *szStr,unsigned *puPos,const TCHAR *szDeli) {
  TCHAR *rve,*rv=szStr;
  if (puPos) rv+= *puPos;
  if (*rv) {
    rv += wcscspn(rv,szDeli);
    rve = rv + wcscspn(rv,szDeli);
    if (*rve) *rve++='\0';
    if (puPos) *puPos = rve-szStr;
  }
  return rv;
}

//http://www.catch22.net/source/snippets.asp
#define ISPOW2(x)  (x && !(x & (x-1)))

// check to see if a number is a power of two
// returns the power+1 such that 1<<(powcheck(num)-1)=num
// returns 0 if the num is not a power of 2, such as when num==0
EXTERNC unsigned powcheck(unsigned num) {
  unsigned rv=0;

  if (ISPOW2(num))
	  for(rv=1; num>1; num>>=1,rv++);

  return(rv);
}

// SUBSECTION Sort routines
/* I discovered the need for a safer sort than qsort which can blow up the
 * stack with cleverly designed input. I also discovered the need for a
 * stable sort. Merge was chosen for the stable sort and Introsort was
 * chosen for the stack safe qsort. A special version for width==sizeof(int)
 * is provided to better optimize the most common record width. Should
 * anything go wrong, the routines bail to the c-lib qsort(). All the
 * sort routines are designed to be call compatible with qsort(). */
#if 1 && NPPDEBUG
#define movect(num) g_uMoves+=(num);
#define setmoves(num) g_uMoves=(num);
#define getmoves() (g_uMoves)
  unsigned long g_uMoves;
#else
#define movect(num)
#define setmoves(num)
#define getmoves() (0LU)
#endif
#define INTROSORTENABLE 1 /* set to 1 for introsort, 0 for original qsort */
#define INTROSTACKLEVEL 16 /* The stack depth permitted before switching to heapsort */
// http://linux.wku.edu/~lamonml/kb.html (GNU GPL version 2)
EXTERNC void _mergesortint(unsigned *pbase,unsigned *ptempo,size_t left,size_t mid,size_t right,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
#define width 1
  unsigned *pleft,*plefte,*pmid,*pright,*ptemp;

  pleft=pbase+left*width;
  pmid=pbase+mid*width;
  plefte=pmid-width;
  ptemp=ptempo;
  pright=pbase+right*width;

  if (pleft <= plefte && pmid <= pright) while(1) {
    if (fcmp(pleft,pmid)<=0) {
      *ptemp=*pleft;
      movect(1)
      ptemp+=width;
      pleft+=width;
      if (pleft>plefte) break;
    } else {
      *ptemp=*pmid;
      movect(1)
      ptemp+=width;
      pmid+=width;
      if (pmid>pright) break;
    }
  }
  while(pleft<=plefte) {
      *ptemp=*pleft;
      movect(1)
      ptemp+=width;
      pleft+=width;
  }
  while(pmid<=pright) {
      *ptemp=*pmid;
      movect(1)
      ptemp+=width;
      pmid+=width;
  }
  memcpy(pbase+left*width,ptempo,(char *)ptemp-(char *)ptempo);
  movect(ptemp-ptempo);
#undef width
}

// this verion isn't much less efficient. The only multiplication is at the beginning
EXTERNC void _mergesort(void *base,void *temp,size_t left,size_t mid,size_t right,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  char *pleft,*plefte,*pmid,*pright,*ptemp;

  pleft=(char *)base+left*width;
  pmid=(char *)base+mid*width;
  plefte=pmid-width;
  ptemp=(char *)temp;
  pright=(char *)base+right*width;

  if (pleft <= plefte && pmid <= pright) while(1) {
    if (fcmp((void *)pleft,(void *)pmid)<=0) {
      memcpy(ptemp,pleft,width);
      movect(1)
      ptemp+=width;
      pleft+=width;
      if (pleft>plefte) break;
    } else {
      memcpy(ptemp,pmid,width);
      movect(1)
      ptemp+=width;
      pmid+=width;
      if (pmid>pright) break;
    }
  }
  while(pleft<=plefte) {
      memcpy(ptemp,pleft,width);
      movect(1)
      ptemp+=width;
      pleft+=width;
  }
  while(pmid<=pright) {
      memcpy(ptemp,pmid,width);
      movect(1)
      ptemp+=width;
      pmid+=width;
  }
  memcpy((char *)base+left*width,temp,(char *)ptemp-(char *)temp);
  movect(((char *)ptemp-(char *)temp)/width)
}

EXTERNC void _mergesortsplitint(unsigned *base,unsigned *temp,size_t left,size_t right,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned mid;

  if (right > left)   {
    mid = (right + left) / 2;
    _mergesortsplitint(base,temp,left ,mid        ,fcmp);
    _mergesortsplitint(base,temp      ,mid+1,right,fcmp);
    _mergesortint(base,temp,left      ,mid+1,right,fcmp);
  }
}

EXTERNC void _mergesortsplit(void *base,void *temp,size_t left,size_t right,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned mid;

  if (right > left)   {
    mid = (right + left) / 2;
    _mergesortsplit(base,temp,left ,mid        ,width,fcmp);
    _mergesortsplit(base,temp      ,mid+1,right,width,fcmp);
    _mergesort(base,temp,left      ,mid+1,right,width,fcmp);
  }
}

EXTERNC void qsortmerge(void *base,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  void *temp;
  if (nelem>1 && width>=1 && (temp=mallocsafe(nelem*width, _T("msort")))) {
    if (1 && width==sizeof(int))
		_mergesortsplitint((unsigned *)base,(unsigned *)temp,0,nelem-1,fcmp);
    else
		_mergesortsplit(base,temp,0,nelem-1,width,fcmp);
    freesafe(temp, _T("msort"));
  }
}

// http://en.wikipedia.org/wiki/Heapsort
// This unified version is much nicer than the one with the sift() function
EXTERNC void _heapsortint(unsigned *base,unsigned *pivottemp,size_t nelem,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned int n = nelem, i = n/2, parent, child;

  while(1) {
    if (i) {
      i--;
      *pivottemp = base[i];
    } else {
      n--;
      if (n == 0) return;
      *pivottemp = base[n];
      base[n] = base[0];
    }
    movect(1)

    parent = i;
    child = parent*2 + 1;

    while(child < n) {
      if (child+1<n && fcmp(&base[child + 1],&base[child])>0) {
        child++;
      }
      if (fcmp(&base[child],pivottemp)<=0) break;
        base[parent] = base[child];
        movect(1)
        parent = child;
        child = parent*2 + 1;
    }
    base[parent] = *pivottemp;
    movect(1)
  }
}

EXTERNC void _heapsort(void *base,void *pivottemp,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned int n = nelem, i = n/2, parent, child;

  while(1) {
    if (i) {
      i--;
      memcpy(pivottemp,(char *)base+i*width,width); //*pivottemp = base[i];
    } else {
      n--;
      if (n == 0) return;
      memcpy(pivottemp,(char *)base+n*width,width); //*pivottemp = base[n];
      memcpy((char *)base+n*width,base,width); //base[n] = base[0];
    }
    movect(1)

    parent = i;
    child = parent*2 + 1;

    while(child < n) {
      if (child+1<n && fcmp((char *)base+(child+1)*width,(char *)base+child*width)>0) {
        child++;
      }
      if (fcmp((char *)base+child*width,pivottemp)<=0) break;
        memcpy((char *)base+parent*width,(char *)base+child*width,width);
        movect(1)
        parent = child;
        child = parent*2 + 1;
    }
    memcpy((char *)base+parent*width,pivottemp,width); // base[parent] = *pivottemp;
    movect(1)
  }
}

EXTERNC void qsortheap(void *base,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  if (nelem>1 && width>=1) {
    if (1 && width==sizeof(int)) {
      unsigned temp;
      _heapsortint((unsigned *)base,&temp,nelem,fcmp);
    } else {
      void *temp;
      if ((temp=mallocsafe(width, _T("qsortheap")))) {
        _heapsort(base,temp,nelem,width,fcmp);
        freesafe(temp, _T("qsortheap"));
      } else qsort(base,nelem,width,fcmp);
    }
  }
}

// http://linux.wku.edu/~lamonml/kb.html (GNU GPL version 2)
EXTERNC void _introsortint(unsigned depth,unsigned *base,unsigned *pivottemp,unsigned nelem,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned *leftpiv=base,*right=base+(--nelem); unsigned prn;

  // median of 3: we want the center value in left, the highest value in right, and the lowest value in the center
  // http://www.java2s.com/ExampleCode/Collections-Data-Structure/Quicksortwithmedianofthreepartitioning.htm
  if (nelem>=16) { // less than 16 produces too many extra compares
    unsigned *center=base+nelem/2;
    if      (fcmp(center ,right )>0) {*pivottemp=*right  ;*right  =*center;*center=*pivottemp;movect(3)}
    if      (fcmp(leftpiv,right )>0) {*pivottemp=*leftpiv;*leftpiv=*right ; *right=*pivottemp;movect(3)}
    else if (fcmp(leftpiv,center)<0) {*pivottemp=*leftpiv;*leftpiv=*center;*center=*pivottemp;movect(3)}
    //if (*leftpiv<*center || *leftpiv>*right) printf("F"); // failed to get the lowest value
  }
  *pivottemp = *leftpiv;
  movect(1)
  while(leftpiv<right) {
    if (1) while (1) {
      if (fcmp(right,pivottemp)>=0) {
        if (leftpiv >= --right) break;
      } else {
        *leftpiv = *right;
        movect(1)
        leftpiv++;
        break;
      }
    }
    if (leftpiv<right) while (1) {
      if (fcmp(leftpiv,pivottemp)<=0) {
        if (++leftpiv >= right) break;
      } else {
        *right = *leftpiv;
        movect(1)
        right--;
        break;
      }
    }
  }
  *leftpiv = *pivottemp;
  movect(1)
  prn=leftpiv-base;
  if (prn    > 1) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/*|| nelem-prn<=2*/)) // if the stack has grown too large, pass off to heap sort
      _heapsortint(base,pivottemp,prn ,fcmp);
    else
      _introsortint(depth+1,base,pivottemp,prn ,fcmp);
  }
  if (nelem > prn+1  ) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/*|| nelem-prn<=2*/)) // all evidence shows that there is no minimum value where heapsort performs better
      _heapsortint (base+prn+1,pivottemp,nelem-prn,fcmp);
    else
      _introsortint(depth+1,base+prn+1,pivottemp,nelem-prn,fcmp);
  }
}

EXTERNC void _introsort(unsigned depth,void *base,void *pivottemp,unsigned nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  char *leftpiv=(char *)base,*right=(char *)base+(--nelem)*width;
  unsigned prn;

  // median of 3: we want the center value in left, the highest value in right, and the lowest value in the center
  // http://www.java2s.com/ExampleCode/Collections-Data-Structure/Quicksortwithmedianofthreepartitioning.htm
  if (nelem>=16) { // less than 16 produces too many extra compares
    char *center=(char *)base+(nelem/2)*width;
    if      (fcmp(center ,right )>0) {memcpy(pivottemp,right,width)  ;memcpy(right,center,width);memcpy(center,pivottemp,width);movect(3)}
    if      (fcmp(leftpiv,right )>0) {memcpy(pivottemp,leftpiv,width);memcpy(leftpiv,right,width);memcpy(right,pivottemp,width);movect(3)}
    else if (fcmp(leftpiv,center)<0) {memcpy(pivottemp,leftpiv,width);memcpy(leftpiv,center,width);memcpy(center,pivottemp,width);movect(3)}
    //if (*leftpiv<*center || *leftpiv>*right) printf("F"); // failed to get the lowest value
  }
  memcpy(pivottemp,leftpiv,width);
  movect(1)
  while(leftpiv<right) {
    if (1) while (1) {
      if (fcmp(right,pivottemp)>=0) {
        right -= width;
        if (leftpiv >= right) break;
      } else {
        memcpy(leftpiv,right,width);
        movect(1)
        leftpiv+=width;
        break;
      }
    }
    if (leftpiv<right) while (1) {
      if (fcmp(leftpiv,pivottemp)<=0) {
        leftpiv += width;
        if (leftpiv >= right) break;
      } else {
        memcpy(right,leftpiv,width);
        movect(1)
        right-=width;
        break;
      }
    }
  }
  memcpy(leftpiv,pivottemp,width);
  movect(1)
  prn=(leftpiv-(char *)base)/width;
  if (prn    > 1) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/*|| nelem-prn<=2*/)) // if the stack has grown too large, pass off to heap sort
      _heapsort(base,pivottemp,prn ,width,fcmp);
    else
      _introsort(depth+1,base,pivottemp,prn ,width,fcmp); // all evidence shows that there is no minimum value where heapsort performs better
  }
  if (nelem > prn+1  ) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/* || nelem-prn<=2*/))
      _heapsort ((void *)((char *)base+(prn+1)*width),pivottemp,nelem-prn,width,fcmp);
    else
      _introsort(depth+1,(void *)((char *)base+(prn+1)*width),pivottemp,nelem-prn,width,fcmp);
  }
}

// introsort starts with quicksort and bails to heapsort when the stack gets too large
EXTERNC void qsortintro(void *base,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  if (nelem>1 && width>=1) {
    if (1 && width==sizeof(int)) {
      unsigned temp;
      _introsortint(0,(unsigned *)base,&temp,nelem,fcmp);
    } else {
      void *temp;
      if ((temp=mallocsafe(width, _T("qsortqsort")))) {
        _introsort(0,base,temp,nelem,width,fcmp);
        freesafe(temp, _T("qsortqsort"));
      } else
		  qsort(base,nelem,width,fcmp);
    }
  }
}
//#undef movect
//#undef setmoves
//#undef getmoves

#ifdef __MSDOS__
#define HANDLE int
#define INVALID_HANDLE_VALUE (-1)
#define FILE_ATTRIBUTE_NORMAL 0
// #include <stdio.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <dos.h>
// #include <io.h>
// #include <mem.h>
// #include <string.h>
// #include <stdlib.h>
// Virtually all of the fopen(), open(),_open,  _dos_open() and related calls are defective in some way
// we'll rewrite ones that work using the lib tools that are closest to fully functional
unsigned _readX(HANDLE handle,void far *buf,unsigned len) {
  unsigned nr;

  nr=0;
  errno=_dos_read(handle,buf,len,&nr);
  return(nr);
}

// lseek() then write() with 0 bytes to truncate
unsigned _truncateX(HANDLE handle,unsigned long len) {
  unsigned nr;
  unsigned long pos;

  nr=0;
  pos=tell(handle);
  lseek(handle,len,SEEK_SET);
  errno=_dos_write(handle,"",0,&nr);
  if (pos<len) lseek(handle,pos,SEEK_SET);
  return(nr);
}

// we assume _write() is defective too.
// prevent len=0 from truncating
unsigned _writeX(HANDLE handle,const void far *buf,unsigned len) {
  unsigned nr;

  nr=0;
  if (len) errno=_dos_write(handle,buf,len,&nr);
  return(nr);
}

HANDLE _creatX(const char *path,int attrib) {
  HANDLE hnd;

  hnd= INVALID_HANDLE_VALUE;
  errno=_dos_creat(path,attrib,&hnd);
  return(hnd);
}

HANDLE _openX(const char *filename,int oflags,int shflags) {
  HANDLE hnd;

  hnd= -1;
  errno=_dos_open(filename,oflags|shflags,&hnd);
  return(hnd);
}

int _closeX(HANDLE hnd) {
  errno=_dos_close(hnd);
  return errno?-1:0;
}
#else
// Windows File I/O routines are a hassle. Here's some wrappers that make them no harder to use than MS-DOS
#define O_DENYWRITE FILE_SHARE_WRITE
#define O_DENYREAD FILE_SHARE_READ
#define O_DENYALL (FILE_SHARE_READ|FILE_SHARE_WRITE)
#define O_DENYNONE 0
#define O_RDONLY GENERIC_READ
#define O_WRONLY GENERIC_WRITE
#define O_RDWR (GENERIC_READ|GENERIC_WRITE)
#define O_BINARY 0
#define _openX(filename,oflags,shflags) CreateFile(filename,oflags,shflags,NULL,OPEN_EXISTING,0,NULL)

#define _creatX(filename,attrib) CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,attrib,NULL)

EXTERNC DWORD _writeX(HANDLE handle,const void *buf, size_t len) {
  DWORD nr; /* the help says this is init to 0 by WriteFile */

  WriteFile(handle,buf,(DWORD)len,&nr,NULL);
  return(nr);
}

EXTERNC DWORD _readX(HANDLE handle,void *buf,DWORD len) {
  DWORD nr;

  nr=0;
  ReadFile(handle,buf,len,&nr,NULL);
  return(nr);
}

#define _closeX(hnd) CloseHandle(hnd)
#endif

const TCHAR eoltypes[3][4]={_T("\r\n"), _T("\r"), _T("\n")};

// SUBSECTION End
// SECTION: END

// SECTION: Beginning of Transforms

// ****************************************************************

/*Many of the following functions have common parameters. We'll describe them here.
  Each of these functions receives a c-string text buffer, performs a transform,
  then returns the number of changes made. convertall()'s job is to allow
  these text transforms to operate on editor text without knowing any of the details for
  accessing it within the Scintilla editor component. Considering that I have debugged all
  of the text transforms in Borland C++ 3.1 for DOS, you should be able to reuse them in
  any project.
dest:
  points to the start of the buffer. Some transforms may need to expand or pass back
  a totally different buffer is a pointer to the start of the buffer thus require the buffer
  to be from a malloc().
destsz:
  This is the total size of the buf when it was malloc()'d. If the transform
  expands or replaces dest it must return the new size.
destlen:
  destlen is the length of the usable text in dest. Transforms that expand or contract
  the text must adjust destlen. Most transforms assume that there is a \0 beyond
  destlen and will maintain this also.
stopeol
  Some functions can be told to perform only on the current line. Pass in the
  start of the line to be processed in *stopeol to transform until a \r or \n is hit.
  *stopeol return the the point stopped at, typically the \r,\n, or \0.
  . *stopeol must point

Some writers claim that Java string handling is so good that it can be faster C. What
doesn't get mentioned is that C is being misused when it performs so poorly. Tracing through a few
common C string functions shows they are fine for small strings but for multi-megabyte buffers they are extremely
inefficient. Some libraries implement the strcpy(dest,source) function as
memcpy(dest,source,strlen(source)+1) which causes a double scan of the string. A function like
strchr(str,ch) implemented as memchr(str,ch,strlen(str)+ch?0:1) repeated many times through
the string to repeatedly find a particular character
would generate a huge amount of wasted string scans refinding the strlen() each time. By using mem
functions instead we only need to calculate the length of the string a single time which may
happen to be already available and every mem function uses an altered version of that same number from then on.
Switching the code from str to mem functions not only allowed
TextFX to support embedded \0 in strings, it equalized performance among all compilers.
Because of certain string library design choices which aren't wrong, MinGW was a
terrible performer with str functions. With mem functions MinGW is a superb performer.

  I don't use function declarations that need to be updated each time a function call's
  parameters are changed. Instead I order the functions so they are created before their first use.
  This makes major coding changes much easier. Less code == less trouble!
*/

// Single character CHRT(), returns number replaced
EXTERNC unsigned memchrtran1(TCHAR *str, unsigned destlen, TCHAR cfind, TCHAR creplace) {
	TCHAR *end;
	unsigned n = 0;

	if (str && cfind != creplace)
		for (end = str + destlen; str < end && (str = (TCHAR *)wmemchr(str, cfind, end - str)); ) {
			*str = creplace;
			n++;
		}

	return(n);
}

// a full implementation of Foxpro's CHRTRAN() function
// Case Sensitive Replace any character found in
// *find with the character at the coresponding position in *repl
// if strlen(repl)<strlen(find) extra chars will be deleted
EXTERNC unsigned memchrtran(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *find, unsigned lfind, const TCHAR *repl, unsigned lrepl) {
#define lold 1
#define lnew 0
	TCHAR *d, *end;
	unsigned n = 0, fpn;
	unsigned quick[256 / (sizeof(unsigned) * 8)];

	if ((d = *dest) && lfind && (lfind != lrepl || wmemcmp(find, repl, lfind)))
		for (memcqspnstart(find, lfind, quick), end = *dest + *destlen, lrepl = wcslen(repl); (d = memcqspn(d, end, quick)) < end; ) {
			fpn = (const TCHAR *)wmemchr(find, *d, lfind) - find;
			if (fpn >= lrepl) {
				d += memmovearmtest((void**)dest, destsz, destlen, d + lnew, d + lold, 1);
				if (!*dest)
					goto failbreak;
				end = *dest + *destlen;
			} else {
				*d = repl[fpn];
				d++;
			}
			n++;
		}
failbreak:
	return(n);
#undef lold
#undef lnew
}

//a full implementation of Foxpro's STRTRAN()
// a non regex search and replace for c-strings
EXTERNC unsigned memstrtran(TCHAR **dest, size_t *destsz, size_t *destlen, TCHAR **stopeol, const TCHAR *oldst, unsigned lold, const TCHAR *newst, unsigned lnew) {
	unsigned n = 0;
	TCHAR *d, *end;

	//DWORD tk=GetTickCount();
	if ((d = *dest) && lold && (lold != lnew || wmemcmp(oldst, newst, lold))) {
		end = d + *destlen;
		if (stopeol)
			end = memcspn(d = *stopeol, end, _T("\r\n"), 2);
		while ((d = memstr(d, end, oldst, lold)) < end) {
			if (lnew != lold) {
				unsigned dx;
				dx = memmovearmtest((void**)dest, destsz, destlen, d + lnew, d + lold, 1);
				if (!*dest)
					goto failbreak; // can't test this. using stopeol always has an intentional \0 in the string.
				d += dx; // thanks to stopeol we can't use the easier method here
				end += dx/CHARSIZE(1) + lnew - lold;
			}
			if (lnew) {
				wmemcpy(d, newst, lnew);
				d += lnew;
			}
			n++;
		}
		if (stopeol) *stopeol = d;
	}
failbreak:
	//MessageBoxFree(g_nppData._nppHandle,smprintf("Tick Count: %u",GetTickCount()-tk),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
	return(n);
}

//strchrstrans: Replaces any in a set of single characters with corresponding strings
//adapted from memstrtran() && memchrtran()
//chrs: the set of chars to search for, and all chrs must be found in repls
//repls: deli + setof[char+string+deli] the first character of repls is the delimiter
//which must be chosen such that it doesn't occur in the data of repls
//stopeol: process only a single line, provide a pointer to the line beginning and it will return a pointer to the line ending \r\n
//  NULL to process entire str, set to NULL if starting at beginning,
// if using stopeol mode, slndx is required and returns the amount that the string was reduced by moving forwards minus alterations so the caller can update their counters
EXTERNC unsigned strchrstrans(TCHAR **dest, size_t *destsz, size_t *destlen, TCHAR **stopeol, const TCHAR *chrs, unsigned chrsl, const TCHAR *repls) {
#define lold 1
  unsigned n=0,lnew;
  TCHAR *d,*newst,*dnew,*end;
  TCHAR ckst[3];
  unsigned quick[256/(sizeof(unsigned)*8)];

  if ((d=*dest) && lold && chrsl) {
    end=d+*destlen;
    if (stopeol) end=memcspn(d=*stopeol,end,L"\r\n",2);
    ckst[0]=*repls;
    ckst[2]='\0';
    memcqspnstart(chrs,chrsl,quick);
    while ( (d=memcqspn(d,end,quick))<end) {
      ckst[1]=*d;
      if (!(newst=(TCHAR *)wcsstr(repls,ckst))) {
#if NPPDEBUG /* { */
        MessageBoxFree(g_nppData._nppHandle, smprintf(_T("strchrstrans: Improper String Format (#%d)\r\n%s"), 1, repls), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
#endif /* } */
        goto failbreak;
      }
      newst+=2;
      if (!(dnew= wcschr(newst,*repls))) { // find the termination character (typically ;)
#if NPPDEBUG /* { */
        MessageBoxFree(g_nppData._nppHandle, smprintf(_T("strchrstrans: Improper String Format (#%d)\r\n%s"), 2, repls), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
#endif /* } */
        goto failbreak;
      }
      lnew=dnew-newst;
      if (lnew != lold) {
        unsigned dx;
        dx=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		if (!*dest)
			goto failbreak;
        d+=dx; // thanks to stopeol we can't use the easier method here
        end+=dx+lnew-lold;
      }
      if (lnew) {
        memcpy(d,newst,lnew);
        d+=lnew;
      }
      n++;
    }
    if (stopeol) *stopeol = d;
  }
failbreak:
  return(n);
#undef lold
}

// For each line:
// adds begin to the start, replaces strings in the middle, then adds end on the end
// the spaces in the first line are counted to produce an indent for remaining lines
// Some transforms are more complicated than others. func=0:memstrtran, func=1:strchrtrans
// Not Binary Ready
EXTERNC unsigned prepostpendlines(TCHAR **dest, size_t *destsz, size_t *destlen, int func,
										const TCHAR *begin, unsigned lbegin, const TCHAR *end, unsigned lend,
										const TCHAR *oldst, unsigned loldst, const TCHAR *repl, unsigned lrepl)
{
#define lold 0
	unsigned n = 0;
	TCHAR *d, *detent;
	int indent, indent2;

	if ((d = *dest) && loldst) for (indent = -1; *d; ) {
		while (*d == '\r' || *d == '\n') d++;
		if (indent < 0)
			for (indent = 0; isspace(*d); d++, indent++); // calc first indent
		else
			for (indent2 = indent; indent2 && isspace(*d); d++, indent2--);  // all future indents are at the same place as the first
		if (!*d) break;
#define newst begin
#define lnew lbegin
		if (lnew != lold) {
			d += memmovearmtest((void**)dest, destsz, destlen, d + lnew, d + lold, 1);
			if (!*dest)
				goto failbreak;
		}
		if (lnew) {
			memcpy(d, newst, lnew);
			d += lnew;
		}
#undef lnew
#undef newst
		detent = d; // this ensures the optimizer can assign a register to d, &variables are always placed on the stack
		n += (!func ? memstrtran(dest, destsz, destlen, &detent, oldst, loldst, repl, lrepl) :
			strchrstrans(dest, destsz, destlen, &detent, oldst, loldst, repl));
		d = detent;
		if (!*dest) goto failbreak;
#define newst end
#define lnew lend
		if (lnew != lold) {
			d += memmovearmtest((void**)dest, destsz, destlen, d + lnew, d + lold, 1); if (!*dest) goto failbreak;
		}
		if (lnew) {
			memcpy(d, newst, lnew);
			d += lnew;
		}
#undef lnew
#undef newst
		n++;
	}
failbreak:
	return(n);
#undef lold
}

#if NPPDEBUG
unsigned g_uPass; // this is only used when rewraptest is being tested by the test function below
#endif

// textwid=0 to unwrap completely, 1 or other invalid width for default of 72
// All removed: multiple whitespace, non space whitespace including tabs,
//   newlines not conforming to eolType, extra newlines, more than double lines
// unwrapping will break to another paragraph at a double line, \r\n\r\n, \r\r, or \n\n
// (It may look easy now but this function took 3 straight days to get it working right.)
// eolType is retrieved from N++ and is used with eoltypes[]
// new feature: wrap text looks at the spacing of the first line to space other lines
// new feature: wrap avoids line comments
// new feature: indent properly for bullets
// Instead of implementing all of the above and more, there are full featured open source text formatters
// available which could be inserted.
// new feature: this no longer wraps correctly, probably becuause of **problem** below

EXTERNC unsigned rewraptext(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned tw, unsigned eoltype) {
  unsigned lold,lnew;
  TCHAR *d,*end,*dp,*lastspace,*newst;
  TCHAR temp[6]; // largest=" \r\n\r\n"
  unsigned flags; // The optimizer handles bit flags much better than separate variables
#define FLAG_QUIT 1
#define FLAG_WASSPACE 2
#define FLAG_MODIFY 4

  if (*dest) {
    if (eoltype>=NELEM(eoltypes)) eoltype=NELEM(eoltypes)-1;
    if (tw && (tw<8 || tw>2048)) tw=72; // values picked to keep within reasonable editor limits
    for(d=*dest,end=d+*destlen; d<end; ) {
      dp=d;
      lastspace=NULL;
      flags=0;
      do {
        switch(*d) {
        case '\r':
        case '\n':
          if (d[0]==d[1] || (d[0]=='\r' && d[1]=='\n' && d[2]=='\r' && d[3]=='\n')) {
            flags |= FLAG_QUIT;
            break;
          }
        case '\t':
          flags |= FLAG_MODIFY; // modify anything other than a space
        case ' ':
          if (!(flags&FLAG_WASSPACE)) lastspace=d;
          flags |= FLAG_WASSPACE;
          if (d==dp || d>lastspace || (flags&FLAG_MODIFY)) {
            flags |= FLAG_MODIFY; // modify anything more than 1 whitespace
            d++;
            continue; // colasce whitespace
          }
          break;
        case '\0':
          flags |= FLAG_QUIT;
          break;
        default:
          flags &= ~FLAG_WASSPACE;
          break;
        }
        if ((flags&FLAG_QUIT) || (tw && !(flags&FLAG_WASSPACE) && (unsigned)(d-dp)>=(((unsigned)(end-dp)<tw)?end-dp:tw))) { // **problem** was textBufferLength
          if (flags&FLAG_QUIT) {
            if (lastspace && (flags&FLAG_WASSPACE)) d=lastspace;
            else lastspace=d;
          }
          if (lastspace) {
            d=lastspace;
            lold=wcsspn(d,L"\r\n\t ");
          } else lold=0;
          newst=temp;
          memcpy(temp," ",2);
          wcscpy(newst+1,/*"\r\n"*/ eoltypes[eoltype]);
          if (flags&FLAG_QUIT) wcscat(newst,/*"\r\n"*/ eoltypes[eoltype]);
          lnew=wcslen(newst);
          flags |= FLAG_QUIT; //=FLAG_QUIT seems to work too
        } else if (flags & FLAG_MODIFY) {
          lold=d-lastspace;
          d=lastspace;
          newst = L" ";
          lnew=1;
          flags &= ~FLAG_MODIFY; // =0 seems to work too
        } else {
          d++;
          continue;
        }
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end = *dest + *destlen;
        }
        if (lnew) {
          memcpy(d,newst,lnew);
          d+=lnew;
        }
      } while(!(flags&FLAG_QUIT));
    }
    return(1); // it is impractical to calculate the actual number of changes
  } else {
failbreak:
    return(0);
  }
#undef FLAG_QUIT
#undef FLAG_MODIFY
#undef FLAG_WASSPACE
}

#if NPPDEBUG
#if !(NPPWINDOWS)
// Under DOS, this prints out the literal characters so space, \r, and \n are visible
EXTERNC void printout(TCHAR *st, TCHAR *nl) {
  for(;*st; st++) switch(*st) {
  case '\r':
    fputs("\\r",stdout);
    break;
  case '\n':
    printf("\\n%s",nl);
    break;
  case ' ':
    fputs(" ",stdout);
    break;
  default:
    putchar(*st);
    break;
  }
  puts("");
}
#endif

// to test rewraptest, we wrap the text then wrap it again. The result should be the same both times.
// Extended testing can vary the text width from lowest to highest to catch the edge cases.
// the pass variable can be used to insert dummy code for breakpoints in case the second pass is
// where the problem is
EXTERNC unsigned rewraptexttest(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned textwid, unsigned eoltype) {
  TCHAR *res;
  unsigned rv=0;

  g_uPass=1; // add code to check breakpoints for the pass number
  rv+=rewraptext(dest, destsz, destlen, textwid, eoltype);
  if (*dest) {
    if ((res=strdupsafe(*dest, _T("rewraptexttest")))) {
      g_uPass=2; // wrap again the wrapped text and we should get the same result
      rv+=rewraptext(dest,destsz,destlen,textwid,eoltype);
      if (wcscmp(res, *dest)) {
#if NPPWINDOWS
        MessageBoxFree(g_nppData._nppHandle,smprintf("Improper Second Conversion at wrap %d",textwid),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#else
        printf("Improper second conversion at wrap %d\n",textwid);
        printout(res, _T(""));
        printf("To:\n");
        printout(*dest, _T(""));
#endif
        freesafe(*dest, _T("rewraptexttest"));
        *dest=NULL;
      }
      freesafe(res, _T("rewraptexttest"));
    } else
		MessageBox(g_nppData._nppHandle, _T("Out of memory at rewraptexttest"), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
  } else
	  MessageBox(g_nppData._nppHandle, _T("Wrap memory failed at rewraptexttest"), _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
  return(rv);
}
#else
#define rewraptexttest rewraptext
#endif

const TCHAR g_ENCODEURISTR[] = _T("!'()*-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~");
// Same function as JavaScript's encodeURIcomponent()
// component refers to the fact that this is not expected to encode a complete URL,
// only a component of it
EXTERNC unsigned encodeURIcomponent(TCHAR **dest, size_t *destsz, size_t *destlen) {
  unsigned n=0;
  TCHAR* d;
  TCHAR *end;
  TCHAR newst[4];
  unsigned quick[256/(sizeof(unsigned)*8)];
#define lold 1
#define lnew (sizeof(newst)-1)

  if ((d=*dest) && lold) {
    TCHAR* newdest = NULL;
	TCHAR *newlast=*dest;
	size_t newsz = *destsz * 2, newlen = 0; // only used for HighPerformance
	for(end=d+*destlen,memcqspnstart(g_ENCODEURISTR,sizeof(g_ENCODEURISTR)-1,quick); (d=memqspn(d,end,quick))<end;) {
      snprintfX(newst, 4, L"%%%2.2X",*(unsigned char *)d);
      if (HighPerformanceon || HighPerformance) {
        if (newlast<d) {
			memcpyarmsafe((void**)&newdest, &newsz, &newlen, newlast, d-newlast, _T("encodeURIcomponent"));
			if (!newdest)
				goto freebreak;
		}
        newlast=(d+=lold);
        armreallocsafe(&newdest, &newsz, CHARSIZE(newlen + lnew + 1) /* 1 provides space for \0 below*/,
			ARMSTRATEGY_INCREASE, 0, _T("encodeURIcomponent"));
		if (!newdest) goto freebreak;
        memcpy(newdest+newlen,newst,lnew);
        newlen+=lnew;
        //end=*dest+*destlen;
      } else {
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest,destsz,destlen,d+lnew,d+lold,1); if (!*dest) goto failbreak;
          end=*dest+*destlen;
        }
        if (lnew) {
          memcpy(d,newst,lnew);
          d+=lnew;
        }
      }
      n++;
    }
    if (HighPerformanceon || HighPerformance) {
        if (n) {
        if (newlast<d)
			strncpyarmsafe(&newdest, &newsz, &newlen, newlast, d-newlast, _T("encodeURIcomponent"));
        else
			newdest[newlen]='\0'; // space provided for up above
freebreak:
        freesafe(*dest, _T("encodeURIcomponent"));
        if (!(*dest=newdest)) n=0;
        *destsz=newsz;
        *destlen=newlen;
      } else if (newdest) freesafe(newdest, _T("encodeURIcomponent"));
    }
  }
failbreak:
  return(n);
#undef lnew
#undef lold
}

// These work for Notepad++ now, later they will be modified to make the code compatible with Code::Blocks by calling currentEdit->WndProc()
//#define SENDMSGTOCED(which,mesg,wpm,lpm) SendMessage(((which)?g_nppData._scintillaSecondHandle:g_nppData._scintillaMainHandle),mesg,(WPARAM)(wpm),(LPARAM)(lpm))
#define SENDMSGTOCED(which,mesg,wpm,lpm) (*gScintillaMessageSender)(which, mesg, (WPARAM)(wpm), (LPARAM)(lpm))
#define INT_CURRENTEDIT int currentEdit
#define GET_CURRENTEDIT SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit)
#define INVALID_CURRENT_EDIT (-1)

// Injectable message sender
// was: SENDMSGTOCED(which,mesg,wpm,lpm)
LRESULT SendScintillaMessage(BOOL which, UINT Msg, WPARAM wParam,LPARAM lParam)
{
	HWND target;
	if (which) {
		target = g_nppData._scintillaSecondHandle;
	} else {
		target = g_nppData._scintillaMainHandle;
	}

	return SendMessage(target, Msg, wParam, lParam);
}
LRESULT(*gScintillaMessageSender)(BOOL, UINT, WPARAM, LPARAM) = &SendScintillaMessage;	// The default, can be overridden.

// dest = space(n)+source
// like sprintf(dest,"%-*s%s",n,"",source) returns number of characters written
// eventually this will be replaced with smprintfpath() when it gets space padding features
EXTERNC unsigned strcatspacel(TCHAR *dest,const TCHAR *source,size_t n) {
  memset(dest,' ',n);
  wcscpy(dest+n,source);

  return(n + wcslen(source));
}

// This function inserts text or an incrementing counter at a specified line position in text lines
// or a specified +-offset1 from the end of the line
// search format [\][$[+-offset.from.end$]][#[startval[+-adder]]/text
// preceed with \ to ensure that $ and # and future symbols are interpreted as text and not a
// preceed with \\ to start your text with a \ (backslash)
// #counter or $offset.from.end. text is not used if a #counter is provided.
// lines that are too short are skipped
// Examples:
// "text" insert text
// "#" insert 0..1..2..3...
// "#5" insert 5..6..7..8..9... (the default increment is +1)
// "#x5+2" insert hex 5..7..9..B...
// "$#5-2" insert 5..3..1..-1... at end of line
// "$$M$soft" insert M$soft at end of line
// "$4$text" insert text 4 characters beyond end of line. line will be ammended as necessary
// "$-3$#-5-2" insert -5..-7..-9..-11... 4 characters before end of line. lines that are too short will be skipped
// new feature: insert text after comma number
EXTERNC unsigned insertclipboardcolumn(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *search, int offset1) {
  unsigned n=0,lnew,lsearch;
  TCHAR *d,*end, *cmd, *newst;
  TCHAR nbuf[32], buf[32];
  const TCHAR *stmp;
#define lold 0
  unsigned flags;
#define FLAG_COUNTING 1
#define FLAG_HEX 2
#define FLAG_END 4
  int offset,cstart=0,cdx=0;

  if (*dest && *search) {
    flags=0;
    do {
      unsigned chn2;
      switch(*search) {
      case '$':
        flags |= FLAG_END;
        search++;
        if ((stmp= wcschr(search,'$'))) {
          offset1= _wtoi(search);
          search=stmp+1;
        } else offset1=0;
        continue;
      case '\\': search++; break;
      case '#':
        flags |= FLAG_COUNTING;
        search++;
        if (*search=='x') {flags |= FLAG_HEX; search++;}
        if (*search=='-') {cstart=-1; search++;} else cstart=1;
        cdx=0;
        if ((cmd=strdupsafe(search, _T("insertclipboardcolumn")))) {
          unsigned uIndex=0;
          chn2= wcscspn(cmd, L"+-");
          if (cmd[chn2]=='+') cdx=1;
          else if (cmd[chn2]=='-') cdx=-1;
          cstart *= _wtoi(strtokX(cmd,&uIndex, L"+-"));
          cdx    *= _wtoi(strtokX(cmd,&uIndex, L"+-"));
          memcpy(buf,"0",2);
          search=buf;
          freesafe(cmd, _T("insertclipboardcolumn"));
        }
        if (!cdx) cdx=1;
        break;
      }
      break;
    } while(1);
    if ((lsearch=wcslen(search))) {
      unsigned chn;
      for(d=*dest,end=d+*destlen; d<end; ) {
        chn=memcspn(d,end,L"\r\n",2)-d;
        offset=offset1;
        if (flags & FLAG_COUNTING) {
          lnew=snprintfX(nbuf, 32, (flags&FLAG_HEX)?L"%X":L"%d", cstart); // this violates const rules but we aren't modifiying the original
          newst=nbuf;
          cstart += cdx;
        } else {
          newst=(TCHAR *)search; // we promise not to modify this
          lnew=lsearch;
        }
        if (flags&FLAG_END) {
          if (offset>0) {
            lnew=strcatspacel(buf,newst,offset);
            newst=buf;
            offset=chn;
          } else if (offset>=0 || chn>= (unsigned)(-offset)) {
            offset+=chn;
          } else {
            lnew=0;
          }
        } else if (offset<0 || (unsigned)offset>chn) {
          lnew=0; // lold=0;
        }
        d+=offset;
        chn-=offset;
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end=*dest+*destlen;
          chn += lnew-lold;
        }
        if (lnew) {
          memcpy(d,newst,lnew);
          n++;
        }
        d += chn;
        while(d<end && (*d=='\r' || *d=='\n')) d++;
      }
    }
  }
failbreak:
  return(n);
#undef lold
}

EXTERNC unsigned numberconvert(TCHAR **dest, size_t *destsz, size_t *destlen, char from, char to) {
  unsigned rv=0;
  char *prefix; char temp[128];
  if (dest) {
    int num;
    switch(from) {
    case 'c': num= wcstol(*dest,NULL,0); break;
    case 'd': num= wcstol(*dest,NULL,10); break;
    case 'h': num= wcstol(*dest,NULL,16); break;
    case 'b': num= wcstol(*dest,NULL,2); break;
    case 'o': num= wcstol(*dest,NULL,8); break;
    default: goto failbreak;
    }
    switch(to) {
    case 'd': itoa(num,temp,10); prefix=""; break;
    case 'h': itoa(num,temp,16); prefix="0x"; break;
    case 'b': itoa(num,temp,2); prefix="$"; break;
    case 'o': itoa(num,temp,8); prefix="0"; break;
    default: goto failbreak;
    }
    *destlen=0;
    sarmprintf(dest, destsz, destlen, _T("%s%s"), prefix,temp);
    if (*dest) rv=1;
  }
failbreak:
  return(rv);
}

/* case insenstive strstr() */
EXTERNC TCHAR *memistr(const TCHAR *str,const TCHAR *end,const TCHAR *search,unsigned lold) {
  const TCHAR *d;
  TCHAR fch[3];

  if (lold) {
    for(d=search+lold-1; d>=search; d--) if (tolower(*d)!=toupper(*d)) goto stristryes;
    //return(lold==1?strchr(str,*search):strstr(str,search));
    return(memstr(str,end,search,lold));
stristryes:
    fch[0]=tolower(*search);
    fch[1]=toupper(*search);
    fch[2]='\0';
    d=str;
    goto bottest; while(d<end) {
      if (!memicmp(d,search,lold)) return((TCHAR *)d);
      d++;
bottest:
      if (fch[0]==fch[1]) {
        if (!(d=(TCHAR *)wmemchr(d,*search,end-d))) break;
      } else {
        d=memcspn(d,end,fch,lold);
      }
    }
  }
  return((TCHAR *)end);
}

// the search will be faster if like first characters are placed together
// the search will be faster if sizeof(_MSTR) is a power of two
struct _MSTR {
// eventually, mstr[0] will be eliminated completely.
// eventually, string will be a pointer and not a buffer
  TCHAR string[27]; // nocase=0: 1+the number of different search first characters
                   // nocase=1: 1+the number of different search first characters with an extra for characters who's case differs
  unsigned char nocase; // Each search is individually case sensitive. The entire table is handled case insensitive until we are iterating through possible matches then pick it out case sensitive if requested.
  char rv;
  unsigned char len; // filled in by the initializer
  // proposed features for faster string searching
  unsigned char lennext; // how many characters are common between the current string and the next string
  unsigned char lenskip; // We can skip lenskip+1 forward if string[lennext-1] does not match. currentpos+lenskip+1 is permitted to exceed NELEM. If !lenskip perform a memcmp()
  //unsigned char minlength; // the smallest length in the lenskip+1 to be skipped. This enables bsearch() to locate the nearest starting location using characters up to the smallest length
} g_mstring[]={
{L""                  ,0,0 ,0,0,0}, // mstring[0] is where the search string is stored from call to call
{L"\t"                ,0,7 ,0,0,0}, // set case insensitive strings to 0 for better efficiency
{L"\n"                ,0,8 ,0,0,0},
{L"\r"                ,0,8 ,0,0,0},
{L" "                 ,0,7 ,0,0,0},
{L"&#"                ,0,0 ,0,0,0},
{L"&AMP;\0&"          ,1,1 ,0,0,0},
{L"&COPY;\0(C)"       ,1,1 ,0,0,0},
{L"&GT;\0>"           ,1,1 ,0,0,0},
{L"&lT;\0<"           ,1,1 ,0,0,0},
{L"&NBSP;\0 "         ,1,1 ,0,0,0},
{L"&quot;\0\""        ,1,1 ,0,0,0},
{L"<!--\0-->"         ,0,2 ,0,0,0},
{L"</CAPTION"         ,1,3 ,0,0,0},
{L"</H"               ,1,3 ,0,0,0},
{L"</LI"              ,1,3 ,0,0,0},
{L"</PRE"             ,1,6 ,0,0,0},
{L"</P"               ,1,3 ,0,0,0},
{L"</TEXTAREA"        ,1,6 ,0,0,0},
{L"</TITLE"           ,1,3 ,0,0,0},
{L"</TR"              ,1,3 ,0,0,0},
{L"<BR"               ,1,3 ,0,0,0},
{L"<HR"               ,1,3 ,0,0,0},
{L"<PRE"              ,1,5 ,0,0,0},
{L"<P"                ,1,3 ,0,0,0},
{L"<STYLE\0</STYLE>"  ,1,2 ,0,0,0},
{L"<SCRIPT\0</SCRIPT>",1,2 ,0,0,0},
{L"<TD"               ,1,4 ,0,0,0},
{L"<TEXTAREA"         ,1,5 ,0,0,0},
{L"<TH"               ,1,4 ,0,0,0},
{L"<\0>"              ,0,2 ,0,0,0},
};

//int mstrnocase=0;
//char *mstrxst=NULL; // if ax or bx is NULL, compare the non null with mstrxst

EXTERNC int QSORT_FCMP fcmpmstrsort(const void *ax,const void *bx) {
  int acu,bcu;
#if 0 /* { Each time I try to use bsearch() I fail because of differing string lengths */
  struct _MSTR *am=ax;
  struct _MSTR *bm=bx;
  //unsigned const char *ac,*bc;
  if (mstrnocase) {
    dif=toupper(am->string[0])-toupper(bm->string[0]);
  } else {
    dif=am->string[0]-bm->string[0];
  }
  if (dif) return(dif);
  dif=bm->len-am->len;
  if (dif) return(dif);
  return(stricmp(am->string,bm->string));
  //        return(stricmp(((struct _MSTR *)ax)->string,((struct _MSTR *)bx)->string));
#else /* } else { */
  TCHAR *ac,*bc;
  for(ac=((struct _MSTR *)ax)->string,bc=((struct _MSTR *)bx)->string; 1; ac++, bc++) {
    //if (mstrnocase) {
      acu=toupper(*ac);
      bcu=toupper(*bc);
    /*} else {
      acu=*ac;
      bcu=*bc;
    }*/
    if (acu == bcu) {
      if (!acu) return(0);
    } else if (!bcu) return(-1); // the longer string is always sorted first
    else if (!acu) return(1); // the shorter string is always sorted last
    else return(acu - bcu);
  }
  return 0;
#endif /* } */
}

#if 0 /* { */
EXTERNC int QSORT_FCMP fcmpmstrbsearch(const void *ax,const void *bx) {
//struct _MSTR *axm=ax;struct _MSTR *bxm=bx;
  char *ac,*bc;
  int acu,bcu;
  if (ax && bx) { // in Borland-C, ax is always the key
    MessageBox(g_nppData._nppHandle,"Impossible",PLUGIN_NAME, MB_OK|MB_ICONWARNING);
    exit(1);
  }
  ac=mstrxst;
  bc=ax?(((struct _MSTR *)ax)->string):(((struct _MSTR *)bx)->string);
  while(1) {
    if (mstrnocase) {
      acu=toupper(*ac);
      bcu=toupper(*bc);
    } else {
      acu=*ac;
      bcu=*bc;
    }
    if (acu == bcu) {
      if (!acu) return(0);
    } else if (!bcu) return(0); // they are considered equal if the right string ends
    //else if (!acu) return(-1);
    //#error If we are at the largest of a run of equals, forward, else subtract
    else return(bx?(acu-bcu):(bcu-acu));
    ac++;
    bc++;
  }
}
#endif /* } */

// returns the number of initial characters that are equal
EXTERNC size_t streqct(TCHAR *t1, TCHAR *t2) {
  size_t rv=0;
  while(*t1 && *t1==*t2) {rv++; t1++; t2++; }
  return(rv);
}

// compute the skip values in the mstring array
EXTERNC void strmstrinit(struct _MSTR *msearch,unsigned nsearch,unsigned *cs/*,int nocase*/) {
  unsigned ch,chn;
  unsigned i,dn,up,lsearch;
#if NPPDEBUG
  if (!powcheck(sizeof(struct _MSTR)))
    MessageBoxFree(g_nppData._nppHandle, smprintf(_T("sizeof(struct _MSTR)==%u is not a power of two"), sizeof(struct _MSTR)), _T(PLUGIN_NAME), MB_OK|MB_ICONWARNING);
#endif
  //mstrnocase=nocase;
  for(i=1; i<nsearch; i++) {
	  msearch[i].len=wcslen(msearch[i].string); msearch[i].lenskip=0;
  }
  qsort(msearch+1,nsearch-1,sizeof(*msearch),fcmpmstrsort);
  for(i=2; i<nsearch; i++)
	  msearch[i-1].lennext=streqct(msearch[i-1].string,msearch[i].string);
  for(i=1; i<nsearch; i++) {
    if ((chn=msearch[i].lennext) && !(msearch[i].lenskip)) {
      for(dn=1; dn<256 && dn+i<nsearch && msearch[dn+i].lennext>=chn; dn++);
      for(up=dn; up!=(unsigned)-1; up--)
		  if (msearch[i+up].lennext==chn)
			  msearch[i+up].lenskip=dn-up;
    }
  }
  //for(i=1; i<nsearch; i++) printf("{\"%24s\",%3d,%3d,%3u,%3u,%3u},\n",msearch[i].string,msearch[i].nocase,msearch[i].rv,msearch[i].rv,msearch[i].len,msearch[i].lennext,msearch[i].lenskip);
  for(i=0; i<256; i++) {cs[i]=(unsigned)-1; cs[i+256]=0; }
  memset(msearch,0,sizeof(msearch[0]));
  for(lsearch=0, i=1; i<nsearch && lsearch<sizeof(msearch[0].string); i++) {
    ch=(unsigned)msearch[i].string[0];
    if (/*nocase && */(tolower(ch)!=toupper(ch))) {
      if (cs[tolower(ch)]>i) cs[tolower(ch)]=i;
      if (cs[tolower(ch)+256]<i) cs[tolower(ch)+256]=i;
      if (cs[toupper(ch)]>i) cs[toupper(ch)]=i;
      if (cs[toupper(ch)+256]<i) cs[toupper(ch)+256]=i;
    } else {
      if (cs[ch]>i) cs[ch]=i;
      if (cs[ch+256]<i) cs[ch+256]=i;
    }
    if (msearch[i].len && !wcschr(msearch[0].string,ch)) {
      msearch[0].string[lsearch++]=toupper(ch);
      if (/*nocase &&*/ tolower(ch)!=toupper(ch)) msearch[0].string[lsearch++]=tolower(ch);
    }
  }
  msearch[0].len=lsearch;
}

// When mstring search gets more advanced the close function will need to clean some stuff up.
#define strmstrclose(mstr)

// This multiple substring match evolved from humble beginnings:
// Try #1: if (stricmp()) {} else if (strcmp()) {} else if (stricmp())...
// Try #2: strcmp() through table to find a match
// Try #3: Iterate through sorted table from quick[ch] to quick[ch+256] to find a match
// Try #4: "Boyer-Moore Prefix Vector Skip Table" because smaller prefixes have greater skip potential
// Purpose: Search through a long non delimited string for the
// largest possible (greedy) of multiple case sensitive or insensitive substrings.
//
// There is a lot of good theory on multiple substring search which someone better than I may
// be able to implement.
// Returns: 0,p=NULL if not found, or mstring position 1...nsearch-1,p=location if found
//
// We could eliminate textBufferLength by padding the string with as many \0 as the longest substring
// This would eliminate the necessity of string scan to calculte strlen().
// The trouble is that the caller has to manage and possibly move the padded \0's and this may
// be more trouble than the REPNZ SCASB that calculates strlen(). Often times
// the caller already needs strlen() for something else so has it readily available.

// new feature: Implement bsearch() and minlength through lenskip to reduce iteration
// new feature: Allow for active use with deletestring, deletestringmultiple, insertstring and insertstringmultiple with autorecalc using insert sorts or recalc entire array
// new feature: When strings are pointers, shiftstrings will change the pointer values in case the caller has to realloc() the memory and the base pointer changes
// new feature: use memcqspn instead of memcspn
EXTERNC TCHAR *strmstr(TCHAR *str, TCHAR *end, unsigned *mno,struct _MSTR *msearch/*,unsigned nsearch*/,unsigned *cs/*,int nocase*/) {
  unsigned i;
  TCHAR *d;
  struct _MSTR *im;

  if (str && str<end) for(d=str; (d=memcspn(d,end,msearch[0].string,msearch[0].len))<end; d++ ) {
      for(i=cs[*(unsigned char *)d],im=msearch+i; i<=cs[*(unsigned char *)d+256]; /*i++,im+=sizeof(*msearch)*/) {
          if (d>=end-im->len) {
            i++; im++; // no space left for string
#if 1 /* { Disables the Prefix Skip system if it doesn't work */
        } else if (im->lennext && toupper(im->string[im->lennext-1])!=toupper(d[im->lennext-1])) { // non match at largest position, skip all possible
          i += im->lenskip+1;
          im = msearch+i; // this is the easiest way to do the multiplication
#endif /* } */
        } else if (!(im->nocase?memicmp(d,im->string,im->len):memcmp(d,im->string,im->len))) { // iterate through the possible matches checking case if requested
          *mno=i; // someday when the iteration counts get too long, we will use bsearch() with the newly created minlength to find the iteration starting point
          return(d);
        } else {
          i++; im++;
        }
      }
  }
  *mno=0;
  return(end);
}

// bug: the editor can double certain line breaks during a paste, this code can't fix it.
// when > is used as data like ">", this code doesn't catch it, and I may not fix it because it is improper HTML
EXTERNC unsigned stripHTMLtags(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *snotabs) {
	//#define ncase 1
	unsigned n = 0, lold = 0, lnew = 0, mno;
	TCHAR *d, *dp, *newst = NULL, *lastspace, *lastnewline, *end;
	TCHAR tempst[2];
	TCHAR space[] = L" ";
	TCHAR tab[] = L"\t";
	unsigned intextarea;
	int notabs = (*snotabs == 'n');
	unsigned quick[512];

	if (*dest) {
		strmstrinit(g_mstring, NELEM(g_mstring), quick);
		for (lastnewline = lastspace = d = *dest, end = d + *destlen, intextarea = 0; (d = strmstr(d, end, &mno, g_mstring, quick)) < end; ) {
			switch (g_mstring[mno].rv) {
			case 0: // & general
				dp = (TCHAR *)wmemchr(d, ';', end - d);
				if (!dp || dp - d > 8 || !isdigit(d[2]) || (int)(lnew = wcstol(d + 2, NULL, 0)) < 0 || lnew > 255) {
					d++;
					continue;
				}
				else {
					lold = dp - d + 1;
					newst = tempst;
					newst[0] = lnew;
					newst[1] = '\0';
					lnew = 1;
				}
				break;
			case 1: // &quot;
				lold = g_mstring[mno].len;
				newst = g_mstring[mno].string + lold + 1;
				lnew = wcslen(newst);
				break;
			case 2: // <!-- stuff to delete -->
				lold = wcslen(g_mstring[mno].string + g_mstring[mno].len + 1);
				newst = g_mstring[mno].string + g_mstring[mno].len + 1; // dummy variable
				dp = memistr(d, end, newst, wcslen(newst));
				lnew = 0;
				goto delcom;
			case 3: // <tag that generates newline
				newst = L"\r\n";
				lnew = 2;
				lastspace = d + lnew;
				lastnewline = d + lnew;
				goto bktcom;
			case 4: // <tag that generates tab
				newst = notabs ? space : tab;
				lnew = 1;
				goto bktcom;
			case 5: // <tag that indicates we are entering a preformatted section
				intextarea++;
				lnew = 0;
				goto bktcom;
			case 6: // <tag that indicates we are leaving a preformatted section
				if (intextarea) intextarea--;
				lnew = 0;
			bktcom:
				lold = 1;
				if (!(dp = (TCHAR *)wmemchr(d, '>', end - d))) continue;

			delcom:
				if (dp == end) {
					d = end;
					continue;
				}
				else {
					lold += dp - d;
				}
				break;
			case 7: // space without return
			case 8: // return
				lold = memspn(d, end, L"\r\n\t ", 5) - d; // 5 includes an occasional \0
				if (intextarea) {
					d += lold;
					continue;
				}
				if (d <= lastspace || d <= lastnewline/* || g_mstring[mno].rv==8*/) {
					lnew = 0;
				}
				else {
					newst = L" ";
					lnew = 1;
					lastspace = d + 1;
				}
				if (g_mstring[mno].rv == 8) lastnewline = d;
				break;
			}
			if (lnew != lold) {
				d += memmovearmtest((void**)dest, destsz, destlen, d + lnew, d + lold, 1);
				if (!*dest)
					goto failbreak;
				end = *dest + *destlen;
			}
			if (lnew) {
				memcpy(d, newst, lnew);
				d += lnew;
			}
			n++;
		}
	failbreak:
		strmstrclose(g_mstring);
	}
	return(n);
}

// returns the number of changes made
EXTERNC unsigned mymemset(TCHAR *s, char c, unsigned num) {
	unsigned n = 0;
	for (; num; num--, s++) {
		if (*s != c) {
			*s = c;
			n++;
		}
	}
	return(n);
}

// Search through a string for quote symbols
// If starting at a non quote, will return the postion of the next quote, possibly end
// If starting at a quote, will return the position after the close quote, possibly end
// If starting at end, returns NULL
// style=0, detect c-string escaping \" or \' or \\ or \anycharacter
// style=1, detect vb-string escaping "" or sql escaping ''
// style=2, Foxpro-string: no escaping
// some routines use 0 for no string detection and style+1 for the styles
EXTERNC TCHAR *findnextquote(TCHAR *str, TCHAR *end,unsigned style) {
  unsigned dx;
  TCHAR cq[3];

  if (str>=end) return(NULL);
  if (*str=='"' || *str=='\'') {
    wcscpy(cq,style?L"'":L"'\\");
    cq[0]=str[0];
    str++;
    dx=1;
  } else {
    wcscpy(cq,L"'\"");
    dx=0;
  }
  while(1) {
    str=memcspn(str,end,cq,wcslen(cq));
    if (str>=end) return(end);
    if (!style) { // C
      if (*str!='\\') return(str+dx);
      str+=2;
    } else { // VB & Foxpro
      if (style==1 && str[0]==str[1]) str+=2;
      else return(str+dx);
    }
  }
  return 0;
}

// This splits all lines after splitch
// style is defined by findnextquote()
// http://www.ultraedit.com/index.php?name=Forums&file=viewtopic&t=1415
EXTERNC unsigned splitlinesatch(TCHAR **dest, size_t *destsz, size_t *destlen, char splitch, unsigned style, unsigned eoltype) {
#define lold 0
  unsigned n=0,lnew;
  TCHAR *d,*end;
  TCHAR splitst[4]=L"?'\"";

  if (*dest) {
    if (eoltype>=NELEM(eoltypes)) eoltype=NELEM(eoltypes)-1;
    lnew=wcslen(eoltypes[eoltype]);
    splitst[0]=splitch;
    for(d=*dest,end=d+*destlen; (d=memcspn(d,end,splitst,sizeof(splitst)-1))<end; ) {
        if (*d=='"' || *d=='\'') {
        d=findnextquote(d,end,style);
        } else if (*d==splitch) {
                d++;
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end = *dest+*destlen;
        }
        if (lnew) {
          memcpy(d,eoltypes[eoltype],lnew);
          d+=lnew;
        }
        n++;
      }
    }
  }
failbreak:
  return(n);
#undef lold
}

// This removes all non quoted text from str. The quoted text segments that remain are separated by newst
// style is defined by findnextquote()
EXTERNC unsigned findqtstrings(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *insertst, unsigned style) {
  unsigned n=0,lold,lnew;
  TCHAR *d,*dp,*end;
  const TCHAR *newst;

  if (*dest) {
    newst=wcslen(insertst)>20 ? L"," : insertst;
    for(dp=*dest,end=dp+*destlen; (d=findnextquote(dp,end,style)); dp=d) {
      switch(*dp) {
      case '"' :
      case '\'':
        lold=0;
        lnew=wcslen(newst);
        break;
      default:
        lold=d-dp;
        lnew=0;
        d=dp;
        break;
      }
      if (lnew != lold) {
        d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		if (!*dest)
			goto failbreak;
        end = *dest+*destlen;
      }
      if (lnew) {
        memcpy(d,newst,lnew);
        d+=lnew;
      }
      n++;
    }
  }
failbreak:
  return(n);
}

// usetabs=1: convert leading spaces to tabs
// usetabs=0: convert leading tabs to spaces
// preserveodd=TRUE: non multiples of tabwidth spacing will be preserved, otherwise truncated (amending might overflow the buffer if we amend a lot of spaces)
EXTERNC unsigned space2tabs(TCHAR **dest, size_t *destsz, size_t *destlen, int usetabs, unsigned tabwidth, int preserveodd) {
  unsigned n=0,lnew,lold,indent;
  TCHAR *d,*dp,*end;

  if (*dest) {
    for(d=*dest,end=d+*destlen; d<end; ) {
      for(indent=0,dp=d; 1; dp++) {
        if (*dp==' ') indent++;
        else if (*dp=='\t') indent+=tabwidth;
        else break;
      }
      lold=(dp-d);
      if (usetabs) {
        lnew=indent/tabwidth;
        if (preserveodd && lnew*tabwidth!=indent) lnew=lold=0;
      } else lnew=indent;
          if (lnew != lold) {
            d += memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
			if (!*dest)
				goto failbreak;
            end = *dest+*destlen;
            n++;
          }
          if (lnew) {
            if (mymemset(d, usetabs ? '\t':' ',lnew) && lnew==lold) n++;
            d+=lnew;
          }
      d=memcspn(d,end,L"\r\n",2);
      while (d<end && (*d=='\r' || *d=='\n')) d++; //d+=strspn(d,"\r\n");
    }
  }
failbreak:
  return(n);
}

struct _MSTR g_cstring[]={
{L""                  ,0,0 ,0,0,0}, // mstring[0] is where the search string is stored from call to call
{L"//"                ,0,1 ,0,0,0}, // set case insensitive strings to 0 for better efficiency
{L"/*\0*/"            ,0,2 ,0,0,0},
{L"\""                ,0,3 ,0,0,0},
{L"'"                 ,0,3 ,0,0,0},
{L"if ("              ,0,4 ,0,0,0},
{L"if("               ,0,4 ,0,0,0},
{L"\r"                ,0,5 ,0,0,0},
{L"\n"                ,0,5 ,0,0,0},
{L"("                 ,0,6 ,0,0,0},
{L")"                 ,0,7 ,0,0,0},
{L"{"                 ,0,8 ,0,0,0},
{L"}"                 ,0,9 ,0,0,0},
{L" "                 ,0,10 ,0,0,0},
{L"\t"                ,0,10 ,0,0,0},
};

/* The MinGW C libraries are more space efficient than any other compiler. The MinGW C++ libraries are
   more wasteful than any other. C++ is unnecessary, inappropriate, and wasteful for projects that can easily
   be done in C. Including any C++ project would ruin the efficiency and reliability of this project.
   AStyle was poor C code in an even worse C++ wrapper, nothing I would want to reuse.
   AStyle also doesn't indent the way I like it and was not planned with an editor in mind.
   This does not handle certain if-else layouts because it is too much work to detect a single statement:
  if (cond)
     stmt;
  else
     stmt;*/
// http://sourceforge.net/project/showfiles.php?group_id=126649 http://sourceforge.net/projects/gcgreatcode/
// GNU Indent
EXTERNC unsigned reindentcode(TCHAR **dest, size_t *destsz, size_t *destlen, int usetabs, unsigned tabwidth) {
  unsigned n=0,bktct,lold,lnew,parenct,tempindent,mno;
  TCHAR *d,*dp,*label,*end;
  unsigned quick[512];

  n+=space2tabs(dest, destsz, destlen, 1, tabwidth, 0);
  if (*dest) {
    strmstrinit(g_cstring,NELEM(g_cstring),quick);
    for(tempindent=parenct=0,bktct= wcscspn(dp=d=*dest,L"\t"),end=d+*destlen; (dp=strmstr(dp,end,&mno,g_cstring,quick))<end; ) switch(g_cstring[mno].rv) {
    case 1: // C++ comment
      dp=memcspn(dp,end,L"\r\n",2); // this does not handle \ comment continuations which shouldn't be used anways
      break;
    case 2: /* C comment */
      dp=memstr(dp+2,end,g_cstring[mno].string+g_cstring[mno].len+1,wcslen(g_cstring[mno].string+g_cstring[mno].len+1));
      if (dp<end) dp+=2;
      //else dp= *dest + *destlen;
      break;
    case 3: // quoted string
      dp = findnextquote(dp,end,0);
      break;
    case 4: // if (
      dp += g_cstring[mno].len;
      parenct=1;
      break;
    case 5: // end of line
      if (*(dp-1)=='\\' && (*d=='#' || tempindent)) tempindent=2;
      dp+=(*dp=='\r' && dp[1]=='\n')?2:1;
      for(d=dp; *dp=='\t'; dp++); // d=bol, dp=currentPos
      lold=dp-d;
      lnew=bktct+parenct+(parenct?1:0)+(tempindent>1?1:0); // indent conditionals separated by lines
      if (*dp!=':') for (label=dp; *label; label++) {
        if (*label==':') {
          if (label[1]==':') label=NULL; // C++ class::function
          break;
        }
        if (!isalnum(*label)) {
          label=NULL;
          break;
        }// label will be non NULL if we hit the end of file with nothing but alpha: but this will be so rare that it's not worth catching
      } else label=NULL;
      if (/*{*/*dp=='}' || !memcmp(dp,"case ",5) || !memcmp(dp,"default:",8) || !memcmp(dp,"public:",7) || !memcmp(dp,"private:",8) || !memcmp(dp,"protected:",10)) {
        if (lnew) lnew--;
      } else if (!*dp || *d=='#' || *d=='\r' || *d=='\n' || label) lnew=0;
      if (lnew != lold) {
        d+=memmovearmtest((void**)dest,destsz,destlen,d+lnew,d+lold,1); if (!*dest) goto failbreak;
        end=*dest+*destlen;
        n++;
      }
      if (lnew) {
        if (mymemset(d,'\t',lnew) && lnew==lold) n++;
        d+=lnew;
      }
      // new feature: if *d=='#' skip to eol
      dp=d;
      if (tempindent) tempindent--;
      break;
    case 6: // (
      if (parenct) parenct++;
      goto dodef;
    case 7: // )
      if (parenct) parenct--;
      goto dodef;
    case 8: // {
      bktct++;
      goto dodef;
    case 9: // {
      bktct--; // fall through to default
    default:
dodef:  dp++;
      break;
    }
failbreak:
    strmstrclose(mstring);
  }
  if (!usetabs)
	  n+=space2tabs(dest, destsz, destlen, usetabs, tabwidth, 0);

  return(n);
}

// This removes all non quoted text from str. The quoted text segments that remain are separated by newst
// style is defined by findnextquote()
EXTERNC unsigned killwhitenonqt(TCHAR *dest, size_t *destlen, unsigned style) {
#define lnew 0
	unsigned n = 0, lold, mno; unsigned quick[512];
	TCHAR *d, *end;

	if (*dest) {
		strmstrinit(g_cstring, NELEM(g_cstring), quick);
		d = dest + wcscspn(dest, _T("\r\n\t "));
		end = dest + *destlen;
		while ((d = strmstr(d, end, &mno, g_cstring, quick)) < end) switch (g_cstring[mno].rv) {
		case 1: // C++ comment
			d = memcspn(d, end, _T("\r\n"), 2); // this does not handle \ comment continuations which shouldn't be used anways
			break;
		case 2: /* C comment */
			d = memstr(d + 2, end, g_cstring[mno].string + g_cstring[mno].len + 1, wcslen(g_cstring[mno].string + g_cstring[mno].len + 1));
			if (d < end)
				d += 2;
			break;
		case 3: // quoted string
			d = findnextquote(d, end, style);
			break;
		case 5: // end of line
			d = memspn(d, end, _T("\r\n\t "), 4);
			break;
		case 10: // space or tab
			lold = memspn(d, end, _T("\t "), 2) - d;
			if (lnew != lold) {
				memmovetest(d + lnew, d + lold, *destlen - (d - dest) - lold + 1);
				*destlen += lnew - lold;
				end += lnew - lold;
				n++;
			}
			break;
		default:
			d++;
			break;
		}
		strmstrclose(mstring);
	}

	return(n);
#undef lnew
}

// lines up text by adding or subtracting spaces to make a selected character
// line up. , and = are common characters. Can be used to line up
// comma separated tables or / marks to make comments even
// style=0, do not detect strings, otherwise style-1 is as findnextquote()
// new feature: bug: one of the NPP Menu structs does not line up properly
EXTERNC unsigned lineup(TCHAR **dest, size_t *destsz, size_t *destlen, char lch, int usetabs, unsigned tabwidth, unsigned style) {
  unsigned n=0;
  TCHAR *d,*dp,*end;
  unsigned tabno,pass,tabnol;
  unsigned mxpos,lold,lnew,flags;
#define FLAG_INDENT 1
#define FLAG_QUIT 2

  if (*dest) {
    if (lch && lch != ' ' && lch != '\t') {
        end=*dest+*destlen;
      for(tabno=0; tabno<999; tabno++) for(pass=0,mxpos=(unsigned)-1; pass<=1; pass++) {
          for(dp=*dest; dp<end; ) {
            for(flags=FLAG_INDENT,tabnol=0,d=dp; !(flags&FLAG_QUIT) && d<end && *d!='\r' && *d!='\n'; d++) {
            lold=lnew=0;
            if (style && (*d=='"' || *d=='\'')) {
              d=findnextquote(d,end,style-1)-1;
            } else if (*d == ' ') { // nothing
            } else if (*d == '\t') {
              lold=1;
              lnew=(flags&FLAG_INDENT)?tabwidth:1; // Scintilla only permits tabs at the beginning of the line without any interspersed spaces
            } else {
              flags &= (~FLAG_INDENT);
              if (*d == lch) {
                tabnol++;
                if (tabnol>tabno) {
                  flags |= FLAG_QUIT;
                  if (!pass) { // calculate the maximum tab
                    do {
                      d--;
                    } while(isspace(*d) && d>=dp);
                    d++;
                    if (mxpos<(unsigned)(d-dp) || mxpos==(unsigned)-1) mxpos=d-dp;
                  } else {   //apply the minimum tab, shrinking extra spaces if necessary
                    if ((unsigned)(d-dp)>mxpos) {
                      lold=(d-dp)-mxpos;
                      lnew=0;
                      d=dp+mxpos;
                    } else {
                      lold=0;
                      lnew=mxpos-(d-dp);
                    }
                  }
                }
              }
            }
            if (lnew != lold) {
              d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
			  if (!*dest)
				  goto failbreak;
              end = *dest + *destlen;
              n++;
            }
            if (lnew) {
              memset(d,' ',lnew);
              if (lnew==lold) n++;
            }
          }
          d=memcspn(d,end,L"\r\n",2); //while(*d && *d!='\r' && *d!='\n') d++;
          while(d<end && (*d =='\r' || *d =='\n')) d++;
          dp=d;
        }
        if (!pass) {
          if (mxpos==(unsigned)-1) break;
          //printf("Tab=%u Max=%u\n%s",tabno,mxpos,str);
        }
      }
    }
failbreak:
    n+=space2tabs(dest,destsz,destlen,usetabs,tabwidth,0);
  }
  return(n);
#undef FLAG_INDENT
#undef FLAG_STOP
}

// new feature: allow overwrite to insert part of newst if there is some space
EXTERNC unsigned filldown(TCHAR **dest, size_t *destsz, size_t *destlen, int insert) {
  unsigned n=0,chn,lold,lnew/*,detent*/;
  TCHAR *d,*end;
#define newst *dest

  if (*dest && *destlen && (d=memcspn(*dest, end=*dest+*destlen, _T("\r\n"), 2))>*dest) {
    lnew=d-*dest;
    goto bottest; do {
      chn=memcspn(d,end,L"\r\n",2)-d;
      if (lnew<=chn || insert) {
        lold=insert?0:lnew;
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end=*dest+*destlen;
          chn += lnew-lold;
        }
        memcpy(d,newst,lnew);
        n++;
      }
      d += chn;
bottest:
      while(d<end && (*d=='\r' || *d=='\n')) d++; // more efficient than strspn();
    } while(d<end);
  }
failbreak:
  return(n);
#undef newst
}

// adds up all the numbers separated by any non number characters
EXTERNC unsigned addup(TCHAR *str) {
	double sum = 0;

	if (str) do {
		switch (*str) {
		case '\0':
			MessageBoxFree(0, smprintf(_T("Sum=%#lg"), sum), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
			break;
		case '+':
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			sum += wcstod(str, &str);
		default:
			str++;
			continue;
		}
	} while (1);

	return(0);
}

EXTERNC unsigned zapchar(TCHAR *str, unsigned destlen, TCHAR ch, int nonprint) {
	TCHAR *end;
	int n = 0;
	if (str)
		for (end = str + destlen; str < end; str++) {
			BOOL isPrintable = iswprint(*str);
			if (*str != '\r' && *str != '\n' && *str != '\t' && *str != ch && (!nonprint || !iswprint(*str))) {
				*str = ch;
				n++;
			}
		}

	return(n);
}

EXTERNC unsigned submitW3C(TCHAR **dest, size_t *destsz, size_t *destlen, const TCHAR *fn, const TCHAR *fno) {
  if (*dest) {
    TCHAR *szPath=NULL;
	size_t uPathSz=0;
    do {
      char buf1[512];
      FILE *fp;
      NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL,fn);
      if (!(szPath /*=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,fn)*/) || !(fp= _wfopen(szPath, L"rt"))) break;
      //freesafe(szPath,"submitW3C");
      memset(buf1,0,sizeof(buf1));
      fread(buf1,1,sizeof(buf1)-1,fp);
      fclose(fp);
      NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXTEMP,NULL,&szPath,&uPathSz,NULL,fno);
      if (!szPath) break;
      //if (!(path=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,fno))) break;
      DeleteFile(szPath);
      //MessageBox(g_nppData._nppHandle,path,"Before",MB_OK|MB_ICONINFORMATION);
      if (!(fp= _wfopen(szPath, _T("wt"))))
		  break;
      //MessageBox(g_nppData._nppHandle,path,"After",MB_OK|MB_ICONINFORMATION);
      memstrtran(dest, destsz, destlen, NULL, _T("\r\n"), 2, _T("\n"), 1);
      strchrstrans(dest, destsz, destlen, NULL, _T("&<>\""), 4, _T(",&&amp;,<&lt;,>&gt;,\"&quot;,"));
      if (*dest) {
        fprintf(fp,buf1,*dest);
        fclose(fp);
        ShellExecute(g_nppData._nppHandle,L"open",szPath,NULL,L".",0);
      } else MessageBoxFree(g_nppData._nppHandle,smprintf(_T("Unable to execute:\r\n%s"), szPath),_T(PLUGIN_NAME),MB_OK|MB_ICONINFORMATION);
    } while(0);
    if (szPath)
		freesafe(szPath, _T("submitW3C"));
  }
  return(0);
}

unsigned g_fcmpcolumn=0;
BOOL g_SortLinesUnique=FALSE;
int g_fcmpnocase=0,g_fcmpascending=1;

// compares up to the end of a line or end of string starting at column
EXTERNC int QSORT_FCMP fcmpline(const void *ax,const void *bx) {
  char *a=*((char **)ax);char *b=*((char **)bx);
  int p,q,dif=0;
  unsigned tempcolumn;

  for(tempcolumn=g_fcmpcolumn; tempcolumn; tempcolumn--,a++,b++) {
    if (!*a || !*b || *a=='\r' || *a=='\n' || *b=='\r' || *b=='\n') return(0); //if either line is not long enough, pretend equal
  }
  while(*a && *b) {
    dif=g_fcmpnocase?(toupper(*a)-toupper(*b)):(*a-*b);
    q=(*b=='\r' || *b=='\n'); // here's a case where short-circuit logic makes us separate this out
    if ((p=(*a=='\r' || *a=='\n')) || q) {
      if (p&&q) dif=0;
      break;
    }
    if (dif) break;
    a++;
    b++;
  }
  return(g_fcmpascending?dif:-dif);
}

// copies only a single line, returns \0 end of dest
TCHAR *strcpyline(TCHAR *dest, TCHAR *src) {
  do {
    *dest=*src++;
    if (!*dest)
		break;
    //if (*dest=='\n' || (*dest=='\r' && *src!='\n') ) {dest++; *dest='\0'; break;}
    if (*dest=='\n' || *dest=='\r') {*dest='\0'; break;}
    dest++;
  } while(1);

  return(dest);
}

// new feature: numeric sort
// new feature: ignore leading spaces,tabs
EXTERNC unsigned strqsortlines(TCHAR **dest, size_t *destsz, size_t *destlen, int nocase, int ascending, unsigned column) {
  unsigned n=0/*,bytes*/,lineno,lineno2;
  size_t linessz;
  TCHAR *d,*dp,*nstr;
  TCHAR **lines=NULL;
  if (*dest) {
    g_fcmpcolumn=column;
    g_fcmpascending=ascending;
    for (d = *dest + wcsspn(*dest, _T("\r\n")), lineno=0,linessz=0; *d ; lineno++) {
      armreallocsafe((TCHAR**)&lines, &linessz, (lineno+1)*sizeof(*lines), ARMSTRATEGY_INCREASE, 0, _T("strqsortlines"));
	  if (!lines)
		  goto failbreak;
      lines[lineno]=d;
      d+= wcsspn(d, _T("\r\n"));
      while(*d && (*d=='\r' || *d=='\n')) d++;
    }
    if (!(nstr=(TCHAR *)mallocsafe(CHARSIZE(*destlen + 1), _T("strqsortlines"))))
		goto failbreak;
    //memset(nstr,0,*destlen+1);
    g_fcmpnocase=nocase;
    qsortintro(lines,lineno,sizeof(lines[0]),fcmpline);
	for (dp = *dest, d = nstr, lineno2 = 0; lineno2 < lineno; lineno2++) {
		if (lineno2 == 0 || !g_SortLinesUnique || fcmpline(lines + lineno2, lines + lineno2 - 1)) {
			if (!g_SortLinesUnique) {
				while (*dp == '\r' || *dp == '\n') {
					*d = *dp;
					d++;
					dp++;
					// import the line endings from the unsorted string
				}
			}
			else {
				if (*dp == '\r' || *dp == '\n') {
					*d = *dp; d++; dp++;
					if (*(dp - 1) == '\r' && *dp == '\n') {
						*d = *dp;
						d++;
						dp++;
					}
				}
			}
			dp += wcsspn(dp, _T("\r\n"));
			d = strcpyline(d, lines[lineno2]);
		}
	}
	if (!g_SortLinesUnique) {
		wcscpy(d,dp);
		d += wcslen(dp);
		// grab extra lines at the end
	}
#if NPPDEBUG /* { */
    if (!g_SortLinesUnique && *destlen!=(unsigned)(d-nstr))
      MessageBox(g_nppData._nppHandle, _T("The size of the sort buffer should not have changed"), _T(PLUGIN_NAME), MB_OK|MB_ICONWARNING);
#endif /* } */
    freesafe(*dest, _T("strqsortlines"));
    *dest=nstr;
    *destsz=*destlen+1;
    *destlen=d-nstr;
    n=1;
  }
failbreak:
  if (lines)
	  freesafe(lines, _T("strqsortlines"));

  return(n);
}

EXTERNC unsigned indentlines(TCHAR **dest, size_t *destsz, size_t *destlen, int usetabs, unsigned tabwidth) {
#if NPPDEBUG
    //DWORD tkc=GetTickCount();
#endif
#define lold 0
  unsigned n=0,lnew;
  int movezero;
  TCHAR *d,*end;
  if (*dest) {
    TCHAR *newdest=NULL,*newlast=*dest;
	size_t newsz = *destsz, newlen=0; // only used for HighPerformance
    for(d=memspn(*dest,end=*dest+*destlen, _T("\r\n"), 2),movezero=!isspace(*d) ; d<end; ) {
      if (movezero || isspace(*d)) {
        lnew=usetabs?1:tabwidth;
        if (HighPerformance) {
          if (newlast<d) {
			  memcpyarmsafe((void**)&newdest, &newsz, &newlen, newlast, d-newlast, _T("indentlines"));
			  if (!newdest)
				  goto freebreak;
		  }
          newlast=(d+=lold);
          armreallocsafe(&newdest, &newsz, CHARSIZE(newlen + lnew + 1) /* 1 reserved for \0 below */,
			  ARMSTRATEGY_INCREASE, 0, _T("indentlines"));
		  if (!newdest)
			  goto freebreak;
          memset(newdest+newlen, usetabs ? '\t':' ',lnew);
          newlen+=lnew;
          //end=*dest+*destlen;
        } else {
          if (lnew != lold) {
            d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
			if (!*dest)
				goto failbreak;
            end=*dest+*destlen;
          }
          memset(d,usetabs?'\t':' ',lnew);
        }
        n++;
      }
      d=memcspn(d, end, L"\r\n", 2);
      while(d<end && (*d=='\r' || *d=='\n')) d++; //d+=strspn(d,"\r\n");
    }
    if (HighPerformance) {
        if (n) {
			if (newlast<d)
				memcpyarmsafe((void**)&newdest, &newsz, &newlen,newlast,d-newlast, _T("indentlines"));
			else
				newdest[newlen]='\0'; // space reserved up above
freebreak:
			freesafe(*dest, _T("indentlines"));
			if (!(*dest=newdest))
				n=0;
			*destsz=newsz;
			*destlen=newlen;
        } else if (newdest)
			freesafe(newdest, _T("indentlines"));
    }
  }
failbreak:
#if NPPDEBUG
  //MessageBoxFree(g_nppData._nppHandle,smprintf("Ticks used %u",GetTickCount()-tkc),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
#endif
  return(n);
#undef lold
}

EXTERNC unsigned indentlinessurround(TCHAR **dest, size_t *destsz, size_t *destlen, int usetabs, unsigned tabwidth, unsigned eoltype) {
#define lold 0
  TCHAR *e,*ind=NULL;
  size_t indsz=0,indln;
  unsigned n=0,/*lnew,*/eollen;
  if (*dest) {
    if (eoltype>=NELEM(eoltypes))
		eoltype=NELEM(eoltypes)-1;
    eollen=wcslen(eoltypes[eoltype]);
    e=memspn(*dest, *dest + *destlen, _T("\r\n"), 2);
    memcpyarmsafe((void**)&ind, &indsz, &indln,*dest, memspn(e,*dest+*destlen, _T(" \t"), 2)-e, _T("indentlinessurround"));
	if (!ind)
		goto failbreak;
    n+=indentlines(dest,destsz,destlen,usetabs,tabwidth);
	if (!*dest)
		goto failbreak;
    memmovearmtest((void**)dest, destsz, destlen, *dest+indln+1+eollen, *dest+lold,1);
	if (!*dest)
		goto failbreak;
    memcpy(*dest,ind,indln);
    *(*dest+indln)='{';
    memcpy(*dest+indln+1,eoltypes[eoltype],eollen);
    n++;
    memcpyarmsafe((void**)dest, destsz, destlen, *dest, indln+eollen, _T("indentlinessurround"));
	if (!*dest)
		goto failbreak;
    //memcpy(*dest+*destlen-eollen-1-indln,ind,indln);
    *(*dest+*destlen-eollen-1)='}';
    memcpy(*dest+*destlen-eollen,eoltypes[eoltype],eollen);
    //(*destlen)-=2;
    //memcpyarmsafe(dest,destsz,destlen,"",1,"indentlinessurround"); if (!*dest) goto failbreak;
    //(*destlen)--;
    freesafe(ind, _T("indentlinessurround"));
  }
failbreak:
  return(n);
#undef lold
}

// http://forum.pspad.com/read.php?f=2&i=1404&t=1404
// new feature: hex editor in words
// new feature: printable hex only (assuming that Notepad++ would try to print characters<32)
EXTERNC unsigned tohex(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned offset, unsigned width) {
	unsigned n = 0, bufl;
	TCHAR *d, *end;
	TCHAR buf[128]; //,hex[3];
	unsigned counter8;
	if (*dest && width >= 16 && width <= sizeof(buf)) {
		TCHAR *newdest = NULL;
		size_t newsz = *destsz * 5, newlen = 0;
		d = *dest;
		end = *dest + *destlen;
		bufl = 0;
		counter8 = 7;
		while (d < end) {
			if (!bufl) sarmprintf(&newdest, &newsz, &newlen, _T("\"%9.9X  "), offset + n);
			sarmprintf(&newdest, &newsz, &newlen, (counter8 || bufl == width - 1) ? _T("%2.2X ") : _T("%2.2X-"), *(unsigned char *)d);
			if (counter8) counter8--; else counter8 = 7;
			buf[bufl++] = (*d == '\r' || *d == '\n' || *d == '\t' || *d == '"') ? '.' : *d;
			d++;
			n++;
			if (bufl == width || d >= end) {
				memsetarmsafe(&newdest, &newsz, &newlen, ' ', (width - bufl) * 3 + 2, _T("tohex"));
				memsetarmsafe(&newdest, &newsz, &newlen, '|', 1, _T("tohex"));
				memcpyarmsafe((void**)&newdest, &newsz, &newlen, buf, bufl, _T("tohex"));
				if (bufl < width) memsetarmsafe(&newdest, &newsz, &newlen, ' ', width - bufl, _T("tohex"));
				bufl = 0;
				strcpyarmsafe(&newdest, &newsz, &newlen, _T("|\"\r\n"), _T("tohex"));
			}
		}
		if (n) {
			memsetarmsafe(&newdest, &newsz, &newlen, '\0', 1, _T("tohex"));
			newlen--;
			//freebreak:
			freesafe(*dest, _T("tohex"));
			if (!(*dest = newdest))
				n = 0;
			*destsz = newsz;
			*destlen = newlen;
		}
		else if (newdest)
			freesafe(newdest, _T("tohex"));
	}
	//failbreak:
	return(n);
}

EXTERNC unsigned fromhex(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned offset) {
  unsigned n=0,val;
  int badoffset;
  TCHAR *d,*dp,*end;
  if (*dest) {
	TCHAR *newdest=NULL;
	size_t newsz=*destsz/5, newlen=0;
    d=*dest;
    end=*dest+*destlen;
    badoffset=FALSE;
    while(d<end) {
      if ((d[0]==' ' && d[1]==' ' && d[2]==' ') || *d=='|') d = memcspn(d,end,L"\r\n",2);
      else if ((*d>='0' && *d<='9') || (*d>='A' && *d<='F') || (*d>='a' && *d<='f')) {
        val=wcstol(d,&dp,16);
        if (dp-d==9) {
                if (offset+n != val) badoffset=offset+n-val;
        } else if (dp-d==2) {
          memsetarmsafe(&newdest, &newsz, &newlen,val,1, _T("fromhex"));
          n++;
        }
        d=dp;
      } else d++;
    }
    if (n) {
      memsetarmsafe(&newdest, &newsz, &newlen,'\0',1, _T("fromhex"));
	  newlen--;
//freebreak:
      freesafe(*dest, _T("fromhex"));
      if (!(*dest=newdest))
		  n=0;
      *destsz=newsz;
      *destlen=newlen;
      if (badoffset)
		  MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Offset has changed by %d"), badoffset), _T(PLUGIN_NAME), MB_OK|MB_ICONWARNING);

    } else if (newdest)
		freesafe(newdest, _T("fromhex"));
  }
//failbreak:
  return(n);
}

EXTERNC unsigned hexbyterunstolittlendian(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned bytewidth) {
  unsigned n=0,lold,lnew;
  TCHAR *d,*dp,*end;
  char buf[9]="??..??..";
  unsigned bufl,dp2;
  if (*dest && bytewidth>=2 && bytewidth<=4 && powcheck(bytewidth)) {
    for(d=*dest,end=*dest+*destlen ; d<end; d++) if ((*d==' ' && d[1]==' ' && d[2]==' ') || *d=='|') {
      d=memcspn(d,end,_T("\r\n"),2);
    } else {
      for(dp2=2,bufl=2*bytewidth,dp=d; dp<end && (unsigned)(dp-d)<3*bytewidth && bufl; dp++,dp2=dp2?(dp2-1):2) if (!dp2) {
        if (!(*dp==' ' || *dp=='-'))
			break;
      } else {
        if (!((*dp>='0' && *dp<='9') || (*dp>='A' && *dp<='F') || (*dp>='a' && *dp<='f')))
			break;
        buf[bufl-2*(1-(bufl&1))]=*dp;
        bufl--;
      }
      if ((dp==end || *dp==' ' || *dp=='-') && (lold=dp-d)==3*bytewidth-1) {
        lnew=2*bytewidth;
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end=*dest+*destlen;
        }
        memcpy(d,buf,lnew);
        d += lnew;
        n++;
      }
    }
  }
failbreak:
  return(n);
}

EXTERNC unsigned littlendiantohexbyteruns(TCHAR **dest, size_t *destsz, size_t *destlen) {
#define lnew buflen
  unsigned n=0;
  TCHAR *d,*dp,*dpe,*end;
  TCHAR *buf=NULL;
  size_t bufsz=0,buflen,lold;
  if (*dest && *destlen) {
    for(dp=d=*dest,end=*dest+*destlen ; 1; d++) {
        if (d>=end || !((*d>='0' && *d<='9') || (*d>='A' && *d<='F') || (*d>='a' && *d<='f'))) {
          if ( !((d-dp)&1)) { // even number of hex characters
            for(buflen=0,dpe=d-2; dpe>=dp; dpe-=2) {
              memcpyarmsafe((void**)&buf, &bufsz, &buflen, dpe, 2, _T("littlendiantohexbyteruns"));
			  if (!buf)
				  goto failbreak;
              if (dpe!=dp)
				  memsetarmsafe(&buf, &bufsz, &buflen,' ',1, _T("littlendiantohexbyteruns"));
			  if (!buf)
				  goto failbreak;
            }
            lold=d-dp;
            d=dp;
            if (lnew != lold) {
              d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
			  if (!*dest)
				  goto failbreak;
              end=*dest+*destlen;
            }
            memcpy(d,buf,lnew);
            d += lnew;
            n++;
          }
          dp=d+1;
        }
      if (d<end && ((*d==' ' && d[1]==' ' && d[2]==' ') || *d=='|')) {
        dp=d=memcspn(d, end, _T("\r\n"), 2);
      }
      if (dp>=end) break;
    }
  }
failbreak:
  if (buf)
	  freesafe(buf, _T("littlendiantohexbyteruns"));
  return(n);
#undef lnew
}

// http://forum.pspad.com/read.php?f=2&i=4073&t=4073
EXTERNC unsigned uudecode(TCHAR **dest, size_t *destsz, size_t *destlen) {
	unsigned n = 0; //,val;
	TCHAR *d, *end, *cp;
	TCHAR collector[3];
	unsigned linelen;
	if (*dest) {
		TCHAR *newdest = NULL;
		size_t newsz = *destsz / 5, newlen = 0;
		d = (TCHAR *)(*dest);
		end = (TCHAR *)(*dest) + *destlen;
		while (d < end) {
			if (*d >= 33 && *d <= 'M') {
				for (linelen = *d - 32, d++; linelen; ) {
					cp = collector;
					*cp = ((*d - 32) & 0x3F) << 2; // 6 bits
					d++;
					*cp |= ((*d - 32) >> 4) & 0x3; // +2 bits leaving 4 bits
					cp++;
					*cp = ((*d - 32) & 0xF) << 4; // 4 bits
					d++;
					*cp |= ((*d - 32) >> 2) & 0xF; // +4 bits leaving 2
					cp++;
					*cp = ((*d - 32) & 0x3) << 6; // 2 bits
					d++;
					*cp |= ((*d - 32) & 0x3F); // +6 bits
					d++;
					memcpyarmsafe((void**)&newdest, &newsz, &newlen, collector, (linelen > 3 ? 3 : linelen), _T("uudecode"));
					n++;
					linelen = linelen > 3 ? linelen - 3 : 0;
				}
			}
			d = memcspn(d, end, L"\r\n", 2);
			while (d < end && (*d == '\r' || *d == '\n'))
				d++;
		}
		if (n) {
			//freebreak:
			freesafe(*dest, _T("uudecode"));
			if (!(*dest = newdest))
				n = 0;
			*destsz = newsz;
			*destlen = newlen;
		}
		else if (newdest)
			freesafe(newdest, _T("uudecode"));
	}
	//failbreak:
	return(n);
}

TCHAR g_pszBase64[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

// http://forum.pspad.com/read.php?f=2&i=4073&t=4073
EXTERNC unsigned base64decode(TCHAR **dest, size_t *destsz, size_t *destlen) {
  unsigned n=0; //,val;
  TCHAR *d,*end,*cp,*t;
  TCHAR collector[64];
  TCHAR bits6;
  unsigned seq;
  if (*dest) {
	TCHAR *newdest=NULL;
	size_t newsz=*destsz/5, newlen=0;
    d=(TCHAR *)(*dest);
    end=(TCHAR *)(*dest)+*destlen;
    for(seq=3,cp=collector; d<end; d++) if ((t=wmemchr(g_pszBase64,*d,sizeof(g_pszBase64)-1))) {
          bits6=t-g_pszBase64;
          switch(seq) {
          case 3:
            *cp=bits6<<2; // set 6 bits, 2 not set
            seq=2;
            break;
          case 2:
            *cp|=(bits6>>4)&0x3;  // set 2 remaining bits
            cp++;
            *cp=(bits6&0xF)<<4; // set 4 bits, 4 not set
            seq=1;
            break;
          case 1:
            *cp|=(bits6>>2)&0xF;  // set 4 remaining bits
            cp++;
            *cp=(bits6&0x3)<<6; // set 2 bits, 6 not set
            seq=0;
            break;
          case 0:
            *cp|=bits6&0x3F; // set 6 remaining bits
            cp++;
            seq=3;
            break;
          }
          n++;
          if ((unsigned)(cp-collector)>=sizeof(collector)-1) {
            memcpyarmsafe((void**)&newdest, &newsz, &newlen,collector,cp-collector, _T("base64decode"));
            cp=collector;
          }
    }
    if (n) {
      if (cp>collector)
		  memcpyarmsafe((void**)&newdest, &newsz, &newlen,collector,cp-collector, _T("base64decode"));
      //memsetarmsafe(&newdest,&newsz,&newlen,'\0',1,"base64decode");
//freebreak:
      freesafe(*dest, _T("base64decode"));
      if (!(*dest=newdest))
		  n=0;
      *destsz=newsz;
      *destlen=newlen;
    } else if (newdest)
		freesafe(newdest, _T("base64decode"));
  }
//failbreak:
  return(n);
}

EXTERNC unsigned converteol(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned eoltype) {
	unsigned n = 0, lold, lnew;
	TCHAR *d, *end;

	if (*dest) {
		if (eoltype >= NELEM(eoltypes))
			eoltype = NELEM(eoltypes) - 1;
		lnew = wcslen(eoltypes[eoltype]);
		d = *dest;
		end = *dest + *destlen;
		goto bottest;
		do {
			lold = (*d == '\r' && d[1] == '\n') ? 2 : 1;
			if (lnew != lold) {
				d += memmovearmtest((void**)dest, destsz, destlen, d + lnew, d + lold, 1);
				if (!*dest)
					goto failbreak;
				end = *dest + *destlen;
			}
			memcpy(d, eoltypes[eoltype], lnew);
			d += lnew;
			n++;
		bottest:
			d = memcspn(d, end, L"\r\n", 2);
		} while (d < end);
	}

failbreak:
	return(n);
}

EXTERNC unsigned insertlinenumbers(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned lno) {
#define lold 0
#define lnew sizeof(buf)
  unsigned n=0;
  TCHAR *d,*end;
  TCHAR buf[9];
  if (*dest) {
    for(d=*dest,end=*dest+*destlen ; d<end; lno++) {
      snprintfX(buf, 9, _T("%*.*u"), sizeof(buf)-1, sizeof(buf)-1, lno);
      buf[lnew-1]=' ';
      if (lnew != lold) {
        d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		if (!*dest) goto failbreak;
        end=*dest+*destlen;
      }
      memcpy(d,buf,lnew);
      n++;
      d=memcspn(d,end,L"\r\n",2);
      if (*d=='\r' && d[1]=='\n')
		  d+=2;
      else
		  if (*d=='\r' || *d=='\n')
			  d++;
    }
  }
failbreak:
  return(n);
#undef lold
#undef lnew
}

// also used to delete line numbers
EXTERNC unsigned deletefirstword(TCHAR **dest, size_t *destsz, size_t *destlen) {
#define lnew 0
  unsigned n=0,lold;
  TCHAR *d,*end;
  if (*dest) {
    for(d=*dest,end=*dest+*destlen ; d<end; ) {
        lold=memcspn(d,end,L"\r\n\t ",4)-d;
        if (d[lold]==' ' || d[lold]=='\t') {
                lold++;
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end=*dest+*destlen;
          n++;
        }
        d=memcspn(d,end,L"\r\n",2);
        } else
			d+=lold;
      while (d<end && (*d=='\r' || *d=='\n'))
		  d++;
    }
  }
failbreak:
  return(n);
#undef lnew
}

// also used to delete line numbers
EXTERNC unsigned cleanemailquoting(TCHAR **dest, size_t *destsz, size_t *destlen) {
#define lnew 0
  unsigned n=0,lold;
  TCHAR *d,*end;
  if (*dest) {
    for(d=*dest,end=*dest+*destlen ; d<end; ) {
        lold=memspn(d,end,L"> \t",3)-d;
        if (lnew != lold) {
          d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		  if (!*dest)
			  goto failbreak;
          end=*dest+*destlen;
          n++;
        }
        d=memcspn(d,end,L"\r\n",2);
      while(d<end && (*d=='\r' || *d=='\n'))
		  d++;
    }
  }
failbreak:
  return(n);
#undef lnew
}

// http://www.textpad.info/forum/viewtopic.php?t=6645
EXTERNC unsigned extendblockspaces(TCHAR **dest, size_t *destsz, size_t *destlen) {
#define lold 0
  unsigned n=0,lnew,maxlength,pass;
  TCHAR *d,*end;
  if (*dest) {
    for(maxlength=0,pass=0; pass<2; pass++)
		for(d=*dest,end=*dest+*destlen ; d<end; ) {
        lnew=memcspn(d, end, _T("\r\n"), 2) - d;
        d += lnew;
        if (!pass) {
                if (maxlength<lnew) maxlength=lnew;
        } else {
                if (lnew<maxlength) {
          lnew=maxlength-lnew;
          if (lold != lnew) {
            d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
			if (!*dest)
				goto failbreak;
            end=*dest+*destlen;
          }
          memset(d,' ',lnew);
          d+=lnew;
          n++;
                }
        }
      if (*d=='\r' && d[1]=='\n')
		  d+=2;
      else
		  if (*d=='\r' || *d=='\n')
			  d++;
    }
  }
failbreak:
  return(n);
#undef lold
}

EXTERNC unsigned trimtrailingspace(TCHAR *dest, size_t *destlen) {
#define lnew 0
	unsigned n = 0, lold;
	TCHAR *d, *dp, *end;

	if (dest) {
		for (d = dest, end = dest + *destlen; d < end; ) {
			dp = d;
			d = memcspn(d, end, L"\r\n", 2);
			for (d--, lold = 0; d >= dp && *d == ' '; d--, lold++);
			d++;
			if (lnew != lold) {
				memmovetest(d + lnew, d + lold, *destlen - (d - dest) - lold + 1);
				*destlen += lnew - lold;
				end += lnew - lold;
				n++;
			}
			d = memspn(d, end, L"\r\n", 2);
		}
	}
	return(n);
#undef lnew
}

// http://www.crimsoneditor.com/english/board/CrazyWWWBoard.cgi?db=forum&mode=read&num=2339&page=161&ftype=6&fval=&backdepth=1
EXTERNC unsigned deleteblanklines(TCHAR *dest, size_t *destlen, unsigned surplus) {
#define lnew 0
	unsigned n = 0, lold;
	TCHAR *d, *end;
	unsigned surplus1;
	if (dest && surplus >= 1) {
		for (d = dest, end = dest + *destlen; d < end; ) {
			d = memcspn(d, end, L"\r\n", 2);
			for (surplus1 = surplus; d < end && surplus1; surplus1--) {
				if (*d == '\r' && d[1] == '\n')
					d += 2;
				else
					d++;
			}
			lold = memspn(d, end, L"\r\n", 2) - d;
			if (lnew != lold) {
				memmovetest(d + lnew, d + lold, *destlen - (d - dest) - lold + 1);
				*destlen += lnew - lold;
				end += lnew - lold;
				n++;
			}
		}
	}
	return(n);
#undef lnew
}

#if 0
EXTERNC unsigned deleteeverynthline(char *dest,unsigned destsz,unsigned *destlen,unsigned nthline) {
#define lnew 0
  unsigned n=0,lold,nthct;
  char *d,*end;
  if (dest && nthline>1) { // deleting every line isn't the purpose here
    //  MessageBoxFree(g_nppData._nppHandle,smprintf("nthct:%u lold:%u",nthct,lold),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
    for(nthline--,nthct=0,d=dest,end=dest+*destlen;d<end; ) {
      lold=memcspn(d,end,"\r\n",2)-d;
      if (d<end-lold) lold+=(d[lold]=='\r' && d[lold+1]=='\n')?2:1;
      if (!nthct) {
        if (lnew != lold) {
          memmovetest(d+lnew,d+lold,*destlen-(d-dest)-lold+1);
          *destlen += lnew-lold;
          end += lnew-lold;
          n++;
        }
        nthct=nthline;
      } else {
        d+=lold;
        nthct--;
      }
    }
  }
  return(n);
#undef lnew
}
#endif

EXTERNC unsigned asciiEBCDIC(TCHAR *str1,unsigned destlen,const TCHAR *tabletofrom,const TCHAR *cvtfile,int secondhalf) {
  TCHAR *str=str1,*end;
  TCHAR temp[513];
  TCHAR *szPath=NULL;
  size_t uPathSz=0;
  HANDLE fp;
  unsigned n=0;
  if (str) {
    if (secondhalf) secondhalf=256;
    memcpy(temp,tabletofrom,sizeof(temp)-1);
    NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL,cvtfile);
    if (szPath/*(path=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,cvtfile))*/) {
      if (INVALID_HANDLE_VALUE!=(fp=_openX(szPath,O_RDONLY,O_DENYWRITE))) {
        if (_readX(fp,temp,sizeof(temp))!=sizeof(temp)-1) {
          MessageBoxFree(g_nppData._nppHandle,
			  smprintf(_T("%s must be exactly 512 bytes.\r\nThe first 256 bytes convert standard->special.\r\nThe remaining 256 bytes convert special->standard\r\nUsing default table!"), szPath),
			  _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
          memcpy(temp,tabletofrom,sizeof(temp)-1);
        }
        _closeX(fp);
      }
      freesafe(szPath, _T("asciiEBCDIC"));
    }
    for(end=str+destlen;str<end;str++) {
      unsigned char och,nch;
      och=str[0];
      nch=temp[och+secondhalf];
      if (och != nch) {
        str[0]=nch;
        n++;
      }
    }
  }
  return(n);
}

//http://www.room42.com/store/computer_center/c_translate.shtml
//http://support.microsoft.com/kb/q216399/
//http://www.usc.edu/isd/doc/statistics/help/multiuse/datasamples/
TCHAR g_tb512ASCII_To_EBCDIC []={ // EBCDIC->ASCII is second half of table
0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xAD,0xE0,0xBD,0x5F,0x6D,
0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07,
0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xE1,
0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
0x58,0x59,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x70,0x71,0x72,0x73,0x74,0x75,
0x76,0x77,0x78,0x80,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x9A,0x9B,0x9C,0x9D,0x9E,
0x9F,0xA0,0xAA,0xAB,0xAC,0x4A,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
0xB8,0xB9,0xBA,0xBB,0xBC,0x6A,0xBE,0xBF,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xDA,0xdB,
0xDC,0xDD,0xDE,0xDF,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
0x20,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xD5,0x2E,0x3C,0x28,0x2B,0x7C,
0x26,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0x21,0x24,0x2A,0x29,0x3B,0x5E,
0x2D,0x2F,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xE5,0x2C,0x25,0x5F,0x3E,0x3F,
0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,0xC2,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
0xC3,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,
0xCA,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
0xD1,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xD2,0xD3,0xD4,0x5B,0xD6,0xD7,
0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0x5D,0xE6,0xE7,
0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,
0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,
0x5C,0x9F,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF};

//http://www.siber.com/sib/russify/ms-windows/
TCHAR g_tb512_CP1251toKOI8_R[]={ // KOI8-R -> ANSI is the second half
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
0x80,0x81,0x27,0x83,0x22,0x85,0x86,0x87,0x88,0x89,0x8A,0x3C,0x8C,0x8D,0x8E,0x8F,
0x90,0x60,0x27,0x22,0x22,0x95,0x2D,0x2D,0x98,0x99,0x9A,0x3E,0x9C,0x9D,0x9E,0x9F,
0x9A,0xA1,0xA2,0xA3,0x24,0xA5,0x7C,0xA7,0xB3,0xBF,0xAA,0x22,0x5E,0x2D,0xAE,0xAF,
0x9C,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0x9E,0xA3,0x23,0xBA,0x22,0xBC,0xBD,0xBE,0xBF,
0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,
0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,
0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1,
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0xA0,0x9B,0xB0,0x9D,0xB7,0x9F,
0xA0,0xA1,0xA2,0xB8,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
0xB0,0xB1,0xB2,0xA8,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xA9,
0xFE,0xE0,0xE1,0xF6,0xE4,0xE5,0xF4,0xE3,0xF5,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,
0xEF,0xFF,0xF0,0xF1,0xF2,0xF3,0xE6,0xE2,0xFC,0xFB,0xE7,0xF8,0xFD,0xF9,0xF7,0xFA,
0xDE,0xC0,0xC1,0xD6,0xC4,0xC5,0xD4,0xC3,0xD5,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,
0xCF,0xDF,0xD0,0xD1,0xD2,0xD3,0xC6,0xC2,0xDC,0xDB,0xC7,0xD8,0xDD,0xD9,0xD7,0xDA};

EXTERNC unsigned wordcount(TCHAR *str, unsigned destlen) {
	TCHAR *end;
	unsigned linelen;
	int wasspace;
	unsigned words = 0, chrasutf8 = 0, chrnsp = 0, chrsp = 0, lines = 0, linesnonblank = 0, linelongest = 0, lineshortest = (unsigned)-1;

	if (str) {
		for (linelen = 0, wasspace = TRUE, end = str + destlen; str < end; str++) {
			switch (*str) {
			case '\r':
				if (str[1] == '\n')
					str++;
			case '\n':
				if (linelen) linesnonblank++;
				if (linelen > 0 && lineshortest > linelen)
					lineshortest = linelen;
				if (linelongest < linelen)
					linelongest = linelen;
				lines++;
				wasspace = TRUE;
				linelen = 0;
				break;
			case '\t':
			case ' ':
				chrasutf8++;
				chrsp++;
				wasspace = TRUE;
				linelen++;
				break;
			default:
				if (*(unsigned char *)str < 0x80 || *(unsigned char *)str>0xC0)
					chrasutf8++; //http://www.cl.cam.ac.uk/~mgk25/unicode.html#mod
				if (wasspace)
					words++;
				chrsp++;
				chrnsp++;
				wasspace = FALSE;
				linelen++;
				break;
			}
		}
		if (linelen) {
			lines++;
			linesnonblank++;
		}
		MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Words:%u\r\nCharacters (no spaces):%u\r\nCharacters:%u\r\nUTF-8 Characters:%u\r\nUTF-8 Efficiency:%u%%\r\nLines:%u\r\nLongest Line:%u\r\nShortest nonblank line:%u\r\nNonblank lines:%u"),
			words, chrnsp, chrsp, chrasutf8, chrnsp ? (100 * chrasutf8) / chrsp : 0, lines, linelongest, lineshortest, linesnonblank), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
	}
	return(0);
}

// SECTION: END
// SECTION: Beginning of Imported Functions

#if ENABLE_TIDYDLL /* { */
// __fastcall is unacceptable as a calling sequence becuase it is not compatible from compiler to compiler http://support.microsoft.com/kb/100832
// I compiled libTidy.dll with __fastcall in MinGW32 and NPPTextFX with __fastcall in Borland-C and I got crash city!
#ifndef TIDY_CALL_CDECL
#define TIDY_CALL_CDECL 1 /* All the web sources for libTidy.dll that I can find (Sep 2005) are using __cdecl which is the best one! */
#endif
#ifndef TIDY_CALL_STDCALL
#define TIDY_CALL_STDCALL 0
#endif

// These #defines are set up for runtime linking. They are easily changed to produce static or dynamic linking.
#if TIDY_CALL_CDECL /* { */
#define TIDY_CALL __cdecl
#define TIDY_CALLQT "__cdecl"
#define TIDYGENDLLINDIRECT2(DLLFUNCTYPE,DLLFUNCNAME,FSTCALLSIZE,DLLFUNCARGS) if (!(TIDYDLLCALL(DLLFUNCNAME)=(TIDYTD_##DLLFUNCNAME *)GetProcAddress(g_hTidyDLL,#DLLFUNCNAME))) {errfunc=#DLLFUNCNAME; goto fail;}
#elif TIDY_CALL_STDCALL
#define TIDY_CALL __stdcall
#define TIDY_CALLQT "__stdcall"
#define TIDYGENDLLINDIRECT2(DLLFUNCTYPE,DLLFUNCNAME,FSTCALLSIZE,DLLFUNCARGS) if (!(TIDYDLLCALL(DLLFUNCNAME)=(TIDYTD_##DLLFUNCNAME *)GetProcAddress(g_hTidyDLL,"_" #DLLFUNCNAME FSTCALLSIZE))) {errfunc="_" #DLLFUNCNAME FSTCALLSIZE; goto fail;}
#else
#error You must pick TIDY_CALL_CDECL or TIDY_CALL_STDCALL
#endif /* } */
#include "../tidy/tidylib/include/tidy.h"
#include "../tidy/tidylib/include/buffio.h"

#define TIDYDLLCALL(DLLFUNCNAME) TIDYFC_##DLLFUNCNAME
#define TIDYGENDLLINDIRECT1(DLLFUNCTYPE,DLLFUNCNAME,FSTCALLSIZE,DLLFUNCARGS) typedef DLLFUNCTYPE (FAR TIDY_CALL TIDYTD_##DLLFUNCNAME)DLLFUNCARGS; TIDYTD_##DLLFUNCNAME *TIDYDLLCALL(DLLFUNCNAME)
#define TIDYGENDLLINDIRECT TIDYGENDLLINDIRECT1
#define TIDYDLLINIT \
  TIDYGENDLLINDIRECT(TidyDoc,tidyCreate,"@0",(void));\
  TIDYGENDLLINDIRECT(void,tidyRelease,"@4",(TidyDoc));\
  TIDYGENDLLINDIRECT(Bool,tidyFileExists,"@4",(ctmbstr));\
  TIDYGENDLLINDIRECT(int,tidyLoadConfig,"@8",(TidyDoc,ctmbstr));\
  TIDYGENDLLINDIRECT(Bool,tidyOptResetToDefault,"@8",(TidyDoc,TidyOptionId));\
  TIDYGENDLLINDIRECT(Bool,tidyOptGetBool,"@8",(TidyDoc,TidyOptionId));\
  TIDYGENDLLINDIRECT(Bool,tidyOptSetBool,"@12",(TidyDoc,TidyOptionId,Bool));\
  TIDYGENDLLINDIRECT(Bool,tidyOptSetInt,"@12",(TidyDoc,TidyOptionId,ulong));\
  TIDYGENDLLINDIRECT(ulong,tidyOptGetInt,"@8",(TidyDoc,TidyOptionId));\
  TIDYGENDLLINDIRECT(ctmbstr,tidyOptGetValue,"@8",(TidyDoc,TidyOptionId));\
  TIDYGENDLLINDIRECT(Bool,tidyOptParseValue,"@12",(TidyDoc,ctmbstr,ctmbstr));\
  TIDYGENDLLINDIRECT(Bool,tidyOptSetValue,"@12",(TidyDoc,TidyOptionId,ctmbstr));\
  TIDYGENDLLINDIRECT(int,tidySetCharEncoding,"@8",(TidyDoc,ctmbstr));\
  TIDYGENDLLINDIRECT(FILE*,tidySetErrorFile,"@8",(TidyDoc,ctmbstr));\
  TIDYGENDLLINDIRECT(int,tidySetErrorBuffer,"@8",(TidyDoc,TidyBuffer*));\
  TIDYGENDLLINDIRECT(uint,tidyErrorCount,"@4",(TidyDoc));\
  TIDYGENDLLINDIRECT(uint,tidyWarningCount,"@4",(TidyDoc));\
  TIDYGENDLLINDIRECT(uint,tidyAccessWarningCount,"@4",(TidyDoc));\
  TIDYGENDLLINDIRECT(int,tidyParseString,"@8",(TidyDoc,ctmbstr));\
  TIDYGENDLLINDIRECT(int,tidyCleanAndRepair,"@4",(TidyDoc));\
  TIDYGENDLLINDIRECT(int,tidyRunDiagnostics,"@4",(TidyDoc));\
  TIDYGENDLLINDIRECT(int,tidySaveBuffer,"@8",(TidyDoc,TidyBuffer*));\
  TIDYGENDLLINDIRECT(void,tidyBufFree,"@4",(TidyBuffer*));\
  TIDYGENDLLINDIRECT(ctmbstr,tidyReleaseDate,"@0",(void));\
  TIDYGENDLLINDIRECT(TidyOptionId,tidyOptGetIdForName,"@4",(ctmbstr));

TIDYDLLINIT /* Here TIDYDLLINIT creates all the typedefs and function pointers */
#undef TIDYGENDLLINDIRECT
#define TIDYGENDLLINDIRECT TIDYGENDLLINDIRECT2
/* Later TIDYDLLINIT is reused to perform the imports. One code block produces both which
   makes it easier to add new functions and ensures that every function pointer is
   guaranteed to get an import address. These macros make LoadLibrary() as easy to use
   as producing an import LIB. */

#define NPPTIDY_CFG_MENU NPPTEXT("HTMLTIDY.CFG")
#define NPPTIDY_CFG L"HTMLTIDY.CFG"
unsigned g_ScintillaEOLtoTidy[3]={TidyCRLF,TidyCR,TidyLF};
// functions are extracted from tidy.c:main() and the sample code in tidy.h
EXTERNC unsigned tidyHTML(TCHAR **dest,unsigned *destsz,unsigned *destlen,unsigned eoltype,unsigned tabwidth) {
  unsigned rv=0;
  if (*dest) {
	TCHAR *str=*dest;
    unsigned sln=*destlen;
    TCHAR *szPath=NULL;
	unsigned uPathSz=0;
    do {
      NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXTEMP,NULL,&szPath,&uPathSz,NULL,NPPTIDY_CFG);
      if (!isFileExist(szPath)) NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL,NPPTIDY_CFG);
      if (!(szPath/*=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,NPPTIDY_CFG)*/)) break;
      TidyBuffer output;
	  memset(&output,0,sizeof(output));
      TidyBuffer errbuf;
	  memset(&errbuf,0,sizeof(errbuf));
      int rc = -1;
      BOOL ok;

	  size_t i;
	  char pMBBuffer[_MAX_PATH];
	  wcstombs_s(&i, pMBBuffer, (size_t)_MAX_PATH, szPath, (size_t)_MAX_PATH);
      TidyDoc tdoc = TIDYDLLCALL(tidyCreate)();                     // Initialize "document"
      if ( TIDYDLLCALL(tidyFileExists)(pMBBuffer) ) {
        int status = TIDYDLLCALL(tidyLoadConfig)( tdoc, pMBBuffer);
        if ( status != 0 ) {
          MessageBoxFree(g_nppData._nppHandle,smprintf("Loading config file \"%s\" failed\r\nErr = %d (bad option?)",szPath,status), _T(PLUGIN_NAME), MB_OK|MB_ICONWARNING);
          //ok=no;
          break;
        } else ok=yes;
      } else {
        if (eoltype>=NELEM(g_ScintillaEOLtoTidy)) eoltype=NELEM(g_ScintillaEOLtoTidy)-1;
        ok = TIDYDLLCALL(tidyOptSetBool)(tdoc, TidyXhtmlOut, yes ) && // Convert to XHTML
             TIDYDLLCALL(tidyOptSetInt)(tdoc, TidyIndentSpaces,tabwidth) &&
             TIDYDLLCALL(tidyOptSetInt)(tdoc, TidyTabSize,tabwidth) &&
             TIDYDLLCALL(tidyOptSetInt)(tdoc, TidyNewline,g_ScintillaEOLtoTidy[eoltype]);
        // there might be a document type we can set from from Scintilla/Notepad++
        MessageBoxFree(g_nppData._nppHandle,smprintf("Config file \"%s\"\r\nnot found! I'll pick whatever defaults I want to!",szPath), _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
      }
      if (ok     ) rc = TIDYDLLCALL(tidySetErrorBuffer)( tdoc, &errbuf );      // Capture diagnostics
// XXX Disabled:
// XXX Character conversion missing
//      if (rc >= 0) rc = TIDYDLLCALL(tidyParseString)( tdoc, str );           // Parse the input
      if (rc >= 0) rc = TIDYDLLCALL(tidyCleanAndRepair)( tdoc );               // Tidy it up!
      if (rc >= 0) rc = TIDYDLLCALL(tidyRunDiagnostics)( tdoc );               // Kvetch
      if (rc > 1 ) rc = (TIDYDLLCALL(tidyOptSetBool)( tdoc, TidyForceOutput, yes)?rc:-1); // If error, force output.
      if (rc >= 0) rc = TIDYDLLCALL(tidySaveBuffer)( tdoc, &output );          // Pretty Print
      if (rc >= 0) {
		TCHAR *str2;
		if (!output.size) {
//		} else if ((str2=strdupsafe((char *)output.bp,"tidyHTML"))) {
//          freesafe(str,"tidyHTML");
//          *dest=str=str2;
//          textBufferLength=output.size;
//          *destsz=textBufferLength+1;
          rv=1;
        } else
			MessageBox(g_nppData._nppHandle,"Out of memory in " _T(PLUGIN_NAME) " (not a HTMLtidy error)", _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
        //freesafe(szPath,"tidyHTML");
        NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXTEMP,NULL,&szPath,&uPathSz,NULL, _T("HTMLTIDY.ERR"));
        if (!(szPath/*=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,"HTMLTIDY.ERR")*/)) goto tidyfree;
        DeleteFile(szPath);
        if (rc > 0) {
          MessageBoxFree(g_nppData._nppHandle,smprintf("Writing error summary to\r\n%s",szPath), _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
          FILE *fo;
          if ((fo=_wfopen(szPath,_T("wt")))) {
            fputs((char *)errbuf.bp,fo);
            fclose(fo);
          }
        }
      } else {
        MessageBoxFree(g_nppData._nppHandle,smprintf("A severe error (%d) occurred.\n",rc), _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
      }
tidyfree:
      TIDYDLLCALL(tidyBufFree)(&output);
      TIDYDLLCALL(tidyBufFree)(&errbuf);
      TIDYDLLCALL(tidyRelease)(tdoc);
    } while(0);
    if (szPath) freesafe(szPath,"tidyHTML");
    *destlen=sln;
  }
  return(rv);
}
#endif /* } */

// SECTION: END
// SECTION: Beginning of Windows Only Library Functions

#include "Scintilla\UniConversion.h" /* REM this out to use the Windows routines */
// returns malloc() clipboard text in UTF-8 or ANSI which the caller is expected to free
// returns NULL if unable or the clipboard is empty
// writes size to *size, which may be NULL if the caller does not need to know the size
// binary==TRUE: size=size of entire buffer; You need to specify a size parameter.
// binary=FALSE: size=size of text not including \0. Buffer size is always +1.
// eolType = SCI_GETEOLMODE; can be set to any value if SCDS_PASTETOEDITOREOL is not set.
// This unified set of flags is used by several routines. They are unified so the flags can
// be passed among them without requiring flag bit filtering or translation. Routines are
// allowed to filter flag bits if they want to. Each routine only
// looks at the bits applicable to itself.
/* At this point it appears that in a roundabout way, these Clipboard routines are as functional
   as the ones in Scintilla. The only difference is in the technique to convert
   UCS2 <-> ANSI. Scintilla prefers CF_UNICODE and uses MultiByteToWideChar/WideCharToMultiByte
   to convert from/to ANSI. I simply read CF_TEXT allowing Windows to do whatever conversion it wants to.
   It remains to be seen whether or not there is an actual difference between these techniques
   and which technique the affected users prefer. */
#define SCDS_COPY 0x1   /* To cut, specify SCDS_COPY|SCDS_DELETE */
#define SCDS_DELETE 0x2
#define SCDS_COPYRECTANGULAR 0x4 /* Mark the text as from a rectangular selection according to the Scintilla standard */
#define SCDS_COPYAPPEND 0x8 /* Reads the old clipboard (depends on flags) and appends the new text to it */
#define SCDS_COPYEOLALWAYSCRLF 0x10 /* EOL are converted CRLF before placing on the clipboard so the text will paste properly in apps that can't handle CR or LF termination. */
#define SCDS_COPYALSOUTF8 0x20 /* UNICODE is always copied in UNICODEMODE, this also copies UTF-8 into CF_TEXT in UNICODE */
#define SCDS_COPYCONVERTNULLSTOSPACE 0x40 /* Convert NUL to space for non binary safe editors */
#define SCDS_COPYNOTUNICODE 0x80 /* Do not convert UTF-8 to UNICODE for CF_UNICODETEXT. System will automatically produce wchar_t UTF-8 which will paste into any application as UTF-8. */
#define SCDS_UNICODEMODE 0x100 /* Paste: Paste CF_TEXT in ANSI mode, CF_UNICODETEXT in UTF-8 OR UCS-2 modes; Copy: produce CF_UNICODETEXT in UTF-8/UCS-2 modes, produce CF_TEXT in ANSI mode */
#define SCDS_COPYPASTEBINARY 0x200 /* Retrieve entire Clipboard with last nul character removed; otherwise paste strlen() */
#define SCDS_PASTEANSIUTF8 0x400 /* Always Paste from CF_TEXT */
#define SCDS_PASTETOEDITOREOL 0x800 /* Convert all EOL's to SCI_GETEOLMODE */

#define CLIPPADSIGTEXT "BinaryDWORDLength:"
#define CLIPPAD 32 /* Probably a power of 2! This value was determined from pfclipboardsizetest() output in Windows 98 */
#define CLIPPADBLOCKNT 1 /* Set to 1 to force padding on NT */
EXTERNC DWORD ClipboardBufferPadSize(DWORD buflen,char *sigtext,unsigned flags) {
  if ((flags&SCDS_COPYPASTEBINARY) && (!g_fOnNT || CLIPPADBLOCKNT)) {
    buflen += strlen(sigtext)+sizeof(buflen)+CLIPPAD-1; /* round up to next multiple of CLIPPAD */
    buflen /= (CLIPPAD);
    buflen *= (CLIPPAD);
  }
  return buflen;
}
#undef CLIPPAD

EXTERNC void ClipboardBufferPad(TCHAR *buf,DWORD bufsz,DWORD buflen,char *sigtext,unsigned flags) {
  if ((flags&SCDS_COPYPASTEBINARY) && bufsz>buflen && (!g_fOnNT || CLIPPADBLOCKNT)) {
    DWORD st=strlen(sigtext);
    if (bufsz>=st+sizeof(bufsz)) {
      ZeroMemory(buf+buflen,bufsz-buflen); //MessageBoxFree(g_nppData._nppHandle,smprintf("bufsz:%u buflen:%u\r\n%s\r\n",bufsz,buflen,sigtext,buf),PLUGIN_NAME, MB_OK);
      memcpy(buf+bufsz-st-sizeof(bufsz),sigtext,st);
      *(DWORD *)(buf+bufsz-sizeof(bufsz))=buflen;
    }
  }
}

EXTERNC DWORD ClipboardBufferUnPad(char *buf,DWORD bufsz,char *sigtext,unsigned flags) {
  if ((flags&SCDS_COPYPASTEBINARY) && (!g_fOnNT || CLIPPADBLOCKNT)) {
    DWORD st=strlen(sigtext);
    if (bufsz>=st+sizeof(DWORD) && !memcmp(buf+bufsz-st-sizeof(bufsz),sigtext,st)) {
      DWORD rv=*(DWORD *)(buf+bufsz-sizeof(bufsz));
      if (rv>0 && rv<=bufsz) return rv;
    }
  }
  return bufsz;
}
#undef CLIPPADBLOCKNT

// new feature: Unlike SCI_PASTE, this does not explicitly perform UNICODE -> Ansi-Codepage but allows Windows to do it
EXTERNC TCHAR *strdupClipboardText(unsigned *size,unsigned flags,unsigned eoltype,BOOL *isrectangular)  {
  TCHAR *rv=NULL;
  UINT cf;
  unsigned chsize;

  //wchar_t uc=0xFF11; unsigned u=uc; MessageBoxFree(g_nppData._nppHandle,smprintf("unsigned:%u wchar_t:%u",u,uc),PLUGIN_NAME, MB_OK); // this is a test to see if wchar_t is unsigned
  if (size) *size=0;
  if (isrectangular) *isrectangular=IsClipboardFormatAvailable(g_cfColumnSelect);
  if (flags&SCDS_COPYPASTEBINARY) flags &= ~(SCDS_COPYEOLALWAYSCRLF|SCDS_PASTETOEDITOREOL|SCDS_COPYCONVERTNULLSTOSPACE|SCDS_UNICODEMODE|SCDS_COPYNOTUNICODE|SCDS_PASTEANSIUTF8);
  if ((flags&SCDS_PASTEANSIUTF8) || !IsClipboardFormatAvailable(CF_UNICODETEXT)) flags &= ~SCDS_UNICODEMODE; // safe as long as we don't pass the flags to another function
  if (flags&SCDS_UNICODEMODE) {
    cf=CF_UNICODETEXT;
    chsize=2;
  } else {
    cf=CF_TEXT;
    chsize=1;
  }
  if (IsClipboardFormatAvailable(cf) && OpenClipboard(g_nppData._nppHandle)) {
    DWORD clipsz;
    HGLOBAL   hglb;
    char *clip;
    if ((hglb = GetClipboardData(cf)) && (clipsz=GlobalSize(hglb))>chsize && (clip = (char *)GlobalLock(hglb))) {
      //MessageBoxFree(g_nppData._nppHandle,smprintf("Binary %s clipsz:%u",(flags&SCDS_COPYPASTEBINARY)?"TRUE":"FALSE",clipsz),PLUGIN_NAME, MB_OK);
      clipsz=ClipboardBufferUnPad(clip,clipsz,CLIPPADSIGTEXT,flags);
      if (flags&SCDS_UNICODEMODE) { // guarantee a \0 for misbehaving applications
        *((wchar_t *)clip+clipsz/2-1)=L'\0';
      } else {
        clip[clipsz-1]='\0';
      }
      unsigned clipszextraorig;
      if (flags&SCDS_COPYPASTEBINARY) {
        clipsz-=chsize;
        clipszextraorig=0;
      } else {
        clipsz=(flags&SCDS_UNICODEMODE)?(wcslen((wchar_t *)clip)*2):strlen(clip);
        clipszextraorig=chsize;
        //MessageBoxFree(g_nppData._nppHandle,smprintf("clipsize:%u clipszextraorig:%u",clipsz,clipszextraorig),PLUGIN_NAME, MB_OK);
      }
      if (clipsz) {
		  size_t rvlen = clipsz;
		  size_t rvsz = clipsz + clipszextraorig;
        if (flags&SCDS_UNICODEMODE) {
#ifdef UNICONVERSION_H
          rvsz=UTF8FromUCS2((wchar_t *)clip,rvsz/2,NULL,0,0);
#else
          rvsz=WideCharToMultiByte(CP_UTF8,0,(wchar_t *)clip,rvsz/2,NULL,0,NULL,NULL);
#endif
          rvlen=rvsz-(clipszextraorig?1:0);
        }
        if ((rv=(TCHAR *)mallocsafe(rvsz, _T("strdupClipboardText")))) {
 		  if (flags&SCDS_UNICODEMODE) {
#ifdef UNICONVERSION_H
				// XXX Disabled for now:
				//            UTF8FromUCS2((wchar_t *)clip,(clipsz+clipszextraorig)/2,rv,rvsz,0);
#else
				WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)clip, (clipsz + clipszextraorig) / 2, rv, rvsz, NULL, NULL);
#endif
		  } else
			  memcpy(rv,clip,rvsz);
          if (flags&SCDS_PASTETOEDITOREOL) {
            //MessageBoxFree(g_nppData._nppHandle,smprintf("Converting EOL to %u",eolType),PLUGIN_NAME, MB_OK);
            converteol(&rv, &rvsz, &rvlen, eoltype);
            if (!(flags&SCDS_COPYPASTEBINARY) && rv) {
              armreallocsafe(&rv, &rvsz, CHARSIZE(rvlen + 1), ARMSTRATEGY_REDUCE, 0, _T("strdupClipboardText"));
              if (rv)
				  rv[rvlen]='\0';
            }
          }
          if (rv && size) *size=rvlen;
         }
       }
       GlobalUnlock(hglb);
     }
    CloseClipboard();
  }
  return rv;
}

// DO NOT \0 NUL terminate the string, it will be added automatically; buflen should be the length of the actual data
// returns TRUE if copying succeeded
// depending on the flags, the original buffer may get modified
// http://www.ultraedit.com/index.php?name=Forums&file=viewtopic&t=664
EXTERNC BOOL SetClipboardDataSingle(TCHAR **buf, size_t *bufsz, size_t *buflen, unsigned flags, unsigned eoltype)  {
  BOOL rv=FALSE;

  if (*buf && *buflen) do {
    HGLOBAL hglbnewclipUTF8;
	TCHAR *pnewclipUTF8;
	TCHAR *apbuf = NULL;
    unsigned apbuflen=0;
    if (flags&SCDS_COPYPASTEBINARY)
		flags &= ~(SCDS_COPYEOLALWAYSCRLF|SCDS_PASTETOEDITOREOL|SCDS_COPYCONVERTNULLSTOSPACE|SCDS_UNICODEMODE|SCDS_COPYNOTUNICODE|SCDS_PASTEANSIUTF8);
    if (flags&SCDS_COPYCONVERTNULLSTOSPACE)
		memchrtran1(*buf,*buflen,'\0',' ');
    if (flags&SCDS_COPYEOLALWAYSCRLF) {
		converteol(buf,bufsz,buflen,SC_EOL_CRLF);
		if (!*buf)
			break;
	}
    if (flags&SCDS_COPYNOTUNICODE) flags &= ~SCDS_UNICODEMODE; // I'm not sure whether before or after strdupClipboardText() is the best way; remember that SCDS_PASTEANSIUTF8 is not a [x]Setting!
    if (flags&SCDS_COPYAPPEND) apbuf=strdupClipboardText(&apbuflen,flags,eoltype,NULL);
    if (OpenClipboard(g_nppData._nppHandle)) {
      EmptyClipboard();
#define UTF8SIZE (apbuflen+*buflen+1) /* always \0 terminated */
      if ((hglbnewclipUTF8 = GlobalAlloc(GHND|GMEM_SHARE,ClipboardBufferPadSize(UTF8SIZE,CLIPPADSIGTEXT,flags)))) { //MessageBoxFree(g_nppData._nppHandle,smprintf("Old size %u Padded Size %u",UTF8SIZE,GlobalSize(hglbnewclipUTF8)),PLUGIN_NAME, MB_OK);
        pnewclipUTF8 = (TCHAR *)GlobalLock(hglbnewclipUTF8);
        if (apbuf) {
          memcpy(pnewclipUTF8,apbuf,apbuflen); // apbuflen doesn't include the \0
          //pnewclipUTF8[apbuflen] = '\0'; MessageBoxFree(g_nppData._nppHandle,smprintf("Reuse %s",pnewclipUTF8),PLUGIN_NAME, MB_OK);
        }
        memcpy(pnewclipUTF8+apbuflen, *buf, *buflen); // buflen doesn't include the \0
        pnewclipUTF8[UTF8SIZE-1] = '\0';
        ClipboardBufferPad(pnewclipUTF8,GlobalSize(hglbnewclipUTF8),UTF8SIZE,CLIPPADSIGTEXT,flags);
        //MessageBoxFree(g_nppData._nppHandle,smprintf("Total %s len:%u",pnewclipUTF8,*buflen+apbuflen),PLUGIN_NAME, MB_OK);
        HGLOBAL hglbnewclipUNICODE=NULL;
        if (flags&SCDS_UNICODEMODE) {
#ifdef UNICONVERSION_H
			unsigned newclipUNICODEsz;
		  //unsigned newclipUNICODEsz=UCS2FromUTF8(pnewclipUTF8,UTF8SIZE,NULL,0,FALSE,NULL);
#else
          unsigned newclipUNICODEsz=MultiByteToWideChar(CP_UTF8,0,pnewclipUTF8,UTF8SIZE,NULL,0);
#endif
          //MessageBoxFree(g_nppData._nppHandle,smprintf("newclipUNICODEsz:%u (UNICODE)",newclipUNICODEsz),PLUGIN_NAME, MB_OK);
          if ((hglbnewclipUNICODE=GlobalAlloc(GHND|GMEM_SHARE,newclipUNICODEsz*sizeof(wchar_t)))) {
            TCHAR *pnewclipUNICODE=(TCHAR *)GlobalLock(hglbnewclipUNICODE);
#ifdef UNICONVERSION_H
            /*if (*/
//			UCS2FromUTF8(pnewclipUTF8,UTF8SIZE,pnewclipUNICODE,newclipUNICODEsz,FALSE,NULL);
			//!=newclipUNICODEsz) MessageBoxFree(g_nppData._nppHandle,smprintf("Wrong size:%u (UNICODE)",newclipUNICODEsz),PLUGIN_NAME, MB_OK);
#else
            MultiByteToWideChar(CP_UTF8,0,pnewclipUTF8,UTF8SIZE,pnewclipUNICODE,newclipUNICODEsz);
#endif
          } else flags |= SCDS_COPYALSOUTF8; // get something copied
        }
        GlobalUnlock(hglbnewclipUTF8);
        if (!g_fOnNT || flags&SCDS_COPYALSOUTF8 || !(flags&SCDS_UNICODEMODE)) {
          SetClipboardData(CF_TEXT,hglbnewclipUTF8);
        } else {
          GlobalFree(hglbnewclipUTF8);
        }
        if (hglbnewclipUNICODE) {
          GlobalUnlock(hglbnewclipUNICODE);
          SetClipboardData(CF_UNICODETEXT,hglbnewclipUNICODE);
        }
#undef UTF8SIZE
        if (flags&SCDS_COPYRECTANGULAR) SetClipboardData(g_cfColumnSelect,0);
        rv=TRUE;
      }
      CloseClipboard();
    }
    if (apbuf)
		freesafe(apbuf, _T("SetClipboardDataSingle"));
  } while(0);

  return(rv);
}

#if NPPDEBUG
// http://www.vbaccelerator.com/home/VB/Tips/Determine_All_Formats_On_Clipboard/article.asp
EXTERNC PFUNCPLUGINCMD pfinsertclipinfo(void) {
  if (OpenClipboard(g_nppData._nppHandle)) {
    TCHAR *buf=NULL;
	size_t bufsz=0, buflen;
	sarmprintf(&buf,&bufsz,&buflen, _T("**CountClipboardFormats()=%u\r\n"), CountClipboardFormats());
    UINT cf; for(cf=0/*,cfsz=0,cfct=0*/; (cf=EnumClipboardFormats(cf)) ; ) {
      HGLOBAL hglb;
      sarmprintf(&buf,&bufsz,&buflen, _T("%5u:"), cf);
      switch(cf) {
      case CF_TEXT        : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Text (ANSI)"), _T("pfinsertclipinfo")); goto getdetails;
      case CF_BITMAP      : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Bitmap"), _T("pfinsertclipinfo")); break;
      case CF_METAFILEPICT: strcpyarmsafe(&buf,&bufsz,&buflen, _T("MetaFile Picture"), _T("pfinsertclipinfo")); break;
      case CF_SYLK        : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Symbolic Link (SYLK)"), _T("pfinsertclipinfo")); break;
      case CF_DIF         : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Data Interchange Format"), _T("pfinsertclipinfo")); break;
      case CF_TIFF        : strcpyarmsafe(&buf,&bufsz,&buflen, _T("TIFF"), _T("pfinsertclipinfo")); break;
      case CF_OEMTEXT     : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Text (OEM)"), _T("pfinsertclipinfo")); goto getdetails;
      case CF_DIB         : strcpyarmsafe(&buf,&bufsz,&buflen, _T("DIB Device Independant Bitmap"), _T("pfinsertclipinfo")); break;
      case CF_PALETTE     : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Palette"), _T("pfinsertclipinfo")); break;
      case CF_PENDATA     : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Pen Data"), _T("pfinsertclipinfo")); break;
      case CF_RIFF        : strcpyarmsafe(&buf,&bufsz,&buflen, _T("RIFF Audio"), _T("pfinsertclipinfo")); break;
      case CF_WAVE        : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Wave Audio"), _T("pfinsertclipinfo")); break;
      case CF_UNICODETEXT :
        strcpyarmsafe(&buf, &bufsz, &buflen, _T("Text (Unicode)"), _T("pfinsertclipinfo"));
getdetails:
        if ((hglb = GetClipboardData(cf))) {
          DWORD clipsz=GlobalSize(hglb);
          sarmprintf(&buf,&bufsz,&buflen, _T(";len=%u;"), clipsz);
          TCHAR *clip = (TCHAR *)GlobalLock(hglb);
          memcpyarmsafe((void**)&buf, &bufsz, &buflen, clip, clipsz, _T("pfinsertclipinfo"));
          GlobalUnlock(hglb);
        }
        break;
      case CF_ENHMETAFILE : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Enhanced MetaFile"), _T("pfinsertclipinfo")); break;
      case CF_HDROP       : strcpyarmsafe(&buf,&bufsz,&buflen, _T("File List (HDROP)"), _T("pfinsertclipinfo")); break;
      case CF_LOCALE      : strcpyarmsafe(&buf,&bufsz,&buflen, _T("Text Locale Identifier"), _T("pfinsertclipinfo")); goto getdetails;
      default             :
        if (cf>=0xC000 && cf<=0xFFFF) {
          TCHAR fmt[128];
          GetClipboardFormatName(cf,fmt,sizeof(fmt));
          sarmprintf(&buf,&bufsz,&buflen, _T("Registered Format: %s"), fmt); break;
        } else {
          strcpyarmsafe(&buf,&bufsz,&buflen, _T("(unknown)"), _T("pfinsertclipinfo")); break;
        }
      }
      strcpyarmsafe(&buf,&bufsz,&buflen, _T("\r\n"), _T("pfinsertclipinfo"));
    }
//failbreak:
    CloseClipboard();
    if (buf) {
      INT_CURRENTEDIT;
	  GET_CURRENTEDIT;
      SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, 0);
      SENDMSGTOCED(currentEdit, SCI_ADDTEXT, buflen, buf);
      freesafe(buf, _T("pfinsertclipinfo"));
    }
  }
}

/* Windows 98 pads the length of GlobalAlloc() buffers to multiples of 8
   and buffers submitted to the Clipboard to 32. Windows XP and NT4 do not
   pad the buffer size at all. This info will be used to produce a reliable
   binary copy-n-paste */
EXTERNC PFUNCPLUGINCMD pfclipboardsizetest(void) {
  unsigned maxpad=0;
  if (OpenClipboard(g_nppData._nppHandle)) {
    EmptyClipboard();
    TCHAR *buf=NULL;
	size_t bufsz=0,buflen;
    unsigned i; for(i=1; i<4096; i++) {
      if (i>1024)
		  i+=10;
      HGLOBAL hMem=GlobalAlloc(GMEM_MOVEABLE,i);
      void *p=GlobalLock(hMem);
      memset(p,0,i);
      GlobalUnlock(hMem);
#if 1 /* 1:Check after going through clipboard */
      SetClipboardData(CF_TEXT,hMem);
      CloseClipboard();
      if (!OpenClipboard(g_nppData._nppHandle))
		  break;
      hMem=GetClipboardData(CF_TEXT);
      unsigned j=GlobalSize(hMem);
#else /* 0:Check padding for GlobalAlloc() without clipboard */
      unsigned j=GlobalSize(hMem);
      GlobalFree(hMem); // only if it isn't sent to the clipboard
#endif
      if (j>i && maxpad<j-i)
		  maxpad=j-i;
      if (i!=j)
		  sarmprintf(&buf, &bufsz, &buflen, _T("Size altered from %u to %u\r\n"), i, j);
    }
    //EmptyClipboard();
    CloseClipboard();
    if (buf) {
      sarmprintf(&buf,&bufsz,&buflen, _T("Largest alteration was %u\r\n"), maxpad);
      INT_CURRENTEDIT;
	  GET_CURRENTEDIT;
      SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0,0);
      SENDMSGTOCED(currentEdit, SCI_ADDTEXT, buflen,buf);
      freesafe(buf, _T("pfinsertclipinfo"));
    }
  }
}

#endif

// To minimize the amount of string reobtaining, you should guess destsz pretty well
// Unlike other ARM functions, this one has no use for append and wants to be mostly
// compatible with the original call so the length is returned
EXTERNC DWORD GetPrivateProfileStringarm(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault,TCHAR **dest, size_t *destsz, LPCTSTR lpFileName) {
  DWORD dwLen=0;
  if (!*dest)
	  armreallocsafe(dest, destsz, CHARSIZE(1), ARMSTRATEGY_INCREASE,FALSE, _T("GetPrivateProfileStringarm"));
  while (*dest) {
    dwLen=GetPrivateProfileString(lpAppName,lpKeyName,lpDefault,*dest,*destsz,lpFileName); //MessageBoxFree(g_nppData._nppHandle,smprintf("dwLen:%u *destsz:%u",dwLen,*destsz),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
    if (dwLen < *destsz -((lpAppName && lpKeyName) ? 1 : 2))
		break; // we must increase at dwLen==*destsz-# because that's the return value when the returned string does not fit
    armreallocsafe(dest, destsz, CHARSIZE((*destsz) + 1),ARMSTRATEGY_INCREASE,FALSE, _T("GetPrivateProfileStringarm")); //MessageBoxFree(g_nppData._nppHandle,smprintf("Reallocated to %u",*destsz),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  }
  return dwLen;
}

struct _CSIDLIST { // we only list sensible CSIDL_ values here, virtual folders and icons are useless
  int nFolder;
  unsigned uNameLen;
  const char *szName;
} CSIDLList[]= { // the first uNameLen must be 0 to ensure INIT runs
  {CSIDL_PERSONAL,0,"CSIDL_PERSONAL"},
  {CSIDL_FAVORITES,0,"CSIDL_FAVORITES"},
  {CSIDL_DESKTOPDIRECTORY,0,"CSIDL_DESKTOPDIRECTORY"},
  {CSIDL_TEMPLATES,0,"CSIDL_TEMPLATES"},
  {CSIDL_COMMON_DESKTOPDIRECTORY,0,"CSIDL_COMMON_DESKTOPDIRECTORY"},
  {CSIDL_APPDATA,0,"CSIDL_APPDATA"},
  {CSIDL_COMMON_FAVORITES,0,"CSIDL_COMMON_FAVORITES"},
  {CSIDL_INTERNET_CACHE,0,"CSIDL_INTERNET_CACHE"}, // here because some admins restrict all but Internet folders
  {CSIDL_COOKIES,0,"CSIDL_COOKIES"}, // here because some admins restrict all but Internet folders
  {CSIDL_COMMON_APPDATA,0,"CSIDL_COMMON_APPDATA"},
// These are TextFX custom folders
  {CSIDLX_TEXTFXDATA,0,"CSIDLX_TEXTFXDATA"},
//  {CSIDLX_NOTEPADPLUSEXE,0,"CSIDLX_NOTEPADPLUSEXE"},
//  {CSIDLX_NOTEPADPLUSPLUGINS,0,"CSIDLX_NOTEPADPLUSPLUGINS"},
  {CSIDLX_TEXTFXTEMP,0,"CSIDLX_TEXTFXTEMP"},
};

EXTERNC unsigned SHGetSpecialFolderLocationarm(int nFolder, TCHAR **pszFolder, size_t *puFolderSz, size_t *puFolderLen) {
  size_t uFolderLen=0;
  if (*pszFolder)
	  (*pszFolder)[0]='\0';
  ITEMIDLIST *pidl;
  TCHAR szPath[MAX_PATH];
  if (NOERROR==SHGetSpecialFolderLocation(NULL, nFolder, &pidl) && SHGetPathFromIDList(pidl,szPath)) {
    strcpyarmsafe(pszFolder, puFolderSz, &uFolderLen,szPath, _T("SHGetSpecialFolderLocationarm")); //MessageBoxFree(g_nppData._nppHandle,smprintf("uFolderLen:%u\r\n%s",uFolderLen,*pszFolder),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
  }
  if (puFolderLen)
	  *puFolderLen=(*pszFolder)?uFolderLen:0; // Unlike normal arm routines, we want our return values to be more dependable

  return uFolderLen;
}

// results are not edited to remove trailing slashes
EXTERNC DWORD GetEnvironmentVariablearm(const TCHAR *lpName, TCHAR **pszFolder, DWORD *puFolderSz, size_t *puFolderLen) {
  DWORD dwLen=GetEnvironmentVariable(lpName,NULL,0);
  if (*pszFolder)
	  (*pszFolder)[0]='\0';
  if (dwLen) {
	size_t realPuFolderSz = *puFolderSz;
    armreallocsafe(pszFolder, &realPuFolderSz, CHARSIZE(dwLen), ARMSTRATEGY_MAINTAIN, FALSE, _T("GetEnvironmentVariablearm"));
	*puFolderSz = realPuFolderSz;
    if (*pszFolder)
		dwLen=GetEnvironmentVariable(lpName,*pszFolder,*puFolderSz); //MessageBoxFree(g_nppData._nppHandle,smprintf("dwLen:%u\r\n%s",dwLen,*pszFolder),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
  }
  if (puFolderLen)
	  *puFolderLen=(*pszFolder) ? dwLen : 0; // Unlike normal arm routines, we want our return values to be more dependable

  return dwLen;
}

// Unlike the ...Free() tools, we don't write anything if passed NULL
EXTERNC BOOL WritePrivateProfileStringFree(LPCTSTR lpAppName,LPCTSTR lpKeyName,TCHAR *lpString,LPCTSTR lpFileName) {
  BOOL rv=FALSE;
  if (lpString) {
    rv=WritePrivateProfileString(lpAppName,lpKeyName,lpString,lpFileName);
    freesafe(lpString, _T("WritePrivateProfileStringFree"));
  }
  return rv;
}

EXTERNC BOOL isFileExist(const TCHAR *fn) {
  if (fn) {
    WIN32_FIND_DATA wfd;
    HANDLE h;
    if (INVALID_HANDLE_VALUE!=(h=FindFirstFile(fn,&wfd))) {
      FindClose(h);
      return TRUE;
    }
  }
  return FALSE;
}

TCHAR *g_TempsList=NULL;
size_t g_uTempsListSz = 0;
size_t g_uTempsListLen = 0;

EXTERNC void AddToTempsList(const TCHAR *szName) {
  unsigned uNameLen=wcslen(szName);
  if (uNameLen) {
    if (g_TempsList && g_uTempsListLen) {
	  TCHAR *psEnd=g_TempsList+g_uTempsListLen;
	  TCHAR *psHead,*psTail=g_TempsList;
      while((psTail=memchrX(psHead=psTail,psEnd,'|'))<psEnd) {
        if ((unsigned)(psTail-psHead)==uNameLen && !memicmp(psHead,szName,uNameLen)) return;
        psTail++;
      }
    }
    strcpyarmsafe(&g_TempsList, &g_uTempsListSz, &g_uTempsListLen, szName, _T("AddToTempsList"));
    strcpyarmsafe(&g_TempsList, &g_uTempsListSz, &g_uTempsListLen, _T("|"), _T("AddToTempsList"));
  }
}

EXTERNC void DelTempsList(void) {
  if (g_TempsList && g_uTempsListLen) {
    //MessageBox(0,g_TempsList,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
	TCHAR *szTemps=NULL;
	size_t uTempsSz=0, uTempsLen;
    memcpyarmsafe((void**)&szTemps, &uTempsSz, &uTempsLen, g_TempsList, g_uTempsListLen, _T("DelTempsList")); // NPPGetSpecialFolderLocationarm will modify the global list during deletes
    if (szTemps) {
	  TCHAR *szPath=NULL;
	  size_t uPathSz=0;

	  TCHAR *psEnd=szTemps+uTempsLen;
	  TCHAR *psHead,*psTail=szTemps;
      while((psTail=memchrX(psHead=psTail,psEnd,'|'))<psEnd) {
        g_uTempsListLen=0; // less waste to clear the list each time before AddToTempsList adds it back
        *psTail='\0';
        NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXTEMP,NULL,&szPath,&uPathSz,NULL,psHead);
        if (szPath)
			DeleteFile(szPath);
        psTail++;
      }
      if (szPath) freesafe(szPath, _T("DelTempsList"));
      freesafe(szTemps, _T("DelTempsList"));
    }
    freesafe(g_TempsList, _T("DelTempsList"));
    g_TempsList=NULL;
  }
}

// Lookup szName in
//  1) then in environment variables
//  2) custom TextFX names or approved <shlobj.h> names
// When szName==NULL, lookup nFolder in TextFX custom constants or pass to ShGetSpecialFolder() as a system constant
// This returns a malloc() buffer that the caller is expected to free or NULL if no folder is available
// folders are always returned without trailing slashes
// The retrieved path may or may not end in a slash. szAppend may or may not begin with a slash.
// If neither have a slash, one will be added. If both have a slash, one will be removed. Multiple slashes in either are not considered.

/* CSIDLX_TEXTFXINIFILE must be called early to ensure that these global values are set properly */

//MODIF Harry: Remove custom doLocalConf check and use NPPM_GETCONFIGDIR (safer)
EXTERNC unsigned NPPGetSpecialFolderLocationarm(int nFolder, const TCHAR *szName, TCHAR **pszFolder, size_t *puFolderSz, unsigned *puFolderLen, const TCHAR *szAppend) {
  size_t uFolderLen=0;
  DWORD realPuFolderSz = 0;

  if (*pszFolder)
	  (*pszFolder)[0]='\0';
  if (szName) {
    if (!GetEnvironmentVariablearm(szName,pszFolder, &realPuFolderSz, &uFolderLen)) {
      if (!CSIDLList[0].uNameLen) {
        unsigned i; for(i=0; i<NELEM(CSIDLList); i++) CSIDLList[i].uNameLen=strlen(CSIDLList[i].szName);
      }
      unsigned uNameLen=wcslen(szName);
      unsigned i;
	  for(i=0; i<NELEM(CSIDLList); i++)
		  if (uNameLen==CSIDLList[i].uNameLen && !memcmp(CSIDLList[i].szName,szName,uNameLen)) {
			  nFolder=CSIDLList[i].nFolder;
			  goto doit;
		  }
    }
  } else { // idea: should we look for environment variables if provided nFolder only? (no!)
    *puFolderSz = realPuFolderSz;
doit:
	// See CSIDLX_TEXTFXDATA for nFolder
    if ((nFolder & 0xFF) >= CSIDLX_USER) {
	  //Combine pluginname with Config dir to get nice TEMP and Data folders
	  TCHAR szPath[MAX_PATH];
      static TCHAR szConfigPath[MAX_PATH];	//here it gets ugly, TextFX requires ANSI paths, Notepad++ can be Unicode
	  static bool hasConfigPath = false;

	  if (!hasConfigPath) {
		BOOL result = (BOOL)SendMessage(g_nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) &szConfigPath[0]);
		if (result == FALSE) {	//npp doesnt support config dir or something else went wrong (ie too small buffer)
			TCHAR* endFile = szConfigPath + GetModuleFileName(g_hInstance, szConfigPath, sizeof(szConfigPath));
			TCHAR* noFile = memchrX(endFile-1, szConfigPath-1, '\\');	// C:\Program Files\Notepad++\plugins
			*noFile = 0;
		}

		TCHAR *szTip1= szPath + GetModuleFileName(g_hInstance, szPath, sizeof(szPath)); // C:\Program Files\Notepad++\plugins\NPPTextFX.DLL
		TCHAR *ext=memchrX(szTip1-1,szPath-1,'.');
	    *ext = 0;	//remove extension
		TCHAR *szFilename=memchrX(szTip1-1,szPath-1,'\\');	//-1 to include backslash
	    wcscat(szConfigPath, szFilename);

		hasConfigPath = true;
	  }

      //char *szTip2=memchrX(szTip1-1,szPath-1,'.');                                // C:\Program Files\Notepad++\plugins\NPPTextFX
	  TCHAR *szTip2=szConfigPath + wcslen(szConfigPath);                                   // C:\Program Files\Notepad++\plugins\NPPTextFX
	  TCHAR *szTip3=memchrX(szTip2-1, szConfigPath-1, '\\');                               // C:\Program Files\Notepad++\plugins
	  TCHAR *szTip4=memchrX(szTip3-1, szConfigPath-1, '\\');                               // C:\Program Files\Notepad++
      nFolder&=0xFF;
      if (szTip4>szConfigPath)
		  switch(nFolder) { // >= would allow zero length szTip4 which we want to discard
		  case CSIDLX_TEXTFXINIFILE:
			  strncpyarmsafe(pszFolder, puFolderSz, &uFolderLen, szConfigPath, szTip2 - szConfigPath, _T("NPPGetSpecialFolderLocationarm"));
			  if (*pszFolder)
				  strcpyarmsafe(pszFolder, puFolderSz, &uFolderLen, _T(".ini"), _T("NPPGetSpecialFolderLocationarm"));
			  break;
		  case CSIDLX_TEXTFXTEMP:
			  strncpyarmsafe(pszFolder, puFolderSz, &uFolderLen, szConfigPath, szTip2 - szConfigPath, _T("NPPGetSpecialFolderLocationarm"));
			  if (*pszFolder && uFolderLen && szAppend)
				  AddToTempsList(szAppend);
			  break;
		  case CSIDLX_TEXTFXDATA:
			  static TCHAR tidyPath[MAX_PATH];
 			  GetModuleFileName(g_hInstance, tidyPath, sizeof(tidyPath)); 
			  TCHAR *pch = wcsrchr(tidyPath, _T('\\'));
			  *pch = '\0';

			  wcscat(tidyPath, _T("\\Config\\tidy"));
			  strncpyarmsafe(pszFolder, puFolderSz, &uFolderLen,tidyPath, sizeof(tidyPath), _T("NPPGetSpecialFolderLocationarm"));
			  break;
	  //the following cases are illegal!
	  /*
      case CSIDLX_NOTEPADPLUSPLUGINS:
        strncpyarmsafe(pszFolder,puFolderSz,&uFolderLen,szConfigPath,szTip3-szConfigPath,"NPPGetSpecialFolderLocationarm");
        break;
      case CSIDLX_NOTEPADPLUSEXE:
        strncpyarmsafe(pszFolder,puFolderSz,&uFolderLen,szConfigPath,szTip4-szConfigPath,"NPPGetSpecialFolderLocationarm");
        break;
	  */
      } //MessageBoxFree(0,smprintf("Command:%u %s\r\n%s\r\n%s",nFolder,szAppend,*pszFolder,szPath),"", MB_OK|MB_ICONWARNING);
    } else
		SHGetSpecialFolderLocationarm(nFolder,pszFolder,puFolderSz,&uFolderLen);
  }
  if (szAppend && *pszFolder) {
    if (!uFolderLen || (*szAppend!='\\' && (*pszFolder)[uFolderLen-1]!='\\'))
		strcpyarmsafe(pszFolder, puFolderSz, &uFolderLen, _T("\\"), _T("NPPGetSpecialFolderLocationarm"));
    else if (*szAppend=='\\' && (*pszFolder)[uFolderLen-1]=='\\')
		uFolderLen--; // prevent double \\ in paths
    strcpyarmsafe(pszFolder, puFolderSz, &uFolderLen,szAppend, _T("NPPGetSpecialFolderLocationarm")); //MessageBoxFree(g_nppData._nppHandle,smprintf("%s\r\n%s",*pszFolder,szAppend),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  }
  if (puFolderLen)
	  *puFolderLen=(*pszFolder) ? uFolderLen : 0; // Unlike normal arm routines, we want our return values to be more dependable

  return uFolderLen;
}

// The retrieved path may or may not end in a slash. The text following may or may not begin with a slash. (This is different from above.)
// If neither have a slash, *none* will be added. If both have a slash, one will be removed. Multiple slashes in either are not considered.
EXTERNC BOOL XlatPathEnvVarsarm(TCHAR **dest, size_t *destsz, size_t *destlen) {
  unsigned n=0;
  if (*dest) {
    unsigned lold,lnew;
	TCHAR *d,*end,*buf=NULL;
	size_t bufsz=0;
    for(d=*dest,end=*dest+*destlen ; (d=memchrX(d,end,'%'))<end-1; ) {
      if (d[1]=='%') {
        d++;
        lold=1;
        lnew=0;
      } else {
        unsigned buflen;
		TCHAR *de=memchrX(d+1,end,'%');
        if (de>=end) {
			d++;
			continue;
		}
        lold=de-d+1; // includes both % symbols
        //memcpyarmsafe(&buf,&bufsz,&buflen,d+1,lold-2,"XlatPathEnvVarsarm"); // MS-DOS testing via an inferior but available function
        *de='\0';
		NPPGetSpecialFolderLocationarm(0,d+1,&buf,&bufsz,&buflen,NULL); *de='%';
        lnew=buf?buflen:0;
        if (lnew && buf[lnew-1]=='\\' && de+1<end && de[1]=='\\')
			lnew--; // supress extras slash
      }
      if (lnew != lold) {
        d+=memmovearmtest((void**)dest, destsz, destlen, d+lnew, d+lold, 1);
		if (!*dest)
			goto failbreak;
        end=*dest+*destlen;
        n++;
      }
      if (lnew) {
        memcpy(d,buf,lnew);
        d+=lnew;
        n++;
      }
    }
    if (buf) freesafe(buf, _T("XlatPathEnvVarsarm"));
  }
failbreak:
  return(n);
}

// SECTION: END
// SECTION: Beginning of Scintilla Interface

//extends anchorPos and currentPos out to bound entire entire lines
//returns TRUE if currentPos or anchorPos was altered, meaning that either currentPos or anchorPos was not originally at a line boundry
//grabextralines: FALSE=extend the minimal amount to enclose lines; TRUE=include additional blank lines after the lowest part of the boundry
EXTERNC BOOL lineextend(INT_CURRENTEDIT,unsigned *anchor,unsigned *curpos,BOOL grabextralines) {
  BOOL rv=FALSE;

  unsigned p1,p2,p2bol,lineno;
  if (*curpos>*anchor) {
	  p1= *anchor;
	  p2= *curpos;
  } else {
	  p1= *curpos;
	  p2= *anchor;
  }
  p1=SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE,SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,p1,0),0);
  lineno=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,p2,0);
  p2bol=SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE,lineno,0);
  if (p2 != p2bol) p2=p2bol+SENDMSGTOCED(currentEdit, SCI_LINELENGTH,lineno++,0);
  unsigned tl;
  if (grabextralines)
	  for(tl=SENDMSGTOCED(currentEdit, SCI_GETLINECOUNT,0,0); lineno<=tl && !(SENDMSGTOCED(currentEdit, SCI_GETLINEENDPOSITION,lineno,0)-p2); lineno++,p2=SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE,lineno,0)) ;
  if (*curpos>*anchor) {
    if (*anchor != p1 || *curpos != p2) {
		rv=TRUE;
	}
    *anchor=p1;
    *curpos=p2;
  } else {
    if (*anchor != p2 || *curpos != p1) {
		rv=TRUE;
	}
    *anchor=p2;
    *curpos=p1;
  }
  return(rv);
}

// bracematch() looks to the left then to the right of currentPos for a brace,
// then it locates the matching brace. SCI_BRACEMATCH is not sufficient because:
//  SCI_BRACEMATCH  only matches brace characters, not keywords as bracematch() eventually will.
//  During a SCN_CHARADDED, SCI_BRACEMATCH always fails to locate a brace
//  SCI_BRACEMATCH does not look on both sides of currentPos for a brace as the user expects
//  SCI_BRACEMATCH does not have flags to correct or adjust position for user needs
// returns TRUE if matches were found, returns curpos1 and matchpos1 of the match positions
// unless otherwise noted, brace refers to all of the characters {([<>])}
// new feature: find <tag></tag> or <tag/> pairs
// new feature: find #if #endif /* } */ pairs
// new feature: find if (){}else{} groups
// new feature: find /* (* *) */ pairs
#define FLAG_IGNOREBEGINBRACE 1 /* fail without searching if the match for an endbrace is requested; Never enable both begin&end options */
#define FLAG_IGNOREENDBRACE 2  /* fail without searching if the match for an beginbrace is requested */
#define CAFLAG_EXTENDTOLINES 4 /* TRUE: extends curposl,matchpos1 to entire lines; This one is reused in convertall() */
#define FLAG_INCLUDEBRACKETS 8 /* TRUE: include bracket characters between curpos1,matchpos1; FALSE: only include between the brackets */

EXTERNC BOOL bracematch(INT_CURRENTEDIT, unsigned *curpos1, unsigned *matchpos1, unsigned flags) {
	char BKTS[] = "{}()[]<>";
	if (currentEdit == INVALID_CURRENT_EDIT)
		GET_CURRENTEDIT;
	unsigned curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
	if (SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, 0) > 1) {
		unsigned anchor;
		if ((anchor = SENDMSGTOCED(currentEdit, SCI_GETANCHOR, 0, 0)) == curpos + 1)
			curpos = anchor;
	}
	TCHAR bracebuf[3];
	char *bkt;
	struct TextRange tr;
	tr.chrg.cpMin = curpos - 1;
	tr.chrg.cpMax = curpos + 1;
	tr.lpstrText = bracebuf;
	if (tr.chrg.cpMin < 0)
		tr.chrg.cpMin = 0;
	int textlen = SENDMSGTOCED(currentEdit, SCI_GETLENGTH, 0, 0);
	if (tr.chrg.cpMax > textlen)
		tr.chrg.cpMax = textlen;
	unsigned sln;
	if ((sln = SENDMSGTOCED(currentEdit, SCI_GETTEXTRANGE, 0, &tr)) < 1)
		return(FALSE);
	//MessageBoxFree(g_nppData._nppHandle,smprintf("textBufferLength:%d currentPos:%d cpmin:%d cpmax:%d buf\r\n%s",textBufferLength,currentPos,tr.chrg.cpMin,tr.chrg.cpMax,tr.lpstrText),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	if ((bkt = strchr(BKTS, bracebuf[0])))
		curpos = tr.chrg.cpMin;
	else if (sln < 2 || !(bkt = strchr(BKTS, bracebuf[1])))
		return(FALSE);
	if (((flags&FLAG_IGNOREBEGINBRACE) && !((bkt - BKTS) & 1)) ||
		(((flags&FLAG_IGNOREENDBRACE) && ((bkt - BKTS) & 1))))
		return(FALSE);
	unsigned bktpos =/*-1;// */SENDMSGTOCED(currentEdit, SCI_BRACEMATCH, curpos, 0);
	if (bktpos == (unsigned)-1) { // in some cases, SCI_BRACEMATCH fails even though we are on a valid brace with a findable opposing brace, such as during a SCNotification
	  //MessageBoxFree(g_nppData._nppHandle,smprintf("BraceMatch Failed"),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
		int dir = ((bkt - BKTS) & 1) ? -1 : 1;
		if (dir > 0) {
			tr.chrg.cpMin = curpos + 1;
			tr.chrg.cpMax = SENDMSGTOCED(currentEdit, SCI_GETLENGTH, curpos, 0);
		} else {
			tr.chrg.cpMin = 0;
			tr.chrg.cpMax = curpos;
		}
		if (!(tr.lpstrText = (TCHAR *)mallocsafe(tr.chrg.cpMax - tr.chrg.cpMin + 2, _T("bracematch"))))
			return(FALSE);
		unsigned sln = SENDMSGTOCED(currentEdit, SCI_GETTEXTRANGE, 0, &tr);
		unsigned level = 1;
		bktpos = curpos;
		TCHAR *d;
		for (d = tr.lpstrText + (dir < 0 ? (sln - 1) : 0); level && ((dir < 0) ? (d >= tr.lpstrText) : (d < tr.lpstrText + sln)); d += dir, bktpos += dir) {
			if (*d == bkt[0])
				level++;
			else if (*d == bkt[dir])
				level--;
		}
		if (level) bktpos = (unsigned)-1;
		//MessageBoxFree(g_nppData._nppHandle,smprintf("cpmin:%d cpmax:%d textBufferLength:%d dir:%d level:%u ch:%c currentPos:%d bktpos:%d *d:%c",tr.chrg.cpMin,tr.chrg.cpMax,textBufferLength,dir,level,bkt[0],currentPos,bktpos,*d),PLUGIN_NAME, MB_OK); //MessageBox(g_nppData._nppHandle,tr.lpstrText,PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
		freesafe(tr.lpstrText, _T("bracematch"));
	}
	if (bktpos != (unsigned)-1) {
		//MessageBoxFree(g_nppData._nppHandle,smprintf("currentPos:%d bktpos:%d includebkts:%d",currentPos,bktpos,includebkts),PLUGIN_NAME, MB_OK); //MessageBox(g_nppData._nppHandle,tr.lpstrText,PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
		if (curpos > bktpos && (flags & FLAG_INCLUDEBRACKETS))
			curpos++;
		if (!(flags & FLAG_INCLUDEBRACKETS) || curpos < bktpos)
			bktpos++;
		if ((flags & CAFLAG_EXTENDTOLINES))
			lineextend(currentEdit, &bktpos, &curpos, TRUE);
		*curpos1 = curpos;
		*matchpos1 = bktpos;
		return(TRUE);
	}

	return(FALSE);
}

// convertall is here to do all the hard work getting text from and back to Scintilla which
// is big now but it all started with this simple goal:
// 1) Extract the selected text from Scintilla into a c-string buffer with spare space
// 2) Call a text transform on the buffer. The transform will halt if spare space runs out.
// 3) Put the buffer back into the document if the transform indicates that the user will want the result.
// 4) Reselect the entire text, or the failed text if a failure occured
// Rectangular selections are handled without any special programming in the transforms.
// Transforms do not need to handle restrictions. Transforms only work with a simple c-string.
#define CAFLAG_DENYBLOCK					    1 /* We typically deny rectangular selection if the tranform has no use for partial lines or transformed text would fit poorly in the rectangular area. */
#define CAFLAG_REQUIREBLOCK					    2 /* Setting both DENY and REQUIRE block would be a bad idea */
/* CAFLAG_EXTENDTOLINES is defined above */
#define CAFLAG_REQUIREMULTILINES			    8 /* Two or more lines must be selected (doesn't always calculate right) */
#define CAFLAG_GETCLIPBOARDTEXT				 0x10 /* Get and submit the clipboard but it is not required */
#define CAFLAG_REQUIRECLIPBOARDTEXT			 0x20 /* Get ... and the clipboard cannot be empty */
#define CAFLAG_REQUIRECLIPBOARDTEXT1CHAR	 0x40 /* Get ... and the clipboard must contain exactly 1 character */
#define CAFLAG_GETALLWHENNOSELECTION		 0x80 /* Get the entire document if the user hasn't selected anything */
//#define CAFLAG_GETCURLINEWHENNOSELECTION 0x100 /* Get the current line the user hasn't selected anything */
#define CAFLAG_DENYBINARY					0x200 /* User is not permitted to select any text with \0 in it */
#define CAFLAG_USEUNICODE					0x400 /* Use the UNICODE version of this function if in the right mode */
#define CAFLAG_UNICODENTONLY				0x800 /* Unicode is automatically blocked if not in NT */
#define IsScintillaUnicode(currentEdit) (SENDMSGTOCED(currentEdit, SCI_GETCODEPAGE, 0, 0)==SC_CP_UTF8)

#define CONVERTALL_CMD_memlowercase			'1'
#define CONVERTALL_CMD_memuppercase			'R'
#define CONVERTALL_CMD_mempropercase		'P'
#define CONVERTALL_CMD_meminvertcase		'p'
#define CONVERTALL_CMD_memsentencecase		's'
#define CONVERTALL_CMD_memchrtran1			'c'
#define CONVERTALL_CMD_asciiEBCDIC			'e'
#define CONVERTALL_CMD_zapchar				'z'
#define CONVERTALL_CMD_addup				'+'
#define CONVERTALL_CMD_wordcount			'-'
#define CONVERTALL_CMD_memstrtran			'S'
#define CONVERTALL_CMD_strchrstrans			'Z'
#define CONVERTALL_CMD_prepostpendlines		'&'
#define CONVERTALL_CMD_lineup				','
#define CONVERTALL_CMD_space2tabs			'\t'
#define CONVERTALL_CMD_insertclipboardcolumn '|'
#define CONVERTALL_CMD_rewraptexttest		'Ww'
#define CONVERTALL_CMD_stripHTMLtags		'h'
#define CONVERTALL_CMD_indentlines			'i'
#define CONVERTALL_CMD_indentlinessurround	'I'
#define CONVERTALL_CMD_strqsortlines		'q'
#define CONVERTALL_CMD_killwhitenonqt		' '
#define CONVERTALL_CMD_findqtstrings		'"'
#define CONVERTALL_CMD_trimtrailingspace	't'
#define CONVERTALL_CMD_deleteblanklines		'd'
#define CONVERTALL_CMD_reindentcode			'A'
#define CONVERTALL_CMD_submitW3C			'3'
#define CONVERTALL_CMD_numberconvert		'n'
#define CONVERTALL_CMD_encodeURIcomponent	'H'
#define CONVERTALL_CMD_filldown				'f'
#define CONVERTALL_CMD_memchrtran			'C'
#define CONVERTALL_CMD_tohex				'X'
#define CONVERTALL_CMD_fromhex				'x'
#define CONVERTALL_CMD_insertlinenumbers	'l'
#define CONVERTALL_CMD_deletefirstword		'L'
#define CONVERTALL_CMD_extendblockspaces	'E'
#define CONVERTALL_CMD_cleanemailquoting	'Q'
#define CONVERTALL_CMD_uudecode				'u'
#define CONVERTALL_CMD_base64decode			'U'
#define CONVERTALL_CMD_hexbyterunstolittlendian '4'
#define CONVERTALL_CMD_littlendiantohexbyteruns	'5'
#define CONVERTALL_CMD_converteol			'O'
#define CONVERTALL_CMD_splitlinesatch		'~'

EXTERNC void convertall(char cmd, unsigned flags, const TCHAR *s1, const TCHAR *s2, const TCHAR *s3, const TCHAR *s4) {
#define CAFLAG_BLOCKMODE 0x1000 /* internal: true if block mode text was selected */
#define CAFLAG_HASBINARY 0x2000 /* internal: true if the text contained a \0 */
#define origexpand 16384
	char *txUCS2 = NULL;
	TCHAR *txUnicode = NULL;
	TCHAR *clip = NULL;
	unsigned *lps = NULL, *lpe = NULL;
	flags |= CAFLAG_UNICODENTONLY;	// Always require Unicode

	do {
		INT_CURRENTEDIT;
		GET_CURRENTEDIT;
		//struct TextRange tr; tr.chrg.cpMin=posStart; tr.chrg.cpMax=posEnd; //tr.lpstrText=buf;
		unsigned currentPos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
		unsigned anchorPos = SENDMSGTOCED(currentEdit, SCI_GETANCHOR, 0, 0);
		unsigned eolType = SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0);

		// Unicode check
		if ((flags & CAFLAG_UNICODENTONLY) && !g_fOnNT) {
			MessageBox(g_nppData._nppHandle,
				_T("This tool can only provide UNICODE/UTF-8 functionality in\r\nWindows NT or better. The tool might work if your text can be viewed in ANSI mode."),
				_T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
			break;
		}

		// Is Scintilla using Code page SC_CP_UTF8 (65001) ?
		if (IsScintillaUnicode(currentEdit)) {
			// Don't really care.
		}
		flags |= CAFLAG_USEUNICODE;	// Always use Unicode

		unsigned posStart, posEnd;
		if (anchorPos < currentPos) {
			posStart = anchorPos;
			posEnd = currentPos;
		} else {
			posStart = currentPos;
			posEnd = anchorPos;
		}
		unsigned p1line = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, posStart, 0);
		unsigned selectionLength = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, 0) - 1;
		if (selectionLength < 1) {
			flags &= ~(CAFLAG_EXTENDTOLINES);
			if (flags & CAFLAG_GETALLWHENNOSELECTION) {
				selectionLength = SENDMSGTOCED(currentEdit, SCI_GETLENGTH, 0, 0);
				p1line = posStart = 0;
				posEnd = selectionLength - 1;
			} else {
				MessageBox(g_nppData._nppHandle, _T("No text selected"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
				break;
			}
			if (selectionLength < 1) {
				MessageBox(g_nppData._nppHandle, _T("No text selected or document is empty"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
				break;
			}
		} else
			flags &= ~(/*CAFLAG_GETCURLINEWHENNOSELECTION|*/CAFLAG_GETALLWHENNOSELECTION);
		if ((flags & (CAFLAG_GETCLIPBOARDTEXT | CAFLAG_REQUIRECLIPBOARDTEXT | CAFLAG_REQUIRECLIPBOARDTEXT1CHAR)) &&
			!(clip = strdupClipboardText(NULL, (IsScintillaUnicode(currentEdit) ? SCDS_UNICODEMODE : 0) | SCDS_PASTETOEDITOREOL, eolType, NULL))) {
			if (flags & CAFLAG_REQUIRECLIPBOARDTEXT) {
				MessageBox(g_nppData._nppHandle, _T("This tool requires some text in the clipboard"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
				break;
			}
		}
		if ((flags & CAFLAG_REQUIRECLIPBOARDTEXT1CHAR) && (!clip || clip[1])) { // this isn't UNICODE ready since one character can be more than one
			MessageBox(g_nppData._nppHandle, _T("This tool requires exactly one character in the clipboard"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
			break;
		}
		unsigned p1diff = 0;
		if (flags & CAFLAG_EXTENDTOLINES) {
			unsigned p1o = posStart;
			if (lineextend(currentEdit, &posStart, &posEnd, FALSE)) {
				p1line = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, posStart, 0); // I'm not sure if this is necessary
				p1diff = p1o - posStart;
			}
			//flags &= ~CAFLAG_BLOCKMODE;
			SENDMSGTOCED(currentEdit, SCI_SETSEL, posStart, posEnd);
			selectionLength = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, 0) - 1;
		}
		if (SENDMSGTOCED(currentEdit, SCI_SELECTIONISRECTANGLE, 0, 0))
			flags |= CAFLAG_BLOCKMODE;
		else
			flags &= ~CAFLAG_BLOCKMODE;
		if (flags & CAFLAG_BLOCKMODE) {
			if (flags&CAFLAG_DENYBLOCK) {
				MessageBox(g_nppData._nppHandle, _T("This tool cannot process a retangular text selection"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
				break;
			}
		} else if (flags & CAFLAG_REQUIREBLOCK) {
			MessageBox(g_nppData._nppHandle, _T("This tool only works with a retangular text selection.\r\nHold down Alt and select some text."), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
			break;
		}
		unsigned p2line = (posStart == posEnd) ? p1line : SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, posEnd, 0);
		unsigned blocklines = p2line - p1line + 1 - (((unsigned)SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE, p2line, 0) == posEnd) ? 1 : 0);
		//MessageBoxFree(g_nppData._nppHandle,smprintf("p1line:%u p2line:%u blocklines:%u posEnd:%u SCI_POS:%u",p1line,p2line,blocklines,posEnd,SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE, p2line, 0)),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
		if (blocklines < 2 /*((flags&CAFLAG_EXTENDTOLINES)?3:2)*/) { // 3:2 won't catch everything but it will catch most
			flags &= ~CAFLAG_BLOCKMODE; // you're not really in block mode if you've only marked one line
			if (flags&CAFLAG_REQUIREMULTILINES) {
				MessageBox(g_nppData._nppHandle, _T("This tool requires more than one line to be selected"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
				break;
			}
		}
		if (flags&CAFLAG_BLOCKMODE /* || hidden lines */) {
			lps = (unsigned *)mallocsafe(blocklines * sizeof(*lps), _T("convertall-bloclines"));
			if (!lps)
				break;
			lpe = (unsigned *)mallocsafe(blocklines * sizeof(*lpe), _T("convertall-bloclines"));
			if (!lpe)
				break;
			unsigned ln;
			for (ln = 0; ln < blocklines; ln++) {
				unsigned lbof = SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE, (p1line + ln), 0);
				lps[ln] = SENDMSGTOCED(currentEdit, SCI_GETLINESELSTARTPOSITION, (p1line + ln), 0) - lbof;
				lpe[ln] = SENDMSGTOCED(currentEdit, SCI_GETLINESELENDPOSITION, (p1line + ln), 0) - lbof;
			}
		}

		// this could be recoded to convert a line at a time but issues such as long line length
		// and possible performance issues in calling into Scintilla so much make me leave it this way.
		// slow: calc line length, select line text, malloc, copy selected text into buffer, convert buffer, replace selection
		// Each select line text may cause a screen jump so it may be best to do it as one big buf
		// also, line by line processing wouldn't permit transforms to operate on \r\n characters
		size_t allocatedTextBufferSize = roundtonextpower(selectionLength + origexpand + 1);
		size_t allocatedConvertedTextBufferSize;
		if (!(txUCS2 = (char*)mallocsafe(allocatedTextBufferSize, _T("convertall-allocatedTextBufferSize")))) {
			MessageBox(g_nppData._nppHandle, _T("Not enough memory"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
			break;
		}

		unsigned rv = 0;
		size_t textBufferLength;
		if (flags & CAFLAG_GETALLWHENNOSELECTION)
			textBufferLength = SENDMSGTOCED(currentEdit, SCI_GETTEXT, (selectionLength + 1), txUCS2);
		else
			textBufferLength = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, txUCS2) - 1;

#if NPPDEBUG
		if (textBufferLength != selectionLength)
			MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Mismatch selectionLength:%u textBufferLength:%u"), selectionLength, textBufferLength), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
#endif
		if (memchr(txUCS2, '\0', textBufferLength)) {
			flags |= CAFLAG_HASBINARY;
			if (flags&CAFLAG_DENYBINARY) {
				MessageBox(g_nppData._nppHandle, _T("This tool is not compatible with binary text. Please select text without [NUL] characters."), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
				break;
			}
		} else
			flags &= ~CAFLAG_HASBINARY;

		// Go convert!
		size_t textBufferConvertedLength = UCS2FromUTF8(txUCS2, textBufferLength, NULL, 0, FALSE, NULL) + 1;
		allocatedConvertedTextBufferSize = roundtonextpower(textBufferConvertedLength + origexpand + 1);
		if (!(txUnicode = (TCHAR*)mallocsafe(allocatedConvertedTextBufferSize, _T("convertall-allocatedTextBufferSize")))) {
			MessageBox(g_nppData._nppHandle, _T("Not enough memory"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
			break;
		}

		// All incoming text is always UCS-2.
		// UCS-2 --> UTF-8
		unsigned sln = UCS2FromUTF8(txUCS2, textBufferLength, txUnicode, allocatedConvertedTextBufferSize, FALSE, NULL);
		txUnicode[sln] = '\0';

		//MessageBoxFree(g_nppData._nppHandle,smprintf("#2 textBufferLength:%u txszW:%u txW:%p",textBufferLength,txszW,txW),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
		//MessageBoxFree(g_nppData._nppHandle,smprintf("%sUsing Unicode",(flags&CAFLAG_USEUNICODE)?"":"Not "),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
		SENDMSGTOCED(currentEdit, SCI_SETCURSOR, SC_CURSORWAIT, 0);
		switch (cmd) {
		case CONVERTALL_CMD_memlowercase:
			rv = memlowercase(txUnicode, textBufferLength);
			break;
		case CONVERTALL_CMD_memuppercase:
			rv = memuppercase(txUnicode, textBufferLength);
			break;
		case CONVERTALL_CMD_mempropercase:
			rv = mempropercase(txUnicode, textBufferLength);
			break;
		case CONVERTALL_CMD_meminvertcase:
			rv = meminvertcase(txUnicode, textBufferLength);
			break;
		case CONVERTALL_CMD_memsentencecase:
			rv = memsentencecase(txUnicode, textBufferLength);
			break;
		case CONVERTALL_CMD_memchrtran1:
			rv = memchrtran1(txUnicode, textBufferLength, *s1, *s2);
			break;
		case CONVERTALL_CMD_asciiEBCDIC:
			rv = asciiEBCDIC(txUnicode, textBufferLength, s2, s3, *s1 == 't' ? 0 : 1);
			break;
		case CONVERTALL_CMD_zapchar:
			rv = zapchar(txUnicode, textBufferLength, *s1, *s2 == 'n' ? 1 : 0);
			break;
		case CONVERTALL_CMD_addup:
			rv = addup(txUnicode);
			break;
		case CONVERTALL_CMD_wordcount:
			rv = wordcount(txUnicode, textBufferLength);
			break;
		case CONVERTALL_CMD_memstrtran:
			rv = memstrtran(&txUnicode, &allocatedTextBufferSize, &textBufferLength, NULL, s1, wcslen(s1), s2, wcslen(s2));
			if (s3 && s4)
				rv += memstrtran(&txUnicode, &allocatedTextBufferSize, &textBufferLength, NULL, s3, wcslen(s3), s4, wcslen(s4));
			break;
		case CONVERTALL_CMD_strchrstrans:
			rv = strchrstrans(&txUnicode, &allocatedTextBufferSize, &textBufferLength, NULL, s1, wcslen(s1), s2);
			break;
		case CONVERTALL_CMD_prepostpendlines:
			rv = prepostpendlines(&txUnicode, &allocatedTextBufferSize, &textBufferLength, *s3 == 's' ? 0 : 1, s1, wcslen(s1), s2, wcslen(s2), s3 + 1, wcslen(s3 + 1), s4, wcslen(s4));
			break;
		case CONVERTALL_CMD_lineup:
			rv = lineup(&txUnicode, &allocatedTextBufferSize, &textBufferLength, clip ? *clip : *s1, SENDMSGTOCED(currentEdit, SCI_GETUSETABS, 0, 0), SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0), 1 /* C-STRING */);
			break;
		case CONVERTALL_CMD_space2tabs:
			rv = space2tabs(&txUnicode, &allocatedTextBufferSize, &textBufferLength, SENDMSGTOCED(currentEdit, SCI_GETUSETABS, 0, 0), s1 ? _wtoi(s1) : SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0), 0);
			break;
		case CONVERTALL_CMD_insertclipboardcolumn:
			rv = insertclipboardcolumn(&txUnicode, &allocatedTextBufferSize, &textBufferLength, clip, p1diff);
			break;
		case CONVERTALL_CMD_rewraptexttest:
			rv = rewraptexttest(&txUnicode, &allocatedTextBufferSize, &textBufferLength, (cmd == 'W' ? 0 : ((clip&&isdigit(*clip)) ? _wtoi(clip) : 1)), eolType);
			break;
		case CONVERTALL_CMD_stripHTMLtags:
			rv = stripHTMLtags(&txUnicode, &allocatedTextBufferSize, &textBufferLength, s1);
			break;
		case CONVERTALL_CMD_indentlines:
			rv = indentlines(&txUnicode, &allocatedTextBufferSize, &textBufferLength, SENDMSGTOCED(currentEdit, SCI_GETUSETABS, 0, 0), SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0));
			break;
		case CONVERTALL_CMD_indentlinessurround:
			rv = indentlinessurround(&txUnicode, &allocatedTextBufferSize, &textBufferLength, SENDMSGTOCED(currentEdit, SCI_GETUSETABS, 0, 0), SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0), eolType);
			break;
		case CONVERTALL_CMD_strqsortlines:
			rv = strqsortlines(&txUnicode, &allocatedTextBufferSize, &textBufferLength, *s1 == 'n' ? 1 : 0, *s2 == 'a' ? 1 : 0, p1diff);
			break;
		case CONVERTALL_CMD_killwhitenonqt:
			rv = killwhitenonqt(txUnicode, &textBufferLength, s1[0] == 'v' ? 1 : 0);
			break;
		case CONVERTALL_CMD_findqtstrings:
			rv = findqtstrings(&txUnicode, &allocatedTextBufferSize, &textBufferLength, clip ? clip : s1, s2[0] == 'v' ? 1 : 0);
			break;
		case CONVERTALL_CMD_trimtrailingspace:
			rv = trimtrailingspace(txUnicode, &textBufferLength);
			break;
		case CONVERTALL_CMD_deleteblanklines:
			rv = deleteblanklines(txUnicode, &textBufferLength, _wtol(s1));
			break;
		case CONVERTALL_CMD_reindentcode:
			rv = reindentcode(&txUnicode, &allocatedTextBufferSize, &textBufferLength, SENDMSGTOCED(currentEdit, SCI_GETUSETABS, 0, 0), SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0));
			break;
		case CONVERTALL_CMD_submitW3C:
			rv = submitW3C(&txUnicode, &allocatedTextBufferSize, &textBufferLength, s1, s2);
			break;
		case CONVERTALL_CMD_numberconvert:
			rv = numberconvert(&txUnicode, &allocatedTextBufferSize, &textBufferLength, *s1, *s2);
			break;
		case CONVERTALL_CMD_encodeURIcomponent:
			rv = encodeURIcomponent(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			break;
		case CONVERTALL_CMD_filldown:
			rv = filldown(&txUnicode, &allocatedTextBufferSize, &textBufferLength, *s1 == 'i' ? 1 : 0);
			break;
		case CONVERTALL_CMD_memchrtran:
			rv = memchrtran(&txUnicode, &allocatedTextBufferSize, &textBufferLength, s1, wcslen(s1), s2, wcslen(s2));
			break;
		case CONVERTALL_CMD_tohex:
			rv = tohex(&txUnicode, &allocatedTextBufferSize, &textBufferLength, posStart, _wtol(s1));
			break;
		case CONVERTALL_CMD_fromhex:
			rv = fromhex(&txUnicode, &allocatedTextBufferSize, &textBufferLength, posStart);
			flags |= CAFLAG_HASBINARY;
			break;
		case CONVERTALL_CMD_insertlinenumbers:
			rv = insertlinenumbers(&txUnicode, &allocatedTextBufferSize, &textBufferLength, p1line + 1);
			break;
		case CONVERTALL_CMD_deletefirstword:
			rv = deletefirstword(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			break;
		case CONVERTALL_CMD_extendblockspaces:
			rv = extendblockspaces(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			break;
		case CONVERTALL_CMD_cleanemailquoting:
			rv = cleanemailquoting(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			break;
		case CONVERTALL_CMD_uudecode:
			rv = uudecode(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			flags |= CAFLAG_HASBINARY;
			break;
		case CONVERTALL_CMD_base64decode:
			rv = base64decode(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			flags |= CAFLAG_HASBINARY;
			break;
		case CONVERTALL_CMD_hexbyterunstolittlendian:
			rv = hexbyterunstolittlendian(&txUnicode, &allocatedTextBufferSize, &textBufferLength, _wtol(s1));
			break;
		case CONVERTALL_CMD_littlendiantohexbyteruns:
			rv = littlendiantohexbyteruns(&txUnicode, &allocatedTextBufferSize, &textBufferLength);
			break;
		case CONVERTALL_CMD_converteol:
			rv = converteol(&txUnicode, &allocatedTextBufferSize, &textBufferLength, eolType);
			break;
		case CONVERTALL_CMD_splitlinesatch:
			rv = splitlinesatch(&txUnicode, &allocatedTextBufferSize, &textBufferLength, (clip && !clip[1]) ? *clip : *s1, s2[0] == 'v' ? 1 : 0, eolType);
			break;
#if ENABLE_TIDYDLL
		case 'T': rv = tidyHTML(&tx, &txsz, &sln, eoltype, SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0)); break;
#endif
		} // end switch switch (cmd)

		if (txUnicode) {
			// Go convert!
			// UTF-8 --> UCS-2
			sln = UTF8FromUCS2(txUnicode, allocatedConvertedTextBufferSize, (char*)txUCS2, textBufferLength, FALSE);
			txUCS2[sln] = '\0';
#if NPPDEBUG
			if (!(flags&CAFLAG_HASBINARY) && strlen(txUCS2) != textBufferLength)
				MessageBoxFree(g_nppData._nppHandle,
					smprintf(_T("selectionLength (textBufferLength) not calculated properly:\nstrlen(txUCS2):%u != textBufferLength:%u"), strlen(txUCS2), textBufferLength),
					_T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
#endif
			// Finally release the converted buffer
			freesafe(txUnicode, _T("convertall-end"));
			txUnicode = NULL;

			if (rv) {
				SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
				if (flags & CAFLAG_BLOCKMODE) {
					unsigned ln;
					char *d, *end;
					for (ln = 0, d = txUCS2, end = d + textBufferLength; ln < blocklines && d < end; ln++) {
						unsigned bytes = memcspn_chr(d, txUCS2 + textBufferLength, "\r\n", 2) - d;
						if (lpe[ln] > lps[ln]) {
							int lbof = SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE, (p1line + ln), 0);
							SENDMSGTOCED(currentEdit, SCI_SETTARGETSTART, lps[ln] + lbof, 0);
							SENDMSGTOCED(currentEdit, SCI_SETTARGETEND, lpe[ln] + lbof, 0);
							SENDMSGTOCED(currentEdit, SCI_REPLACETARGET, bytes, d);
						}
						d += bytes;
						if ((d[0] == '\r' && d[1] == '\n') || (d[0] == '\n' && d[1] == '\r'))
							d += 2;
						else if (d[0] == '\r' || d[0] == '\n')
							d++;
					}
				} else {
					posEnd += textBufferLength - selectionLength;
					if (rv) {
						if (flags&CAFLAG_GETALLWHENNOSELECTION) {
							SENDMSGTOCED(currentEdit, SCI_SETTEXT, 0, "");
							SENDMSGTOCED(currentEdit, SCI_ADDTEXT, textBufferLength, txUCS2);
						}
						else {
							SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, "");
							SENDMSGTOCED(currentEdit, SCI_ADDTEXT, textBufferLength, txUCS2);
						}
					}
					if (currentPos < anchorPos)
						SENDMSGTOCED(currentEdit, SCI_SETSEL, posEnd, posStart);
					else
						SENDMSGTOCED(currentEdit, SCI_SETSEL, posStart, posEnd);
				}
			} // end if (rv)
			SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
		} else {
			MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Out of memory%s"), HighPerformance ? _T(". Try turning off High Performance.") : _T("")),
				_T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
		} // end if (txUCS2)

		SENDMSGTOCED(currentEdit, SCI_SETCURSOR, SC_CURSORNORMAL, 0);
	} while (0);

	if (txUCS2)
		freesafe(txUCS2, _T("convertall-end"));
	if (txUnicode)
		freesafe(txUnicode, _T("convertall-end"));
	if (lps)
		freesafe(lps, _T("convertall-end"));
	if (lpe)
		freesafe(lpe, _T("convertall-end"));
	if (clip)
		freesafe(clip, _T("convertall-end"));
#undef origexpand
#undef CAFLAG_HASBINARY
#undef CAFLAG_BLOCKMODE
#undef CAFLAG_LESS
#undef MAXLINES
}
// SECTION: END

// SECTION: Beginning of Notepad++ interface

// SUBSECTION: Single line menu functions
EXTERNC PFUNCPLUGINCMD pfdummy(void) { } // does absolutely nothing at all
EXTERNC PFUNCPLUGINCMD pfconvert1q2q(void)			{ convertall(CONVERTALL_CMD_memchrtran1, 0, _T("'"), _T("\""), NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfconvert2q1q(void)			{ convertall(CONVERTALL_CMD_memchrtran1, 0, _T("\""), _T("'"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertqtswap(void)		{ convertall(CONVERTALL_CMD_memchrtran, 0, _T("\"'"), _T("'\""), NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfconvertqtdrop(void)		{ convertall(CONVERTALL_CMD_memchrtran, 0, _T("\"'"), _T(""), NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfconvertescapesq(void)		{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\""), _T("\\\""), NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfconvertescape1qsq(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("'"), _T("\\\""), NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfconvertescape1qs1q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("'"), _T("\\'"), NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfconvertescapeboth(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("'"), _T("\\'"), _T("\""), _T("\\\""));}
EXTERNC PFUNCPLUGINCMD pfconvertunescapesq(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\\\""), _T("\""), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertunescapesq1q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\\\""), _T("'"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertunescapes1q1q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\\'"), _T("'"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertunescapeboth(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\\'"), _T("'"), _T("\\\""), _T("\"")); }
EXTERNC PFUNCPLUGINCMD pfconvertescape2q22q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\""), _T("\"\""), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertescape1q22q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("'"), _T("\"\""), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertunescape22q2q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\"\""), _T("\""), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertunescape22q1q(void)	{ convertall(CONVERTALL_CMD_memstrtran, 0, _T("\"\""), _T("'"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertuppercase(void)		{ convertall(CONVERTALL_CMD_memuppercase, CAFLAG_USEUNICODE, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertlowercase(void)		{ convertall(CONVERTALL_CMD_memlowercase, CAFLAG_USEUNICODE, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertpropercase(void)	{ convertall(CONVERTALL_CMD_mempropercase, CAFLAG_USEUNICODE, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertsentencecase(void)	{ convertall(CONVERTALL_CMD_memsentencecase, CAFLAG_USEUNICODE, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconvertinvertcase(void)	{ convertall(CONVERTALL_CMD_meminvertcase, CAFLAG_USEUNICODE, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfencodeHTML(void)			{ convertall(CONVERTALL_CMD_strchrstrans, CAFLAG_DENYBLOCK, _T("&<>\""), _T(",&&amp;,<&lt;,>&gt;,\"&quot;,"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfencodeURIcomponent(void)	{ convertall(CONVERTALL_CMD_encodeURIcomponent, CAFLAG_DENYBLOCK, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfrot13(void)				{ convertall(CONVERTALL_CMD_memchrtran, 0, _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), _T("NOPQRSTUVWXYZABCDEFGHIJKLMnopqrstuvwxyzabcdefghijklm"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfunwraptext(void)			{ convertall(CONVERTALL_CMD_rewraptexttest, CAFLAG_DENYBLOCK, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfrewraptext(void)			{ convertall(CONVERTALL_CMD_rewraptexttest, CAFLAG_DENYBLOCK|CAFLAG_GETCLIPBOARDTEXT, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pflineupcomma(void)			{ convertall(CONVERTALL_CMD_lineup, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_REQUIREMULTILINES, _T(","), NULL, NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pflineupequals(void)			{ convertall(CONVERTALL_CMD_lineup, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_REQUIREMULTILINES, _T("="), NULL, NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pflineupclipboard(void)		{ convertall(CONVERTALL_CMD_lineup, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_REQUIREMULTILINES|CAFLAG_REQUIRECLIPBOARDTEXT1CHAR, NULL, NULL, NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfinsertclipboardcolumn(void){ convertall(CONVERTALL_CMD_insertclipboardcolumn, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_REQUIREMULTILINES|CAFLAG_REQUIRECLIPBOARDTEXT, NULL, NULL, NULL, NULL);}
EXTERNC PFUNCPLUGINCMD pfemptyundobuffer(void)		{ INT_CURRENTEDIT; GET_CURRENTEDIT; SENDMSGTOCED(currentEdit, SCI_EMPTYUNDOBUFFER, 0, 0);}
EXTERNC PFUNCPLUGINCMD pfsubmitHTML(void)			{ convertall(CONVERTALL_CMD_submitW3C, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY|CAFLAG_GETALLWHENNOSELECTION, _T("W3C-HTMLValidator.htm\0Extra unused space for l33t haxors to specify their own filenames\0"), _T("temp.htm\0Extra unused space for l33t haxors to specify their own filenames\0"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfsubmitCSS(void)			{ convertall(CONVERTALL_CMD_submitW3C, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY|CAFLAG_GETALLWHENNOSELECTION, _T("W3C-CSSValidator.htm\0Extra unused space for l33t haxors to specify their own filenames\0"), _T("temp.htm\0Extra unused space for l33t haxors to specify their own filenames\0"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfdecimal2binary(void)		{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("d"), _T("b"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfdecimal2octal(void)		{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("d"), _T("o"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfdecimal2hex(void)			{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("d"), _T("h"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfhex2decimal(void)			{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("h"), _T("d"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfoctal2decimal(void)		{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("o"), _T("d"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfbinary2decimal(void)		{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("b"), _T("d"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfcnum2decimal(void)			{ convertall(CONVERTALL_CMD_numberconvert, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("c"), _T("d"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfstripHTMLtags(void)		{ convertall(CONVERTALL_CMD_stripHTMLtags, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("t"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfstripHTMLnotabs(void)		{ convertall(CONVERTALL_CMD_stripHTMLtags, CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY, _T("n"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfkillwhitenonqtvb(void)		{ convertall(CONVERTALL_CMD_killwhitenonqt, 0, _T("v"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfkillwhitenonqtc(void)		{ convertall(CONVERTALL_CMD_killwhitenonqt, 0, _T("c"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pffindqtstringvb(void)		{ convertall(CONVERTALL_CMD_findqtstrings, CAFLAG_DENYBLOCK|CAFLAG_GETCLIPBOARDTEXT, _T(","), _T("v"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pffindqtstringc(void)		{ convertall(CONVERTALL_CMD_findqtstrings, CAFLAG_DENYBLOCK|CAFLAG_GETCLIPBOARDTEXT, _T(","), _T("c"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pffilldownover(void)			{ convertall(CONVERTALL_CMD_filldown, CAFLAG_REQUIREBLOCK, _T("o"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pffilldownins(void)			{ convertall(CONVERTALL_CMD_filldown, CAFLAG_REQUIREBLOCK, _T("i"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfaddup(void)				{ convertall(CONVERTALL_CMD_addup, CAFLAG_DENYBINARY ,NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfzapspace(void)				{ convertall(CONVERTALL_CMD_zapchar, 0, _T(" "), _T(""), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfzapnonprint(void)			{ convertall(CONVERTALL_CMD_zapchar, 0, _T("#"), _T("n"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfspace2tabs(void)			{ convertall(CONVERTALL_CMD_space2tabs, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfspace2tabs8(void)			{ convertall(CONVERTALL_CMD_space2tabs, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES, _T("8"), NULL, NULL, NULL); } //http://forum.pspad.com/read.php?f=2&i=2799&t=2799
EXTERNC PFUNCPLUGINCMD pfindentlinessurround(void)  { convertall(CONVERTALL_CMD_indentlinessurround, CAFLAG_DENYBLOCK, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pftrimtrailingspace(void)	{ convertall(CONVERTALL_CMD_trimtrailingspace, 0, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfdeleteblanklines(void)		{ convertall(CONVERTALL_CMD_deleteblanklines, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_REQUIREMULTILINES, _T("1"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfdeleteblanklines2(void)	{ convertall(CONVERTALL_CMD_deleteblanklines, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_REQUIREMULTILINES, _T("2"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfreindentcode(void)			{ convertall(CONVERTALL_CMD_reindentcode, CAFLAG_EXTENDTOLINES|CAFLAG_GETALLWHENNOSELECTION|CAFLAG_DENYBLOCK|CAFLAG_REQUIREMULTILINES, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfascii2EBCDIC(void)			{ convertall(CONVERTALL_CMD_asciiEBCDIC, 0, _T("t"), g_tb512ASCII_To_EBCDIC, _T("AsciiToEBCDIC.bin"), NULL); }
EXTERNC PFUNCPLUGINCMD pfEBCDIC2ascii(void)			{ convertall(CONVERTALL_CMD_asciiEBCDIC, 0, _T("f"), g_tb512ASCII_To_EBCDIC, _T("AsciiToEBCDIC.bin"), NULL); }
EXTERNC PFUNCPLUGINCMD pfcp1251toKOI8_R(void)		{ convertall(CONVERTALL_CMD_asciiEBCDIC, 0, _T("t"), g_tb512_CP1251toKOI8_R, _T("ANSItoKOI8_R.bin"), NULL); }
EXTERNC PFUNCPLUGINCMD pfKOI8_Rtocp1251(void)		{ convertall(CONVERTALL_CMD_asciiEBCDIC, 0, _T("f"), g_tb512_CP1251toKOI8_R, _T("ANSItoKOI8_R.bin"), NULL); }
EXTERNC PFUNCPLUGINCMD pftohex16(void)				{ convertall(CONVERTALL_CMD_tohex, 0, _T("16"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pftohex32(void)				{ convertall(CONVERTALL_CMD_tohex, 0, _T("32"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pftohex64(void)				{ convertall(CONVERTALL_CMD_tohex, 0, _T("64"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pftohex128(void)				{ convertall(CONVERTALL_CMD_tohex, 0, _T("128"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pffromhex(void)				{ convertall(CONVERTALL_CMD_fromhex, 0, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfwordcount(void)			{ convertall(CONVERTALL_CMD_wordcount, 0, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfinsertlinenumbers(void)	{ convertall(CONVERTALL_CMD_insertlinenumbers, 0, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfdeletefirstword(void)		{ convertall(CONVERTALL_CMD_deletefirstword, 0, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfextendblockspaces(void)	{ convertall(CONVERTALL_CMD_extendblockspaces, CAFLAG_REQUIREBLOCK|CAFLAG_REQUIREMULTILINES, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfcleanemailquoting(void)	{ convertall(CONVERTALL_CMD_cleanemailquoting, 0, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfuudecode(void)				{ convertall(CONVERTALL_CMD_uudecode, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_DENYBINARY, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfbase64decode(void)			{ convertall(CONVERTALL_CMD_base64decode, CAFLAG_DENYBLOCK|CAFLAG_EXTENDTOLINES|CAFLAG_DENYBINARY, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfhexbyterunstolittlendian2(void){ convertall(CONVERTALL_CMD_hexbyterunstolittlendian, CAFLAG_DENYBINARY, _T("2"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfhexbyterunstolittlendian4(void){ convertall(CONVERTALL_CMD_hexbyterunstolittlendian, CAFLAG_DENYBINARY, _T("4"), NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pflittlendiantohexbyteruns(void) { convertall(CONVERTALL_CMD_littlendiantohexbyteruns, CAFLAG_DENYBINARY, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfconverteol(void)			{ convertall(CONVERTALL_CMD_converteol, CAFLAG_DENYBLOCK, NULL, NULL, NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfsplitlinesatchvb(void)		{ convertall(CONVERTALL_CMD_splitlinesatch, CAFLAG_DENYBLOCK|CAFLAG_GETCLIPBOARDTEXT, _T(","), _T("v"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfsplitlinesatchc(void)		{ convertall(CONVERTALL_CMD_splitlinesatch, CAFLAG_DENYBLOCK|CAFLAG_GETCLIPBOARDTEXT, _T(","), _T("c"), NULL, NULL); }
// SUBSECTION: END

// SUBSECTION: Multiline menu functions
EXTERNC PFUNCPLUGINCMD pfindentlines(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	if (SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, 0) > 1 /*&& !SENDMSGTOCED(currentEdit, SCI_SELECTIONISRECTANGLE, 0, 0)*/) {
		convertall('i', CAFLAG_EXTENDTOLINES | CAFLAG_DENYBLOCK/*|CAFLAG_GETCURLINEWHENNOSELECTION*/, NULL, NULL, NULL, NULL);
	} else
		SENDMSGTOCED(currentEdit, SCI_TAB, 0, 0);
}

EXTERNC PFUNCPLUGINCMD pffindmatchchar(void) {
  INT_CURRENTEDIT;
  GET_CURRENTEDIT;
  unsigned curpos,mtpos;
  if (bracematch(currentEdit,&curpos,&mtpos,0))
	  SENDMSGTOCED(currentEdit, SCI_GOTOPOS, (mtpos), 0);
}

EXTERNC PFUNCPLUGINCMD pfmarkmatchchar(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned curpos, mtpos;
	if (bracematch(currentEdit, &curpos, &mtpos, FLAG_INCLUDEBRACKETS))
		SENDMSGTOCED(currentEdit, SCI_SETSEL, curpos, mtpos);
}

EXTERNC PFUNCPLUGINCMD pfdeletebracepair(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned curpos, mtpos;
	if (bracematch(currentEdit, &curpos, &mtpos, FLAG_INCLUDEBRACKETS)) {
		unsigned p1, p2;
		if (curpos < mtpos) {
			p1 = curpos;
			p2 = mtpos;
		}
		else {
			p1 = mtpos;
			p2 = curpos;
		}
		//MessageBoxFree(g_nppData._nppHandle,smprintf("Deleting posStart:%u posEnd:%d",posStart,posEnd),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
		SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_SETTARGETSTART, p2 - 1, 0);
		SENDMSGTOCED(currentEdit, SCI_SETTARGETEND, p2, 0);
		SENDMSGTOCED(currentEdit, SCI_REPLACETARGET, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_SETTARGETSTART, p1, 0);
		SENDMSGTOCED(currentEdit, SCI_SETTARGETEND, p1 + 1, 0);
		SENDMSGTOCED(currentEdit, SCI_REPLACETARGET, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
	}
}

EXTERNC PFUNCPLUGINCMD pfmarkmatchline(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned curpos, mtpos;
	if (bracematch(currentEdit, &curpos, &mtpos, FLAG_INCLUDEBRACKETS | CAFLAG_EXTENDTOLINES))
		SENDMSGTOCED(currentEdit, SCI_SETSEL, curpos, mtpos);
}

#if ENABLE_AutoShowMatchline
EXTERNC PFUNCPLUGINCMD pfshowmatchline(void) {
  INT_CURRENTEDIT; GET_CURRENTEDIT;
  unsigned curpos,mtpos; if (bracematch(currentEdit,&curpos,&mtpos,FLAG_IGNOREBEGINBRACE)) {
    int lineno=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,mtpos,0);
    int linelen=SENDMSGTOCED(currentEdit, SCI_GETLINE,lineno,0);
    char *buf;
    if (buf=(char *)mallocsafe(linelen,"pfshowmatchline")) {
      SENDMSGTOCED(currentEdit, SCI_GETLINE,lineno,buf);
      SetWindowText(g_nppData._nppHandle,buf);
      freesafe(buf,"pfshowmatchline");
    }
  }
}
#endif
// converts text lines to code lines, escaping quote and slash symbols where necessary.
// Sample Javascript result: Response.write("text \" lines");
// new feature: convert code lines back to text lines, unescaping where necessary. Only will be implemented if there is a need.
EXTERNC PFUNCPLUGINCMD pfprepostpendlines(void) { // if necessary, the inverse can be implemented as '-'
	enum LangType docType; SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
	switch (docType) {
	case L_PHP:
		convertall(CONVERTALL_CMD_prepostpendlines,
			CAFLAG_DENYBLOCK | CAFLAG_EXTENDTOLINES | CAFLAG_DENYBINARY,
			_T("echo \""), _T("\\n\";"), _T("c\"\\"), _T(";\\\\\\;\"\\\";")); // start, end, search, replace
		break;
	case L_C:
	case L_CPP:
		convertall(CONVERTALL_CMD_prepostpendlines,
			CAFLAG_DENYBLOCK | CAFLAG_EXTENDTOLINES | CAFLAG_DENYBINARY,
			_T("\""), _T("\\n\""), _T("c\"\\"), _T(";\\\\\\;\"\\\";")); // start, end, search, replace
		break;
	case L_ASP: // used for ASP-VBScript
		convertall(CONVERTALL_CMD_prepostpendlines,
			CAFLAG_DENYBLOCK | CAFLAG_EXTENDTOLINES | CAFLAG_DENYBINARY,
			_T("Response.Write(\""), _T("\"&vbCrLf)"), _T("s\""), _T("\"\"")); // start, end, search, replace
		break;
	case L_JS: // used for ASP-JScript
		convertall(CONVERTALL_CMD_prepostpendlines,
			CAFLAG_DENYBLOCK | CAFLAG_EXTENDTOLINES | CAFLAG_DENYBINARY,
			_T("Response.Write(\""), _T("\\n\");"), _T("c\"\\"), _T(";\\\\\\;\"\\\";")); // start, end, search, replace
		break;
	default:
		MessageBox(g_nppData._nppHandle,
			_T("At this time only PHP, ASP (VB & JSP), C, and C++ are supported.\r\n"
				"At this time ASP is ASP-VBScript and Javascript is ASP-JScript\r\n"
				"To get Javascript to display correctly, select Python then select Javascript\r\n"
				"Please select a supported language then select some text lines to convert."),
			_T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
		break;
	}
}

EXTERNC PFUNCPLUGINCMD pfinsertasciichart(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned i = 0, ie = 255;
	size_t chartsz = 12000; // arm tools do not require us to get this number right
	if (SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, 0) - 1 == 1) {
		unsigned char tx[2];
		SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, tx);
		i = ie = tx[0];
		chartsz = 100;
	}
	TCHAR *chart = NULL;
	size_t chartlen;
	for (; i <= ie; i++) {
		char str[6]; char lit[3]; char bin[9];
		strcpy(lit, "  ");
		str[0] = i; str[1] = '\0';
		itoa(i, bin, 2);
		switch (i) {
		case 0: strcpy(str, "%"); strcpy(lit, "\\0"); break; // this % will be replaced with a \0 later!
		case 7: strcpy(lit, "\\a"); break;
		case 8: strcpy(lit, "\\b"); break;
		case 9: strcpy(str, "TAB"); strcpy(lit, "\\t"); break;
		case 10: strcpy(str, "LF"); strcpy(lit, "\\n"); break;
		case 11: strcpy(lit, "\\v"); break;
		case 12: strcpy(lit, "\\f"); break;
		case 13: strcpy(str, "CR"); strcpy(lit, "\\r"); break;
		case 32: strcpy(str, "SPACE"); break;
		case '\"': strcpy(lit, "\\\""); break;
		case '\\': strcpy(lit, "\\\\"); break;
		case '\'': strcpy(lit, "\\'"); break;
		}
		sarmprintf(&chart, &chartsz, &chartlen, _T("%3u \\x%2.2X (0x%2.2X) \\%-3o (0%-3o) %8s %s %c%c %s\n"), i, i, i, i, i, bin, lit, (i < 32 ? '^' : ' '), (i < 32 ? (i + 64) : ' '), str);
		if (!chart) break;
	}
	if (chart) {
		char *p; if ((p = (char *)memchr(chart, '%', chartlen))) *p = '\0'; // sarmprintf doesn't handle \0 properly so we'll redact it
		SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, "");
		SENDMSGTOCED(currentEdit, SCI_ADDTEXT, chartlen, chart);
		SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, "");
		freesafe(chart, _T("pfinsertasciichart"));
	}
}

EXTERNC PFUNCPLUGINCMD pfinsertruler(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned eoltype = SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0);
	if (eoltype >= NELEM(eoltypes))
		eoltype = NELEM(eoltypes) - 1;
	TCHAR *st1 = NULL;
	size_t sz1 = 103, sl1;
	TCHAR *st2 = NULL;
	size_t sz2 = 103, sl2;
	int i;
	for (i = 0; i < 10; i++) {
		sarmprintf(&st1, &sz1, &sl1, _T("---%3d---|"), i * 10);
		strcpyarmsafe(&st2, &sz2, &sl2, _T("123456789|"), _T("pfinsertruler"));
		if (!st1 || !st2)
			goto fail;
	}
	sarmprintf(&st1, &sz1, &sl1, _T("%s%s%s"), eoltypes[eoltype], st2, eoltypes[eoltype]);
	if (st1)
		SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, st1);
fail:
	if (st1)
		freesafe(st1, _T("pfinsertruler"));
	if (st2)
		freesafe(st2, _T("pfinsertruler"));
}

EXTERNC PFUNCPLUGINCMD pfswitchseltorectangular(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
	unsigned anchor = SENDMSGTOCED(currentEdit, SCI_GETANCHOR, 0, 0);
	if (anchor != curpos && !SENDMSGTOCED(currentEdit, SCI_SELECTIONISRECTANGLE, 0, 0)) {
		SENDMSGTOCED(currentEdit, SCI_SETANCHOR, anchor, 0);
		SENDMSGTOCED(currentEdit, SCI_SETSELECTIONMODE, SC_SEL_RECTANGLE, 0);
		SENDMSGTOCED(currentEdit, SCI_SETCURRENTPOS, curpos, 0);
	}
}

EXTERNC PFUNCPLUGINCMD pfhelp(void) {
  MessageBox(g_nppData._nppHandle,
	_T("A Demo File NPP" _T(PLUGIN_NAME) "Demo.TXT was included with your distribution.\r\n"
    "Load this file and find the feature you need help on. A description and\r\n"
    "sample text is provided so you can see how each tool works. Read the entire\r\n"
    "demo file to get an idea of what all the tools do."),
	_T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}
// ensure this version number matches those in the Dev-C++ version resource
EXTERNC PFUNCPLUGINCMD pfabout(void) {
	MessageBox(g_nppData._nppHandle,
		_T(PLUGIN_NAME) " " _T(PLUGIN_VERSION) /* "Special " */ " ("
#if defined(_MSC_VER)
		_T("Microsoft Visual C++ 2017")
#else
		"-unknown-"
#endif
		"), a GNU GPL Plugin for Notepad++ by Chris Severance,\r\n"// and other authors.\r\n"
		"performs a variety of common conversions on selected text.\r\n"
#if NPPDEBUG
		"\r\nThis DEBUG edition functions exactly like the non debug but the DLL is larger and you may "
		"see error boxes pop up if any internal structures are handled incorrectly. These are the programmer's "
		"fault, not yours, and they will be fixed if you report them. I prefer that feature requests be posted "
		"to the N++ plugins forum so everyone can add ideas. Bugs should be reported to me so I can look better!\r\n"
		"severach@users.sourceforge.net"
#else
		/*"\r\n" PLUGIN_NAME " is distributed with Notepad++ but is not a part\r\n"
		"of Notepad++. Please do not post " PLUGIN_NAME " bugs in the Notepad++\r\n"
		"bug tracker unless you believe the cause is Notepad++ and not "PLUGIN_NAME "."*/
#endif
		, _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
}

#if NPPDEBUG
EXTERNC PFUNCPLUGINCMD pfAboutExperimental(void) {
  MessageBox(g_nppData._nppHandle,
	  _T("Experimental functions are not unsafe but their function and/or output may be blank or incomprehensible. "
		  "They help me write the program, they don't help you to use it.\r\nIt's best to just ignore them and get a non DEBUG version."),
	  _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}
#endif
EXTERNC PFUNCPLUGINCMD pfhNotepadweb(void) { LaunchURL(_T("http://notepad-plus.sourceforge.net/")); }

// From PSPAD
BOOL g_fMarkWordFindCaseSensitive = FALSE;
BOOL g_fMarkWordFindWholeWord = FALSE;
EXTERNC void MarkWordFind(int dir) {
	struct TextToFind tr;
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
	unsigned p1 = SENDMSGTOCED(currentEdit, SCI_GETSELECTIONSTART, 0, 0);
	unsigned p2 = SENDMSGTOCED(currentEdit, SCI_GETSELECTIONEND, 0, 0);
	unsigned sellen;
	if ((sellen = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, 0)) <= 1) {
		unsigned word1 = SENDMSGTOCED(currentEdit, SCI_WORDSTARTPOSITION, curpos, TRUE);
		unsigned word2 = SENDMSGTOCED(currentEdit, SCI_WORDENDPOSITION, curpos, TRUE);
		if (word1 != word2)
			SENDMSGTOCED(currentEdit, SCI_SETSEL, dir < 0 ? word2 : word1, dir < 0 ? word1 : word2);
		//MessageBoxFree(g_nppData._nppHandle,smprintf("selectionLength:%u word1:%u word2:%u",selectionLength,word1,word2),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	} else if ((p1 > 0 || dir > 0) && (tr.lpstrText = (TCHAR *)mallocsafe(sellen, _T("MarkWordFind")))) {
		// Note: Abusing wide-char buffer in tr.lpstrText !
		SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, tr.lpstrText);
		tr.chrg.cpMin = dir < 0 ? p1 : p2;
		tr.chrg.cpMax = dir < 0 ? 0 : SENDMSGTOCED(currentEdit, SCI_GETLENGTH, 0, 0);
		//MessageBoxFree(g_nppData._nppHandle,smprintf("cpMin:%u cpMax:%u text:%s",tr.chrg.cpMin,tr.chrg.cpMax,tr.lpstrText),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
		int findpos;
		if ((findpos = SENDMSGTOCED(currentEdit, SCI_FINDTEXT, (g_fMarkWordFindCaseSensitive ? SCFIND_MATCHCASE : 0) | (g_fMarkWordFindWholeWord ? SCFIND_WHOLEWORD : 0), &tr)) != -1) {
			SENDMSGTOCED(currentEdit, SCI_GOTOPOS, dir < 0 ? (findpos + sellen - 1) : findpos, 0);
			SENDMSGTOCED(currentEdit, SCI_SETSEL, dir < 0 ? (findpos + sellen - 1) : findpos, dir < 0 ? findpos : (findpos + sellen - 1));
		} //else MessageBeep(MB_ICONHAND);
		freesafe(tr.lpstrText, _T("MarkWordFind"));
	}
}

EXTERNC PFUNCPLUGINCMD pfMarkWordFindReverse(void) { MarkWordFind(-1); }
EXTERNC PFUNCPLUGINCMD pfMarkWordFindForward(void) { MarkWordFind(+1); }

#if NPPDEBUG
// produces hex dumps suitable for source code of function results
EXTERNC PFUNCPLUGINCMD pfInsertUnicodeCapsTables(void) {
	unsigned find;

	wchar_t obuf[65536]; wchar_t temp;
	ZeroMemory(obuf, sizeof(obuf));
	for (find = 0; find < 65536; find++) {
		temp = (wchar_t)(unsigned)CharLowerW((wchar_t *)(unsigned)find);
		obuf[find] = (temp == (wchar_t)find) ? 0 : temp; /* zeroing the characters that do not convert keeps the table sparse enough to compress */
	}

	TCHAR *buf = NULL;
	size_t bufsz = 0, buflen;
	lzo_uint out_len = sizeof(obuf);
	for (find = 0; find <= out_len / sizeof(unsigned); find++)
		sarmprintf(&buf, &bufsz, &buflen, _T("%s0x%8.8x,"), (find % 16) ? _T("") : _T("\r\n"), *((unsigned *)obuf + find));
	if (buf) {
		INT_CURRENTEDIT;
		GET_CURRENTEDIT;
		SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, buf);
		freesafe(buf, _T("pfInsertUnicodeCapsTables"));
	}
}
#endif

EXTERNC void insertCurrentPath(int msg) {
	char path[MAX_PATH];
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	SendMessage(g_nppData._nppHandle, msg, 0, (LPARAM)path);
	SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, path);
}
PFUNCPLUGINCMD pfinsertCurrentFullPath(void) {insertCurrentPath(NPPM_GETFULLCURRENTPATH);}
PFUNCPLUGINCMD pfinsertCurrentFileName(void) {insertCurrentPath(NPPM_GETFILENAME);}
PFUNCPLUGINCMD pfinsertCurrentDirectory(void){insertCurrentPath(NPPM_GETCURRENTDIRECTORY);}

EXTERNC void insertDateTime(DWORD format) {
	TCHAR date[128], time[128];
	TCHAR *dateTime;
	SYSTEMTIME st;

	GetLocalTime(&st);
	GetDateFormat(LOCALE_USER_DEFAULT, format, &st, NULL, date, sizeof(date));
	//GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "yyyy.MM.dd", date, sizeof(date)); // 2006.05.01
	GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, time, sizeof(time));
	//GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, "HH:mm", time, sizeof(time));

	dateTime = smprintf(_T("%s %s"), time, date);
	//dateTime=smprintf("%s %s", date, time); // 2006.05.01 16:31
	if (dateTime) {
		INT_CURRENTEDIT;
		GET_CURRENTEDIT;
		SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, dateTime);
		freesafe(dateTime, _T("insertDateTime"));
	}
}
EXTERNC PFUNCPLUGINCMD pfinsertShortDateTime(void) {insertDateTime(DATE_SHORTDATE);}
EXTERNC PFUNCPLUGINCMD pfinsertLongDateTime(void) {insertDateTime(DATE_LONGDATE);}

EXTERNC PFUNCPLUGINCMD pfDuplicateLineOrBlock(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	TCHAR *sSel;
	unsigned uSelLen;
	if (!SENDMSGTOCED(currentEdit, SCI_SELECTIONISRECTANGLE, 0, 0) &&
		(uSelLen = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, NULL) - 1) > 0 &&
		(sSel = (TCHAR *)mallocsafe(uSelLen + 1, _T("pfDupLineDown")))) {
		uSelLen = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, sSel) - 1;
		unsigned uAnchor = SENDMSGTOCED(currentEdit, SCI_GETSELECTIONEND, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_GOTOPOS, uAnchor, 0);
		SENDMSGTOCED(currentEdit, SCI_ADDTEXT, uSelLen, sSel);
		freesafe(sSel, _T("pfDupLineDown"));
		SENDMSGTOCED(currentEdit, SCI_SETSEL, uAnchor, uAnchor + uSelLen);
	} else
		SENDMSGTOCED(currentEdit, SCI_LINEDUPLICATE, 0, 0);
}

// SUBSECTION: END
// SECTION: END

EXTERNC unsigned findmenuitem(PFUNCPLUGINCMD_MSC *_pFunc); // a couple of well placed forward references kills a ton of other ones
extern struct FuncItem funcItem[];

// SECTION: Large multi-routine menu functions
// SUBSECTION: SHOWHIDE
// http://sourceforge.net/forum/forum.php?thread_id=1112903&forum_id=195402
TCHAR *g_pszVizSequence=NULL;
size_t g_uVizSz=0,g_uVizLen=0,g_uVisPos=0; // for single stepping
BOOL g_fVizCaseSensitive=FALSE;
BOOL g_fVizWholeWords=FALSE;
BOOL g_fVizRegex=FALSE;
#define VIZMENUFLAGS ((g_fVizCaseSensitive?SCFIND_MATCHCASE:0)|(g_fVizWholeWords?SCFIND_WHOLEWORD:0)|(g_fVizRegex?SCFIND_REGEXP:0))
/* Commands: \r\n, \r, or \n terminate each command, all commands sequences start with an implicit SHOW ALL LINES
   +:r:searchtext show lines with found reg-exp text
   +:!:searchtext show lines without found text
   -:w:searchtext hide lines with found text whole words only
   -:!^:searchtext hide lines without found text case insensitive
   +[L1-L2]\r\n      Show lines between L1-L2
   -[L1-L2]\r\n      Hide lines between L1-L2
   ![L1-L2]\r\n      Invert lines from L1-L2
   +* Show all lines
   -* Hide all lines
   !* Invert all lines
*/
EXTERNC void hidelinerange(INT_CURRENTEDIT,int curline,int L1,int L2) {
        if (L1==0) L1++; // Scintilla does not allow us to hide the first line AKA Line#0.
  if (L1==curline) L1++;
  if (L2==curline) L2--;
  if (L2>=L1) {
    //MessageBoxFree(g_nppData._nppHandle,smprintf("Hiding from %d-%d",L1,L2),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
    if (L1<curline && curline<L2) {
      SENDMSGTOCED(currentEdit, SCI_HIDELINES,L1,curline-1); // don't hide the current line
      SENDMSGTOCED(currentEdit, SCI_HIDELINES,curline+1,L2);
    } else {
      SENDMSGTOCED(currentEdit, SCI_HIDELINES,L1,L2);
    }
  }
}

// This robot routine expects the fake line numbers 1..SCI_GETLINECOUNT, not 0..SCI_GETLINECOUNT-1
// call with L1=0,L2=0 to do all lines
// reveal: '-'=hide; '+'=show, '!'=invert
EXTERNC void robotvizinvert(INT_CURRENTEDIT, char reveal, int curline, int L1, int L2) {
	if (L1 > L2) {
		int temp = L1;
		L1 = L2;
		L2 = temp;
	}
	int L2x = SENDMSGTOCED(currentEdit, SCI_GETLINECOUNT, 0, 0);
	if (L1<1 || L1>L2x || L2<1 || L2>L2x) {
		L1 = 1;
		L2 = L2x;
	} else {
		L2--; L1++;
		if (L1 >= L2) {
			L1 = 1;
			L2 = L2x;
		}
	}
	//MessageBoxFree(g_nppData._nppHandle,smprintf("%c from %d-%d",reveal,L1,L2),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	if (reveal == '!') {
		for (; L2 >= L1; L2--)
			if (L2 != curline) {
				if (SENDMSGTOCED(currentEdit, SCI_GETLINEVISIBLE, L2 - 1, 0))
					SENDMSGTOCED(currentEdit, SCI_HIDELINES, L2 - 1, L2 - 1);
				else
					SENDMSGTOCED(currentEdit, SCI_SHOWLINES, L2 - 1, L2 - 1);
			}
	} else {
		if (reveal == '+')
			SENDMSGTOCED(currentEdit, SCI_SHOWLINES, L1 - 1, L2 - 1);
		else
			hidelinerange(currentEdit, curline - 1, L1 - 1, L2 - 1);
	}
}

EXTERNC void vizinvertselectedalllines(char reveal) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	int curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
	int curline = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, curpos, 0);
	int L2 = curline + 1;
	int L1 = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, SENDMSGTOCED(currentEdit, SCI_GETANCHOR, 0, 0), 0) + 1;
	if (L1 > L2) { int temp = L1; L1 = L2; L2 = temp; }
	if (L1 >= L2) {
		L2 = L1 = 0;
		if ((reveal == '+' || reveal == '-') && g_pszVizSequence) {
			freesafe(g_pszVizSequence, _T("vizinvertselectedalllines"));
			g_pszVizSequence = NULL;
			g_uVizSz = 0;
		}
		if (reveal != '+')
			sarmprintf(&g_pszVizSequence, &g_uVizSz, &g_uVizLen, _T("%c*\r\n"), reveal);
	} else {
		sarmprintf(&g_pszVizSequence, &g_uVizSz, &g_uVizLen, _T("%c[%d-%d]\r\n"), reveal, L1, L2);
	}
	robotvizinvert(currentEdit, reveal, curline + 1, L1, L2);
	SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
}
EXTERNC PFUNCPLUGINCMD pfvizhideselectedalllines(void) { vizinvertselectedalllines('-'); }
EXTERNC PFUNCPLUGINCMD pfvizshowselectedalllines(void) { vizinvertselectedalllines('+'); }
EXTERNC PFUNCPLUGINCMD pfvizinvertselectedalllines(void){vizinvertselectedalllines('!'); }

// new feature: show container lines with text at column 0
// new feature: gutter width, how many lines around the found text get displayed?
EXTERNC BOOL robothidecliplines(INT_CURRENTEDIT, TCHAR *str,char reveal,BOOL complementary,unsigned searchflags) {
#define FLAG_CONTINUE 1
#define FLAG_FIRST 2
  BOOL rv=TRUE;

  if (str) {
    //MessageBoxFree(g_nppData._nppHandle,smprintf("%c%s%s Searching for %s",reveal,complementary?" complementary":"",casesensitive?" casesensitive":"",str),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
    struct TextToFind tr;
    tr.chrg.cpMin=0;
	tr.chrg.cpMax=SENDMSGTOCED(currentEdit, SCI_GETLENGTH,0,0);
	tr.lpstrText=str;
    int curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
    int curline=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,curpos,0);
    unsigned flags=FLAG_FIRST; do {
      int L1=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,tr.chrg.cpMin,0);
      if (SENDMSGTOCED(currentEdit, SCI_FINDTEXT,searchflags,&tr)>=0) flags |= FLAG_CONTINUE; else flags &= ~FLAG_CONTINUE;
      if ((flags&(FLAG_FIRST|FLAG_CONTINUE)) == FLAG_FIRST /* halt when first search fails */) {
		  MessageBox(g_nppData._nppHandle, _T("I'm not going to hide all the lines! Copy something to the Clipboard that can be found!"), _T(PLUGIN_NAME), MB_OK||MB_ICONSTOP);
		  rv=FALSE;
		  break;
	  }
      int L2=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,((flags&FLAG_CONTINUE)?tr.chrgText.cpMin:tr.chrg.cpMax),0);
      if (reveal=='+') {
        if (!complementary) L2=L1;
        SENDMSGTOCED(currentEdit, SCI_SHOWLINES,L1,L2);
      } else {
        if (complementary) hidelinerange(currentEdit,curline,L1+1 /* This shouldn't always be +1 but thanks to Scintilla not allowing us to hide line 0, it is! */,L2-((flags&FLAG_CONTINUE)?1:0));
        else if (L1 != curline) SENDMSGTOCED(currentEdit, SCI_HIDELINES,L1,L1);
      }
      tr.chrg.cpMin=tr.chrgText.cpMax;
      flags &= ~FLAG_FIRST;
    } while(flags&FLAG_CONTINUE);
    SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos,0);
  }
  return(rv);
#undef FLAG_CONTINUE
#undef FLAG_FIRST
}

EXTERNC void vizcliplines(char reveal, BOOL complementary, unsigned searchflags) {
	TCHAR *va;

	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	if ((va = strdupClipboardText(NULL, (IsScintillaUnicode(currentEdit) ? SCDS_UNICODEMODE : 0), SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0), NULL))) {
		int pr = wcscspn(va, _T("\r\n"));
		if (va[pr] != '\0')
			MessageBox(g_nppData._nppHandle, _T("Search text cannot contain end of line characters"), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
		else {
			if (robothidecliplines(currentEdit, va, reveal, complementary, searchflags)) {
				sarmprintf(&g_pszVizSequence, &g_uVizSz, &g_uVizLen, _T("%c:%s%s%s%s:%s\r\n"), reveal, complementary ? "!" : "",
					(searchflags&SCFIND_MATCHCASE) ? "" : "^",
					(searchflags&SCFIND_WHOLEWORD) ? "w" : "",
					(searchflags&SCFIND_REGEXP) ? "r" : "",
					va);
			}
		}
		freesafe(va, _T("vizcliplines"));
	}
}
EXTERNC PFUNCPLUGINCMD pfvizhidecliplines(void)  { vizcliplines('-',FALSE,VIZMENUFLAGS);}
EXTERNC PFUNCPLUGINCMD pfvizhideclipclines(void) { vizcliplines('-',TRUE ,VIZMENUFLAGS);}
EXTERNC PFUNCPLUGINCMD pfvizshowcliplines(void)  { vizcliplines('+',FALSE,VIZMENUFLAGS);}
EXTERNC PFUNCPLUGINCMD pfvizshowclipclines(void) { vizcliplines('+',TRUE ,VIZMENUFLAGS);}

EXTERNC PFUNCPLUGINCMD pfvizinsertsequence(void){
  INT_CURRENTEDIT;
  GET_CURRENTEDIT;
  if (g_pszVizSequence)
	  SENDMSGTOCED(currentEdit, SCI_REPLACESEL,0,g_pszVizSequence);
  else
	  MessageBox(g_nppData._nppHandle, _T("You have not performed any show hide operations since your last Show All-Reset Lines"), _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}

EXTERNC PFUNCPLUGINCMD pfvizshowmorelines(void) {
	MessageBox(g_nppData._nppHandle, _T("Hold down Caps-Lock and move up or down against hidden text.\r\nLine by line the hidden text will be exposed."), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
}

EXTERNC PFUNCPLUGINCMD pfvizsequencenext(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	int curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
	int curline = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, curpos, 0);
	unsigned eol = g_uVisPos + wcscspn(g_pszVizSequence + g_uVisPos, _T("\r\n"));
	char chtemp = g_pszVizSequence[eol];
	g_pszVizSequence[eol] = '\0'; // allows strdup(void)
	int L1 = 0, L2 = 0;
	TCHAR *p, *end;
	char reveal;
	BOOL complementary = FALSE;
	unsigned searchflags;
	//MessageBoxFree(g_nppData._nppHandle,smprintf("Running Command %s",g_pszVizSequence+g_uVisPos),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	switch (reveal = g_pszVizSequence[g_uVisPos]) {
	case '\0': break;
	case '+':
	case '-':
	case '!':
		switch (g_pszVizSequence[g_uVisPos + 1]) {
		case ':':
			p = g_pszVizSequence + g_uVisPos + 2;
			searchflags = SCFIND_MATCHCASE;
			if (!(end = wcschr(p, ':'))) {
				MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Can't find : terminator for search text, be sure to use :: if there are no flags")), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
				break;
			}
			for (; p < end; p++) switch (*p) {
			case 'w': searchflags |= SCFIND_WHOLEWORD; break;
			case 'r': searchflags |= SCFIND_REGEXP; break;
			case '^': searchflags &= ~SCFIND_MATCHCASE; break;
			case '!': complementary = TRUE; break;
			}
			if (wcslen(end + 1))
				robothidecliplines(currentEdit, end + 1, reveal, complementary, searchflags);
			break;
		case '[':
			if ((p = strdupsafe(g_pszVizSequence + g_uVisPos + 1, _T("pfvizsequencenext")))) {
				unsigned uIndex = 0;
				L1 = _wtol(strtokX(p, &uIndex, _T("[-]")));
				L2 = _wtol(strtokX(p, &uIndex, _T("[-]")));
				freesafe(p, _T("pfvizsequencenext"));
			}
		case '*': robotvizinvert(currentEdit, reveal, curline + 1, L1, L2); break;
		default: MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Invalid target %c, try :[*"), *(g_pszVizSequence + g_uVisPos + 1)), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION); break;
		}
		break;
	default:
		MessageBoxFree(g_nppData._nppHandle, smprintf(_T("Invalid reveal %c, try +-!"), reveal), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
		break;
	}
	g_pszVizSequence[g_uVisPos = eol] = chtemp;
	while (g_pszVizSequence[g_uVisPos] == '\r' || g_pszVizSequence[g_uVisPos] == '\n')
		g_uVisPos++;
	SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
}

EXTERNC PFUNCPLUGINCMD pfvizsequencerest(void) {
	if (!g_pszVizSequence) MessageBox(g_nppData._nppHandle, _T("You have not performed any show hide operations since your last Show All Lines"),
		_T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);
	else
		while (g_pszVizSequence[g_uVisPos]) pfvizsequencenext();
}

EXTERNC PFUNCPLUGINCMD pfvizsequencestart(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	int curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
	g_uVisPos = 0; // we can do this even if the sequence is invalid
	robotvizinvert(currentEdit, '+', 0, 0, 0);
	SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
}

EXTERNC PFUNCPLUGINCMD pfvizsequenceall(void) {
	pfvizsequencestart();
	pfvizsequencerest();
}

EXTERNC PFUNCPLUGINCMD pfvizselectassequence(void) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	TCHAR *rv, *end;
	unsigned st, ln;
	if ((st = SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, NULL)) > 1 && (rv = (TCHAR *)mallocsafe(st, _T("pfvizselectassequence")))) {
		SENDMSGTOCED(currentEdit, SCI_GETSELTEXT, 0, rv);
		if ((ln = wcslen(rv))) {
			for (end = rv + ln - 1; end >= rv; end--)
				if (*end == '\r' || *end == '\n') *end = '\0';  // trim off eol that the user may or may not have included
			if ((ln = wcslen(rv))) {
				if (g_pszVizSequence) freesafe(g_pszVizSequence, _T("pfvizselectassequence"));
				g_pszVizSequence = rv;
				g_uVizSz = st;
				g_uVizLen = ln;
				strcpyarmsafe(&g_pszVizSequence, &g_uVizSz, &g_uVizLen, _T("\r\n"), _T("pfvizselectassequence")); // guarantee a \r\n ending
				pfvizsequencestart();
			}
		}
	}
	else MessageBox(g_nppData._nppHandle, _T("No text selected"), _T(PLUGIN_NAME), MB_OK | MB_ICONSTOP);
}

// which: *=all lines, +:visible only, -:invisible only
BOOL g_fVizClipboardAlwaysCRLF=FALSE;
BOOL g_fVizClipboardReplaceNulls=FALSE;
BOOL g_fVizClipboardCopyAlsoUTF8=FALSE;
BOOL g_fVizClipboardNotUnicode=FALSE;
BOOL g_fVizPasteBinary=FALSE;
BOOL g_fVizPasteToEditorEOL=TRUE;
EXTERNC void copycutdeletevisible(unsigned flags, char which) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	TCHAR *buf = NULL;
	size_t bufsz = 0;
	size_t buflen;
	unsigned *lps = NULL, *lpe = NULL;
	size_t lpssz = 0, lpesz = 0;
	unsigned lplen;
	int p1 = SENDMSGTOCED(currentEdit, SCI_GETSELECTIONSTART, 0, 0);
	int p2 = SENDMSGTOCED(currentEdit, SCI_GETSELECTIONEND, 0, 0);
	if (p1 < p2) {
		unsigned eoltype = SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0);
		if (eoltype >= NELEM(eoltypes))
			eoltype = NELEM(eoltypes) - 1;
		if (SENDMSGTOCED(currentEdit, SCI_SELECTIONISRECTANGLE, 0, 0))
			flags |= SCDS_COPYRECTANGULAR; else
			flags &= ~SCDS_COPYRECTANGULAR;
		unsigned blockstart = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, p1, 0);
		unsigned blocklines = SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, p2, 0) - blockstart + 1;
		unsigned ln; for (lplen = ln = 0; ln < blocklines; ln++) {
			int vis = SENDMSGTOCED(currentEdit, SCI_GETLINEVISIBLE, (blockstart + ln), 0);
			if (which == '*' || (vis && which == '+') || (!vis && which == '-')) {
				unsigned ls; if ((unsigned)INVALID_POSITION == (ls = SENDMSGTOCED(currentEdit, SCI_GETLINESELSTARTPOSITION, (blockstart + ln), 0)))
					continue;
				unsigned le; if ((unsigned)INVALID_POSITION == (le = SENDMSGTOCED(currentEdit, SCI_GETLINESELENDPOSITION, (blockstart + ln), 0)))
					continue;
				if (!lplen || lpe[lplen - 1] != ls) {
					armreallocsafe((TCHAR **)&lps, &lpssz, (lplen + 1) * sizeof(*lps), ARMSTRATEGY_INCREASE, 0, _T("pfvizcopyvisible"));
					if (!lps)
						goto failbreak;
					armreallocsafe((TCHAR **)&lpe, &lpesz, (lplen + 1) * sizeof(*lps), ARMSTRATEGY_INCREASE, 0, _T("pfvizcopyvisible"));
					if (!lpe)
						goto failbreak;
					lps[lplen] = ls;
					lpe[lplen] = le;
					lplen++;
					//MessageBoxFree(g_nppData._nppHandle,smprintf("[%u] %s line:%u from:%u to:%u",lplen,"Added New",blockstart+ln+1,ls,le),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
				} else {
					lpe[lplen - 1] = le;
					//MessageBoxFree(g_nppData._nppHandle,smprintf("[%u] %s line:%u from:%u to:%u",lplen,"Appended",blockstart+ln+1,ls,le),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
				}
			}
		}
		if (!lplen)
			goto failmessage;
		if (flags & SCDS_COPY) {
			for (buflen = 0, ln = 0; ln < lplen; ln++) {
				struct TextRange tr;
				tr.chrg.cpMin = lps[ln];
				tr.chrg.cpMax = lpe[ln];
				//MessageBoxFree(g_nppData._nppHandle,smprintf("[%u] Copy from:%u to:%u",ln,tr.chrg.cpMin,tr.chrg.cpMax),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
				armreallocsafe(&buf, &bufsz, CHARSIZE(buflen + tr.chrg.cpMax - tr.chrg.cpMin + 1), ARMSTRATEGY_INCREASE, 0, _T("pfvizcopyvisible"));
				if (!buf)
					goto failbreak;
				tr.lpstrText = buf + buflen; buflen += SENDMSGTOCED(currentEdit, SCI_GETTEXTRANGE, 0, &tr);
				if (flags&SCDS_COPYRECTANGULAR) {
					strcpyarmsafe(&buf, &bufsz, &buflen, eoltypes[eoltype], _T("pfvizcopyvisible"));
					if (!buf)
						goto failbreak;
				}
			}
			if (!buflen)
				goto failmessage;
			if (!SetClipboardDataSingle(&buf, &bufsz, &buflen, flags |
				(g_fVizClipboardAlwaysCRLF ? SCDS_COPYEOLALWAYSCRLF : 0) |
				(g_fVizClipboardReplaceNulls ? SCDS_COPYCONVERTNULLSTOSPACE : 0) |
				(g_fVizClipboardCopyAlsoUTF8 ? SCDS_COPYALSOUTF8 : 0) |
				(g_fVizClipboardNotUnicode ? SCDS_COPYNOTUNICODE : 0) |
				(g_fVizPasteBinary ? SCDS_COPYPASTEBINARY : 0) |
				(g_fVizPasteToEditorEOL ? SCDS_PASTETOEDITOREOL : 0) |
				(SENDMSGTOCED(currentEdit, SCI_SELECTIONISRECTANGLE, 0, 0) ? SCDS_COPYRECTANGULAR : 0) |
				(IsScintillaUnicode(currentEdit) ? SCDS_UNICODEMODE : 0)
				, SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0)))
				goto failbreak;
		}
		if (flags & SCDS_DELETE) {
			unsigned curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
			SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
			for (ln = lplen; ln > 0; ln--) {
				SENDMSGTOCED(currentEdit, SCI_SETTARGETSTART, lps[ln - 1], 0);
				SENDMSGTOCED(currentEdit, SCI_SETTARGETEND, lpe[ln - 1], 0);
				SENDMSGTOCED(currentEdit, SCI_REPLACETARGET, 0, "");
				if (curpos >= lpe[ln - 1]) {						//MODIF Harry: first check if out of entire selection range
					curpos -= lpe[ln - 1] - lps[ln - 1];
				}
				else if (curpos > lps[ln - 1]) {					//MODIF Harry: >= to >, position not affected if selection starts at that position (this is the original check)
					curpos -= (curpos - lps[ln - 1]);				//Only subtract difference
				}
			}
			SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
			SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
		}
	} else {
		//failmessage: ;
			//MessageBox(g_nppData._nppHandle,"No text selected",PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	}
failmessage:
failbreak:
	if (buf)
		freesafe(buf, _T("pfvizcopyvisible"));
	if (lps)
		freesafe(lps, _T("pfvizcopyvisible"));
	if (lpe)
		freesafe(lpe, _T("pfvizcopyvisible"));
}

EXTERNC PFUNCPLUGINCMD pfcopyallnoappend(void)    {copycutdeletevisible(SCDS_COPY,'*');}
EXTERNC PFUNCPLUGINCMD pfcopyallappend(void)      {copycutdeletevisible(SCDS_COPYAPPEND|SCDS_COPY,'*');}
EXTERNC PFUNCPLUGINCMD pfcutallnoappend(void)     {copycutdeletevisible(SCDS_COPY|SCDS_DELETE,'*');}
EXTERNC PFUNCPLUGINCMD pfcutallappend(void)       {copycutdeletevisible(SCDS_COPYAPPEND|SCDS_COPY|SCDS_DELETE,'*');}
//EXTERNC PFUNCPLUGINCMD pfdeleteall(void) {copycutdeletevisible(SCDS_DELETE,'*');} // Copyall and cutall are there for append and other features. Delete gets nothing from append!
EXTERNC PFUNCPLUGINCMD pfvizdeletevisible(void)   {copycutdeletevisible(SCDS_DELETE,'+');}
EXTERNC PFUNCPLUGINCMD pfvizdeleteinvisible(void) {copycutdeletevisible(SCDS_DELETE,'-');}
BOOL g_fVizCutCopyAppend=FALSE;
EXTERNC PFUNCPLUGINCMD pfvizcopyvisible(void)     {copycutdeletevisible(SCDS_COPY|(g_fVizCutCopyAppend?SCDS_COPYAPPEND:0),'+');}
EXTERNC PFUNCPLUGINCMD pfvizcutvisible(void)      {copycutdeletevisible(SCDS_COPY|(g_fVizCutCopyAppend?SCDS_COPYAPPEND:0)|SCDS_DELETE,'+');}
EXTERNC PFUNCPLUGINCMD pfvizcopyinvisible(void)   {copycutdeletevisible(SCDS_COPY|(g_fVizCutCopyAppend?SCDS_COPYAPPEND:0),'-');}
EXTERNC PFUNCPLUGINCMD pfvizcutinvisible(void)    {copycutdeletevisible(SCDS_COPY|(g_fVizCutCopyAppend?SCDS_COPYAPPEND:0)|SCDS_DELETE,'-');}
BOOL g_fVizPasteRetainsPosition=FALSE;

EXTERNC void VizPaste(unsigned flags) {
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	unsigned txsz;
	TCHAR *tx;
	BOOL isrectangular;

	if ((tx = strdupClipboardText(&txsz, flags |
		(g_fVizPasteBinary ? SCDS_COPYPASTEBINARY : 0) |
		(g_fVizPasteToEditorEOL ? SCDS_PASTETOEDITOREOL : 0) |
		(IsScintillaUnicode(currentEdit) ? SCDS_UNICODEMODE : 0)
		, SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0), &isrectangular))) {
		int curpos = 0;
		if (g_fVizPasteRetainsPosition)
			curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
		if (isrectangular) {
			SENDMSGTOCED(currentEdit, SCI_PASTE, 0, 0); // we can't handle binary rectangular text and may never need to
		} else {
			SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, "");
			SENDMSGTOCED(currentEdit, SCI_ADDTEXT, txsz, tx);
		}
		freesafe(tx, _T("pfVizPaste"));
		if (!g_fVizPasteRetainsPosition)
			curpos = SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
	}
}
EXTERNC PFUNCPLUGINCMD pfVizPasteUTF8(void)    { VizPaste(SCDS_PASTEANSIUTF8); }
EXTERNC PFUNCPLUGINCMD pfVizPasteUNICODE(void) { VizPaste(0); }

// SUBSECTION: End
#if ENABLE_PUSHPOP /* SUBSECTION: Poplists */
/* new feature: this needs a CRC-32 hash path instead of the real path */
struct _POPLISTS {
  char path[MAX_PATH];
  unsigned mru;
  unsigned poplist[30];
  unsigned popcount;
  unsigned poptextlen;
} g_PopLists[5];
unsigned g_uPopListNo;

EXTERNC void detectprplist(void) {
  char path[MAX_PATH];

  SendMessage(g_nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)path);
  int found;unsigned i,minmru,maxmru; for(found=0,minmru=(unsigned)-1,maxmru=0,i=0; i<NELEM(g_PopLists); i++) {
    if (!found && minmru>g_PopLists[i].mru) {minmru=g_PopLists[i].mru; g_uPopListNo=i;}
    if (maxmru<g_PopLists[i].mru) maxmru=g_PopLists[i].mru;
    if (!strcmp(path,g_PopLists[i].path)) {found=1; g_uPopListNo=i;}
  }
  if (!found) {
    memset(g_PopLists+g_uPopListNo,0,sizeof(g_PopLists[0]));
    strcpy(g_PopLists[g_uPopListNo].path,path);
  }
  g_PopLists[g_uPopListNo].mru=maxmru+1;
  //MessageBoxFree(g_nppData._nppHandle,smprintf(buf,"Detected %s in poplist:%u",path,g_uPopListNo),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
}

// new feature: It would be nice to destroy g_PopLists when files are closed but we probably won't be able to do that.
EXTERNC void adjustprplist(INT_CURRENTEDIT,unsigned curpos) {
  unsigned t1=SENDMSGTOCED(currentEdit, SCI_GETLENGTH,0,0);
  if (g_PopLists[g_uPopListNo].poptextlen) {
    unsigned i; for(i=0; i<g_PopLists[g_uPopListNo].popcount; i++) if (g_PopLists[g_uPopListNo].poplist[i]>curpos) g_PopLists[g_uPopListNo].poplist[i]+=t1-g_PopLists[g_uPopListNo].poptextlen;
  }
  g_PopLists[g_uPopListNo].poptextlen=t1;
}

EXTERNC void pushpositionex(INT_CURRENTEDIT){
  detectprplist(void);
  unsigned curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
  adjustprplist(currentEdit,curpos);
  if (g_PopLists[g_uPopListNo].popcount>=NELEM(g_PopLists[g_uPopListNo].poplist)) {
    memmove(g_PopLists[g_uPopListNo].poplist,g_PopLists[g_uPopListNo].poplist+1,sizeof(g_PopLists[g_uPopListNo].poplist)-sizeof(g_PopLists[g_uPopListNo].poplist[0]));
    g_PopLists[g_uPopListNo].popcount--;
  }
  g_PopLists[g_uPopListNo].poplist[g_PopLists[g_uPopListNo].popcount]=curpos;
  g_PopLists[g_uPopListNo].popcount++;
}

EXTERNC PFUNCPLUGINCMD pfpushposition(void){ INT_CURRENTEDIT; GET_CURRENTEDIT; pushpositionex(currentEdit); }

EXTERNC PFUNCPLUGINCMD pfpushjumpposition(void){
  INT_CURRENTEDIT; GET_CURRENTEDIT;
  struct TextToFind tr;
  tr.chrg.cpMin=0; tr.chrg.cpMax=SENDMSGTOCED(currentEdit, SCI_GETLENGTH,0,0); if (tr.lpstrText=strdupClipboardText(NULL,0)) {
    int findpos=SENDMSGTOCED(currentEdit, SCI_FINDTEXT,SCFIND_MATCHCASE,&tr);
     if (findpos>=0) {
      pushpositionex(currentEdit);
      SENDMSGTOCED(currentEdit, SCI_GOTOPOS, findpos, 0);
     }
    freesafe(tr.lpstrText,"pfpushjumpposition");
  }
}

EXTERNC PFUNCPLUGINCMD pfpopposition(void){
#define linedx 1
  detectprplist(void);
  INT_CURRENTEDIT; GET_CURRENTEDIT;
  unsigned curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
  adjustprplist(currentEdit,curpos);
  if (g_PopLists[g_uPopListNo].popcount) {
    g_PopLists[g_uPopListNo].popcount--;
    unsigned curline=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,curpos,0);
    unsigned goline=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,(g_PopLists[g_uPopListNo].poplist[g_PopLists[g_uPopListNo].popcount]),0);
    if (g_PopLists[g_uPopListNo].popcount && ((goline>curline)?(goline-curline):(curline-goline))<=linedx) {
      SENDMSGTOCED(currentEdit, SCI_GOTOPOS, (g_PopLists[g_uPopListNo].poplist[g_PopLists[g_uPopListNo].popcount-1]),0);
    } else {
      SENDMSGTOCED(currentEdit, SCI_GOTOPOS, (g_PopLists[g_uPopListNo].poplist[g_PopLists[g_uPopListNo].popcount]),0);
      g_PopLists[g_uPopListNo].popcount++;
    }
  }
#undef linedx
}
#endif /*SUBSECTION: End */

// SUBSECTION: Multiplex Subclass Menu System
#define PFUNCPLUGINCMX void __cdecl /* Some compilers cannot use the typedef in every case. This define works to alter function declarations with all compilers */
typedef void (__cdecl PFUNCPLUGINCMX_MSC)(unsigned); /* MSC is unable to declare variables without a typedef. Compilers other than Watcom prefer this method also but often work with warnings with the #define. */
#define PFUNCPLUGINCMY USHORT __cdecl /* Some compilers cannot use the typedef in every case. This define works to alter function declarations with all compilers */

typedef USHORT (__cdecl PFUNCPLUGINCMY_MSC)(unsigned,unsigned); /* MSC is unable to declare variables without a typedef. Compilers other than Watcom prefer this method also but often work with warnings with the #define. */

extern struct _wIDLOOKUP {
	PFUNCPLUGINCMD_MSC *_pFunc;
	PFUNCPLUGINCMX_MSC *buildfunc;
	PFUNCPLUGINCMY_MSC *usefunc; // returns 0 if fully processed or replacement wParamLo to pass call onto
	USHORT wParamLo,len;
	TCHAR *appdata1;
	HMENU   hSubMenu;
	unsigned dummy,dummy2;
#ifdef _WIN64
	/* On 64-bit platform, suppress:
	pfbuildmenu(void) has:
		if (!powcheck(sizeof(struct _wIDLOOKUP)))
			MessageBoxFree(g_nppData._nppHandle,smprintf(_T("sizeof(struct _wIDLOOKUP)==%zu is not a power of two"), sizeof(struct _wIDLOOKUP)), _T(PLUGIN_NAME), MB_OK|MB_ICONWARNING);
	*/
	unsigned dummy3, dummy4;
#endif
} g_wIDlookup[];
EXTERNC void mpxbuildmenu(TCHAR *buf,unsigned idx,WORD ic);
EXTERNC unsigned findmpxmenuitem(PFUNCPLUGINCMX_MSC *_pFunc);

PFUNCPLUGINCMX pxmpxhelpmpxbuild(unsigned idx) {
  //MessageBoxFree(g_nppData._nppHandle,smprintf("wParamLo:%u wParamHi:%u",wParamLo,wParamHi),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
}
EXTERNC PFUNCPLUGINCMD pfmpxhelprebuild(void) {pxmpxhelpmpxbuild(findmpxmenuitem(pxmpxhelpmpxbuild));}

PFUNCPLUGINCMY pympxhelpmpxuse(unsigned idx,unsigned ic) {
  //MessageBoxFree(g_nppData._nppHandle,smprintf("wParamLo:%u wParamHi:%u",wParamLo,wParamHi),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
return(0);
}

#if ENABLE_TIDYDLL
EXTERNC PFUNCPLUGINCMD pfhtmltidy(void)       { convertall('T',CAFLAG_GETALLWHENNOSELECTION|CAFLAG_DENYBLOCK|CAFLAG_DENYBINARY,NULL,NULL,NULL,NULL);  }

TCHAR *g_szTidyDLL_error=NULL;
HINSTANCE g_hTidyDLL=NULL;

extern unsigned g_miDisableSubclassing;
#define NPPTIDY_INI_MENU NPPTEXT("TidyCFG.INI")
#define NPPTIDY_INI _T("TidyCFG.INI")
EXTERNC PFUNCPLUGINCMD pfabouttidy(void) {
  if (!g_hTidyDLL)
	  MessageBox(g_nppData._nppHandle, g_szTidyDLL_error?g_szTidyDLL_error : _T("Unknown HTMLtidy error"),
	    _T(PLUGIN_NAME), MB_OK|MB_ICONSTOP);
  else MessageBoxFree(g_nppData._nppHandle,
	  smprintf("HTML Tidy Release Date: %s\r\nThanks to PSPAD for %s\r\n%s",TIDYDLLCALL(tidyReleaseDate)(),NPPTIDY_INI,funcItem[g_miDisableSubclassing]._init2Check?"The Tidy menu is partly disabled.\r\nEnable Subclassing for full functionality":""),
	  _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}

EXTERNC PFUNCPLUGINCMD pfhtmltidyweb(void) {LaunchURL(_T("http://tidy.sourceforge.net/"));}
EXTERNC PFUNCPLUGINCMD pfgethtmltidyweb(void) {LaunchURL(_T("http://notepad-plus.sourceforge.net/uk/download.php"));}

EXTERNC void control_tidy(int);
EXTERNC PFUNCPLUGINCMD pfmpxtidyrebuild(void);
EXTERNC PFUNCPLUGINCMD pfreloadtidydll(void) {
  if (!g_hTidyDLL) {
        control_tidy(0);
        if (!g_hTidyDLL) pfabouttidy();
        pfmpxtidyrebuild();
  }
}

EXTERNC void control_tidy(int stop) { // FALSE to start, TRUE to stop.
  TCHAR *szPath=NULL,*errfunc=NULL;
  unsigned uPathSz=0;
  if (!stop) {
    if (g_szTidyDLL_error) {freesafe(g_szTidyDLL_error,"control_tidy"); g_szTidyDLL_error=NULL;}
    NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL, _T("libTidy.dll"));
    //path=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,"libTidy.dll");
    if ((g_hTidyDLL=LoadLibrary(_T("libTidy.dll"))) || (szPath && (g_hTidyDLL=LoadLibrary(szPath)))) {
//      TIDYDLLINIT
		TIDYGENDLLINDIRECT2(TidyDoc, tidyCreate, _T("@0"), (void));
		TIDYGENDLLINDIRECT2(void, tidyRelease, _T("@4"), (TidyDoc));
			TIDYGENDLLINDIRECT(Bool, tidyFileExists, "@4", (ctmbstr));
			TIDYGENDLLINDIRECT(int, tidyLoadConfig, "@8", (TidyDoc, ctmbstr));
			TIDYGENDLLINDIRECT(Bool, tidyOptResetToDefault, "@8", (TidyDoc, TidyOptionId));
			TIDYGENDLLINDIRECT(Bool, tidyOptGetBool, "@8", (TidyDoc, TidyOptionId));
			TIDYGENDLLINDIRECT(Bool, tidyOptSetBool, "@12", (TidyDoc, TidyOptionId, Bool));
			TIDYGENDLLINDIRECT(Bool, tidyOptSetInt, "@12", (TidyDoc, TidyOptionId, ulong));
			TIDYGENDLLINDIRECT(ulong, tidyOptGetInt, "@8", (TidyDoc, TidyOptionId));
			TIDYGENDLLINDIRECT(ctmbstr, tidyOptGetValue, "@8", (TidyDoc, TidyOptionId));
			TIDYGENDLLINDIRECT(Bool, tidyOptParseValue, "@12", (TidyDoc, ctmbstr, ctmbstr));
			TIDYGENDLLINDIRECT(Bool, tidyOptSetValue, "@12", (TidyDoc, TidyOptionId, ctmbstr));
			TIDYGENDLLINDIRECT(int, tidySetCharEncoding, "@8", (TidyDoc, ctmbstr));
			TIDYGENDLLINDIRECT(FILE*, tidySetErrorFile, "@8", (TidyDoc, ctmbstr));
			TIDYGENDLLINDIRECT(int, tidySetErrorBuffer, "@8", (TidyDoc, TidyBuffer*));
			TIDYGENDLLINDIRECT(uint, tidyErrorCount, "@4", (TidyDoc));
			TIDYGENDLLINDIRECT(uint, tidyWarningCount, "@4", (TidyDoc));
			TIDYGENDLLINDIRECT(uint, tidyAccessWarningCount, "@4", (TidyDoc));
			TIDYGENDLLINDIRECT(int, tidyParseString, "@8", (TidyDoc, ctmbstr));
			TIDYGENDLLINDIRECT(int, tidyCleanAndRepair, "@4", (TidyDoc));
			TIDYGENDLLINDIRECT(int, tidyRunDiagnostics, "@4", (TidyDoc));
			TIDYGENDLLINDIRECT(int, tidySaveBuffer, "@8", (TidyDoc, TidyBuffer*));
			TIDYGENDLLINDIRECT(void, tidyBufFree, "@4", (TidyBuffer*));
			TIDYGENDLLINDIRECT(ctmbstr, tidyReleaseDate, "@0", (void));
			TIDYGENDLLINDIRECT(TidyOptionId, tidyOptGetIdForName, "@4", (ctmbstr));
      DeleteMenu(GetMenu(g_nppData._nppHandle),funcItem[findmenuitem(pfgethtmltidyweb)]._cmdID, MF_BYCOMMAND);
      DeleteMenu(GetMenu(g_nppData._nppHandle),funcItem[findmenuitem(pfreloadtidydll)]._cmdID, MF_BYCOMMAND);
      EnableMenuItem(GetMenu(g_nppData._nppHandle), funcItem[findmenuitem(pfhtmltidy)]._cmdID, MF_BYCOMMAND);
      EnableMenuItem(GetMenu(g_nppData._nppHandle), funcItem[findmenuitem(pfmpxtidyrebuild)]._cmdID, MF_BYCOMMAND);
    } else { // Only disable the menu on startup. The menu may no longer be valid when exiting the DLL
fail: if (errfunc) g_szTidyDLL_error=smprintf("Exported function %s not found in %s.\r\nThis " PLUGIN_NAME " build requires " TIDY_CALLQT " exports.",errfunc,"libTidy.dll");
      else g_szTidyDLL_error=smprintf("Unable to find %s in the system path\r\nor %s","libTidy.dll",szPath);
      EnableMenuItem(GetMenu(g_nppData._nppHandle), funcItem[findmenuitem(pfhtmltidy)]._cmdID, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(GetMenu(g_nppData._nppHandle), funcItem[findmenuitem(pfmpxtidyrebuild)]._cmdID, MF_BYCOMMAND | MF_GRAYED);
      stop=TRUE;
    }
    DrawMenuBar(g_nppData._nppHandle);
  }
  if (szPath) freesafe(szPath,"control_tidy");
  if (stop && g_hTidyDLL) {
    FreeLibrary(g_hTidyDLL);
    g_hTidyDLL=NULL;
  }
}

PFUNCPLUGINCMX pxmpxtidympxbuild(unsigned idx) {
  //MessageBoxFree(g_nppData._nppHandle,smprintf("wParamLo:%u wParamHi:%u",wParamLo,wParamHi),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
  char *buf=NULL; unsigned bufsz=0,buflen=0;
  USHORT *bufs=NULL; unsigned bufssz=0;
  unsigned lim;

  if (g_hTidyDLL) {
    FILE *fp;
	TCHAR *szPath=NULL;
	unsigned uPathSz=0;
    NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL,NPPTIDY_INI);
    if (!(szPath/*=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,NPPTIDY_INI)*/) || !(fp=fopen(szPath,"rt"))) goto failbreak;
    freesafe(szPath,"pxmpxtidympxbuild");
    WORD ic;
	char *t;
	char ln[256];
	for(ic=0,lim=50; lim && fgets(ln,sizeof(ln)-1,fp); )  if (ln[0]=='[' && strlen(t=strtokX(ln,NULL,"[]"))) {
      lim--;
      armreallocsafe((char **)&bufs, (size_t*)&bufssz,(ic+1)*sizeof(*bufs),ARMSTRATEGY_INCREASE,0,"pxmpxtidympxbuild"); if (!bufs) goto failbreak;
      bufs[ic++]=buflen;
      strcpyarmsafe(&buf, (size_t*)&bufsz, (size_t*)&buflen,t,"pxmpxtidympxbuild"); if (!buf) goto failbreak;
      buflen++; // preserve the \0
    }
    fclose(fp);
    WORD i; for(i=0; i<ic; i++) bufs[i]+=ic*sizeof(*bufs);
    memmovearmtest(&buf,&bufsz,&buflen,buf+ic*sizeof(*bufs),buf,1); if (!buf) goto failbreak;
    memcpy(buf,bufs,ic*sizeof(*bufs));
    //INT_CURRENTEDIT; GET_CURRENTEDIT; SENDMSGTOCED(currentEdit, SCI_ADDTEXT, buflen, buf); //SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, "\r\n");SENDMSGTOCED(currentEdit, SCI_ADDTEXT, ic*sizeof(*bufs), bufs);
    mpxbuildmenu(buf,idx,ic);
    buf=NULL;
failbreak:
    if (buf) freesafe(buf,"pxmpxtidympxbuild");
    if (bufs) freesafe(bufs,"pxmpxtidympxbuild");
  }
}
EXTERNC PFUNCPLUGINCMD pfmpxtidyrebuild(void) {pxmpxtidympxbuild(findmpxmenuitem(pxmpxtidympxbuild));}

EXTERNC PFUNCPLUGINCMY pympxtidympxuse(unsigned idx,unsigned ic) {
  USHORT rv=0;
  char *szPath=NULL; unsigned uPathSz=0;

  do {
    char *c=g_wIDlookup[idx].appdata1; USHORT *cs=(USHORT *)c;
    char *opt=smprintf("[%s]",c+cs[ic]); if (!opt) break;
    unsigned optsz=strlen(opt);
    NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL,NPPTIDY_INI);
    FILE *fp; if (!(szPath/*=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,NPPTIDY_INI)*/) || !(fp=fopen(szPath,"rt"))) break;
    //freesafe(szPath,"pympxtidympxuse");
    NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXTEMP,NULL,&szPath,&uPathSz,NULL,NPPTIDY_CFG);
    if (!(szPath/*=smprintfpath("%s%?\\%s%?\\%s",g_pszPluginpath,SUPPORT_PATH,NPPTIDY_CFG)*/) ) break;
    FILE *fo=NULL;
    char *failed=NULL; unsigned failedsz=0,failedlen;
    int fnd=FALSE; char ln[256]; while(fgets(ln,sizeof(ln)-1,fp)) {
      if (ln[0]!=';') {
        if (!fnd) {
          if (ln[0]=='[' && !memcmp(ln,opt,optsz)) {
            fnd=TRUE;
            if (!(fo=fopen(szPath,"wt"))) goto failbreak;
          }
        } else {
          if (ln[0]=='[') break;
          fputs(ln,fo);
          if (!isspace(*ln)) {
            char *t=strchr(ln,':');
            if (t) {
              *t='\0';
              TidyOptionId mytid=TIDYDLLCALL(tidyOptGetIdForName)(ln);
              //MessageBoxFree(g_nppData._nppHandle,smprintf("tid:%u=tidyOptGetIdForName(%s)",mytid,ln),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
              if (N_TIDY_OPTIONS==mytid || TidyUnknownOption==mytid) sarmprintf(&failed, (size_t*)&failedsz, (size_t*)&failedlen,"%s\r\n",ln);
              *t=':';
            }
          }
        }
      }
    }
    if (fo) {
      fclose(fo);
      if (failed) {
        MessageBoxFree(g_nppData._nppHandle,smprintf("Invalid TIDY options:\r\n%s",failed),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
        freesafe(failed,"pympxtidympxuse");
      } else rv=(USHORT)funcItem[findmenuitem(pfhtmltidy)]._cmdID;
    } else MessageBoxFree(g_nppData._nppHandle,smprintf("Not Found idx:%u ic:%u option:%s",idx,ic,opt),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
    freesafe(opt,"pympxtidympxuse");
  } while(0);
failbreak:
  if (szPath) freesafe(szPath,"pympxtidympxuse");
  return(rv);
}
#endif

// ??? What is this structure?
struct _wIDLOOKUP g_wIDlookup[] = {
#if ENABLE_TIDYDLL
{pfmpxtidyrebuild,pxmpxtidympxbuild,pympxtidympxuse,0,0,0,NULL,0,0},
#endif
{pfmpxhelprebuild,pxmpxhelpmpxbuild,pympxhelpmpxuse,0,0,0,NULL,0,0},
};

EXTERNC unsigned findmpxmenuitem(PFUNCPLUGINCMX_MSC *_pFunc) {
	unsigned itemno;
	for (itemno = 0; itemno < NELEM(g_wIDlookup); itemno++)
		if (_pFunc == g_wIDlookup[itemno].buildfunc)
			return(itemno);
	return(0);
}

EXTERNC void freeIDLOOKUP(unsigned id) {
  if (g_wIDlookup[id].wParamLo) {
        WORD wID; for(wID=0; wID<g_wIDlookup[id].len; wID++) DeleteMenu(g_wIDlookup[id].hSubMenu,g_wIDlookup[id].wParamLo+wID,MF_BYCOMMAND);
          if (g_wIDlookup[id].appdata1) {
			  freesafe(g_wIDlookup[id].appdata1, _T("freeIDLOOKUP"));
			  g_wIDlookup[id].appdata1=NULL; /*g_wIDlookup[id].appdata1sz=0;*/
		  }
          //if (g_wIDlookup[id].appdata2) {freesafe(g_wIDlookup[id].appdata2,"freeIDLOOKUP"); g_wIDlookup[id].appdata2=NULL; }
          g_wIDlookup[id].wParamLo=0;
          g_wIDlookup[id].len=0;
  }
}

USHORT g_usMinwID=0,g_usNextwID=0;
EXTERNC void mallocIDLOOKUP(unsigned rv,WORD len) {
  if (len) {
    WORD wID; for(wID=g_usMinwID; wID<g_usNextwID; ) {
      unsigned wln; for(wln=0; wln<NELEM(g_wIDlookup); wln++) {
        if (wln != rv && g_wIDlookup[wln].wParamLo) {
                WORD mn=g_wIDlookup[wln].wParamLo,mx=mn+g_wIDlookup[wln].len-1;
          //MessageBoxFree(g_nppData._nppHandle,smprintf("wID:%u wID+len:%u -- mn:%u mx:%u",wID,wID+len,mn,mx),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
          //INT_CURRENTEDIT; GET_CURRENTEDIT; char *t;t=smprintf("mn:%u >= wID:%u && mn:%u < wID+len-1:%u || mx:%u >= wID:%u && mx:%u < wID+len-1:u\r\n",mn,wID,mn,wID+len-1,mx,wID,mx,wID+len-1);  SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, t); freesafe(t,"mallocIDLOOKUP");
          if ((mn>=wID && mn<wID+len-1) || (mx>=wID && mx<=wID+len-1)) {wID=mx+1; goto cnt;}
        }
      }
      goto found;
cnt: ;
    }
found:
    g_wIDlookup[rv].wParamLo=wID; g_wIDlookup[rv].len=len;
	if (g_usNextwID<wID+len)
		g_usNextwID=wID+len;
  }
}

EXTERNC void mpxbuildmenu(TCHAR *buf,unsigned idx,WORD ic) {
  MENUITEMINFO mi;
  freeIDLOOKUP(idx);
  mallocIDLOOKUP(idx,ic);
  g_wIDlookup[idx].appdata1=buf;
  TCHAR *c = g_wIDlookup[idx].appdata1;
  USHORT *cs=(USHORT *)c;
  WORD i;
  for(i=0; i<ic; i++) {
    //MessageBoxFree(g_nppData._nppHandle,smprintf("i:%u str:(%x + %u)",i,c,cs[i]),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
    mi.cbSize=sizeof(mi);
	mi.fMask=MIIM_ID|MIIM_TYPE;
	mi.wID=g_wIDlookup[idx].wParamLo+i;
	mi.dwTypeData=(TCHAR*)c+cs[i];
	mi.cch=wcslen(mi.dwTypeData);
	mi.fType=MFT_STRING;
	InsertMenuItem(g_wIDlookup[idx].hSubMenu,GetMenuItemCount(g_wIDlookup[idx].hSubMenu),TRUE,&mi);
  }
}

#if ENABLE_FINDREPLACE
#include "poppad/popfind.h"

EXTERNC PFUNCPLUGINCMD pffindreplace(void) {
	INT_CURRENTEDIT; GET_CURRENTEDIT;
	PopFindReplaceDlg(g_nppData._nppHandle, (currentEdit ? g_nppData._scintillaSecondHandle : g_nppData._scintillaMainHandle), g_hInstance, NULL, FALSE);
}

#endif

#include "nppsrc/resource.h" /* From Notepad ++ */
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/winui/windowsuserinterface/windowing/windowprocedures/usingwindowprocedures.asp
WNDPROC g_wpNPPOrigProc=NULL;
// This init() is correct but I think g_wpNPPOrigProc should be set before SetWindowLong() sets the new proc. This would mean doing it in two steps.
#define NPPSubclassProc_init() if (!g_wpNPPOrigProc /*&& g_usMinwID && g_usNextwID*/) g_wpNPPOrigProc=(WNDPROC)SetWindowLongPtr(g_nppData._nppHandle,GWLP_WNDPROC,(LONG_PTR)NPPSubclassProc)
#define NPPSubclassProc_close() if (g_wpNPPOrigProc) do {SetWindowLongPtr(g_nppData._nppHandle,GWLP_WNDPROC,(LONG_PTR)g_wpNPPOrigProc); g_wpNPPOrigProc=NULL;} while(0)
EXTERNC LRESULT APIENTRY NPPSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_COMMAND) {
		WORD wParamLo = LOWORD(wParam), wParamHi = HIWORD(wParam);
		if (g_usMinwID && wParamLo >= g_usMinwID && wParamLo < g_usNextwID) {
			unsigned i; for (i = 0; i < NELEM(g_wIDlookup); i++)
				if (wParamLo >= g_wIDlookup[i].wParamLo && wParamLo < g_wIDlookup[i].wParamLo + g_wIDlookup[i].len) {
					if (!(wParamLo = g_wIDlookup[i].usefunc(i, wParamLo - g_wIDlookup[i].wParamLo)))
						return(0);
					wParam = MAKEWPARAM(wParamLo, wParamHi);
					break;
				}
		} else if (wParamHi)
			switch (wParamLo) { // we are only capturing N++ accelerator keys, not menuitems
	 //case IDM_EDIT_COPY: wParam=MAKEWPARAM(funcItem[findmenuitem(pfcopyall)]._cmdID,wParamHi); break;
	 //case IDM_EDIT_LINE_UP: wParam=MAKEWPARAM(funcItem[findmenuitem(pfMarkWordFindReverse)]._cmdID,wParamHi); break;
	 //case IDM_EDIT_LINE_DOWN: wParam=MAKEWPARAM(funcItem[findmenuitem(pfMarkWordFindForward)]._cmdID,wParamHi); break;
			case IDM_SEARCH_GOTOMATCHINGBRACE: wParam = MAKEWPARAM(funcItem[findmenuitem(pffindmatchchar)]._cmdID, wParamHi); break;
#if ENABLE_FINDREPLACE && 0
			case IDM_SEARCH_FIND: wParam = MAKEWPARAM(funcItem[findmenuitem(pffindreplace)]._cmdID, wParamHi); break;
#endif
			}
	}

	return CallWindowProc(g_wpNPPOrigProc, hwnd, uMsg, wParam, lParam);
}

EXTERNC PFUNCPLUGINCMD freeallIDLOOKUP(void) {
  unsigned wln;
  for(wln=0; wln<NELEM(g_wIDlookup); wln++)
	  freeIDLOOKUP(wln);
}

EXTERNC PFUNCPLUGINCMD buildallIDLOOKUP(void) {
  unsigned wln;
  for(wln=0; wln<NELEM(g_wIDlookup); wln++)
	  g_wIDlookup[wln].buildfunc(wln);
}

#if NPPDEBUG
EXTERNC PFUNCPLUGINCMD pfdisplaywIDlookup(void) {
  INT_CURRENTEDIT;
  GET_CURRENTEDIT;
  TCHAR *dt=NULL;
  size_t dtsz=0,dtlen;
  sarmprintf(&dt,&dtsz,&dtlen, _T("g_usMinwID:%u g_usNextwID:%u\r\n"), g_usMinwID, g_usNextwID);
  unsigned wln;
  for(wln=0; wln<NELEM(g_wIDlookup); wln++)
	  sarmprintf(&dt,&dtsz,&dtlen, _T("dummy:0x%X build:0x%X use:0x%X mi:%u wParamLo:%d len:%d appdata1:%X hSubMenu:%x\r\n"),
		  g_wIDlookup[wln]._pFunc,g_wIDlookup[wln].buildfunc,g_wIDlookup[wln].usefunc,g_wIDlookup[wln].wParamLo,g_wIDlookup[wln].len,g_wIDlookup[wln].appdata1,g_wIDlookup[wln].hSubMenu);
  if (dt) {
    SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, dt);
    freesafe(dt, _T("pfdisplaywIDlookup"));
  }
}

EXTERNC PFUNCPLUGINCMD pfdisplaywIDtest(void) {
	buildallIDLOOKUP();
	pfdisplaywIDlookup();
	//mallocIDLOOKUP(0,20);
	pfdisplaywIDlookup();
}
#endif

// SUBSECTION: END
EXTERNC BOOL altercheck(int itemno, char action) {
	switch (action) {
	case '1': // set
		funcItem[itemno]._init2Check = TRUE;
		break;
	case '0': // clear
		funcItem[itemno]._init2Check = FALSE;
		break;
	case '!': // invert
		funcItem[itemno]._init2Check = !funcItem[itemno]._init2Check;
		break;
	case '-': // change nothing but apply the current check value
		break;
	}
	CheckMenuItem(GetMenu(g_nppData._nppHandle), funcItem[itemno]._cmdID, MF_BYCOMMAND | ((funcItem[itemno]._init2Check) ? MF_CHECKED : MF_UNCHECKED));

	return(funcItem[itemno]._init2Check);
}

// these are used to instantly access the menu items
unsigned g_miSortAscending;
EXTERNC PFUNCPLUGINCMD pfSortAscending(void)    {altercheck(g_miSortAscending,'!');}
EXTERNC PFUNCPLUGINCMD pfqsortlinesnc(void)   { convertall('q', CAFLAG_EXTENDTOLINES|CAFLAG_DENYBINARY, _T("n"), funcItem[g_miSortAscending]._init2Check ? _T("a") : _T("d"), NULL, NULL); }
EXTERNC PFUNCPLUGINCMD pfqsortlinesc(void)    { convertall('q', CAFLAG_EXTENDTOLINES|CAFLAG_DENYBINARY, _T("c"), funcItem[g_miSortAscending]._init2Check ? _T("a") : _T("d"), NULL, NULL); }

unsigned g_miBlockOverwrite;
EXTERNC PFUNCPLUGINCMD pfBlockOverwrite(void) {
	if (altercheck(g_miBlockOverwrite, '!')) {
		INT_CURRENTEDIT;
		GET_CURRENTEDIT;
		SENDMSGTOCED(currentEdit, SCI_SETOVERTYPE, 0, 0);
	}
}

unsigned g_miDisableSubclassing;
EXTERNC PFUNCPLUGINCMD pfDisableSubclassing(void)
{
	if (altercheck(g_miDisableSubclassing,'!')) {
		NPPSubclassProc_close();
		freeallIDLOOKUP();
	} else {
		buildallIDLOOKUP();
		NPPSubclassProc_init();
	}
}

#ifndef SCMOD_NORM
#define SCMOD_NORM 0
#endif
#if ENABLE_WrapFriendlyHomeEnd
EXTERNC void WrapFriendlyHomeEnd(BOOL enable) {
  INT_CURRENTEDIT; GET_CURRENTEDIT;
  if (enable) {
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_HOME,SCMOD_NORM) , SCI_HOMEWRAP);
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_HOME,SCMOD_SHIFT), SCI_HOMEWRAPEXTEND);
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_END,SCMOD_NORM)  , SCI_LINEENDWRAP);
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_END,SCMOD_SHIFT) , SCI_LINEENDWRAPEXTEND);
  } else {
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_HOME,SCMOD_NORM) , SCI_VCHOME);
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_HOME,SCMOD_SHIFT), SCI_VCHOMEEXTEND);
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_END,SCMOD_NORM)  , SCI_LINEEND);
    SENDMSGTOCED(currentEdit,SCI_ASSIGNCMDKEY, MAKEWPARAM(SCK_END,SCMOD_SHIFT) , SCI_LINEENDEXTEND);
  }
}
unsigned g_miWrapFriendlyHomeEnd; EXTERNC PFUNCPLUGINCMD pfWrapFriendlyHomeEnd(void) {WrapFriendlyHomeEnd(altercheck(g_miWrapFriendlyHomeEnd,'!'));}
#endif

EXTERNC BOOL CaptureCutCopyPaste(BOOL capture) {
#if NPPFUNCITEMARRAYVERSION >= 2
  if (!capture) {
    funcItem[findmenuitem(pfcopyallnoappend)]._pShKey=NULL;
    funcItem[findmenuitem(pfcutallnoappend)]._pShKey=NULL;
    funcItem[findmenuitem(pfVizPasteUNICODE)]._pShKey=NULL;
  }
#endif

  return(capture);
}
unsigned g_miCaptureCutCopyPaste; EXTERNC PFUNCPLUGINCMD pfCaptureCutCopyPaste(void) {
  MessageBoxFree(g_nppData._nppHandle,smprintf(_T("Next time you load Notepad++, Ctrl X,C,V will%s be captured!"), CaptureCutCopyPaste(altercheck(g_miCaptureCutCopyPaste,'!'))?"":" not"),
	  _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}

unsigned g_miSeparateQuickMenus       ;EXTERNC PFUNCPLUGINCMD pfSeparateQuickMenus(void) {
  MessageBoxFree(g_nppData._nppHandle,smprintf(_T("Next time you load Notepad++, quick menus will%s be separated!"), altercheck(g_miSeparateQuickMenus,'!')?"":" not"),
	  _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}

EXTERNC BOOL CaptureCtrlDAlsoDupsBlock(BOOL capture) {
#if NPPFUNCITEMARRAYVERSION >= 2
  if (!capture) {
    funcItem[findmenuitem(pfDuplicateLineOrBlock)]._pShKey=NULL;
  }
#endif

  return(capture);
}
unsigned g_miCtrlDAlsoDupsBlock; EXTERNC PFUNCPLUGINCMD pfCtrlDAlsoDupsBlock(void) {
  MessageBoxFree(g_nppData._nppHandle,smprintf(_T("Next time you load Notepad++, Ctrl D will%s duplicate blocks!"), CaptureCtrlDAlsoDupsBlock(altercheck(g_miCtrlDAlsoDupsBlock,'!'))?"":" not"),
	  _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
}

unsigned g_miAutoCloseHTMLtag         ;EXTERNC PFUNCPLUGINCMD pfAutoCloseHTMLtag(void)         {altercheck(g_miAutoCloseHTMLtag,'!');}
unsigned g_miAutoCloseBrace           ;EXTERNC PFUNCPLUGINCMD pfAutoCloseBrace(void)           {altercheck(g_miAutoCloseBrace,'!');}
unsigned g_miAutoSpace2Tab            ;EXTERNC PFUNCPLUGINCMD pfAutoSpace2Tab(void)            {altercheck(g_miAutoSpace2Tab,'!');}
#if ENABLE_AutoShowMatchline
unsigned g_miAutoShowMatchline        ;EXTERNC PFUNCPLUGINCMD pfAutoShowMatchline(void)        {if (altercheck(g_miAutoShowMatchline,'!')) pfshowmatchline(void);}
#endif
unsigned g_miAutoConvertHTML          ;EXTERNC PFUNCPLUGINCMD pfAutoConvertHTML(void)          {altercheck(g_miAutoConvertHTML,'!');}
unsigned g_miSortLinesUnique          ;EXTERNC PFUNCPLUGINCMD pfSortLinesUnique(void)          {g_SortLinesUnique=altercheck(g_miSortLinesUnique,'!');}
unsigned g_miMarkWordFindCaseSensitive;EXTERNC PFUNCPLUGINCMD pfMarkWordFindCaseSensitive(void){g_fMarkWordFindCaseSensitive=altercheck(g_miMarkWordFindCaseSensitive,'!');}
unsigned g_miMarkWordFindWholeWord    ;EXTERNC PFUNCPLUGINCMD pfMarkWordFindWholeWord(void)    {g_fMarkWordFindWholeWord=altercheck(g_miMarkWordFindWholeWord,'!');}
#ifndef HighPerformance
unsigned g_miHighPerformance          ;EXTERNC PFUNCPLUGINCMD pfHighPerformance(void)          {HighPerformance=altercheck(g_miHighPerformance,'!');}
#endif

unsigned g_miVizCaseSensitive         ;EXTERNC PFUNCPLUGINCMD pfVizCaseSensitive(void)         {g_fVizCaseSensitive=altercheck(g_miVizCaseSensitive,'!');}
unsigned g_miVizWholeWords            ;EXTERNC PFUNCPLUGINCMD pfVizWholeWords(void)            {g_fVizWholeWords=altercheck(g_miVizWholeWords,'!');}
unsigned g_miVizRegex                 ;EXTERNC PFUNCPLUGINCMD pfVizRegex(void)                 {g_fVizRegex=altercheck(g_miVizRegex,'!');}
unsigned g_miVizCutCopyAppend         ;EXTERNC PFUNCPLUGINCMD pfVizCutCopyAppend(void)         {g_fVizCutCopyAppend=altercheck(g_miVizCutCopyAppend,'!');}
unsigned g_miVizClipboardAlwaysCRLF   ;EXTERNC PFUNCPLUGINCMD pfVizClipboardAlwaysCRLF(void)   {g_fVizClipboardAlwaysCRLF=altercheck(g_miVizClipboardAlwaysCRLF,'!');}
unsigned g_miVizClipboardReplaceNulls ;EXTERNC PFUNCPLUGINCMD pfVizClipboardReplaceNulls(void) {g_fVizClipboardReplaceNulls=altercheck(g_miVizClipboardReplaceNulls,'!');}
unsigned g_miVizClipboardCopyAlsoUTF8 ;EXTERNC PFUNCPLUGINCMD pfVizClipboardCopyAlsoUTF8(void) {g_fVizClipboardCopyAlsoUTF8=altercheck(g_miVizClipboardCopyAlsoUTF8,'!');}
unsigned g_miVizClipboardNotUnicode   ;EXTERNC PFUNCPLUGINCMD pfVizClipboardNotUnicode(void)   {g_fVizClipboardNotUnicode=altercheck(g_miVizClipboardNotUnicode,'!');}
unsigned g_miVizPasteRetainsPosition  ;EXTERNC PFUNCPLUGINCMD pfVizPasteRetainsPosition(void)  {g_fVizPasteRetainsPosition=altercheck(g_miVizPasteRetainsPosition,'!');}
unsigned g_miVizPasteBinary           ;EXTERNC PFUNCPLUGINCMD pfVizPasteBinary(void)           {g_fVizPasteBinary=altercheck(g_miVizPasteBinary,'!');}
unsigned g_miVizPasteToEditorEOL      ;EXTERNC PFUNCPLUGINCMD pfVizPasteToEditorEOL(void)      {g_fVizPasteToEditorEOL=altercheck(g_miVizPasteToEditorEOL,'!');}

EXTERNC void iniSaveSettings(/*char *ppath,*/BOOL save) {
	g_miCaptureCutCopyPaste = findmenuitem(pfCaptureCutCopyPaste);
	g_miSeparateQuickMenus = findmenuitem(pfSeparateQuickMenus);
	g_miCtrlDAlsoDupsBlock = findmenuitem(pfCtrlDAlsoDupsBlock);
	g_miBlockOverwrite = findmenuitem(pfBlockOverwrite);
	g_miAutoCloseHTMLtag = findmenuitem(pfAutoCloseHTMLtag);
	g_miAutoCloseBrace = findmenuitem(pfAutoCloseBrace);
	g_miAutoSpace2Tab = findmenuitem(pfAutoSpace2Tab);
#if ENABLE_AutoShowMatchline
	g_miAutoShowMatchline = findmenuitem(pfAutoShowMatchline);
#endif
	g_miAutoConvertHTML = findmenuitem(pfAutoConvertHTML);
	g_miSortAscending = findmenuitem(pfSortAscending);
	g_miSortLinesUnique = findmenuitem(pfSortLinesUnique);
	g_miVizCaseSensitive = findmenuitem(pfVizCaseSensitive);
	g_miVizWholeWords = findmenuitem(pfVizWholeWords);
	g_miVizRegex = findmenuitem(pfVizRegex);
	g_miVizCutCopyAppend = findmenuitem(pfVizCutCopyAppend);
	g_miVizClipboardAlwaysCRLF = findmenuitem(pfVizClipboardAlwaysCRLF);
	g_miVizClipboardReplaceNulls = findmenuitem(pfVizClipboardReplaceNulls);
	g_miVizClipboardCopyAlsoUTF8 = findmenuitem(pfVizClipboardCopyAlsoUTF8);
	g_miVizClipboardNotUnicode = findmenuitem(pfVizClipboardNotUnicode);
	g_miVizPasteRetainsPosition = findmenuitem(pfVizPasteRetainsPosition);
	g_miVizPasteBinary = findmenuitem(pfVizPasteBinary);
	g_miVizPasteToEditorEOL = findmenuitem(pfVizPasteToEditorEOL);
	g_miDisableSubclassing = findmenuitem(pfDisableSubclassing);
	g_miMarkWordFindCaseSensitive = findmenuitem(pfMarkWordFindCaseSensitive);
	g_miMarkWordFindWholeWord = findmenuitem(pfMarkWordFindWholeWord);
#if ENABLE_WrapFriendlyHomeEnd
	g_miWrapFriendlyHomeEnd = findmenuitem(pfWrapFriendlyHomeEnd);
#endif
#ifndef HighPerformance
	g_miHighPerformance = findmenuitem(pfHighPerformance);
#endif
	//g_mimpxhelpdummy  =findmenuitem(pfmpxhelprebuild);
	TCHAR *szINIPath = NULL;
	size_t uINIPathSz = 0;
	unsigned uINIPathLen;
  NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXINIFILE, NULL, &szINIPath, &uINIPathSz, &uINIPathLen, NULL);
  if (szINIPath) {
    //MessageBoxFree(0/*g_nppData._nppHandle*/,smprintf("Result: %s",szINIPath),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
    if (!save) { //
      PopFindReplaceDlgINI(save,/*ppath,*/szINIPath);
      altercheck(g_miSeparateQuickMenus,GetPrivateProfileInt(_T("Settings"), _T("SeparateQuickMenus"), 0, szINIPath)?'1':'0');
      altercheck(g_miBlockOverwrite    ,GetPrivateProfileInt(_T("Settings"), _T("BlockOverwrite")    , 0, szINIPath)?'1':'0');
      altercheck(g_miAutoCloseBrace    ,GetPrivateProfileInt(_T("Settings"), _T("AutoCloseBrace")    , 0, szINIPath)?'1':'0');
      altercheck(g_miAutoCloseHTMLtag  ,GetPrivateProfileInt(_T("Settings"), _T("AutoCloseHTMLtag")  , 0, szINIPath)?'1':'0');
      altercheck(g_miAutoSpace2Tab     ,GetPrivateProfileInt(_T("Settings"), _T("AutoSpace2Tab")     , 0, szINIPath)?'1':'0');
#if ENABLE_AutoShowMatchline
      altercheck(g_miAutoShowMatchline ,GetPrivateProfileInt("Settings", "AutoShowMatchline" , 0, szINIPath)?'1':'0');
#endif
      altercheck(g_miAutoConvertHTML   ,GetPrivateProfileInt(_T("Settings"), _T("AutoConvertHTML")   , 0, szINIPath)?'1':'0');
      altercheck(g_miSortAscending     ,GetPrivateProfileInt(_T("Settings"), _T("SortAscending")     , 1, szINIPath)?'1':'0');
      altercheck(g_miDisableSubclassing,GetPrivateProfileInt(_T("Settings"), _T("DisableSubclassing"), 0, szINIPath)?'1':'0');
      g_SortLinesUnique          =altercheck(g_miSortLinesUnique          ,GetPrivateProfileInt(_T("Settings"), _T("SortLinesUnique")          , 0, szINIPath)?'1':'0');
      g_fMarkWordFindCaseSensitive=altercheck(g_miMarkWordFindCaseSensitive,GetPrivateProfileInt(_T("Settings"), _T("MarkWordFindCaseSensitive"), 0, szINIPath)?'1':'0');
      g_fMarkWordFindWholeWord=altercheck(g_miMarkWordFindWholeWord,GetPrivateProfileInt(_T("Settings"), _T("MarkWordFindWholeWord"), 0, szINIPath)?'1':'0');
#if ENABLE_WrapFriendlyHomeEnd
      altercheck(g_miWrapFriendlyHomeEnd,GetPrivateProfileInt("Settings", "WrapFriendlyHomeEnd", 0, szINIPath)?'1':'0');
#endif
      CaptureCutCopyPaste(altercheck(g_miCaptureCutCopyPaste,GetPrivateProfileInt(_T("Settings"), _T("CaptureCutCopyPaste"), 0, szINIPath)?'1':'0'));
      CaptureCtrlDAlsoDupsBlock(altercheck(g_miCtrlDAlsoDupsBlock,GetPrivateProfileInt(_T("Settings"), _T("CtrlDAlsoDupsBlock"), 0, szINIPath)?'1':'0'));
#ifndef HighPerformance
      HighPerformance          =altercheck(g_miHighPerformance         ,GetPrivateProfileInt("Settings", "HighPerformance"         , 1, szINIPath)?'1':'0');
#endif
      g_fVizCaseSensitive        =altercheck(g_miVizCaseSensitive        ,GetPrivateProfileInt(_T("Viz"), _T("VizCaseSensitive")        , 0, szINIPath)?'1':'0');
      g_fVizWholeWords           =altercheck(g_miVizWholeWords           ,GetPrivateProfileInt(_T("Viz"), _T("VizWholeWords")           , 0, szINIPath)?'1':'0');
      g_fVizRegex                =altercheck(g_miVizRegex                ,GetPrivateProfileInt(_T("Viz"), _T("VizRegex")                , 0, szINIPath)?'1':'0');
      g_fVizCutCopyAppend        =altercheck(g_miVizCutCopyAppend        ,GetPrivateProfileInt(_T("Viz"), _T("VizCutCopyAppend")        , 0, szINIPath)?'1':'0');
      g_fVizClipboardAlwaysCRLF  =altercheck(g_miVizClipboardAlwaysCRLF  ,GetPrivateProfileInt(_T("Viz"), _T("VizClipboardAlwaysCRLF")  , 1, szINIPath)?'1':'0');
      g_fVizClipboardReplaceNulls=altercheck(g_miVizClipboardReplaceNulls,GetPrivateProfileInt(_T("Viz"), _T("VizClipboardReplaceNulls"), 0, szINIPath)?'1':'0');
      g_fVizClipboardCopyAlsoUTF8=altercheck(g_miVizClipboardCopyAlsoUTF8,GetPrivateProfileInt(_T("Viz"), _T("VizClipboardCopyAlsoUTF8"), 0, szINIPath)?'1':'0');
      g_fVizClipboardNotUnicode  =altercheck(g_miVizClipboardNotUnicode  ,GetPrivateProfileInt(_T("Viz"), _T("VizClipboardNotUnicode")  , 0, szINIPath)?'1':'0');
      g_fVizPasteRetainsPosition =altercheck(g_miVizPasteRetainsPosition ,GetPrivateProfileInt(_T("Viz"), _T("VizPasteRetainsPosition") , 0, szINIPath)?'1':'0');
      g_fVizPasteBinary          =altercheck(g_miVizPasteBinary          ,GetPrivateProfileInt(_T("Viz"), _T("VizPasteBinary")          , 0, szINIPath)?'1':'0');
      g_fVizPasteToEditorEOL     =altercheck(g_miVizPasteToEditorEOL     ,GetPrivateProfileInt(_T("Viz"), _T("VizPasteToEditorEOL")     , 1, szINIPath)?'1':'0');
    } else {
      WritePrivateProfileString(_T("Settings"), _T("SeparateQuickMenus")       ,(funcItem[g_miSeparateQuickMenus       ]._init2Check)? _T("1"):_T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("BlockOverwrite")           ,(funcItem[g_miBlockOverwrite           ]._init2Check)? _T("1"):_T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("AutoCloseBrace")           ,(funcItem[g_miAutoCloseBrace           ]._init2Check)? _T("1"):_T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("AutoCloseHTMLtag")         ,(funcItem[g_miAutoCloseHTMLtag         ]._init2Check)? _T("1"):_T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("AutoSpace2Tab")            ,(funcItem[g_miAutoSpace2Tab            ]._init2Check)? _T("1"):_T("0"),szINIPath);
  #if ENABLE_AutoShowMatchline
      WritePrivateProfileString("Settings","AutoShowMatchline"        ,(funcItem[g_miAutoShowMatchline        ]._init2Check)?"1":"0",szINIPath);
  #endif
      WritePrivateProfileString(_T("Settings"), _T("AutoConvertHTML")          ,(funcItem[g_miAutoConvertHTML          ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("SortAscending")            ,(funcItem[g_miSortAscending            ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("SortLinesUnique")          ,(funcItem[g_miSortLinesUnique          ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("MarkWordFindCaseSensitive"),(funcItem[g_miMarkWordFindCaseSensitive]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("MarkWordFindWholeWord")    ,(funcItem[g_miMarkWordFindWholeWord    ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("DisableSubclassing")       ,(funcItem[g_miDisableSubclassing       ]._init2Check)? _T("1") : _T("0"),szINIPath);
  #if ENABLE_WrapFriendlyHomeEnd
      WritePrivateProfileString("Settings","WrapFriendlyHomeEnd"      ,(funcItem[g_miWrapFriendlyHomeEnd      ]._init2Check)?"1":"0",szINIPath);
  #endif
      WritePrivateProfileString(_T("Settings"), _T("CaptureCutCopyPaste")      ,(funcItem[g_miCaptureCutCopyPaste      ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Settings"), _T("CtrlDAlsoDupsBlock")       ,(funcItem[g_miCtrlDAlsoDupsBlock       ]._init2Check)? _T("1") : _T("0"),szINIPath);
  #ifndef HighPerformance
      WritePrivateProfileString("Settings","HighPerformance"          ,(funcItem[g_miHighPerformance          ]._init2Check)?"1":"0",szINIPath);
  #endif
      WritePrivateProfileString(_T("Viz")     , _T("VizCaseSensitive")         ,(funcItem[g_miVizCaseSensitive         ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizWholeWords")            ,(funcItem[g_miVizWholeWords            ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizRegex")                 ,(funcItem[g_miVizRegex                 ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizCutCopyAppend")         ,(funcItem[g_miVizCutCopyAppend         ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizClipboardAlwaysCRLF")   ,(funcItem[g_miVizClipboardAlwaysCRLF   ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizClipboardReplaceNulls") ,(funcItem[g_miVizClipboardReplaceNulls ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizClipboardCopyAlsoUTF8") ,(funcItem[g_miVizClipboardCopyAlsoUTF8 ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizClipboardNotUnicode")   ,(funcItem[g_miVizClipboardNotUnicode   ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizPasteRetainsPosition")  ,(funcItem[g_miVizPasteRetainsPosition  ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizPasteBinary")           ,(funcItem[g_miVizPasteBinary           ]._init2Check)? _T("1") : _T("0"),szINIPath);
      WritePrivateProfileString(_T("Viz")     , _T("VizPasteToEditorEOL")      ,(funcItem[g_miVizPasteToEditorEOL      ]._init2Check)? _T("1") : _T("0"),szINIPath);
      PopFindReplaceDlgINI(save,/*ppath,*/szINIPath);
    }
  }
  if (szINIPath)
	freesafe(szINIPath, _T("iniSaveSettings"));
}

// partial string case insensitive compare skipping & for matching menu items that may have an accelerator in them: strxicmpamp("&Paste && Remove","Paste & Remove")==0
// returns 0 if the same or at least equal to where s2 ends
// if & is found in s1, it is skipped, && is treated as &
// compare is case insenstive
EXTERNC int strxicmpamp(TCHAR *s1, TCHAR *s2) {
	int rv = 0;
	if (!*s2 || !*s1)
		return(-1);
	while (1) {
		if (*s1 == '&')
			s1++;
		if (!*s2)
			break;
		else if ((rv = (toupper(*s1) - toupper(*s2))))
			break;
		else {
			s1++;
			s2++;
		}
	}
	return(rv);
}

struct _MENUWALK {    // Set-by-caller values marked with (*) do not or may not under some circumstances persist between calls and must be reset each time
	char chCmd;         // set by caller 'f' for find, 'd' for display; 'w' to find largest wId
	BOOL fRecurse;      // set by caller: FALSE, only search the level specified in the function call; TRUE; additionally search all submenus
	TCHAR szMenuString[128];
	//unsigned cchMenuString; // set by caller to sizeof(szMenuString)
	TCHAR *szBuf;        // chCmd='d': set by caller to NULL or a malloc()'d buffer, may be NULL; chCmd='f': small malloc()'d buffer 'text to find'
	size_t cbBuf;     // set by caller to the malloc()'d size of the buffer, or the desired starting size of the buffer to be malloc()'d if szBuf is NULL.
	size_t cchBufLen; //*chCmd='d': does not need to be set if szBuf starts as NULL, if szBuf starts as a malloc()'d buffer, must be set to starting point to append new stuff; chCmd='f', not used
	HMENU hMenuFound;   //*chCmd='f': set by caller to NULL, will return HMENU when item is found, or NULL if nothing found
	UINT uItemFound;    // set to the item number during 'f' when item is found
	HMENU hMenuStack[8];
	UINT uItemStack[8];
	int nItemFoundLevel;
	//unsigned nLevel;    // not set by caller
	BOOL fQuit;         //*set by caller to FALSE, will be set to TRUE if recursion needs to be aborted
	MENUITEMINFO mii;   // not set by caller, used constantly during operation and will return MENUITEM details for chCmd='f' upon valid hMenuFound
	USHORT maxwID;      //*set by caller to 0, returns largest wID for non submenus on completion for chCmd='w'
};

#define cbMENUITEMINFO sizeof(MENUITEMINFO)
#ifdef _WIN64
#define expected_cbMENUITEMINFO 80
#else
#define expected_cbMENUITEMINFO 44
#endif

EXTERNC void menuwalkr(unsigned nLevel, struct _MENUWALK *mx) {
	int uItemMax = GetMenuItemCount(mx->hMenuStack[nLevel]);
	if (uItemMax > 0) {
#if NPPDEBUG
		if (mx->chCmd == 'd')
			sarmprintf(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("%-*s%2u:Menu:0x%X:%d\n"), (nLevel + 1) * 2, "", nLevel, mx->hMenuStack[nLevel], uItemMax);
#endif
		for (mx->uItemStack[nLevel] = 0; (int)(mx->uItemStack[nLevel]) < uItemMax; mx->uItemStack[nLevel]++) {
			mx->mii.fMask =/*MIIM_CHECKMARKS*/MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
			mx->mii.cbSize = cbMENUITEMINFO;
			mx->mii.dwTypeData = mx->szMenuString;
			mx->mii.cch = NELEM(mx->szMenuString); // cbSize must be set each time
			if (GetMenuItemInfo(mx->hMenuStack[nLevel], mx->uItemStack[nLevel], TRUE, &(mx->mii)))
				switch (mx->chCmd) {
#if NPPDEBUG
				case 'd':
					sarmprintf(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("%-*s%2u-dwTypeData:'%-12s'@cch:%u wID=%d dwItemData=0x%x Submenu=0x%x")/* fType=0x%X fState=0x%X"*/,
						(nLevel + 1) * 2, "", mx->uItemStack[nLevel], mx->mii.dwTypeData, mx->mii.cch, mx->mii.wID, mx->mii.dwItemData, mx->mii.hSubMenu/*,mx->mii.fType,mx->mii.fState*/);
					if (!mx->szBuf) {
						mx->fQuit = TRUE;
						break;
					}
					if (mx->mii.fType&MFT_BITMAP)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_BITMAP"), _T("menuwalk"));
					if (mx->mii.fType&MFT_MENUBARBREAK)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_MENUBARBREAK"), _T("menuwalk"));
					if (mx->mii.fType&MFT_MENUBREAK)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_MENUBREAK"), _T("menuwalk"));
					if (mx->mii.fType&MFT_OWNERDRAW)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_OWNERDRAW"), _T("menuwalk"));
					if (mx->mii.fType&MFT_RADIOCHECK)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_RADIOCHECK"), _T("menuwalk"));
					if (mx->mii.fType&MFT_RIGHTJUSTIFY)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_RIGHTJUSTIFY"), _T("menuwalk"));
					if (mx->mii.fType&MFT_SEPARATOR)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFT_SEPARATOR"), _T("menuwalk"));
					//if (mx->mii.fType&MFT_STRING       ) strcpyarmsafe(&mx->szBuf,&mx->cbBuf,&mx->cchBufLen,"|MFT_STRING"      ,"menuwalk");
					if (mx->mii.fState&MFS_CHECKED)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFS_CHECKED"), _T("menuwalk"));
					if (mx->mii.fState&MFS_DEFAULT)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFS_DEFAULT"), _T("menuwalk"));
					if (mx->mii.fState&MFS_DISABLED)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFS_DISABLED"), _T("menuwalk"));
					//if (mx->mii.fState&MFS_ENABLED     ) strcpyarmsafe(&mx->szBuf,&mx->cbBuf,&mx->cchBufLen,"|MFS_ENABLED"     ,"menuwalk");
					if (mx->mii.fState&MFS_GRAYED)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFS_GRAYED"), _T("menuwalk"));
					if (mx->mii.fState&MFS_HILITE)
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("|MFS_HILITE"), _T("menuwalk"));
					//if (mx->mii.fState&MFS_UNCHECKED   ) strcpyarmsafe(&mx->szBuf,&mx->cbBuf,&mx->cchBufLen,"|MFS_UNCHECKED"   ,"menuwalk");
					//if (mx->mii.fState&MFS_UNHILITE    ) strcpyarmsafe(&mx->szBuf,&mx->cbBuf,&mx->cchBufLen,"|MFS_UNHILITE"    ,"menuwalk");
					if (!mx->szBuf)
						mx->fQuit = TRUE;
					else {
						strcpyarmsafe(&mx->szBuf, &mx->cbBuf, &mx->cchBufLen, _T("\n"), _T("menuwalk"));
						if (!mx->szBuf)
							mx->fQuit = TRUE;
					}
					break;
#endif
				case 'f':
					if (!mx->szMenuString)
						mx->fQuit = TRUE;
					else if (!strxicmpamp(mx->szMenuString, mx->szBuf)) {
						mx->hMenuFound = mx->hMenuStack[nLevel];
						mx->uItemFound = mx->uItemStack[nLevel];
						mx->nItemFoundLevel = nLevel;
						mx->fQuit = TRUE;
					}
					break;
				case 'w':
					if (!(mx->mii.hSubMenu) && mx->maxwID < LOWORD(mx->mii.wID))
						mx->maxwID = LOWORD(mx->mii.wID);
					break;
				}
			if (mx->fQuit)
				break;
			if (mx->fRecurse && mx->mii.hSubMenu && nLevel < NELEM(mx->hMenuStack) - 1) {
				mx->hMenuStack[nLevel + 1] = mx->mii.hSubMenu;
				menuwalkr(nLevel + 1, mx);
				if (mx->fQuit)
					break;
			}
		}
	}
}

// returns TRUE on success
extern "C" BOOL menuwalk(char chCmd, BOOL fRecurse, HMENU hmn, struct _MENUWALK *mx) {
	mx->chCmd = chCmd;
	mx->fRecurse = fRecurse;
	mx->fQuit = FALSE;
	mx->nItemFoundLevel = -1;
	mx->hMenuFound = NULL;
	mx->uItemFound = 0;
	ZeroMemory(mx->hMenuStack, sizeof(mx->hMenuStack));
	ZeroMemory(mx->uItemStack, sizeof(mx->uItemStack));
	ZeroMemory(&mx->mii, sizeof(mx->mii));
	mx->maxwID = 0;
	mx->hMenuStack[0] = hmn;
	menuwalkr(0, mx);
	return mx->nItemFoundLevel != -1;
}

#if NPPDEBUG
EXTERNC PFUNCPLUGINCMD pfshowmenuinfo(void) {
	struct _MENUWALK mx;
	mx.szBuf = NULL; mx.cbBuf = 512;
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	menuwalk('d', TRUE, GetMenu(g_nppData._nppHandle), &mx);
	if (mx.szBuf) {
		SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, mx.szBuf);
		freesafe(mx.szBuf, _T("pfshowmenuinfo"));
	}
}
#endif

// Define menu structrue:
// Main menu items in TextFX menu
static struct _MENUMOVE {
	int location;		// 0=item goes into the plugin menu; 1=a new menu under hMenuOurPlugin; 2=a new menu on the menu bar
	char prefix;		// character: of menu to detect
	TCHAR menuname[22]; // applies to 1&2
	HMENU hmn;
} mainMenuMove[] = {
	{2,'Q',_T(PLUGIN_NAME) " Quick",NULL},
	{1,'E',_T(PLUGIN_NAME) " Edit",NULL},
	{1,'C',_T(PLUGIN_NAME) " Convert",NULL},
	{1,'I',_T(PLUGIN_NAME) " Insert",NULL},
	//{1,'H',PLUGIN_NAME " Online Help",NULL},
	{1,'D',_T(PLUGIN_NAME) " HTML Tidy",NULL},
	{1,'T',_T(PLUGIN_NAME) " Tools",NULL},
	{2,'V',_T(PLUGIN_NAME) " Viz",NULL},
	{1,'W',_T(PLUGIN_NAME) " Viz Settings",NULL},
	{1,'S',_T(PLUGIN_NAME) " Settings",NULL},
};


#if NPPDEBUG
HMENU g_mFormatSubmenu;
int ANSImenubaritem, UTF8menubaritem, UCS2BEmenubaritem, UCS2LEmenubaritem, UTF8NBmenubaritem;
int MACmenubaritem, UNIXmenubaritem, PCmenubaritem;

EXTERNC BOOL IsMenuItemChecked(HMENU hMenu, UINT uItem, BOOL fByPosition)
{
	MENUITEMINFO mi;

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = cbMENUITEMINFO;
	mi.fMask = MIIM_STATE;
	GetMenuItemInfo(hMenu, uItem, fByPosition, &mi);
	return (mi.fState&MFS_CHECKED) ? TRUE : FALSE;
}
#endif

// Rearrange the plugin menu and apply separator mods
// new feature: ! makes menu item disappear
EXTERNC PFUNCPLUGINCMD pfbuildmenu(void)
{
	struct _MENUWALK mx;
	mx.szBuf = NULL;
	mx.cbBuf = 0;
	if (cbMENUITEMINFO != expected_cbMENUITEMINFO)
		MessageBoxFree(0, smprintf(_T("This compiler has the wrong size for MENUITEMINFO: %zu, %d required!"), cbMENUITEMINFO, expected_cbMENUITEMINFO), _T("Fatal Error"), MB_OK);
#if NPPDEBUG
	if (!powcheck(sizeof(struct _MENUMOVE)))
		MessageBoxFree(g_nppData._nppHandle, smprintf(_T("sizeof(struct _MENUMOVE)==%zu is not a power of two"), sizeof(struct _MENUMOVE)), _T(PLUGIN_NAME), MB_OK | MB_ICONWARNING);
	if (!powcheck(sizeof(struct _wIDLOOKUP)))
		MessageBoxFree(g_nppData._nppHandle, smprintf(_T("sizeof(struct _wIDLOOKUP)==%zu is not a power of two"), sizeof(struct _wIDLOOKUP)), _T(PLUGIN_NAME), MB_OK | MB_ICONWARNING);
#endif
	unsigned wln, elems = NELEM(g_wIDlookup);
	for (wln = 0; wln < elems; wln++)
		g_wIDlookup[wln].wParamLo = (USHORT)funcItem[findmenuitem(g_wIDlookup[wln]._pFunc)]._cmdID;
	INT_CURRENTEDIT;
	GET_CURRENTEDIT;
	do {
		if (!funcItem[g_miSeparateQuickMenus]._init2Check) {
			unsigned i;
			for (i = 0; i < NELEM(mainMenuMove); i++)
				if (mainMenuMove[i].location > 1)
					mainMenuMove[i].location = 1;
		}

#if NPPDEBUG && !TEXTFX_TOP_MENU
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Format"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, GetMenu(g_nppData._nppHandle), &mx))
			break;
		g_mFormatSubmenu = mx.mii.hSubMenu;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Encode in ANSI"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		ANSImenubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Encode in UTF-8"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		UTF8menubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Encode in UCS-2 Big Endian"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		UCS2BEmenubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Encode in UCS-2 Little Endian"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		UCS2LEmenubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("UTF-8 without BOM"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		UTF8NBmenubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Convert to Windows Format"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		PCmenubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Convert to UNIX Format"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		UNIXmenubaritem = mx.uItemFound;
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("Convert to MAC Format"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', FALSE, g_mFormatSubmenu, &mx))
			break;
		MACmenubaritem = mx.uItemFound;
		//MessageBoxFree(g_nppData._nppHandle,smprintf("%u %u %u %u %u",IsMenuItemChecked(g_mFormatSubmenu,ANSImenubaritem,TRUE),IsMenuItemChecked(g_mFormatSubmenu,UTF8menubaritem,TRUE),IsMenuItemChecked(g_mFormatSubmenu,UCS2BEmenubaritem,TRUE),IsMenuItemChecked(g_mFormatSubmenu,UCS2LEmenubaritem,TRUE),IsMenuItemChecked(g_mFormatSubmenu,UTF8NBmenubaritem,TRUE)),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
#endif

		// Now there is a flat Plugins->TextFX -menu.
		// Typically we want to fold that into own TextFX menu outside plugins having multiple submenus.
		// See PluginsManager::addInMenuFromPMIndex() on Notepad++ code for menu building.
		HMENU hMenuContainingPluginMenu = GetMenu(g_nppData._nppHandle); // language independant search; this is the menu that contains the "Plugin" menu
		strcpyarmsafe(&mx.szBuf, &mx.cbBuf, NULL, _T("!:TEXTFX NULL FUNCTION"), _T("pfbuildmenu"));
		if (!mx.szBuf || !menuwalk('f', TRUE, hMenuContainingPluginMenu, &mx) || mx.nItemFoundLevel != 2)
			break;
		int uItemContainingPluginMenu = mx.uItemStack[0];
		HMENU hMenuContainingOurPlugin = mx.hMenuStack[1];
		int uItemContainingOurPlugin = mx.uItemStack[1];
		HMENU hMenuOurPlugin = mx.hMenuStack[2];
#if TEXTFX_TOP_MENU // set to 1 for TextFX to be separate, 0 for it to be in Plugins
		HMENU hMenuNewPluginMenu = CreateMenu();
		int uItemNewPluginMenu = 0;
#define MENUEXTRA (0)
#else
#define hMenuNewPluginMenu (hMenuContainingOurPlugin)
#define uItemNewPluginMenu (uItemContainingOurPlugin)
#define MENUEXTRA (1)
#endif
		menuwalk('w', TRUE, hMenuContainingOurPlugin, &mx);
		g_usNextwID = g_usMinwID = mx.maxwID + 1; // This is not a safe way to calculate minimax with other plugins doing the same thing
		int uItemContainingPluginMenuOffset = 0;
		unsigned i;
		int uItem = 0;
		while (mx.mii.fMask =/*MIIM_CHECKMARKS|*/MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE,
			mx.mii.cbSize = cbMENUITEMINFO, mx.mii.dwTypeData = mx.szMenuString, mx.mii.cch = NELEM(mx.szMenuString), // cbSize must be set each time
			GetMenuItemInfo(hMenuOurPlugin, uItem, TRUE, &(mx.mii))) {
			if (mx.szMenuString[0] == '!') {
				if (DeleteMenu(hMenuOurPlugin, uItem, MF_BYPOSITION))
					--uItem;
			} else
				if (mx.szMenuString[0] && mx.szMenuString[1] == ':') {
					for (i = 0; i < NELEM(mainMenuMove); ++i) {
						// Match top-level menu with
						if (mainMenuMove[i].prefix == mx.mii.dwTypeData[0]) {
							if (DeleteMenu(hMenuOurPlugin, uItem, MF_BYPOSITION)) {
								--uItem;
								memmovetest(mx.mii.dwTypeData, mx.mii.dwTypeData + 2, mx.mii.cch - 2 + 1);
								if (mx.mii.dwTypeData[0] == '-')
									mx.mii.fType = MFT_SEPARATOR;
								if (!mainMenuMove[i].location) {
									if (InsertMenuItem(hMenuNewPluginMenu, uItemNewPluginMenu, TRUE, &mx.mii))
										++uItemNewPluginMenu;
								}
								else {
									if (!mainMenuMove[i].hmn)
										mainMenuMove[i].hmn = CreateMenu();
									InsertMenuItem(mainMenuMove[i].hmn, GetMenuItemCount(mainMenuMove[i].hmn), TRUE, &mx.mii);
									for (wln = 0; wln < NELEM(g_wIDlookup); wln++)
										if (mx.mii.wID == g_wIDlookup[wln].wParamLo)
											g_wIDlookup[wln].hSubMenu = mainMenuMove[i].hmn; // MultiPlex Menu
								}
							}
							break;
						}
					}
				}
			++uItem;
		} // end while
		ZeroMemory(&mx.mii, sizeof(mx.mii));
		mx.mii.fMask =/*MIIM_CHECKMARKS|MIIM_DATA|MIIM_ID|MIIM_STATE|*/MIIM_SUBMENU | MIIM_TYPE;
		mx.mii.cbSize = cbMENUITEMINFO;
		if (hMenuNewPluginMenu != hMenuContainingOurPlugin) {
			mx.mii.dwTypeData = mx.szMenuString; mx.mii.cch = NELEM(mx.szMenuString);
			if (GetMenuItemInfo(hMenuContainingOurPlugin, uItemContainingOurPlugin, TRUE, &(mx.mii)) &&
				RemoveMenu(hMenuContainingOurPlugin, uItemContainingOurPlugin, MF_BYPOSITION) &&
				InsertMenuItem(hMenuNewPluginMenu, uItemNewPluginMenu, TRUE, &mx.mii))
				++uItemNewPluginMenu;
		}
		for (i = 0; i < NELEM(mainMenuMove); ++i)
			if (mainMenuMove[i].hmn) {
				mx.mii.dwTypeData = mainMenuMove[i].menuname;
				mx.mii.hSubMenu = mainMenuMove[i].hmn;
				if (mainMenuMove[i].location == 1) {
					if (InsertMenuItem(hMenuNewPluginMenu, uItemNewPluginMenu + MENUEXTRA, TRUE, &mx.mii))
						++uItemNewPluginMenu;
				}
				else {
					if (InsertMenuItem(hMenuContainingPluginMenu, uItemContainingPluginMenu + uItemContainingPluginMenuOffset, TRUE, &mx.mii))
						++uItemContainingPluginMenuOffset;
				}
			}
		if (hMenuNewPluginMenu != hMenuContainingOurPlugin) {
			mx.mii.dwTypeData = _T(PLUGIN_NAME);
			mx.mii.hSubMenu = hMenuNewPluginMenu;
			InsertMenuItem(hMenuContainingPluginMenu, uItemContainingPluginMenu, TRUE, &mx.mii);
		}
	} while (0);

	for (wln = 0; wln < NELEM(g_wIDlookup); wln++)
		g_wIDlookup[wln].wParamLo = 0; // MultiPlex Menu
	if (mx.szBuf)
		freesafe(mx.szBuf, _T("pfbuildmenu"));
}

// As we write this and other programs, we think of needed features and list them here.
// Features that are pretty easy:
//  new feature: text to decimal, decimal to text
//  new feature: never hide the last line of the document
//  new feature: sort: output only non duplicated lines (exclusive)
//  new feature: sort compare ends at first whitespace: http://www.ultraedit.com/index.php?name=Forums&file=viewtopic&t=1106
//  new feature: insert line number of width #
//  new feature: Autoblock editing before this column: http://www.ultraedit.com/index.php?name=Forums&file=viewtopic&t=1282
//  new feature: BIGG find and replace; options, ignore trivial crlf differences, starts..ends with
//  new feature: HTML tags to lowercase http://www.crimsoneditor.com/english/board/CrazyWWWBoard.cgi?db=forum&mode=read&num=5226&page=43&ftype=6&fval=&backdepth=1
//  new feature: viz search between columns
//  new feature: sort: remove duplicates, preserve order
//  new feature: multiple copy buffers
//  new feature: copy line
//  new feature: insert line length, sort by line length. http://www.textpad.info/forum/viewtopic.php?t=6687
//  new feature: insert random number: sort by random number
//  new feature: CSV sorting: http://www.textpad.info/forum/viewtopic.php?t=6040
//  new feature: offer stable sort http://en.wikipedia.org/wiki/Sort_algorithm#Stable (can be done if unique length is different than sort length) http://www.auto.tuwien.ac.at/~blieb/woop/merge.html
//                http://linux.wku.edu/~lamonml/algor/sort/merge.html
//  new feature: delete brace pair
//  new feature: read shortcut keys from file
//  new feature: Keyboard Shortcut for last operation
//  new feature: option: all transforms use entire file when no selection
//  new feature: http://en.wikipedia.org/wiki/Windows-1250 (super codepage converter)

// Features that are harder but still possible
//  new feature: analyze C code function usage summary
//  new feature: save recent copy blocks, present them as a screen, click on each block in order that you want them pasted
//  new feature: Search&Replace Column http://www.crimsoneditor.com/english/board/CrazyWWWBoard.cgi?db=forum&mode=read&num=5438&page=34&ftype=6&fval=&backdepth=1
//  new feature: NiceSQL, tidy SQL http://home.broadpark.no/~ihalvor/
//  new feature: http://cscope.sourceforge.net/ http://ctags.sourceforge.net/ http://forum.pspad.com/read.php?f=2&i=2664&t=2664
//  new feature: xmlformat http://forum.pspad.com/read.php?f=2&i=1449&t=1449
//  new feature: chars to html entity http://forum.pspad.com/read.php?f=2&i=2200&t=2200 (watch for license)
//  new feature: show duplicate lines (Viz)
//  new feature: PSPAD: Reformat HTML; is this separate from HTML Tidy?
//  new feature: Copy lines with line numbers: http://www.ultraedit.com/index.php?name=Forums&file=viewtopic&t=1745
//  new feature: Swap-Cut-Paste http://www.ornj.net/forums/viewtopic.php?t=576
//  new feature: close tag and text: http://developers.evrsoft.com/forum/showthread.php?t=523
//  new feature: definition finder http://sourceforge.net/tracker/index.php?func=detail&aid=988438&group_id=58130&atid=486612
//  new feature: find unmatched brace
//  new feature: launch HTML
//  new feature: prefab HTML templates
//  new feature: View in browser
//  new feature: codeblocks plugin
//  new feature: insert/remove ULAII (Universal Language Auto Include Insert) strings
//  new feature: find text after comma #, used to search through comma delimited text
//  new feature: sort lines at comma number (think delimited text)
//  new feature: paste-overwrite lines for multi line text,paste-overwrite characters for single line text
//  new feature: bugfix: MarkWord Find Forward-reverse should skip hidden text
// Features that won't work properly until N++ allows us to retrieve editor settings
//  new feature: auto find bookmarks preferrably on file load
//  New Feature: Prefab search & replace strings
// Features that are easy enough but won't be useful until plugins can add custom keyboard accelerators
// Features that are too hard for me
//  new feature: column editing http://www.crimsoneditor.com/english/board/CrazyWWWBoard.cgi?db=forum&mode=read&num=2900&page=141&ftype=6&fval=&backdepth=2
//  new feature: Spellcheck, only comments area http://www.crimsoneditor.com/english/board/CrazyWWWBoard.cgi?db=forum&mode=read&num=3858&page=104&ftype=6&fval=&backdepth=1
//  new feature: Detect SDF, write conversion rule line, perform convertion from SDF to comma or tab DELI
//  new feature: BIGG-Edit Live MagnaDiff
//  new feature: find code block that is __% similar to ___
//  new feature: Goto column
//  new feature: Copy as HTML
//  new feature: #include files should be made into hotlinks
//  new feature: label or relabel with comment end of #if,#ifdef,switch,if,while,do { ... } /* autolabel: if:else (... */
//  new feature: vb to jscript converter
//  new feature: diff: compare clipboard to highlighted text, possibly hightligh, WinDiff, KDiff3
//  new feature: FC.EXE/diff/WinDiff Compare two open files or two selected text blocks in two files, write result into 3rd file

// Features that N++ will need to implement
/*Scintilla is pretty good about providing all it's internals but I need a few more things from N++ to get plugins to be more functional.
* The ability to retrieve or set styles translated by Notepad++, the ability to identify regions
* I need to retrieve the N++ version numbers. At this time N++ is actively developed and later versions are universally more desirable than earlier versions. Should this not be the case in the future, I would like plugins to add or not add functionality based on version number.
* I need another DLLEXPORT callback. It would be called when the menus and screens are created so menuitems, hotkeys, and other stuff can be altered. A RunOnce in the Notify callback seems to work but it should be standardised.
* N++ needs a find next/previous bar just like Firefox.
* setinfo() needs to accept a sizeof(struct) parameter so the struct can evolve and old and new plugins will be supported
* N++ needs a way for plugins to generate and regenerate dynamic menus; wParamLo should be sent to PFUNCPLUGINS
* Clone View Multi Monitor
* Jump to column number
* Print Dialog: Print Line Numbers, ^L pagenates http://www.crimsoneditor.com/english/board/CrazyWWWBoard.cgi?db=forum&mode=read&num=645&page=242&ftype=6&fval=&backdepth=1
* Scroll Lock locks clone views together
* Search and replace static filters for syntax highlighting
* Scintilla: I need access to the lexer styles so I can know if text is a comment, a string, code, brace, or whatever
* Scintilla find options need to be [x] do not search comments, [x] do not search quoted strings (language dependent)
*/

#if NPPDEBUG
// SUBSECTION: Load/Save Functions

/* This should run a bajillion times faster than the buggy C++ code in SciTE or Notepad++ */
/* fFileFormat can be 1=ANSI,SC_CP_UTF8,2=UCS2BE,3-UCS2LE
  FILE * tools like fwrite() should never be used on any platform but Unix. They are slow and not binary safe! */
EXTERNC void SaveEditorToFile(const TCHAR *path, INT_CURRENTEDIT, unsigned fFileFormat, BOOL fWriteBOM) {
	TCHAR sBufA[16384]; /* sBufA and sBufW do not necessairly hold ASCII or WIDE text */
	HANDLE hFo;
	unsigned cchDoc = SENDMSGTOCED(currentEdit, SCI_GETLENGTH, 0, 0);
	struct TextRange tr;
	tr.lpstrText = sBufA;
	tr.chrg.cpMin = 0;
	if (INVALID_HANDLE_VALUE != (hFo = _creatX(path, 0))) {
		if (fWriteBOM) switch (fFileFormat) { // ANSI never gets a BOM
		case 2:
			_writeX(hFo, "\376\377", 2);
			break;
		case 3:
			_writeX(hFo, "\377\376", 2);
			break;
		case SC_CP_UTF8:
			_writeX(hFo, "\357\273\277", 3);
			break;
		}
		while (cchDoc) {
			TCHAR sBufW[sizeof(sBufA)];
			unsigned cchBytesToWriteW, cchUnused;
			tr.chrg.cpMax = tr.chrg.cpMin + (cchDoc >= sizeof(sBufA) ? sizeof(sBufA) - 1 : cchDoc);
			unsigned cchBytesToWriteA = SENDMSGTOCED(currentEdit, SCI_GETTEXTRANGE, 0, &tr); //MessageBoxFree(g_nppData._nppHandle,smprintf("DocSize:%u Writing %u BytesA from %u-%u",cchDoc,cchBytesToWriteA,tr.chrg.cpMin,tr.chrg.cpMax),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
			switch (fFileFormat) {
			case 1:
				_writeX(hFo, tr.lpstrText, cchBytesToWriteA);
				break;
			case SC_CP_UTF8:
				cchBytesToWriteW = UTF8Validated(tr.lpstrText, cchBytesToWriteA, sBufW, sizeof(sBufW), &cchUnused);
				goto wrtUCS;
			case 2:
			case 3:
// XXX Disabled by JaTu:
//				cchBytesToWriteW=UCS2FromUTF8(tr.lpstrText,cchBytesToWriteA,sBufW,sizeof(sBufW),fFileFormat==2,&cchUnused)*sizeof(sBufW[0]);
wrtUCS:
				_writeX(hFo, (char *)sBufW, cchBytesToWriteW); //if (cchUnused) MessageBoxFree(g_nppData._nppHandle,smprintf("Rereading %u BytesA",cchUnused),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
				if (cchBytesToWriteA == sizeof(sBufA) - 1)
					cchBytesToWriteA -= cchUnused; // no endless loops ending with invalid partial UTF-8 characters
				break;
			}
			cchDoc -= cchBytesToWriteA;
			tr.chrg.cpMin += cchBytesToWriteA;
		}
		_closeX(hFo);
	}
}

EXTERNC PFUNCPLUGINCMD pfSave(void) {
	TCHAR path[MAX_PATH];
	SendMessage(g_nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)path); /* Path + Filename */
	if (!wcschr(path, ':')) {
		MessageBox(g_nppData._nppHandle, path, _T("No file name, can't save!"), MB_OK);
	} else {
		INT_CURRENTEDIT;
		GET_CURRENTEDIT;
		unsigned fFileFormat = 1;
		if (IsMenuItemChecked(g_mFormatSubmenu, ANSImenubaritem, TRUE))
			fFileFormat = 1;
		else if (IsMenuItemChecked(g_mFormatSubmenu, UTF8menubaritem, TRUE))
			fFileFormat = SC_CP_UTF8;
		else if (IsMenuItemChecked(g_mFormatSubmenu, UCS2BEmenubaritem, TRUE))
			fFileFormat = 2;
		else if (IsMenuItemChecked(g_mFormatSubmenu, UCS2LEmenubaritem, TRUE))
			fFileFormat = 3;
		SaveEditorToFile(path, currentEdit, fFileFormat,/*(IsMenuItemChecked(g_mFormatSubmenu,UTF8NBmenubaritem,*/TRUE/*)*/);
		SENDMSGTOCED(currentEdit, SCI_SETSAVEPOINT, 0, 0);
	}
}

// returns 0 if unable to detect EOL or 1+ScintillaEOL if detected
EXTERNC unsigned DetectEOL(TCHAR *buf, unsigned len) {
	TCHAR *cr, *lf;

	if (!len)
		return 0;
	cr = (TCHAR *)memchr(buf, '\r', len);
	lf = (TCHAR *)memchr(buf, '\n', len);
	//MessageBoxFree(g_nppData._nppHandle,smprintf("cr:%p lf:%p",cr,lf),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	if (!cr && !lf)
		return 0;
	if (cr && cr == buf + len - 1)
		return(0);
	if (cr && !lf) {
		SendMessage(g_nppData._nppHandle, WM_COMMAND, MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu, MACmenubaritem), 0), (LPARAM)g_mFormatSubmenu);
		return SC_EOL_CR + 1;
	}
	if (!cr && lf) {
		SendMessage(g_nppData._nppHandle, WM_COMMAND, MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu, UNIXmenubaritem), 0), (LPARAM)g_mFormatSubmenu);
		return SC_EOL_LF + 1;
	}
	//SendMessage(g_nppData._nppHandle,WM_COMMAND,MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu,PCmenubaritem),0),(LPARAM)g_mFormatSubmenu);

	return SC_EOL_CRLF + 1;
}

/*  FILE * tools like fread() should never be used on any platform but Unix. They are slow and not binary safe! */
// if a fnAskUser is supplied, that function can ask the user which encoding they want. The dialog should look like this:
/*  Please select the desired encoding for this file (preselect with * the provided best guess)
   ( ) ANSI                 [Each time the user selects one]
   (*) UTF-8                [of the radio buttons, sample  ]
   ( ) UCS-2 Big Endian     [text is decoded in that form  ]
   ( ) UCS-2 Little Endian  [in this textbox for review.   ]
   [OK] [CANCEL]
  returns the unicode mode or -1 if the user clicks cancel
*/
EXTERNC int LoadEditorFromFile(const TCHAR *path,INT_CURRENTEDIT,unsigned *uEOLmode,int iAutoDetect,BOOL fAlwaysAsk,int (__cdecl *fnAskUser)(char *,unsigned,unsigned *,int)) {
  int fUnicode=0;
  TCHAR sBufW[8192]; /* This must be an even number, only the first sizeof(sBufW) bytes are tested for UNICODE or UTF8 */
  HANDLE hFi;
  //unsigned cchDoc=SENDMSGTOCED(currentEdit,SCI_GETLENGTH,0,0);
  struct TextRange tr;
  tr.lpstrText=sBufW;
  tr.chrg.cpMin=0;
  if (uEOLmode) *uEOLmode=0;
  if (INVALID_HANDLE_VALUE!=(hFi=_openX(path,O_RDONLY,O_DENYWRITE))) {
    unsigned uEOLmodeX=0;
    unsigned cchBurn=0,cchBytesRead; while((cchBytesRead=_readX(hFi,sBufW,sizeof(sBufW)))) {
      if (!fUnicode && cchBytesRead>=3) {
        BOOL fUncertain=fAlwaysAsk;
        if (     !memcmp(sBufW,"\376\377"    ,2)) {cchBurn=2; fUnicode=2; }
        else if (!memcmp(sBufW,"\377\376"    ,2)) {cchBurn=2; fUnicode=3; }
        else if (!memcmp(sBufW,"\357\273\277",3)) {cchBurn=3; goto testUTF8;}
        else {
          fUncertain=TRUE;
testUTF8: if (isUTF8_16(sBufW+cchBurn,cchBytesRead-cchBurn,NULL)>1) {
            fUnicode=SC_CP_UTF8;
          } else if (iAutoDetect) {
            IsTextUnicode(sBufW+cchBurn,cchBytesRead-cchBurn,&iAutoDetect); //MessageBoxFree(g_nppData._nppHandle,smprintf("Autodetect:0x%p",pi),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
            if (iAutoDetect&(IS_TEXT_UNICODE_REVERSE_MASK)) fUnicode=2;
            else if (iAutoDetect&(IS_TEXT_UNICODE_UNICODE_MASK)) fUnicode=3;
            else fUnicode=1;
          } else fUnicode=1;
        }
        if (fUncertain && fnAskUser) {
          unsigned cchBurnX=cchBurn; //* cchBurn is too critial to force on the stack */
          if ((fUnicode=fnAskUser((char *)sBufW,cchBytesRead,&cchBurnX,fUnicode))==-1) break;
          cchBurn=cchBurnX;
        }
      }
      if (fUnicode>=2 && fUnicode<=3) {
        TCHAR sBufA[sizeof(sBufW)/2*3]; // each 2 byte UNICODE characters can produce at most 3 UTF-8 characters
        //unsigned cchToEditorx=UTF8FromUCS2((wchar_t *)(sBufW+cchBurn),(cchBytesRead-cchBurn)/2,0,0,fUnicode==2);
        unsigned cchToEditor=UTF8FromUCS2((TCHAR *)(sBufW+cchBurn), (cchBytesRead-cchBurn)/2, (char*)sBufA, sizeof(sBufA), fUnicode==2);
        //if (cchToEditorx!=cchToEditor) MessageBoxFree(g_nppData._nppHandle,smprintf("Unicode Mode:%u Burn:%u cchBytesRead:%u cchToEditor:%u",fUnicode,cchBurn,cchBytesRead,cchToEditor),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
        SENDMSGTOCED(currentEdit,SCI_ADDTEXT,cchToEditor,sBufA);
        if (!uEOLmodeX)
			uEOLmodeX=DetectEOL(sBufA,cchToEditor);
      } else {
        SENDMSGTOCED(currentEdit,SCI_ADDTEXT,cchBytesRead-cchBurn,sBufW+cchBurn);
        if (!uEOLmodeX)
			uEOLmodeX=DetectEOL(sBufW+cchBurn,cchBytesRead-cchBurn);
      }
      cchBurn=0;
    }
    _closeX(hFi);
    if (uEOLmode) *uEOLmode=uEOLmodeX;
  }
  return fUnicode;
}

EXTERNC PFUNCPLUGINCMD pfReLoad(void) {
	TCHAR path[MAX_PATH];
	SendMessage(g_nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)path); /* Path + Filename */
	if (!wcschr(path, ':')) {
		MessageBox(g_nppData._nppHandle, path, _T("No file name, can't reload!"), MB_OK);
	} else {
		INT_CURRENTEDIT;
		GET_CURRENTEDIT;
		SENDMSGTOCED(currentEdit, SCI_CLEARALL, 0, 0);
		ShowWindow((currentEdit) ? g_nppData._scintillaSecondHandle : g_nppData._scintillaMainHandle, SW_HIDE); // this blocks all WM_PAINT for the entire LOAD
		SENDMSGTOCED(currentEdit, SCI_SETUNDOCOLLECTION, FALSE, 0);
		SENDMSGTOCED(currentEdit, SCI_EMPTYUNDOBUFFER, 0, 0);

		switch (LoadEditorFromFile(path, currentEdit, NULL, IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK, FALSE, NULL)) {
		case 1: SendMessage(g_nppData._nppHandle, WM_COMMAND, MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu, ANSImenubaritem), 0), (LPARAM)g_mFormatSubmenu); break;
		case 2: SendMessage(g_nppData._nppHandle, WM_COMMAND, MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu, UCS2BEmenubaritem), 0), (LPARAM)g_mFormatSubmenu); break;
		case 3: SendMessage(g_nppData._nppHandle, WM_COMMAND, MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu, UCS2LEmenubaritem), 0), (LPARAM)g_mFormatSubmenu); break;
		case SC_CP_UTF8: SendMessage(g_nppData._nppHandle, WM_COMMAND, MAKEWPARAM(GetMenuItemID(g_mFormatSubmenu, UTF8menubaritem), 0), (LPARAM)g_mFormatSubmenu); break;
		}

		SENDMSGTOCED(currentEdit, SCI_GOTOPOS, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_SETSAVEPOINT, 0, 0);
		SENDMSGTOCED(currentEdit, SCI_SETUNDOCOLLECTION, TRUE, 0);
		ShowWindow((currentEdit) ? g_nppData._scintillaSecondHandle : g_nppData._scintillaMainHandle, SW_SHOW);
		SetFocus((currentEdit) ? g_nppData._scintillaSecondHandle : g_nppData._scintillaMainHandle);
	}
}

// These functions make some options in the editor
// [ ] Save without BOM UTF-8
// [ ] Save without BOM UCS2
// [x] Autodetect format without BOM on Load
// [x] Autodetect format without BOM on Drag-n-drop
// [ ] Never Autodetect Big Endian
// [ ] Never Autodetect Little Endian
// Reload With Manual Detection... (brings up dialog box)

// SUBSECTION: END

EXTERNC PFUNCPLUGINCMD pftemp(void) {
  //XMLTest("C:\\test.xml","C:\\test3.xml");
  //struct MICROXML mx;
  //if (XMLOpen(&mx,"c:\\Program Files\\Notepad++\\config.xml",O_RDONLY,O_DENYWRITE)) {
     //XMLClose(&mx);
  //}
  INT_CURRENTEDIT;
  GET_CURRENTEDIT;
  //int codepage=SENDMSGTOCED(currentEdit, SCI_GETCODEPAGE, 0, 0); MessageBoxFree(g_nppData._nppHandle,smprintf("CP:%d",codepage),PLUGIN_NAME, MB_OK|MB_ICONWARNING);
  //SENDMSGTOCED(currentEdit, SCI_SETCODEPAGE, 437, 0);
}
#endif

// Hotkey definitions:
//                                        CTRL ALT SHIFT
struct ShortcutKey skMarkWordFindReverse = { TRUE,TRUE,FALSE,VK_LEFT };
struct ShortcutKey skMarkWordFindForward = { TRUE,TRUE,FALSE,VK_RIGHT };
struct ShortcutKey skfindmatchchar = { TRUE,FALSE,FALSE,'B' };
struct ShortcutKey skmarkmatchchar = { TRUE,FALSE,TRUE,'B' };
struct ShortcutKey skdeletebracepair = { TRUE,TRUE,FALSE,'B' };
struct ShortcutKey skmarkmatchline = { TRUE,TRUE,TRUE,'B' };
struct ShortcutKey skcopyallnoappend = { TRUE,FALSE,FALSE,'C' };
struct ShortcutKey skcopyallappend = { TRUE,FALSE,TRUE,'C' };
struct ShortcutKey skcutallnoappend = { TRUE,FALSE,FALSE,'X' };
struct ShortcutKey skcutallappend = { TRUE,FALSE,TRUE,'X' };
struct ShortcutKey skVizPasteUNICODE = { TRUE,FALSE,FALSE,'V' };
struct ShortcutKey skindentlines = { FALSE,FALSE,FALSE,VK_TAB };
#if ENABLE_FINDREPLACE
struct ShortcutKey skfindreplace = { TRUE,FALSE,FALSE,'R' };
#endif
struct ShortcutKey skinsertShortDateTime = { TRUE,FALSE,FALSE,VK_F5 };
//#endif
struct ShortcutKey skDuplicateLineOrBlock = { TRUE,FALSE,FALSE,'D' };


// This is the list of menu items and the functions doing the actual work.
// This does NOT describe the menu tree structure!
struct FuncItem funcItem[] = {
	{NPPTEXT("!:TEXTFX NULL FUNCTION"),pfdummy,0,FALSE, NULL},
	#if NPPDEBUG /* { it wasn't automatic at first */
	{NPPTEXT("Q:(Experimental) Temp Function"),pftemp,0,FALSE, NULL},
	{NPPTEXT("Q:(Experimental) Display Clipboard Info"),pfinsertclipinfo,0,FALSE, NULL},
	//{NPPTEXT("Q:Convert EOLs")),pfconverteol,0,FALSE, NULL},
	{NPPTEXT("Q:(Experimental) Insert Menu Info"),pfshowmenuinfo,0,FALSE, NULL},
	{NPPTEXT("Q:(Experimental) ReSave"),pfSave,0,FALSE, NULL},
	{NPPTEXT("Q:(Experimental) ReLoad"),pfReLoad,0,FALSE, NULL},
	{NPPTEXT("Q:(Experimental) Insert Unicode Caps Tables"),pfInsertUnicodeCapsTables,0,FALSE, NULL},
	{NPPTEXT("Q:(Experimental) Clipboard GlobalAlloc size test"),pfclipboardsizetest,0,FALSE, NULL},
	//{NPPTEXT("Q:(Experimental) Build New Menu"),pfbuildmenu,0,FALSE, NULL},
	//{NPPTEXT("Q:(Experimental) Display wID"),pfdisplaywIDtest,0,FALSE, NULL},
	//{NPPTEXT("Q:(Experimental) Build Dynamic Menus"),buildallIDLOOKUP,0,FALSE, NULL},
	//{NPPTEXT("Q:(Experimental) Destroy Dynamic Menus"),freeallIDLOOKUP,0,FALSE, NULL},
	{NPPTEXT("Q:About (Experimental) Tools"),pfAboutExperimental,0,FALSE, NULL},
	{NPPTEXT("Q:-"),pfdummy,0,FALSE, NULL}, // Notepad++ does not allow NULL functions so we pick a dummy
	#endif /* } */
	{NPPTEXT("Convert quotes to \""),pfconvert1q2q,0,FALSE, NULL},
	{NPPTEXT("Convert quotes to '"),pfconvert2q1q,0,FALSE, NULL},
	{NPPTEXT("Swap quotes (\" <-> ')"),pfconvertqtswap,0,FALSE, NULL},
	{NPPTEXT("Drop quotes \" && '"),pfconvertqtdrop,0,FALSE, NULL},
	{NPPTEXT("-"),pfdummy,0,FALSE, NULL}, // Notepad++ does not allow NULL functions so we pick a dummy
	{NPPTEXT("Escape \" to \\\""),pfconvertescapesq,0,FALSE, NULL},
	{NPPTEXT("Escape ' to \\'"),pfconvertescape1qs1q,0,FALSE, NULL},
	{NPPTEXT("Escape ' to \\\""),pfconvertescape1qsq,0,FALSE, NULL},
	{NPPTEXT("Escape both \"&&' to \\\"&&\\'"),pfconvertescapeboth,0,FALSE, NULL},
	{NPPTEXT("unEscape \\\" to \""),pfconvertunescapesq,0,FALSE, NULL},
	{NPPTEXT("unEscape \\' to '"),pfconvertunescapes1q1q,0,FALSE, NULL},
	{NPPTEXT("unEscape \\\" to '"),pfconvertunescapesq1q,0,FALSE, NULL},
	{NPPTEXT("unEscape both \\\"&&\\' to \"&&'"),pfconvertunescapeboth,0,FALSE, NULL},
	{NPPTEXT("Escape \" to \"\""),pfconvertescape2q22q,0,FALSE, NULL},
	{NPPTEXT("Escape ' to \"\""),pfconvertescape1q22q,0,FALSE, NULL},
	{NPPTEXT("unEscape \"\" to \""),pfconvertunescape22q2q,0,FALSE, NULL},
	{NPPTEXT("unEscape \"\" to '"),pfconvertunescape22q1q,0,FALSE, NULL},
	{NPPTEXT("-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("UPPER CASE"),pfconvertuppercase,0,FALSE, NULL},
	{NPPTEXT("lower case"),pfconvertlowercase,0,FALSE, NULL},
	{NPPTEXT("Proper Case"),pfconvertpropercase,0,FALSE, NULL},
	{NPPTEXT("Sentence case."),pfconvertsentencecase,0,FALSE, NULL},
	{NPPTEXT("iNVERT cASE"),pfconvertinvertcase,0,FALSE, NULL},
	{NPPTEXT("Zap all characters to space"),pfzapspace,0,FALSE, NULL},
	{NPPTEXT("Zap all non printable characters to #"),pfzapnonprint,0,FALSE, NULL},
	{NPPTEXT("Q:Mark Word or Find Reverse")/*NPPTEXT(" ????")*/,pfMarkWordFindReverse,0,FALSE, NULL },
	{NPPTEXT("Q:Mark Word or Find Forward"),pfMarkWordFindForward,0,FALSE, NULL },
	{NPPTEXT("Q:+Mark Word or Find Case Sensitive"),pfMarkWordFindCaseSensitive,0,FALSE, NULL},
	{NPPTEXT("Q:+Mark Word or Find Whole Words"),pfMarkWordFindWholeWord,0,FALSE, NULL},
	//{NPPTEXT("Q:Show Style"),pfshowstyle,0,FALSE, NULL},
	//{NPPTEXT("Q:Find In Other View"),pffindinotherview,0,FALSE, NULL},
	//{NPPTEXT("Q:Switch selection to rectangular"),pfswitchseltorectangular,0,FALSE, NULL},
	{NPPTEXT("Q:Find matching {([<Brace>])}"),pffindmatchchar,0,FALSE, &skfindmatchchar},//Ctrl-B in Notepad++
	{NPPTEXT("Q:Mark to matching {([<Brace>])}"),pfmarkmatchchar,0,FALSE, NULL },
	{NPPTEXT("Q:Delete Marked {([<Brace>])} Pair"),pfdeletebracepair,0,FALSE, NULL },
	//{NPPTEXT("Q:Delete Between Marked {([<Brace>])} Pair"),pfdeletebracepaircontents,0,FALSE, NULL},
	{NPPTEXT("Q:Mark lines to matching {([<Brace>])}"),pfmarkmatchline,0,FALSE, NULL},
	#if ENABLE_AutoShowMatchline
	{NPPTEXT("Q:Show line matching Brace])}"),pfshowmatchline,0,FALSE, NULL},
	#endif
	#if ENABLE_FINDREPLACE
	{NPPTEXT("Q:Find/Replace"),pffindreplace,0,FALSE, NULL },
	#endif
	{NPPTEXT("Q:Duplicate Line or Block"),pfDuplicateLineOrBlock,0,FALSE, NULL },
	#if ENABLE_PUSHPOP
	{NPPTEXT("Q:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("Q:Push position"),pfpushposition,0,FALSE, NULL},
	{NPPTEXT("Q:Push position and jump to first occurance of (Clipboard)"),pfpushjumpposition,0,FALSE, NULL},
	{NPPTEXT("Q:Recall/Pop position"),pfpopposition,0,FALSE, NULL},
	#endif
	//{NPPTEXT("V:Show/Hide 'All' Set Line Range"),pfvizshowselectedalllines,0,FALSE, NULL}, // proposed
	//{NPPTEXT("V:Show/Hide 'All' Clear Range"),pfvizshowselectedalllines,0,FALSE, NULL}, // proposed
	{NPPTEXT("V:Show Between-Selected or All-Reset Lines"),pfvizshowselectedalllines,0,FALSE, NULL},
	{NPPTEXT("V:Hide Between-Selected or All-Reset Lines"),pfvizhideselectedalllines,0,FALSE, NULL},
	{NPPTEXT("V:Invert Visibility Between-Selected or All Lines"),pfvizinvertselectedalllines,0,FALSE, NULL},
	{NPPTEXT("V:Hide Lines with (Clipboard) text"),pfvizhidecliplines,0,FALSE, NULL},
	{NPPTEXT("V:Hide Lines without (Clipboard) text"),pfvizhideclipclines,0,FALSE, NULL},
	{NPPTEXT("V:Show Lines with (Clipboard) text"),pfvizshowcliplines,0,FALSE, NULL},
	{NPPTEXT("V:Show Lines without (Clipboard) text"),pfvizshowclipclines,0,FALSE, NULL},
	{NPPTEXT("V:Show More Lines around my position..."),pfvizshowmorelines,0,FALSE, NULL},
	{NPPTEXT("V:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("V:Hide/Show sequence all steps"),pfvizsequenceall,0,FALSE, NULL},
	{NPPTEXT("V:Hide/Show sequence singlestep start"),pfvizsequencestart,0,FALSE, NULL},
	{NPPTEXT("V:Hide/Show sequence singlestep next"),pfvizsequencenext,0,FALSE, NULL},
	{NPPTEXT("V:Hide/Show sequence singlestep rest"),pfvizsequencerest,0,FALSE, NULL},
	{NPPTEXT("V:Select as Hide/Show sequence"),pfvizselectassequence,0,FALSE, NULL},
	{NPPTEXT("V:Insert Show/Hide Sequence"),pfvizinsertsequence,0,FALSE, NULL},
	{NPPTEXT("V:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("V:Copy Visible Selection"),pfvizcopyvisible,0,FALSE, NULL},
	{NPPTEXT("V:Cut Visible Selection"),pfvizcutvisible,0,FALSE, NULL},
	{NPPTEXT("V:Delete Visible Selection"),pfvizdeletevisible,0,FALSE, NULL},
	{NPPTEXT("V:Copy Invisible Selection"),pfvizcopyinvisible,0,FALSE, NULL},
	{NPPTEXT("V:Cut Invisible Selection"),pfvizcutinvisible,0,FALSE, NULL},
	{NPPTEXT("V:Delete Invisible Selection"),pfvizdeleteinvisible,0,FALSE, NULL},
	{NPPTEXT("V:Copy Entire Selection (no append)"),pfcopyallnoappend,0,FALSE, NULL },
	{NPPTEXT("V:Cut Entire Selection (no append)"),pfcutallnoappend,0,FALSE, NULL },
	{NPPTEXT("V:Copy && Append Entire Selection"),pfcopyallappend,0,FALSE, NULL },
	{NPPTEXT("V:Cut && Append Entire Selection"),pfcutallappend,0,FALSE, NULL },
	{NPPTEXT("V:Paste as UTF-8/ANSI"),pfVizPasteUTF8,0,FALSE, NULL},
	{NPPTEXT("V:Paste"),pfVizPasteUNICODE,0,FALSE, NULL},
	//{NPPTEXT("V:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Text Search Case Sensitive"),pfVizCaseSensitive,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Text Search Whole Words"),pfVizWholeWords,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Text Search Regex"),pfVizRegex,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Copy-Cut Appends to clipboard"),pfVizCutCopyAppend,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Copy-Cut always converts to CRLF"),pfVizClipboardAlwaysCRLF,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Copy-Cut replace [NUL] with space"),pfVizClipboardReplaceNulls,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Copy-Cut also in UTF-8"),pfVizClipboardCopyAlsoUTF8,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Copy-Cut not in UNICODE"),pfVizClipboardNotUnicode,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Paste retains position"),pfVizPasteRetainsPosition,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Paste/Append binary"),pfVizPasteBinary,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Paste converts EOL to editor"),pfVizPasteToEditorEOL,0,FALSE, NULL},
	{NPPTEXT("W:+Viz Capture Keyboard Ctrl-C,X,V"),pfCaptureCutCopyPaste,0,FALSE, NULL},
	{NPPTEXT("E:Fill Down Insert"),pffilldownins,0,FALSE, NULL},
	{NPPTEXT("E:Fill Down Overwrite"),pffilldownover,0,FALSE, NULL},
	{NPPTEXT("E:Insert (Clipboard) through lines"),pfinsertclipboardcolumn,0,FALSE, NULL},
	{NPPTEXT("E:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("E:Reindent C++ code"),pfreindentcode,0,FALSE, NULL},
	{NPPTEXT("E:Leading space to tabs or tabs to spaces"),pfspace2tabs,0,FALSE, NULL},
	{NPPTEXT("E:Leading space to tabs or tabs to spaces width=8"),pfspace2tabs8,0,FALSE, NULL},
	{NPPTEXT("E:Trim Trailing Spaces"),pftrimtrailingspace,0,FALSE, NULL}, // Notepad++ 3.2 is slow and only trims the entire file
	{NPPTEXT("E:Indent text sticky left margin"),pfindentlines,0,FALSE, NULL},
	{NPPTEXT("E:Indent && surround { text lines }"),pfindentlinessurround,0,FALSE, NULL},
	{NPPTEXT("E:Delete Blank Lines"),pfdeleteblanklines,0,FALSE, NULL},
	{NPPTEXT("E:Delete Surplus Blank Lines"),pfdeleteblanklines2,0,FALSE, NULL},
	//{NPPTEXT("E:Delete Every Other Line"),pfdeleteevery2lines,0,FALSE, NULL}, // completely handled by viz now
	{NPPTEXT("E:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("E:Strip unquoted text (VB) separate by (Clipboard<=20)"),pffindqtstringvb,0,FALSE, NULL},
	{NPPTEXT("E:Strip unquoted text (C) separate by (Clipboard<=20)"),pffindqtstringc,0,FALSE, NULL},
	{NPPTEXT("E:Kill unquoted (VB) whitespace"),pfkillwhitenonqtvb,0,FALSE, NULL},
	{NPPTEXT("E:Kill unquoted (C) whitespace"),pfkillwhitenonqtc,0,FALSE, NULL},
	{NPPTEXT("E:Split lines at (clipboard character) or , (VB)"),pfsplitlinesatchvb,0,FALSE, NULL},
	{NPPTEXT("E:Split lines at (clipboard character) or , (C)"),pfsplitlinesatchc,0,FALSE, NULL},
	{NPPTEXT("E:Line up multiple lines by (,)"),pflineupcomma,0,FALSE, NULL},
	{NPPTEXT("E:Line up multiple lines by (=)"),pflineupequals,0,FALSE, NULL},
	{NPPTEXT("E:Line up multiple lines by (Clipboard Character)"),pflineupclipboard,0,FALSE, NULL},
	{NPPTEXT("E:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("E:Unwrap Text"),pfunwraptext,0,FALSE, NULL},
	{NPPTEXT("E:ReWrap Text to (Clipboard or 72) width"),pfrewraptext,0,FALSE, NULL},
	//{NPPTEXT("E:Extend selection as rectangular to end of file"),pfextendblock,0,FALSE, NULL},
	{NPPTEXT("E:Pad rectangular selection with spaces"),pfextendblockspaces,0,FALSE, NULL},
	{NPPTEXT("C:Encode URI Component"),pfencodeURIcomponent,0,FALSE, NULL},
	{NPPTEXT("C:Encode HTML (&&<>\")"),pfencodeHTML,0,FALSE, NULL},
	{NPPTEXT("C:Strip HTML tags table tabs"),pfstripHTMLtags,0,FALSE, NULL},
	{NPPTEXT("C:Strip HTML tags table nontabs"),pfstripHTMLnotabs,0,FALSE, NULL},
	{NPPTEXT("C:Submit to W3C HTML Validator"),pfsubmitHTML,0,FALSE, NULL},
	{NPPTEXT("C:Submit to W3C CSS Validator"),pfsubmitCSS,0,FALSE, NULL},
	{NPPTEXT("C:Convert text to code command(\"text=\\\"value\\\"\");"),pfprepostpendlines,0,FALSE, NULL},
	{NPPTEXT("C:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("C:Convert Decimal Number to Binary"),pfdecimal2binary,0,FALSE, NULL},
	{NPPTEXT("C:Convert Decimal Number to Octal"),pfdecimal2octal,0,FALSE, NULL},
	{NPPTEXT("C:Convert Decimal Number to Hex"),pfdecimal2hex,0,FALSE, NULL},
	{NPPTEXT("C:Convert Hex Number to Decimal"),pfhex2decimal,0,FALSE, NULL},
	{NPPTEXT("C:Convert Octal Number to Decimal"),pfoctal2decimal,0,FALSE, NULL},
	{NPPTEXT("C:Convert Binary Number to Decimal"),pfbinary2decimal,0,FALSE, NULL},
	{NPPTEXT("C:Convert C-style Number to Decimal"),pfcnum2decimal,0,FALSE, NULL},
	{NPPTEXT("C:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("C:Convert text to Hex-16"),pftohex16,0,FALSE, NULL},
	{NPPTEXT("C:Convert text to Hex-32"),pftohex32,0,FALSE, NULL},
	{NPPTEXT("C:Convert text to Hex-64"),pftohex64,0,FALSE, NULL},
	{NPPTEXT("C:Convert text to Hex-128"),pftohex128,0,FALSE, NULL},
	{NPPTEXT("C:Convert hex byte runs into LE-WORDS"),pfhexbyterunstolittlendian2,0,FALSE, NULL},
	{NPPTEXT("C:Convert hex byte runs into LE-DWORDS"),pfhexbyterunstolittlendian4,0,FALSE, NULL},
	{NPPTEXT("C:Convert LE-words to hex byte runs"),pflittlendiantohexbyteruns,0,FALSE, NULL},
	{NPPTEXT("C:Convert Hex to text"),pffromhex,0,FALSE, NULL},
	{NPPTEXT("C:ROT13 Text"),pfrot13,0,FALSE, NULL},
	{NPPTEXT("C:Convert EBCDIC to ASCII"),pfEBCDIC2ascii,0,FALSE, NULL},
	{NPPTEXT("C:Convert ASCII to EBCDIC"),pfascii2EBCDIC,0,FALSE, NULL},
	{NPPTEXT("C:Convert KOI8_R to CP1251"),pfKOI8_Rtocp1251,0,FALSE, NULL},
	{NPPTEXT("C:Convert CP1251 to KOI8_R"),pfcp1251toKOI8_R,0,FALSE, NULL},
	{NPPTEXT("T:Sort lines case sensitive (at column)"),pfqsortlinesc,0,FALSE, NULL},
	{NPPTEXT("T:Sort lines case insensitive (at column)"),pfqsortlinesnc,0,FALSE, NULL},
	{NPPTEXT("T:+Sort ascending"),pfSortAscending,0,TRUE , NULL},
	{NPPTEXT("T:+Sort outputs only UNIQUE (at column) lines"),pfSortLinesUnique,0,TRUE , NULL},
	{NPPTEXT("T:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("T:Insert Ascii Chart or Character"),pfinsertasciichart,0,FALSE, NULL},
	{NPPTEXT("T:Insert Ruler"),pfinsertruler,0,FALSE, NULL},
	{NPPTEXT("T:Insert Line Numbers"),pfinsertlinenumbers,0,FALSE, NULL},
	{NPPTEXT("T:Delete Line Numbers or First Word"),pfdeletefirstword,0,FALSE, NULL},
	{NPPTEXT("T:Clean eMail >Quoting"),pfcleanemailquoting,0,FALSE, NULL},
	{NPPTEXT("T:UUdecode"),pfuudecode,0,FALSE, NULL},
	{NPPTEXT("T:Base64 Decode"),pfbase64decode,0,FALSE, NULL},
	{NPPTEXT("T:Word Count"),pfwordcount,0,FALSE, NULL},
	{NPPTEXT("T:Add up numbers"),pfaddup,0,FALSE, NULL},
	{NPPTEXT("T:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("T:Empty Undo Buffer (be sure to save)"),pfemptyundobuffer,0,FALSE, NULL},
	{NPPTEXT("I:Current Full Path"),pfinsertCurrentFullPath,0,FALSE, NULL},
	{NPPTEXT("I:Current File Name"),pfinsertCurrentFileName,0,FALSE, NULL},
	{NPPTEXT("I:Current Directory"),pfinsertCurrentDirectory,0,FALSE, NULL},
	{NPPTEXT("I:Date && Time - short format"),pfinsertShortDateTime,0,FALSE, NULL},
	{NPPTEXT("I:Date && Time - long format"),pfinsertLongDateTime,0,FALSE, NULL},
	#ifndef HighPerformance
	{NPPTEXT("S:+Go Faster, use more memory"),pfHighPerformance,0,TRUE , NULL},
	#endif
	{NPPTEXT("S:+Cancel Overwrite Mode moving from current line"),pfBlockOverwrite,0,TRUE , NULL},
	{NPPTEXT("S:+Autoclose XHTML/XML <Tag>"),pfAutoCloseHTMLtag,0,TRUE , NULL},
	{NPPTEXT("S:+Autoclose {([Brace"),pfAutoCloseBrace,0,TRUE , NULL},
	{NPPTEXT("S:+Autoconvert typed leading spaces to tabs"),pfAutoSpace2Tab,0,TRUE , NULL},
	#if ENABLE_AutoShowMatchline
	{NPPTEXT("S:+Autoshow line matching Brace])}"),pfAutoShowMatchline,0,TRUE , NULL},
	#endif
	{NPPTEXT("S:+Autoconvert typed HTML/XML to &&entities;"),pfAutoConvertHTML,0,TRUE , NULL},
	{NPPTEXT("S:+Disable Subclassing && advanced features"),pfDisableSubclassing,0,FALSE, NULL},
	{NPPTEXT("S:+Move quick menus out of 'Plugins' menu"),pfSeparateQuickMenus,0,FALSE, NULL},
	#if ENABLE_WrapFriendlyHomeEnd
	{NPPTEXT("S:+Improve Home-End Movement for Wrapped Text"),pfWrapFriendlyHomeEnd,0,FALSE, NULL},
	#endif
	{NPPTEXT("S:+Ctrl-D also dups marked text"),pfCtrlDAlsoDupsBlock,0,FALSE, NULL},
	{NPPTEXT("S:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("S:Visit Notepad++ && ") PLUGIN_NAME_MENU NPPTEXT(" website"),pfhNotepadweb,0,FALSE, NULL},
	{NPPTEXT("S:Help"),pfhelp,0,FALSE, NULL},
	{NPPTEXT("S:About ")PLUGIN_NAME_MENU,pfabout,0,FALSE, NULL},
	#if NPPDEBUG
	{NPPTEXT("H:>Online Help"),pfmpxhelprebuild,0,FALSE, NULL},
	#endif
	#if ENABLE_TIDYDLL
	{NPPTEXT("D:About Tidy"),pfabouttidy,0,FALSE, NULL},
	{NPPTEXT("D:Visit HTML Tidy SourceForge website"),pfhtmltidyweb,0,FALSE, NULL},
	{NPPTEXT("D:Download libTidy.DLL from SourceForge"),pfgethtmltidyweb,0,FALSE, NULL},
	{NPPTEXT("D:Reload libTidy.DLL"),pfreloadtidydll,0,FALSE, NULL},
	{NPPTEXT("D:Refresh Menu from ") NPPTIDY_INI_MENU,pfmpxtidyrebuild,0,FALSE, NULL},
	{NPPTEXT("D:-"),pfdummy,0,FALSE, NULL},
	{NPPTEXT("D:Tidy (most recent ") NPPTIDY_CFG_MENU NPPTEXT(")"),pfhtmltidy,0,FALSE, NULL},
	#endif
};

EXTERNC unsigned findmenuitem(PFUNCPLUGINCMD_MSC *_pFunc) {
  unsigned itemno, items = NELEM(funcItem);
  for (itemno = 0; itemno < items; itemno++) {
  	if (_pFunc == funcItem[itemno]._pFunc) {
		return(itemno);
	}
  }
  return(0);
}

/*
 *--------------------------------------------------
 * The 5 extern functions are mandatory
 * They will be called by Notepad++ plugins system
 *--------------------------------------------------
*/

// The setInfo function gets the needed infos from Notepad++ plugins system
extern "C" __declspec(dllexport) void setInfo(struct NppData notpadPlusData) {
  //MessageBox(0 , "setInfo",PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
  g_nppData = notpadPlusData;
  //Got N++ Handlke, retrieve config paths and whatnot
#if ENABLE_FINDREPLACE
    PopFindReplaceDlgInit(FALSE/*,g_pszPluginpath*/);
#endif
    iniSaveSettings(/*g_pszPluginpath,*/FALSE); // must be after g_pszPluginpath is set
#if NPPDEBUG /* { */
    testsprintfuncs();
#endif /* } */
}

// The getName function tells Notepad++ plugins system its name
// NOTE: If not using TextFX as main menu item, this is what Plugins-menu will display.
// NOTE 2: If using TextFX as main menu item, this will be the first menu item in TextFX.
extern "C" __declspec(dllexport) const NPPCHAR * getName(void) {
  //MessageBox(0 , "getName",PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
	return PLUGIN_NAME_MENU
#if NPPDEBUG /* { */
		NPPTEXT(" (debug)")
#endif /* } */
#if TEXTFX_TOP_MENU
		NPPTEXT(" Characters")
#endif
		;
}

// The getFuncsArray function gives Notepad++ plugins system the pointer FuncItem Array
// and the size of this array (the number of functions)
extern "C" __declspec(dllexport) struct FuncItem * getFuncsArray(int *nbF) {
  //MessageBox(0 , "getFuncsArray",PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
  *nbF = NELEM(funcItem);

  return funcItem;
}

#ifdef NPP_UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
	return true;
}
#endif

// If you don't need get the notification from Notepad++,
// just let it be empty.
extern "C" __declspec(dllexport) void beNotified(struct SCNotification *notifyCode)
{
	static unsigned prevline;
	static BOOL block=FALSE;
	BOOL kscapital;
	static TCHAR chPerformKey = '\0';
	INT_CURRENTEDIT;

	if (!block) { // with enough delay, beNotified ends up rentrant
		static unsigned runOnce = 0;
		block=TRUE;
		if (!runOnce && g_fLoadonce) {
			pfbuildmenu();
			MENUITEMINFO menuItemSeparator = {
				sizeof(MENUITEMINFO),
				MIIM_TYPE,
				MFT_SEPARATOR,
				0,0,0,0,0,0,NULL,0
			};

			// Add menu separators
			for (runOnce=0; runOnce < NELEM(funcItem); runOnce++)
				if (funcItem[runOnce]._itemName[0] == '-')
					SetMenuItemInfo(GetMenu(g_nppData._nppHandle), funcItem[runOnce]._cmdID, FALSE, &menuItemSeparator);

#if ENABLE_TIDYDLL
    control_tidy(FALSE);
#endif
		    DrawMenuBar(g_nppData._nppHandle);
			CloseHandle(g_fLoadonce); // closing here will allow another invocation of Notepad++ + NPPTextFX to load if N++ allows this
			g_fLoadonce=NULL;
			if (!funcItem[g_miDisableSubclassing]._init2Check) {
				buildallIDLOOKUP();
				NPPSubclassProc_init(); // must be after pfbuildmenu();
			}
	/*for(runOnce=1; runOnce<3; runOnce++) {
      SENDMSGTOCED(runOnce==1, SCI_CLEARCMDKEY, MAKEWPARAM('C',SCMOD_CTRL), 0); // Notepad++ isn't handling these keys but will give them to us through the new features
      SENDMSGTOCED(runOnce==1, SCI_CLEARCMDKEY, MAKEWPARAM('V',SCMOD_CTRL), 0);
      SENDMSGTOCED(runOnce==1, SCI_CLEARCMDKEY, MAKEWPARAM('X',SCMOD_CTRL), 0);
      SENDMSGTOCED(runOnce==1, SCI_CLEARCMDKEY, MAKEWPARAM(SCK_TAB,SCMOD_NORM), 0); // Convert All needs to get the current line to make this work
    }*/
#if ENABLE_WrapFriendlyHomeEnd
			WrapFriendlyHomeEnd(funcItem[g_miWrapFriendlyHomeEnd]._init2Check);
#endif
		} // end if (!runOnce && g_fLoadonce)

		switch (notifyCode->nmhdr.code) {
		case SCN_UPDATEUI:
			kscapital = (GetAsyncKeyState(VK_CAPITAL) & 0x8000) ? TRUE : FALSE;
			if (funcItem[g_miBlockOverwrite]._init2Check || kscapital) {
				GET_CURRENTEDIT;
				unsigned curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
				unsigned curline=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, curpos,0);
				if (prevline!=curline) {
					if (funcItem[g_miBlockOverwrite]._init2Check && SENDMSGTOCED(currentEdit, SCI_GETOVERTYPE, 0, 0)) {
						SENDMSGTOCED(currentEdit, SCI_SETOVERTYPE, 0, 0);
          //MessageBoxFree(g_nppData._nppHandle,smprintf("First unhandled code after insert change %d",notifyCode->nmhdr.code),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
			        }
					int dir=curline>prevline?1:-1;
					if (kscapital && prevline+dir != curline && !SENDMSGTOCED(currentEdit, SCI_GETLINEVISIBLE, prevline+dir, 0)) {
						unsigned col=SENDMSGTOCED(currentEdit, SCI_GETCOLUMN, curpos, 0);
          //MessageBoxFree(g_nppData._nppHandle,smprintf("%X Jumped from line %d to %d dir %d maintain column:%u",GetKeyState(VK_CAPITAL),prevline,curline,dir,col),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
						curline=prevline+dir;
						SENDMSGTOCED(currentEdit, SCI_SHOWLINES, curline, curline);
						SENDMSGTOCED(currentEdit, SCI_GOTOPOS,SENDMSGTOCED(currentEdit, SCI_FINDCOLUMN, curline, col), 0);
					}
				}
				prevline=curline;
			}
#if ENABLE_AutoShowMatchline
    if (funcItem[g_miAutoShowMatchline]._init2Check) pfshowmatchline();
#endif
		    if (chPerformKey) {
				TCHAR *rch;
				switch(chPerformKey) {
				case '(':
					rch= _T(")");
					goto acb;
				case '[':
					rch= _T("]");
					goto acb;
				case '{':
					rch= _T("}");
acb:
					if (funcItem[g_miAutoCloseBrace]._init2Check) { // auto close brace
						enum LangType docType;
						SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
						if (docType == L_PHP || docType == L_C || docType == L_CPP || docType == L_CS || docType == L_OBJC ||
							docType == L_JAVA || docType == L_ASP || docType ==  L_SQL || docType ==  L_JS || docType ==  L_PERL ||
							docType ==  L_LUA || docType ==  L_FLASH) {
							GET_CURRENTEDIT;
							unsigned curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
							unsigned curposl=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, curpos, 0);
							//if (currentPos!=SENDMSGTOCED(currentEdit, SCI_GETLINEENDPOSITION, curposl, 0)) break; // can only autoclose at end of line http://www.textpad.info/forum/viewtopic.php?t=2838
							TCHAR *buf=NULL;
							size_t bufsz=2,buflen;
							if (chPerformKey=='{') {
								unsigned eoltype=SENDMSGTOCED(currentEdit, SCI_GETEOLMODE, 0, 0); if (eoltype>=NELEM(eoltypes)) eoltype=NELEM(eoltypes)-1;
								unsigned extra=wcslen(eoltypes[eoltype]);
								bufsz=extra+SENDMSGTOCED(currentEdit, SCI_GETLINE,curposl,NULL)+1;
								strcpyarmsafe(&buf, &bufsz, &buflen, eoltypes[eoltype], _T("scNotify-acb"));
								if (!buf)
									break;
								buflen += SENDMSGTOCED(currentEdit, SCI_GETLINE,curposl,buf+buflen);
								buf[buflen]='\0';
								//MessageBoxFree(g_nppData._nppHandle,smprintf("reqbufsz:%u bufsz:%u buflen:%u strlen:%u\r\n%s",extra+SENDMSGTOCED(currentEdit, SCI_GETLINE,curposl,NULL)+1,bufsz,buflen,strlen(buf),buf),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
								buflen=extra + wcsspn(buf+extra, _T(" \t"));
								strcpyarmsafe(&buf, &bufsz, &buflen, rch, _T("scNotify-acb"));
								if (!buf)
									break;
								SENDMSGTOCED(currentEdit, SCI_GOTOPOS, SENDMSGTOCED(currentEdit, SCI_GETLINEENDPOSITION,curposl,0), 0);
							} else {
								strcpyarmsafe(&buf, &bufsz, &buflen, rch, _T("scNotify-acb"));
							if (!buf)
								break;
							}
							SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
							SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, buf);
							SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
							SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
							freesafe(buf, _T("acb"));
						} // end if () doctype PHP, C, C++, C#, ObjC, Java, Asp, SQL, JavaScript, Perl, Lua, Flash (ActiveScript)
					}
					break;
				case '>': // Autoclose HTML tag
					if (funcItem[g_miAutoConvertHTML]._init2Check) {
						rch= _T("&gt;");
						goto acH;
					} // ach: will pass back to act2: if > is not to be converted to an entity because in a tag
					if (funcItem[g_miAutoCloseHTMLtag]._init2Check ) {
						enum LangType docType;
						SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
						if ((docType == L_HTML || docType == L_XML /*|| docType == L_PHP || docType == L_ASP*/)) {
							unsigned curpos,bktpos;
							GET_CURRENTEDIT;
act2:
							if (bracematch(currentEdit,&curpos,&bktpos,FLAG_INCLUDEBRACKETS) && bktpos<curpos && curpos-bktpos>2 /* 2 ensures no null <> tags */) {
								bktpos++;
#define SZPREFIX CHARSIZE(2) /* sizeof("<\"); */
								TCHAR *buf=NULL;
								size_t bufsz = SZPREFIX + curpos - bktpos + 1, buflen;
              //MessageBoxFree(g_nppData._nppHandle,smprintf("currentPos:%u bktpos:%u currentPos-bktpos+%u+1:%u bufsz:%u buflen:%u",currentPos,bktpos,SZPREFIX,currentPos-bktpos+SZPREFIX+1,bufsz,buflen),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
								strcpyarmsafe(&buf, &bufsz, &buflen, _T("</"), _T("scNotify-act2"));
								if (!buf)
									break;
              //MessageBoxFree(g_nppData._nppHandle,smprintf("currentPos:%u bktpos:%u currentPos-bktpos+%u+1:%u bufsz:%u buflen:%u strlen:%u\r\n%s",currentPos,bktpos,SZPREFIX,currentPos-bktpos+SZPREFIX+1,bufsz,buflen,strlen(buf),buf),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
								struct TextRange tr;
								tr.chrg.cpMin=bktpos;
								tr.chrg.cpMax=curpos;
								tr.lpstrText=buf+SZPREFIX;
								if ( (buflen+=SENDMSGTOCED(currentEdit, SCI_GETTEXTRANGE, 0, &tr))>1+SZPREFIX /* 1 to prevent close from null <> tags*/
									  && buf[buflen-2]!='/' /* prevent from self-closing XHTML tags */
									  && isgraph(buf[2])    /* prevent close from strange characters */
									  && memicmp(buf+2,"BR",2) // http://codex.wordpress.org/HTML_to_XHTML
									  && memicmp(buf+2,"HR",2)
									  && memicmp(buf+2,"META",4)
									  && memicmp(buf+2,"LINK",4)
									  && memicmp(buf+2,"IMG",3)
									  && !strchr("/%?!",buf[2])  /* prevent from close and special tags */ ) {
                //MessageBoxFree(g_nppData._nppHandle,smprintf("currentPos:%u bktpos:%u currentPos-bktpos+%u+1:%u bufsz:%u buflen:%u strlen:%u\r\n%s",currentPos,bktpos,SZPREFIX,currentPos-bktpos+SZPREFIX+1,bufsz,buflen,strlen(buf),buf),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
									TCHAR *sp;
									if ((sp=wcschr(buf+2,' ')))
										wcscpy(sp, _T(">"));
									SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
									SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, buf);
									SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos, 0);
									SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
								}
								freesafe(buf, _T("scNotify-act2"));
#undef SZPREFIX
							}
						}
					}
					break;
				case '"':
					rch= _T("&quot;");
					goto acH; // " not converted if in a tag
				case '&':
					rch= _T("&amp;");
					goto acH; // & always converted
				case '<':
					rch= _T("&lt;");
					goto acH; // < not converted if not in a tag;
				case 13 :
					rch= _T("<br/>");            // > not converted if in a tag
acH:
					if (funcItem[g_miAutoConvertHTML]._init2Check) { // auto convert HTML entities
						enum LangType docType;
						SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
						if ((docType == L_HTML || docType == L_XML /*|| docType == L_PHP || docType == L_ASP*/)) {
							GET_CURRENTEDIT;
							unsigned curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
							unsigned curposl=SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION, curpos, 0);
							unsigned bol=SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE,curposl,0);
							BOOL intag=FALSE;
							if (curpos-1>bol) {
								char *t;
								if ((t=(char *)mallocsafe(curpos-bol, _T("scNotify-acH")))) {
									int sln=SENDMSGTOCED(currentEdit, SCI_GETCURLINE,curpos-bol,t);
                //MessageBoxFree(g_nppData._nppHandle,smprintf("currentPos-1:%u bol:%u currentPos-bol:%u test:%u lastchs:%d:%d:%d:%d:%d\r\n%s",currentPos-1,bol,currentPos-bol,test,t[currentPos-bol-1],t[currentPos-bol],t[currentPos-bol+1],t[currentPos-bol+2],t[currentPos-bol+3],t),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
									char *tx;
									for(tx=t+sln-1; tx>=t; tx--)
										if (*tx=='<') {
											intag=TRUE;
											break;
										} else if (*tx=='>')
											break;
										freesafe(t, _T("scNotify-acH"));
								}
							}
            //MessageBoxFree(g_nppData._nppHandle,smprintf("intag:%s",intag?"TRUE":"FALSE"),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
							if (chPerformKey=='>' && intag) {
								if (funcItem[g_miAutoCloseHTMLtag]._init2Check)
									goto act2; // act gave up this character for us and we need to send it back validated if we can't use it
								else
									break;
							} else if (chPerformKey=='<' && !intag) {
								break;
							} else if (chPerformKey=='"' &&  intag) {
								break;
							}
							if (curpos>0) {
								unsigned s1,s2;
								if (chPerformKey==13) {
									s1=s2=SENDMSGTOCED(currentEdit, SCI_GETLINEENDPOSITION, curposl-1, 0);
									curpos=SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE, curposl,0);
								} else {
									curpos--;
									s1=curpos;
									s2=curpos+1;
								}
								SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
								SENDMSGTOCED(currentEdit, SCI_SETSEL, s1, s2);
								SENDMSGTOCED(currentEdit, SCI_REPLACESEL, 0, rch);
								SENDMSGTOCED(currentEdit, SCI_GOTOPOS, curpos+wcslen(rch), 0);
								SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
							}
						}
					} // end if auto convert HTML entities
					break;
				} // end switch switch(chPerformKey)
				chPerformKey='\0';
			}
			break;

		case SCN_CHARADDED:
			switch(notifyCode->ch) {
			case ' ': // Autoconvert spaces to tabs
				if (funcItem[g_miAutoSpace2Tab]._init2Check) {
					GET_CURRENTEDIT;
					unsigned curpos=SENDMSGTOCED(currentEdit, SCI_GETCURRENTPOS, 0, 0);
					unsigned bol=SENDMSGTOCED(currentEdit, SCI_POSITIONFROMLINE,SENDMSGTOCED(currentEdit, SCI_LINEFROMPOSITION,curpos,0),0);
					size_t buflen = curpos - bol;
					size_t bufsz = buflen + 1;
					TCHAR *buf;
					if (!(buf=(TCHAR *)mallocsafe(bufsz, _T("scNotify-space"))))
						break;
					int tabwidth;
					if (SENDMSGTOCED(currentEdit, SCI_GETUSETABS, 0, 0) && (tabwidth=SENDMSGTOCED(currentEdit, SCI_GETTABWIDTH, 0, 0)) /*&& currentPos-bol<sizeof(buf)*/) {
						struct TextRange tr;
						tr.chrg.cpMin=bol;
						tr.chrg.cpMax=curpos;
						tr.lpstrText=buf;
						if (tr.chrg.cpMin>=0 && SENDMSGTOCED(currentEdit, SCI_GETTEXTRANGE, 0, &tr) == tr.chrg.cpMax-tr.chrg.cpMin) {
							curpos-=bol;
							unsigned curposcol;
							TCHAR *d;
							for(d=buf,curposcol=0; *d && curpos!=(unsigned)-1; curposcol+=(*d=='\t'?tabwidth:1),d++,curpos--);
							if (space2tabs(&buf, &bufsz, &buflen, 1, tabwidth, 1)) {
								for(d=buf; *d && curposcol>0; curposcol-=(*d=='\t'?tabwidth:1),d++);
              //MessageBoxFree(g_nppData._nppHandle,smprintf("currentPos:%d bol:%d curposcol:%d cpmin:%d cpmax:%d buf\r\n%s",currentPos,bol,curposcol,tr.chrg.cpMin,tr.chrg.cpMax,tr.lpstrText),PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
									SENDMSGTOCED(currentEdit, SCI_BEGINUNDOACTION, 0, 0);
									SENDMSGTOCED(currentEdit, SCI_SETTARGETSTART, tr.chrg.cpMin, 0);
									SENDMSGTOCED(currentEdit, SCI_SETTARGETEND, tr.chrg.cpMax, 0);
									SENDMSGTOCED(currentEdit, SCI_REPLACETARGET, -1,buf);
									SENDMSGTOCED(currentEdit, SCI_GOTOPOS, (d-buf+bol),0);
									SENDMSGTOCED(currentEdit, SCI_ENDUNDOACTION, 0, 0);
							}
						}
					}
					freesafe(buf, _T("scNotify-space"));
				}
				break;
			case '(': // Autoclose Brace
			case '[': /* SCI_BRACEMATCH does not function during SCN_CHARADDED so we delay it to SCN_UPDATEUI */
			case '{':
			case '>': // Autoclose HTML tag
			case '"': // Autoclose HTML
			case '&':
			case '<':
			case 13 :
				chPerformKey=notifyCode->ch;
				break;
			} // end switch(notifyCode->ch)

		} // end switch (notifyCode->nmhdr.code)

		block=FALSE;
	} // end if (!block)
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
  return(0);
}



// SECTION: END
// SECTION: Beginning of DllMain

//----------------------------------------------------------//
// Standard Windows DLL Entry Point
//----------------------------------------------------------//

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID lpvReserved) {
  OSVERSIONINFO osv;

  g_hInstance=hInst;
  switch (reason) {
  case DLL_PROCESS_ATTACH:
#if NPPDEBUG && 0
    {WCHAR str[256];
    _snwprintf(str,NELEM(str),L"%s %s %s",L"ABC",L"DEF",L"GHI"); // result is A D G
    MessageBoxW(0 , L"DllMain",str, MB_OK|MB_ICONINFORMATION);}
    sntest();
#endif
#ifdef mallocsafeinit
    mallocsafeinit(); // must be first
    testmallocsafe();
#endif
    osv.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&osv);
	g_fOnNT = osv.dwPlatformId == VER_PLATFORM_WIN32_NT;
	if (!(g_fLoadonce = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4, _T("Notepad++" PLUGIN_NAME)))) {
		// Most likely a: ERROR_INVALID_HANDLE meaning, that the file mapping already exists, aka. DLL is already loaded.
#if 0
		DWORD errorCode = GetLastError();
		LPWSTR messageBuffer = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

		MessageBox(0, smprintf(_T("Internal " PLUGIN_NAME ": DllMain() CreateFileMapping() failed with (%d):\n%s"), errorCode, messageBuffer), _T(PLUGIN_NAME), MB_OK | MB_ICONINFORMATION);

		//Free the buffer.
		LocalFree(messageBuffer);
#endif

		return FALSE;
	}
    // Windows automatically closes this FileMapping when the last owner quits.
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
      MessageBox(0, _T("You can't load " PLUGIN_NAME " twice. Please delete the extra DLL in your Notepad ++ plugins folder") , _T(PLUGIN_NAME), MB_OK|MB_ICONINFORMATION);
      return(FALSE);
    }
#if ENABLE_PUSHPOP
    memset(g_PopLists,0,sizeof(g_PopLists));
#endif
    //GetModuleFileName(hInst, pathtemp, sizeof(pathtemp));
    //for(t=pathtemp+strlen(pathtemp); t>=pathtemp && *t != '\\'; t--);
    //t[1]='\0'; //MessageBox(0 , pathtemp,PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
    //if (!(g_pszPluginpath=strdupsafe(pathtemp,"DllMain-g_pszPluginpath"))) return(FALSE);
    /*{ unsigned uTemp;
      NPPGetSpecialFolderLocationarm(CSIDLX_NOTEPADPLUSPLUGINS,NULL,&g_pszPluginpath,&uTemp,NULL,NULL);
    } if (!g_pszPluginpath) return FALSE;*/
#if ENABLE_FINDREPLACE
    //PopFindReplaceDlgInit(FALSE/*,g_pszPluginpath*/);	//Delay untill SetInfo
#endif
    //iniSaveSettings(/*g_pszPluginpath,*/FALSE); // must be after g_pszPluginpath is set	//Delay untill SetInfo
    g_cfColumnSelect = RegisterClipboardFormat(_T("MSDEVColumnSelect"));
    break;

  case DLL_PROCESS_DETACH:
    //MessageBox(0 , "DLL_PROCESS_DETACH",PLUGIN_NAME, MB_OK|MB_ICONINFORMATION);
    NPPSubclassProc_close();
	freeallIDLOOKUP();
    iniSaveSettings(/*g_pszPluginpath,*/TRUE);
    if (g_fLoadonce)
		CloseHandle(g_fLoadonce);
#if ENABLE_FINDREPLACE
    PopFindReplaceDlgInit(TRUE/*,g_pszPluginpath*/);
#endif
    DelTempsList();
    //freesafe(g_pszPluginpath,"DllMain-g_pszPluginpath");
    if (g_pszVizSequence)
		freesafe(g_pszVizSequence, _T("DllMain"));
#if ENABLE_TIDYDLL
    control_tidy(TRUE);
    if (g_szTidyDLL_error) freesafe(g_szTidyDLL_error,"DllMain-g_szTidyDLL_error");
#endif
#ifdef mallocsafeinit
    mallocsafedone(); // must be after any safe calls
#endif
    break;

  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}

/* Execution sequence:
  1) DllMain is called when Notepad++ loads the plugin
  2) N++ calls getFuncsArray, setInfo, getName to get initial operating info
  3) beNotified is called constantly while editing.
  4) messageProc is called for select Notepad++ messages.
  5) Occasionally the user will select a plugin menu item.
  6) The user will quit N++ which will call DllMain for the last time. */

// SECTION: END
