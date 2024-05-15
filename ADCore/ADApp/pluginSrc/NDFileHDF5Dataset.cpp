#include <iostream>
#include <stdlib.h>
#include <string.h>

#include <osiSock.h>

#include <hdf5_hl.h>

#include "NDFileHDF5Dataset.h"

#ifndef htonll
#define htonll(x) ( ( (uint64_t)(htonl( (uint32_t)(((uint64_t)x << 32) >> 32)))<< 32) | htonl( ((uint32_t)((uint64_t)x >> 32)) ))
#endif

static const char *fileName = "NDFileHDF5Dataset";

/** Constructor.
 * \param[in] pAsynUser - asynUser that is used to control debugging output
 * \param[in] name - String name of the dataset.
 * \param[in] dataset - HDF5 handle to the dataset.
 */
NDFileHDF5Dataset::NDFileHDF5Dataset(asynUser *pAsynUser, const std::string& name, hid_t dataset) :
                                     pAsynUser_(pAsynUser), name_(name), dataset_(dataset), nextRecord_(0)
{
  this->maxdims_     = NULL;
  this->dims_        = NULL;
  this->chunkdims_   = (hsize_t*) calloc(ND_ARRAY_MAX_DIMS, sizeof(hsize_t));
  this->offset_      = NULL;
  this->virtualdims_ = NULL;
  this->virtualchunkdims_ = NULL;
}

NDFileHDF5Dataset::~NDFileHDF5Dataset()
{
  if (this->chunkdims_   != NULL) free(this->chunkdims_);
  if (this->maxdims_     != NULL) free(this->maxdims_);
  if (this->dims_        != NULL) free(this->dims_);
  if (this->offset_      != NULL) free(this->offset_);
  if (this->virtualdims_ != NULL) free(this->virtualdims_);
  if (this->virtualchunkdims_ != NULL) free(this->virtualchunkdims_);
}

/** configureDims.
 * Setup any extra dimensions required for this dataset
 * \param[in] pArray - A pointer to an NDArray which contains dimension information.
 * \param[in] multiframe - Is this multiframe?
 * \param[in] extradimensions - The number of extra dimensions.
 * \param[in] extra_dims - The size of extra dimensions.
 * \param[in] extra_dim_chunking - Array of extra dimension chunking.
 * \param[in] user_chunking - Array of user defined chunking dimensions.
 */
asynStatus NDFileHDF5Dataset::configureDims(NDArray *pArray, bool multiframe, int extradimensions, int *extra_dims, int *extra_dim_chunking, int *user_chunking)
{
  int i=0,j=0, extradims = 0, ndims=0;
  asynStatus status = asynSuccess;

  extradims = extradimensions;

  ndims = pArray->ndims + extradims;

  // Store chunk dimensions
  for (i=0; i<=pArray->ndims; i++) {
    this->chunkdims_[i] = user_chunking[i];
  }

  // first check whether the dimension arrays have been allocated
  // or the number of dimensions have changed.
  // If necessary free and reallocate new memory.
  if (this->maxdims_ == NULL || this->rank_ != ndims){
    if (this->maxdims_          != NULL) free(this->maxdims_);
    if (this->dims_             != NULL) free(this->dims_);
    if (this->offset_           != NULL) free(this->offset_);
    if (this->virtualdims_      != NULL) free(this->virtualdims_);
    if (this->virtualchunkdims_ != NULL) free(this->virtualchunkdims_);

    this->maxdims_          = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->dims_             = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->offset_           = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->virtualdims_      = (hsize_t*)calloc(extradims, sizeof(hsize_t));
    this->virtualchunkdims_ = (hsize_t*)calloc(extradims, sizeof(hsize_t));
  }

  if (multiframe){
    this->multiFrame_ = true;
    // Configure the virtual dimensions -i.e. dimensions in addition to the frame format.
    // Normally set to just 1 by default or -1 unlimited (in HDF5 terms)
    for (i=0; i<extradims; i++){
      this->maxdims_[i]     = H5S_UNLIMITED;
      this->dims_[i]        = 1;
      this->offset_[i]      = 0; // because we increment offset *before* each write we need to start at -1
      this->virtualdims_[i] = extra_dims[i];
      this->virtualchunkdims_[i] = extra_dim_chunking[i];
    }
  } else {
    this->multiFrame_ = false;
  }

  this->rank_ = ndims;
  this->extra_rank_ = extradims;

  for (j=pArray->ndims-1,i=extradims; i<this->rank_; i++,j--){
    this->maxdims_[i]    = pArray->dims[j].size;
    this->dims_[i]       = pArray->dims[j].size;
    this->offset_[i]     = 0;
  }
  return status;
}

/** extendDataSet.
 * Extend this dataset as necessary.  If no extra dimensions are specified
 * then the dataset is simply increased in the frame number direction.
 * \param[in] extradims - The number of extra dimensions.
 */
asynStatus NDFileHDF5Dataset::extendDataSet(int extradims)
{
  int i=0;
  bool growdims = true;
  bool growoffset = true;

  // Add the n'th frame dimension (for multiple frames per scan point)
  extradims += 1;

  // first frame already has the offsets and dimensions preconfigured so
  // we dont need to increment anything here
  if (this->nextRecord_ == 0) return asynSuccess;

  // in the simple case where dont use the extra X,Y dimensions we
  // just increment the n'th frame number
  if (extradims == 1){
    this->dims_[0]++;
    this->offset_[0]++;
    return asynSuccess;
  }

  // run through the virtual dimensions in reverse order: n,X,Y
  // and increment, reset or ignore the offset of each dimension.
  for (i=extradims-1; i>=0; i--){
    if (this->dims_[i] == this->virtualdims_[i]) growdims = false;

    if (growoffset){
      this->offset_[i]++;
      growoffset = false;
    }

    if (growdims){
      if (this->dims_[i] < this->virtualdims_[i]) {
        this->dims_[i]++;
        growdims = false;
      }
    }

    if (this->offset_[i] == this->virtualdims_[i]) {
      this->offset_[i] = 0;
      growoffset = true;
      growdims = true;
    }
  }
  return asynSuccess;
}

asynStatus NDFileHDF5Dataset::extendDataSet(int extradims, hsize_t *offsets)
{
  asynStatus status = asynSuccess;
  static const char *functionName = "extendDataSet";

  // If this method has been called then we are being asked to extend to a particular index
  for (int index = 0; index <= extradims; index++){
    // Check the requested offset is not outside the dimension maximum
    if (offsets[index] < this->virtualdims_[index]){
      if (this->dims_[index] < offsets[index]+1){
        // Increase the dimension to accomodate the new position
        this->dims_[index] = offsets[index]+1;
      }
      // Always set the offset position even if we don't increase the dims
      this->offset_[index] = offsets[index];
    } else {
      // We have been unable to extend, this is a bad position request
      asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
                "%s::%s ERROR extending the dataset [%s] failed\n",
                fileName, functionName, this->name_.c_str());
      status = asynError;
    }
  }
  return status;
}

/**
 * Check if pArray dimensions and codec match hdf5 dataset definition
 * \param[in] pArray - The NDArray containing the data to verify.
 */
asynStatus NDFileHDF5Dataset::verifyChunking(NDArray *pArray)
{
  // If compression is enabled, it must match our configuration
  if (!pArray->codec.empty() || !this->codec.empty()) {
    if (pArray->codec != this->codec) {
      asynPrint(this->pAsynUser_, ASYN_TRACE_FLOW,
                "Dataset codec '%s' [%d, %d, %d] does not match NDArray codec '%s' [%d, %d, %d]\n",
                this->codec.name.c_str(), this->codec.level, this->codec.shuffle, this->codec.compressor,
                pArray->codec.name.c_str(), pArray->codec.level, pArray->codec.shuffle, pArray->codec.compressor);
      return asynError;
    }
  }
  bool mismatch = false;
  if (this->multiFrame_) {
    // If chunk spans multiple frames (or multiple rows of a 1D dataset), then we require the HDF5
    // processing pipeline to stitch together the NDArrays
    if (this->chunkdims_[pArray->ndims] != 1) {
      mismatch = true;
    }
  }
  // Remaining dimensions must match chunk definition
  for (int index = 0; index < pArray->ndims; index++) {
    if (pArray->dims[index].size != this->chunkdims_[index]) {
      mismatch = true;
    }
  }
  if (mismatch) {
    asynPrint(this->pAsynUser_, ASYN_TRACE_FLOW,
              "Dataset chunk dimensions [%d, %d, %d] do not match Array dimensions [%d, %d]\n",
              (int)this->chunkdims_[0], (int)this->chunkdims_[1], (int)this->chunkdims_[2],
              (int)pArray->dims[0].size, (int)pArray->dims[1].size);
      return asynError;
  }
  // Final check to make sure all extra dimension chunk sizes are set to 1 (or 0 which would default to 1)
  // All extra dimension chunk sizes must be set to 0 or 1 if we are going to use the direct chunk write.
  for (int index = 0; index < this->extra_rank_; index++) {
    if (this->virtualchunkdims_[index] > 1) {
      // Chunk size is not set to 1 so we cannot use direct chunk write
      return asynError;
    }
  }
  return asynSuccess;
}

/**
 * Store codec definition
 * \param[in] codec - Codec definition.
 */
void NDFileHDF5Dataset::configureCompression(Codec_t codec)
{
  this->codec = codec;
}

/** writeFile.
 * Write the data using the HDF5 library calls.
 * \param[in] pArray - The NDArray containing the data to write.
 * \param[in] datatype - The HDF5 datatype of the data.
 * \param[in] dataspace - A handle to the HDF5 dataspace for this dataset.
 * \param[in] framesize - The size of the data to write.
 */
asynStatus NDFileHDF5Dataset::writeFile(NDArray *pArray, hid_t datatype, hid_t dataspace, hsize_t *framesize)
{
  herr_t hdfstatus;
  static const char *functionName = "writeFile";

  // Increase the size of the dataset
  asynPrint(this->pAsynUser_, ASYN_TRACE_FLOW,
            "%s::%s: set_extent dims={%d,%d,%d}\n",
            fileName, functionName, (int)this->dims_[0], (int)this->dims_[1], (int)this->dims_[2]);

  hdfstatus = H5Dset_extent(this->dataset_, this->dims_);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
              "%s::%s ERROR Increasing the size of the dataset [%s] failed\n",
              fileName, functionName, this->name_.c_str());
    return asynError;
  }
  // Select a hyperslab.
  hid_t fspace = H5Dget_space(this->dataset_);
  if (fspace < 0){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
              "%s::%s ERROR Unable to get a copy of the dataspace for dataset [%s]\n",
              fileName, functionName, this->name_.c_str());
    return asynError;
  }
  hdfstatus = H5Sselect_hyperslab(fspace, H5S_SELECT_SET, this->offset_, NULL, framesize, NULL);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
              "%s::%s ERROR Unable to select hyperslab\n",
              fileName, functionName);
    H5Sclose(fspace);
    return asynError;
  }

  // Write the data to the hyperslab.
  if (H5_VERSION_GE(1, 8, 11) && verifyChunking(pArray) == asynSuccess) {
    // The chunking and compression settings match - use direct chunk write
    asynPrint(this->pAsynUser_, ASYN_TRACE_FLOW,
              "%s::%s NDArray correctly chunked. Using direct chunk write\n",
              fileName, functionName);
    size_t size = pArray->compressedSize;
    void *pData = pArray->pData;
    char *temp=0;
    NDArrayInfo_t info;
    pArray->getInfo(&info);
    if (pArray->codec.empty()) {
        size = info.totalBytes;
    }
    else if (pArray->codec.name == codecName[NDCODEC_LZ4]) {
        // We need to add a 16-byte header to the lz4 compressed data
        temp = (char *)malloc(16 + size);
        // First 8 bytes is the uncompressed array size
        unsigned long long ui64 = htonll(info.totalBytes);
        memcpy(temp, &ui64, 8);
        // Next 4 bytes is the block size = uncompressed size as long as < 1GB which we assume here
        epicsUInt32 ui32 = htonl((int)info.totalBytes);
        memcpy(temp+8, &ui32, 4);
        // Next 4 bytes is the compressed size
        ui32 = htonl((int)size);
        memcpy(temp+12, &ui32, 4);
        // Now copy the data
        memcpy(temp+16, pArray->pData, size);
        pData = temp;
        size += 16;
    }
    else if (pArray->codec.name == codecName[NDCODEC_BSLZ4]) {
        // We need to add a 12-byte header to the bs/lz4 compressed data
        temp = (char *)malloc(12 + size);
        // First 8 bytes is the uncompressed array size
        unsigned long long ui64 = htonll(info.totalBytes);
        memcpy(temp, &ui64, 8);
        // Next 4 bytes is the block size * elem_size;  8192 is the default in bitshuffle
        epicsUInt32 ui32 = htonl(8192);
        memcpy(temp+8, &ui32, 4);
        // Now copy the data
        memcpy(temp+12, pArray->pData, size);
        pData = temp;
        size += 12;
    }
    #if H5_VERSION_GE(1, 10, 3)
    hdfstatus = H5Dwrite_chunk(this->dataset_, H5P_DEFAULT, 0x0,
                               this->offset_, size, pData);
    #else  // Use deprecated method
    hdfstatus = H5DOwrite_chunk(this->dataset_, H5P_DEFAULT, 0x0,
                                this->offset_, size, pData);
    #endif
    if (temp) {
        free(temp);
    }
  } else {
    // Either direct chunk write is not available, or we need to use the HDF5 pipeline for
    // compression / chunk buffering - use standard write method
    if (!pArray->codec.empty()) {
      // We can't use the standard write method for pre-compressed data
      asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
                "%s::%s ERROR Unable to write pre-compressed data - mismatched chunk definition\n",
                fileName, functionName);
      H5Sclose(fspace);
      return asynError;
    }
    asynPrint(this->pAsynUser_, ASYN_TRACE_FLOW,
              "%s::%s NDArray not correctly chunked. Using standard write\n",
              fileName, functionName);
    hdfstatus = H5Dwrite(this->dataset_, datatype, dataspace, fspace, H5P_DEFAULT, pArray->pData);
  }

  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
              "%s::%s ERROR Unable to write data to hyperslab\n",
              fileName, functionName);
    H5Sclose(fspace);
    return asynError;
  }

  hdfstatus = H5Sclose(fspace);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
              "%s::%s ERROR Unable to close the dataspace\n",
              fileName, functionName);
    return asynError;
  }

  this->nextRecord_++;

  return asynSuccess;
}

/** getHandle.
 * Returns the HDF5 handle to this dataset.
 */
hid_t NDFileHDF5Dataset::getHandle()
{
  return this->dataset_;
}

asynStatus NDFileHDF5Dataset::flushDataset()
{
  static const char *functionName = "flushDataset";
  // flushDataset is a no-op if the HDF version doesn't support it
  #if H5_VERSION_GE(1,9,178)

  herr_t hdfstatus;

  // Flush the dataset
  hdfstatus = H5Dflush(this->dataset_);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
              "%s::%s ERROR Unable to flush the dataset [%s]\n",
              fileName, functionName, this->name_.c_str());
    return asynError;
  }
  #else
  // If this is called when we do not support SWMR then someone has done something
  // bad, so return an asynError
  asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR,
            "%s::%s SWMR dataset flush attempted but the library compiled against doesn't support it.\n",
            fileName, functionName);
  return asynError;
  #endif

  return asynSuccess;
}

/** Return the requested dimension size.
  * \param[in] index of dimension
  * \return size of the dimension
  */
hsize_t NDFileHDF5Dataset::getDim(int index)
{
  hsize_t value = -1;
  if (index >= 0 && index < this->rank_){
    value = this->dims_[index];
  }
  return value;
}

/** Return the requested max dimension size.
  * \param[in] index of dimension
  * \return size of the dimension
  */
hsize_t NDFileHDF5Dataset::getMaxDim(int index)
{
  hsize_t value = -1;
  if (index >= 0 && index < this->rank_){
    value = this->maxdims_[index];
  }
  return value;
}

/** Return the requested offset size.
  * \param[in] index of offset
  * \return size of the offset
  */
hsize_t NDFileHDF5Dataset::getOffset(int index)
{
  hsize_t value = -1;
  if (index >= 0 && index < this->rank_){
    value = this->offset_[index];
  }
  return value;
}

/** Return the requested virtual dimension size.
  * \param[in] index of dimension
  * \return size of the dimension
  */
hsize_t NDFileHDF5Dataset::getVirtualDim(int index)
{
  hsize_t value = -1;
  if (index >= 0 && index < this->extra_rank_){
    value = this->virtualdims_[index];
  }
  return value;
}


