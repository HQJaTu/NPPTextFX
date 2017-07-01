struct MICROXML {
//public: vars do not start with _
  unsigned uLevel;
  BOOL iUsed;
  TCHAR *sValue;
  unsigned uValueLen; // (almost) always points into _sBuf
  TCHAR *sValueConverted;
  unsigned uValueConvertedSz,uValueConvertedLen; // NULL or a malloc()'d string with the converted contents of sValue
  TCHAR *sAttribute;
  unsigned uAttributeLen; // always points into _sBuf
  TCHAR *sPath;
  unsigned uPathSz;
  unsigned uPathLen;
  TCHAR *sLeaf; // always points into sXMLPath
  unsigned uLeafLen;
  unsigned cQuote;
  TCHAR sVersion[16];
  TCHAR sEncoding[16];
  //void *vUserData;    // caller defined value
  //unsigned uUserData; // caller defined value
//private: vars start with _
  TCHAR *_psBufBegin,*_psBufEnd; // these point within _sBuf
  TCHAR *_sBuf;
  unsigned _uSbufSz;
  HANDLE _hFileHandle;
  int _iState;
  int _iLevelDeferred;
};

/* both states and return values */
#define MX_FAIL      0 /* XML is invalid */
#define MX_DONE      1 /* end of file */
#define MX_LEVELDOWN 2 /* returns sValue=exiting sLeaf (sLeaf is now the previous leaf), returns NULL for self closing leaf (sAttribute=NULL) */
#define MX_LEVELDOWNSAME 3 /* same as above but indicates specially that there were no sub-tags */
#define MX_DOWNLIMIT 3 /* values <= this mean the end of the current level */
#define MX_LEVELUP   4 /* returns sValue=sLeaf=newly entered leaf */
#define MX_ATTRIBUTE 5 /* returns sAttribute='sValue' */
#define MX_TEXT      6 /* returns sValue (sAttribute=NULL) */
#define MX_COMMENT   7 /* returns sValue (sAttribute=NULL) */

#define MX_ATTRIBUTESPECIAL 255 /* This is a state allowing <?xml ... ?>, this is never returned as a value */

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

EXTERNC int XMLRead(struct MICROXML *pmx);
EXTERNC BOOL XMLReadOpen(struct MICROXML *pmx,const TCHAR *fn,DWORD oflags,DWORD shflags);
EXTERNC void XMLReadClose(struct MICROXML *pmx);

EXTERNC void XMLWriteClose(HANDLE fo);
EXTERNC void XMLWriteLevelDown(HANDLE fo,unsigned uLevel);
EXTERNC void XMLWriteLevelUp(HANDLE fo, unsigned uLevel, const TCHAR *sTag, size_t uTagLen);
EXTERNC void XMLWriteLevelDownSame(HANDLE fo,const TCHAR *sTag,unsigned uTagLen);
EXTERNC void XMLWriteAttribute(HANDLE fo,const TCHAR *sAttribute,unsigned uAttributeLen,const TCHAR *sValue,unsigned uValueLen,unsigned cQuote);
EXTERNC void XMLWriteText(HANDLE fo,const TCHAR *sValue, unsigned uValueLen,const TCHAR *szPfxSfx);
EXTERNC void XMLWriteComment(HANDLE fo,unsigned uLevel, const TCHAR *sValue, unsigned uValueLen);
EXTERNC HANDLE XMLWriteCreat(const TCHAR *szfn, const TCHAR *szVersion, const TCHAR *szEncoding);
#if NPPDEBUG
EXTERNC void XMLTest(const TCHAR *fn1, const TCHAR *fn2);
#endif
EXTERNC int XMLReadLevel(struct MICROXML *pmx,unsigned uLevel,int iXMLReadOld);
