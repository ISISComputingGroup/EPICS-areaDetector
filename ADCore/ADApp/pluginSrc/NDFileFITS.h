/*
 * NDFileFITS.h
 * Writes NDArrays to FITS files.
 * Gabriele Salvato
 * March, 2014
 */

#ifndef DRV_NDFileFITS_H
#define DRV_NDFileFITS_H

#include "NDPluginFile.h"
#include <fitsio.h> // as modified by (Gabriele Salvato) see comments inside the file.

#define NDFITSFileHeaderFullPathnameString  "FITS_FILE_HEADER_FULL_PATHNAME"

#define MAX_PATH 256

/** Writes NDArrays in the FITS file format
  */
class NDFileFITS : public NDPluginFile {
public:
    NDFileFITS(const char *portName, int queueSize, int blockingCallbacks,
               const char *NDArrayPort, int NDArrayAddr,
               int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

protected:
    int NDFITSFileHeaderFullPathname;
    #define FIRST_NDFILE_FITS_PARAM NDFITSFileHeaderFullPathname
    #define LAST_NDFILE_FITS_PARAM NDFITSFileHeaderFullPathname

private:
  asynStatus WriteKeys(fitsfile *fitsFilePtr, int* fitsStatus);
  char fits_status_str[FLEN_STATUS];
  fitsfile *fitsFilePtr;
  int fitsStatus;

};
#define NUM_NDFILE_FITS_PARAMS ((int)(&LAST_NDFILE_FITS_PARAM - &FIRST_NDFILE_FITS_PARAM + 1))

#endif
