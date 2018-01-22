/**
 *  Class for storing errors from detector. Originally written for ccd cameras, but works for anything...
 *    
 *
 *@author Tim Madden
 *@date  June 2003
 *
 *
 */

/*
 * Include files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shareLib.h>
/*
 * Double incluson protection.
 */
#ifndef _CCD_EXCEPTION_H
#define _CCD_EXCEPTION_H

//#ifdef _WINDOWS
// #define EXPORT __declspec(dllexport)
//#else
// #define EXPORT

//#endif
/**
 *
 *    Class ccd_exception
 *
 *
 *    contains char* error passed as exception. not using std::string because
 *  it has questionable thread safety.
 *
 */

// class EXPORT ccd_exception
class epicsShareClass ccd_exception {
 public:
  enum error_code { ok, xfer_not_starting, not_implemented, unknown };

  // make err message mess
  ccd_exception(const char *mess);
  // make default err message
  ccd_exception(error_code er, const char *mess);
  // make default err message
  ccd_exception(error_code er);
  // make default err message
  ccd_exception();
  // Return error message.
  char *err_mess(void);

  // Return error message.
  error_code getErrCode(void);

 protected:
  // Error message in this exception.
  char err[256];

  error_code code;
};

#endif
