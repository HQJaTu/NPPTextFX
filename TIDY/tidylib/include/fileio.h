#ifndef __FILEIO_H__
#define __FILEIO_H__

/** @file fileio.h - does standard C I/O

  Implementation of a FILE* based TidyInputSource and 
  TidyOutputSink.

  (c) 1998-2006 (W3C) MIT, ERCIM, Keio University
  See tidy.h for the copyright notice.

  CVS Info:
    $Author: arnaud02 $ 
    $Date: 2006/01/25 14:17:34 $ 
    $Revision: 1.6 $ 
*/

#include "buffio.h"
#ifdef __cplusplus
extern "C" {
#endif

/** Allocate and initialize file input source */
void TIDY_CALL initFileSource( TidyInputSource* source, FILE* fp );

/** Free file input source */
void TIDY_CALL freeFileSource( TidyInputSource* source, Bool closeIt );

/** Initialize file output sink */
void TIDY_CALL initFileSink( TidyOutputSink* sink, FILE* fp );

/* Needed for internal declarations */
void TIDY_CALL filesink_putByte( void* sinkData, byte bv );

#ifdef __cplusplus
}
#endif
#endif /* __FILEIO_H__ */
