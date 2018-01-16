/*******************************************************************************
 *
 *    cl_com_port.h
 *
 *    Author: Tim Madden
 *    Date:    8/01/06
 *    Project: CCD Detector DAQ.
 *
 *
 ******************************************************************************/

/*
 * Include files.
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"

#include <shareLib.h>

/*
 * Double incluson protection.
 */
#ifndef _STOPWATCH_H
#define _STOPWATCH_H

/*
         * Class for
         */

class epicsShareClass stopWatch {
 public:
  stopWatch();

  void wait(int us);

  // for timing things. call tic. toc returns time in s since last tic.
  void tic();
  double toc();
  bool isElapsed(double sec);

 protected:
  double currenttime, elapsedtime;
};

#endif
