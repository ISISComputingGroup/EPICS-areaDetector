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

 #include <epicsExport.h>
#include "ccd_exception.h"

/**
 * Constructor. default, puts "error" as a message. code is enum unmknown. 
 */
 
ccd_exception::ccd_exception() {
  strcpy(err, "error");
  code = unknown;
}

/**
 * Constructor to spec. error string and code as int. 
 * @param   er      enum or int for error code. user picks it.
 * @param   mess    Some string to put into error message, like "camera error blah blah.."
 */
 
ccd_exception::ccd_exception(error_code er, const char *mess) {
  strcpy(err, mess);
  code = er;
}

/**
 * construcor for speced error code. 
 * @param   er      enum or int for error code. user picks it.
 */
 
ccd_exception::ccd_exception(error_code er) { code = er; }

/**
 * Constructor to spec error string. . 
 * @param   mess    Some string to put into error message, like "camera error blah blah.."
 */
 
ccd_exception::ccd_exception(const char *x) {
  strcpy(err, x);
  code = unknown;
}

/**
 * Return error message. 
 * @return string in the error exception.
 */
 
char *ccd_exception::err_mess(void) { return err; }

/**
 * return error code.
 * @return  int that is error code.
 */
 
ccd_exception::error_code ccd_exception::getErrCode(void) { return code; }
