//#define PLUGIN_NAME "TextFX"
//#define SUPPORT_PATH "NPPTextFX" /* thanks to smprintfpath we don't need to put a slash on the end of this */

#ifdef _MSC_VER
#define vsnprintf _vsnwprintf /* why doesn't Microsoft provide this now standard function? */
#endif
#if NPPDEBUG
#if (defined(__DMC__) || defined(_MSC_VER))
#define snprintf _snprintf /* snprintf is unsafe so we only want to use our snprintfX() */
#endif
#else
#define snprintf snprintf_unsafe
#endif
#define sprintf sprintf_unsafe
#define strncpy strncpy_unsafe

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

#ifndef NELEM
#define NELEM(xyzzy) (sizeof(xyzzy)/sizeof(xyzzy[0]))
#endif

// Wide char helper
#define CHARSIZE(size) (size_t)((size)*sizeof(TCHAR))

#ifndef NPPDEBUG
#error NPPDEBUG Must be defined as a -D option for all source files
#endif

// Scintilla message sender function pointer
EXTERNC LRESULT(*gScintillaMessageSender)(BOOL, UINT, WPARAM, LPARAM);
EXTERNC BOOL g_fOnNT;

EXTERNC void *ReallocFree(void *memblock, size_t size);
#if NPPDEBUG
EXTERNC void *freesafebounds(void *bf, unsigned ct, const TCHAR *title);
EXTERNC void *reallocsafebounds(void *bf, size_t ct);
EXTERNC void freesafe(void *bf, const TCHAR *title);
EXTERNC void *reallocsafeX(void *bf, unsigned ct, TCHAR *title, int allownull);

/* This will only be set by source code that promises to do this later */
#ifndef NPPTextFX_DELAYUNSAFE
#define mallocsafe(ct,ti) reallocsafeX(NULL,ct,ti,1)
#define reallocsafe(bf,ct,ti) reallocsafeX(bf,ct,ti,0)
#define reallocsafeNULL(bf,ct,ti) reallocsafeX(bf,ct,ti,1)
#define malloc malloc_unsafe
#define realloc realloc_unsafe
#define calloc calloc_unsafe /* I never use calloc() but just in case someone tries */
#define free free_unsafe
#define strdup strdup_unsafe
#endif

EXTERNC TCHAR *strdupsafe(const TCHAR *source, TCHAR *title);
#else
#define mallocsafe(ct,ti) malloc(ct)
#define reallocsafe(bf,ct,ti) ReallocFree(bf,ct)
#define reallocsafeNULL(bf,ct,ti) ReallocFree(bf,ct)
#define freesafe(bf,ti) free(bf)
#if defined(__POCC__) /* Pelles C */ || defined(_MSC_VER)
#define strdupsafe(bf,ti) _strdup(bf)
#else
#define strdupsafe(bf,ti) strdup(bf)
#endif
#endif

#if NPPDEBUG
EXTERNC TCHAR *memdupsafe(const TCHAR *source, unsigned ls, TCHAR *title);
#else
#define memdupsafe(bf,ln,ti) memdup(bf,ln)
EXTERNC TCHAR *memdup(const TCHAR *source, unsigned ls);
#endif

#if NPPDEBUG
EXTERNC TCHAR *memdupzsafe(const TCHAR *source, unsigned ls, TCHAR *title);
#else
#define memdupzsafe(bf,ln,ti) memdupz(bf,ln)
EXTERNC TCHAR *memdupz(const TCHAR *source, unsigned ls);
#endif

EXTERNC size_t roundtonextpower(size_t numo); /* round to next power */

#define ARMSTRATEGY_INCREASE 0 //increase buffer with slack space when necessary; good for a constantly expanding buffer
#define ARMSTRATEGY_MAINTAIN 1 //increase buffer only the minimum amount, buffer will not be reduced if too large; good for a buffer that will be reused with some large and some small allocations
#define ARMSTRATEGY_REDUCE 2   //increase buffer only the minimum amount, reduce buffer if too large; good for buffers of known size or that will only be used once
EXTERNC int armrealloc(TCHAR **dest, size_t *destsz, size_t newsize, int strategy, int clear
#undef THETITLE
#if NPPDEBUG
, TCHAR *title
#define armreallocsafe armrealloc
#define THETITLE title
#else
#define armreallocsafe(dt,ds,ns,st,cl,ti) armrealloc(dt,ds,ns,st,cl)
#define THETITLE "armrealloc" /* the macros discards this */
#endif
);

EXTERNC int strncpyarm(TCHAR **dest,size_t *destsz,size_t *destlen,const TCHAR *source,size_t maxlen
#undef THETITLE
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
);

EXTERNC int memcpyarm(void **dest,size_t *destsz,size_t *destlen,const TCHAR *source,size_t slen
#undef THETITLE
#if NPPDEBUG
, TCHAR *title
#define memcpyarmsafe memcpyarm
#define THETITLE title
#else
#define memcpyarmsafe(dt,ds,dl,st,ml,ti) memcpyarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
);

EXTERNC int memsetarm(TCHAR **dest,size_t *destsz,size_t *destlen,int chr,size_t slen
#undef THETITLE
#if NPPDEBUG
, TCHAR *title
#define memsetarmsafe memsetarm
#define THETITLE title
#else
#define memsetarmsafe(dt,ds,dl,st,ml,ti) memsetarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
);
#undef THETITLE

EXTERNC size_t snprintfX(TCHAR *buffer,size_t buffersz,const TCHAR *format,...);
EXTERNC size_t vsarmprintf(TCHAR **dest,size_t *destsz,size_t *destlen,const TCHAR *format,va_list ap2);
EXTERNC size_t sarmprintf(TCHAR **dest,size_t *destsz,size_t *destlen,const TCHAR *format,...);
EXTERNC TCHAR *smprintf(const TCHAR *format,...);
EXTERNC size_t vsarmprintfpath(TCHAR **dest, size_t *destsz, size_t *destlen,const TCHAR *format,va_list ap2);
EXTERNC size_t sarmprintfpath(TCHAR **dest,size_t *destsz,size_t *destlen,const TCHAR *format,...);
EXTERNC TCHAR *smprintfpath(const TCHAR *format,...);
EXTERNC int MessageBoxFree(HWND hWnd,TCHAR *lpText,LPCTSTR lpCaption,UINT uType);
EXTERNC int memmovearm(void **dest, size_t *destsz, size_t *destlen, TCHAR *destp, TCHAR *sourcep
#if NPPDEBUG
,int notest
#endif
);

#if NPPDEBUG
#define memmovearmtest memmovearm
#else
#define memmovearmtest(dt,ds,dl,dp,sp,nt) memmovearm(dt,ds,dl,dp,sp)
#endif

EXTERNC void memcqspnstart(const TCHAR *find, unsigned findl, unsigned *quick);
EXTERNC TCHAR *memcqspn(const TCHAR *buf, const TCHAR *end, const unsigned *quick);
EXTERNC TCHAR *memqspn(const TCHAR *buf, const TCHAR *end, const unsigned *quick);
EXTERNC TCHAR *memcspn(const TCHAR *buf, const TCHAR *end, const TCHAR *find, unsigned findl);
EXTERNC char *memcspn_chr(const char *buf, const char *end, const char *find, unsigned findl);
EXTERNC TCHAR *memspn(const TCHAR *buf, const TCHAR *end, const TCHAR *find, unsigned findl);
EXTERNC TCHAR *memstr(const TCHAR *buf, const TCHAR *end, const TCHAR *find, unsigned findl);
EXTERNC TCHAR *memchrX(const TCHAR *buf, const TCHAR *end, unsigned find);
EXTERNC TCHAR *strncpymem(TCHAR *szDest, size_t uDestSz, const TCHAR *sSource, unsigned uSourceLen);
#define strncpy strncpy_unsafe
TCHAR *strtokX(TCHAR *szStr, unsigned *puPos, const TCHAR *szDeli);
#define strtok strtok_unsafe
EXTERNC unsigned powcheck(unsigned num);

EXTERNC unsigned converteol(TCHAR **dest, size_t *destsz, size_t *destlen, unsigned eoltype);


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
EXTERNC DWORD _writeX(HANDLE handle, const void *buf, size_t len);
EXTERNC DWORD _readX(HANDLE handle,void *buf,DWORD len);
#define _closeX(hnd) CloseHandle(hnd)

EXTERNC TCHAR *findnextquote(TCHAR *str, TCHAR *end,unsigned style);

EXTERNC DWORD GetPrivateProfileStringarm(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, TCHAR **dest, size_t *destsz, LPCTSTR lpFileName);
EXTERNC BOOL WritePrivateProfileStringFree(LPCTSTR lpAppName,LPCTSTR lpKeyName,TCHAR *lpString,LPCTSTR lpFileName);

EXTERNC BOOL isFileExist(const TCHAR *fn);

/* examine shlobj.h for a valid range */
#define CSIDLX_TEXTFXDATA         0x00FE // Static Data provided with install
//#define CSIDLX_NOTEPADPLUSEXE     0x00FD
//#define CSIDLX_NOTEPADPLUSPLUGINS 0x00FC
#define CSIDLX_TEXTFXTEMP         0x00FB
#define CSIDLX_TEXTFXINIFILE      0x00FA
#define CSIDLX_USER               0x00FA /* set this to the lowest custom value */

EXTERNC unsigned NPPGetSpecialFolderLocationarm(int nFolder, const TCHAR *szName, TCHAR **pszFolder, size_t *puFolderSz, unsigned *puFolderLen, const TCHAR *);
EXTERNC BOOL XlatPathEnvVarsarm(TCHAR **dest, size_t *destsz, size_t *destlen);

EXTERNC unsigned tidyHTML(char ** dest, unsigned * destsz, unsigned * destlen, unsigned eoltype, unsigned tabwidth);


// See PluginInterface.h
#define PFUNCPLUGINCMD void __cdecl /* Some compilers cannot use the typedef in every case. This define works to alter function declarations with all compilers */


// SUBSECTION: Single line menu functions
EXTERNC PFUNCPLUGINCMD pfdummy(void);
EXTERNC PFUNCPLUGINCMD pfconvert1q2q(void);
EXTERNC PFUNCPLUGINCMD pfconvert2q1q(void);
EXTERNC PFUNCPLUGINCMD pfconvertqtswap(void);
EXTERNC PFUNCPLUGINCMD pfconvertqtdrop(void);
EXTERNC PFUNCPLUGINCMD pfconvertescapesq(void);
EXTERNC PFUNCPLUGINCMD pfconvertescape1qsq(void);
EXTERNC PFUNCPLUGINCMD pfconvertescape1qs1q(void);
EXTERNC PFUNCPLUGINCMD pfconvertescapeboth(void);
EXTERNC PFUNCPLUGINCMD pfconvertunescapesq(void);
EXTERNC PFUNCPLUGINCMD pfconvertunescapesq1q(void);
EXTERNC PFUNCPLUGINCMD pfconvertunescapes1q1q(void);
EXTERNC PFUNCPLUGINCMD pfconvertunescapeboth(void);
EXTERNC PFUNCPLUGINCMD pfconvertescape2q22q(void);
EXTERNC PFUNCPLUGINCMD pfconvertescape1q22q(void);
EXTERNC PFUNCPLUGINCMD pfconvertunescape22q2q(void);
EXTERNC PFUNCPLUGINCMD pfconvertunescape22q1q(void);
EXTERNC PFUNCPLUGINCMD pfconvertuppercase(void);
EXTERNC PFUNCPLUGINCMD pfconvertlowercase(void);
EXTERNC PFUNCPLUGINCMD pfconvertpropercase(void);
EXTERNC PFUNCPLUGINCMD pfconvertsentencecase(void);
EXTERNC PFUNCPLUGINCMD pfconvertinvertcase(void);
EXTERNC PFUNCPLUGINCMD pfencodeHTML(void);
EXTERNC PFUNCPLUGINCMD pfencodeURIcomponent(void);
EXTERNC PFUNCPLUGINCMD pfrot13(void);
EXTERNC PFUNCPLUGINCMD pfunwraptext(void);
EXTERNC PFUNCPLUGINCMD pfrewraptext(void);
EXTERNC PFUNCPLUGINCMD pflineupcomma(void);
EXTERNC PFUNCPLUGINCMD pflineupequals(void);
EXTERNC PFUNCPLUGINCMD pflineupclipboard(void);
EXTERNC PFUNCPLUGINCMD pfinsertclipboardcolumn(void);
EXTERNC PFUNCPLUGINCMD pfemptyundobuffer(void);
EXTERNC PFUNCPLUGINCMD pfsubmitHTML(void);
EXTERNC PFUNCPLUGINCMD pfsubmitCSS(void);
EXTERNC PFUNCPLUGINCMD pfdecimal2binary(void);
EXTERNC PFUNCPLUGINCMD pfdecimal2octal(void);
EXTERNC PFUNCPLUGINCMD pfdecimal2hex(void);
EXTERNC PFUNCPLUGINCMD pfhex2decimal(void);
EXTERNC PFUNCPLUGINCMD pfoctal2decimal(void);
EXTERNC PFUNCPLUGINCMD pfbinary2decimal(void);
EXTERNC PFUNCPLUGINCMD pfcnum2decimal(void);
EXTERNC PFUNCPLUGINCMD pfstripHTMLtags(void);
EXTERNC PFUNCPLUGINCMD pfstripHTMLnotabs(void);
EXTERNC PFUNCPLUGINCMD pfkillwhitenonqtvb(void);
EXTERNC PFUNCPLUGINCMD pfkillwhitenonqtc(void);
EXTERNC PFUNCPLUGINCMD pffindqtstringvb(void);
EXTERNC PFUNCPLUGINCMD pffindqtstringc(void);
EXTERNC PFUNCPLUGINCMD pffilldownover(void);
EXTERNC PFUNCPLUGINCMD pffilldownins(void);
EXTERNC PFUNCPLUGINCMD pfaddup(void);
EXTERNC PFUNCPLUGINCMD pfzapspace(void);
EXTERNC PFUNCPLUGINCMD pfzapnonprint(void);
EXTERNC PFUNCPLUGINCMD pfspace2tabs(void);
EXTERNC PFUNCPLUGINCMD pfspace2tabs8(void);
EXTERNC PFUNCPLUGINCMD pfindentlinessurround(void);
EXTERNC PFUNCPLUGINCMD pftrimtrailingspace(void);
EXTERNC PFUNCPLUGINCMD pfdeleteblanklines(void);
EXTERNC PFUNCPLUGINCMD pfdeleteblanklines2(void);
EXTERNC PFUNCPLUGINCMD pfreindentcode(void);
EXTERNC PFUNCPLUGINCMD pfascii2EBCDIC(void);
EXTERNC PFUNCPLUGINCMD pfEBCDIC2ascii(void);
EXTERNC PFUNCPLUGINCMD pfcp1251toKOI8_R(void);
EXTERNC PFUNCPLUGINCMD pfKOI8_Rtocp1251(void);
EXTERNC PFUNCPLUGINCMD pftohex16(void);
EXTERNC PFUNCPLUGINCMD pftohex32(void);
EXTERNC PFUNCPLUGINCMD pftohex64(void);
EXTERNC PFUNCPLUGINCMD pftohex128(void);
EXTERNC PFUNCPLUGINCMD pffromhex(void);
EXTERNC PFUNCPLUGINCMD pfwordcount(void);
EXTERNC PFUNCPLUGINCMD pfinsertlinenumbers(void);
EXTERNC PFUNCPLUGINCMD pfdeletefirstword(void);
EXTERNC PFUNCPLUGINCMD pfextendblockspaces(void);
EXTERNC PFUNCPLUGINCMD pfcleanemailquoting(void);
EXTERNC PFUNCPLUGINCMD pfuudecode(void);
EXTERNC PFUNCPLUGINCMD pfbase64decode(void);
EXTERNC PFUNCPLUGINCMD pfhexbyterunstolittlendian2(void);
EXTERNC PFUNCPLUGINCMD pfhexbyterunstolittlendian4(void);
EXTERNC PFUNCPLUGINCMD pflittlendiantohexbyteruns(void);
EXTERNC PFUNCPLUGINCMD pfconverteol(void);
EXTERNC PFUNCPLUGINCMD pfsplitlinesatchvb(void);
EXTERNC PFUNCPLUGINCMD pfsplitlinesatchc(void);
// SUBSECTION: END
