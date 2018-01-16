/**
 *
 *    Provide a log file to screen and files. writes log messages to fie with date and times. 
 *
 *@author  Tim Madden
 *@date 2003
 *
 */

/*
 * Include files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shareLib.h>

//let logfile.cpp use asynPrint instead of printf
#ifdef LOGFILE_USE_ASYN
#include "asynDriver.h"
#endif

/*
 * Double incluson protection.
 */
#ifndef _LOG_FILE_H
#define _LOG_FILE_H

/**
 *
 *    Class LOG_FILE
 *
 */
 

class epicsShareClass log_file {
 public:
  // make file object.
  log_file(char* filename);

  // destroy file object.
  ~log_file();

  void enableLog(bool is_en);
  // output to log ile.
  void log(char* message);
  void logNoDate(char* message);
  void puts(char* message, int len);
  void enablePrintf(bool is_pr);
  #ifdef LOGFILE_USE_ASYN
  void setAsynUser(asynUser *pasynUser);
  #endif
  
 protected:
  enum { num_saved_files = 5 };

  // file pointer
  FILE* fp;
  char log_file_name[255];
  bool is_enabled;
  bool is_printf;
  
   #ifdef LOGFILE_USE_ASYN
   // pointer to asyn user, so we can use asynPrint
   asynUser *pasynUserSelf;
   #endif
  
};

#endif
