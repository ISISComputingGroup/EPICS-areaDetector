/**
 * Class that inherits grabberIntefface. This class hides all Dalsa grabber API calls into an interface.
 * use when building for Dalsa Grabber. Define USE_SAP in makefile to build AD driver for dalsa grabber.
 *
 *@author timothy madden
 *@date 2006
 *
 */
 

// AncDemoRotationExpDlg.cpp : implementation file
//

#ifdef USE_SAP

//#include "stdafx.h"
#include <iostream>

#include "coreco.h"

#include "ccd_exception.h"

static char *g_pDesignStr = EXPANSION_RTPRO_ROTATION_STR;

volatile long coreco::frames_to_cpu = 0;
volatile long coreco::frame_count = 0;

volatile long coreco::missed_frames = 0;
volatile bool coreco::is_missed_frame = false;
volatile int coreco::sap_buffer_count = 0;
volatile long coreco::recent_missed_frames = 0;

// genCamController* coreco::cam_control=0;

coreco *coreco::mycard = 0;

/////////////////////////////////////////////////////////////////////////////
// corecoFPGA dialog


/**
 * Open new dalsa (coreco) grabber, init grabber. if grabber has fpga on board spec if we use it.
 * if no fpga, set to false. 
 * @param   is_use_fpga True to use on board fpga. Most grabbers not have one.
 */
 
coreco::coreco(bool is_use_fpga) {
  //    m_bEnableRtPro = is_use_fpga;
  m_Xfer = NULL;
  m_Acq = NULL;
  m_Buffers = NULL;
  // m_View            = NULL;

  m_View = NULL;
  m_View2 = NULL;

  m_IsSignalDetected = TRUE;
  is_force_size = false;

  is_rst_server = false;
  // m_RtproDesign = NULL;
  // setFpgaFileName("null");
  setConfigFileName("D:/corecofiles/c1k1k16bit.ccf");

  is_double_width = false;

  coreco::frames_to_cpu = 0;
  coreco::frame_count = 0;

  coreco::missed_frames = 0;
  coreco::is_missed_frame = false;
  coreco::sap_buffer_count = 0;
  coreco::recent_missed_frames = 0;

  num_buffers = 2;
  is_destroyed = true;

  coreco::mycard = this;
}

/**
 * Return enum spec'ing grabber vendor. 
 * @return  Enum for whic grabber type, like dalsa or sisw. whch company made grabber card.
 */
 
int coreco::getGrabberType() { return ((int)gDALSA); }


/**
 * If vendor has gui image dusplay, then create it on screen. often vendor has lib to do this. 
 */
 
void coreco::makeView(void) {
  m_View = new SapView(m_Buffers);
  m_View->Create();
}

/**
 *  set num memory buffers (num images) that grabber stores in memory. 
 * @param   b   Num mem buff to make.
 */
 
void coreco::setNumBuffers(int b) { num_buffers = b; }

/**
 * return num buffers in grabber that are free.  
 * @return num buffers in grabber that are free.  
 */
 
 int coreco::getNumFreeBuffers(void) {
  return (sap_buffer_count - (frame_count - frames_to_cpu));
}

/**
 * return num buffers in grabber .  
 * @return  num buffers in grabber .  
 */
 

int coreco::getNumBuffers(void) { return (sap_buffer_count); }

/**
 * return true of new frame is available. else return false.  
 * @return  T of frame available, else F.
 */
 
bool coreco::isFrameAvailable(void) {
  if (frame_count > frames_to_cpu) return true;

  return false;
}

/**
 * sime grabbers have double width mode, where if we have 1k image size in x direction , we have to set to 2x.
 * this has to do with data size of pixels in the grabber.  
 * @param  isdw 1 or 0 for using double width mode. only SISW in 10 tap mode need this. 
 */
 
void coreco::setDoubleWidth(int isdw) { is_double_width = isdw; }


/**
 * return true if grabber missed a frame. else false.
 * @return  true if grabber missed a frame. else false.
 */
 
bool coreco::isMissedFrame(void) { return (is_missed_frame); }

/**
 * clear flags for missed frames.  
 */
 
void coreco::clearMissedFrames(void) {
  frames_to_cpu = frame_count;
  is_missed_frame = false;
  recent_missed_frames = 0;
}

/**
 * return num missed frames since grabber was initialized.  
 * @return  num missed frames since grabber was initialized
 */
 
long coreco::getTotalMissedFrames(void) { return (missed_frames); }

/**
 * return num missed frames since we last checked and cleared flags. 
 * @return  num missed frames since we last checked and cleared flags. 
 */
 
long coreco::getRecentMissedFrames(void) { return (recent_missed_frames); }
//


/**
 * get latest frame fram camera.  
 * @param   copy_memory Mem in which to put image.
 * @param   coreco_timestamp    int in which to put HW timestamp from grabber.
 * @return  true of we got image correctly, lse false.
 */
 
bool coreco::getFrame(void *copy_memory, unsigned int *coreco_timestamp) {
  // bool sap_result;
  // int grab_index;
  // unsigned short  *image_address;

  if (isFrameAvailable()) {
    // getIndex returns last buffer camera dumped to. not the next one to read
    // from...
    // grab_index = m_Buffers->GetIndex();
    grab_index = (grab_index + 1) % sap_buffer_count;

    sap_result = m_Buffers->GetAddress(grab_index, (void **)&image_address);
    m_Buffers->GetCounterStamp(grab_index, (int *)coreco_timestamp);

    memcpy(copy_memory, image_address,
           (sensor_height * sensor_width * sizeof(unsigned short)));
    frames_to_cpu++;
    sap_result = m_Buffers->ReleaseAddress(grab_index, (void *)image_address);
    if (sap_result == false) throw ccd_exception("release Address failed");

    if (sap_result == false) throw ccd_exception("SAP getAddress failed");
  }

  return false;
}

/**
 *  clear frames and mem buffers. 
 */
 
void coreco::resetBufferCount(void) {
  clearMissedFrames();
  m_Buffers->ResetIndex();
  grab_index = m_Buffers->GetIndex();
}


/**
 * get latest frame, supply mem to copy, get time stamp in to supplied mem. tell max num bytes. to return. 
 * @param   copy_memory Mem in which to put new frame
 * @param   coreco_timestamp    int in which to put HW timestamp
 * @param   nbytes  max num bytes to copy
 * @return  true of success, else false.
 */
 
bool coreco::getFrame(void *copy_memory, unsigned int *coreco_timestamp,
                      int nbytes) {
  //    bool sap_result;
  //    int grab_index;
  //    unsigned short  *image_address;

  if (isFrameAvailable()) {
    // grab_index = m_Buffers->GetIndex();
    grab_index = (grab_index + 1) % sap_buffer_count;

    sap_result = m_Buffers->GetAddress(grab_index, (void **)&image_address);
    m_Buffers->GetCounterStamp(grab_index, (int *)coreco_timestamp);

    memcpy(copy_memory, image_address, nbytes);
    frames_to_cpu++;
    sap_result = m_Buffers->ReleaseAddress(grab_index, (void *)image_address);
    if (sap_result == false) throw ccd_exception("release Address failed");

    if (sap_result == false) throw ccd_exception("SAP getAddress failed");
  }

  return false;
}

/**
 * get lastes frame into supplied memroy.  
 * @param   copy_memory mem in  which to put new frame
 * @return  t or f based on success.
 */
 
bool coreco::getFrame(void *copy_memory) {
  ///    bool sap_result;
  //    int grab_index;
  //    unsigned short  *image_address;

  if (isFrameAvailable()) {
    // grab_index = m_Buffers->GetIndex();
    grab_index = (grab_index + 1) % sap_buffer_count;

    sap_result = m_Buffers->GetAddress(grab_index, (void **)&image_address);

    if (copy_memory != 0)
      memcpy(copy_memory, image_address,
             (sensor_height * sensor_width * sizeof(unsigned short)));

    frames_to_cpu++;
    sap_result = m_Buffers->ReleaseAddress(grab_index, (void *)image_address);
    if (sap_result == false) throw ccd_exception("release Address failed");

    if (sap_result == false) throw ccd_exception("SAP getAddress failed");
  }

  return false;
}
/////////////////////////////////////////////////////////////////////////////
// corecoFPGA message handlers


/**
 * when frame comes in to grabber, this callback uis called. Called by vendor grabber thread
* under the hood. this callback passed to vendor driver.  
 * @param   pInfo SapXferCallbackInfo obj.
 */
 
void coreco::XferCallback(SapXferCallbackInfo *pInfo) {
  //!!    corecoFPGA *pDlg= (corecoFPGA *) pInfo->GetContext();
  int i;

  i = 1;
  frame_count++;
  if (frame_count - frames_to_cpu > sap_buffer_count) {
    is_missed_frame = true;
    missed_frames += 1;
    recent_missed_frames += 1;
  }
  if (mycard->m_View != NULL) mycard->m_View->Show();

  // if (cam_control!= 0)
  //    cam_control->imageCallback(NULL);
}

/**
 * callback when signal changes on grabber like clock etc. 
 * @param   SapAcqCallbackInfo obj.
 */
 
void coreco::SignalCallback(SapAcqCallbackInfo *pInfo) {
  //    corecoFPGA *pDlg = (corecoFPGA *) pInfo->GetContext();
  //  pDlg->GetSignalStatus(pInfo->GetSignalStatus());
  // pInfo->GetSignalStatus()
  int i;

  i = 1;
}

/**
 * deprecated 
 */
 
void coreco::setCamController(void *cc) {
  // cam_control=(genCamController*)cc;
}


/**
 * get image width. 
 * @return  image width
 */
 
int coreco::getWidth(void) { return (sensor_width); }

/**
 * get image height 
 * @return  image height 
 */
 
int coreco::getHeight(void) { return (sensor_height); }

//***********************************************************************************
// Initialize Demo Dialog based application
//***********************************************************************************


/**
 * Init the grabber for x and y size images.  reads the config file set in object.
 * @param  size_x  x size of image. 
 * @param  size_y  y size of image.
 * @return  true on success
 * 
 */
 
bool coreco::initialize(int size_x, int size_y) {
  // if (dlg.DoModal() == IDOK)
  //{
  // Define on-line objects
  m_Acq = makeAcquision();
  //        m_Buffers    = new SapBufferWithTrash(2, m_Acq);
  m_Buffers = new SapBuffer(num_buffers, m_Acq);
  m_Buffers->SetFormat(SapFormatMono16);
  m_Buffers->SetHeight(size_y);
  m_Buffers->SetWidth(size_x);
  m_Buffers->SetType(SapBuffer::TypeContiguous);

  this->sensor_height = size_y;
  this->sensor_width = size_x;

  m_Xfer = new SapTransfer(XferCallback, this);
  //        if (m_bEnableRtPro)
  //            m_RtproDesign = NewRtproDesign( m_Acq->GetLocation());

  // Create all objects
  if (!CreateObjects()) {
    return false;
  }

  grab_index = m_Buffers->GetIndex();
  // Get current input signal connection status
  GetSignalStatus();
  m_Acq->SaveParameters("D:/corecofiles/current_params.ccf");

  return TRUE;  // return TRUE  unless you set the focus to a control
}

/**
 * We generally use this one. sets uip gravbber and forces img size to x and y.  
 * @param  size_x  x size of image
 * @param  size_y  y size of image
 * @param   is_force_size  force image size. we ususlly set to true. ignore any other settigs on config file re image size.
 * @return  true of success.
 */
 
bool coreco::initialize(int size_x, int size_y, bool is_force_size) {
  // if (dlg.DoModal() == IDOK)
  //{
  // Define on-line objects
  m_Acq = makeAcquision();

  this->is_force_size = is_force_size;

  //        m_Buffers    = new SapBufferWithTrash(2, m_Acq);
  m_Buffers = new SapBuffer(num_buffers, m_Acq);
  m_Buffers->SetFormat(SapFormatMono16);
  m_Buffers->SetHeight(size_y);
  m_Buffers->SetWidth(size_x);
  m_Buffers->SetType(SapBuffer::TypeContiguous);

  this->sensor_height = size_y;
  this->sensor_width = size_x;

  m_Xfer = new SapTransfer(XferCallback, this);
  //        if (m_bEnableRtPro)
  //            m_RtproDesign = NewRtproDesign( m_Acq->GetLocation());

  // Create all objects
  if (!CreateObjects()) {
    return false;
  }
  grab_index = m_Buffers->GetIndex();

  // Get current input signal connection status
  GetSignalStatus();

  m_Acq->SaveParameters("D:/corecofiles/current_params.ccf");

  return TRUE;  // return TRUE  unless you set the focus to a control
}

/**
 * set grabber config file name. we init from a file, and edit the x and y sizes.
 * vendor config file has 100 params or so, and all we set is x y size programattically.
 * @param   
 * 
 */
 
void coreco::setConfigFileName(char *name) { strcpy(camera_format_file, name); }

/**
 * Dalsa needs this for it sdriver. makes SapAcq object.  
 * @return  SapAcquisition pointer
 */
 
SapAcquisition *coreco::makeAcquision() {
  bool sap_result;
  SapAcquisition *sap_acquisition;

  acq_device_number = 0;

  SapManager::GetServerName(1, acq_server_name, sizeof(acq_server_name));
  if (is_rst_server)
    sap_result = SapManager::ResetServer(acq_server_name, NULL, NULL);

  SapManager::GetServerName(1, acq_server_name, sizeof(acq_server_name));
  SapManager::GetResourceName(acq_server_name, SapManager::ResourceAcq,
                              acq_device_number, device_name,
                              sizeof(device_name));

  sap_location = new SapLocation(acq_server_name, acq_device_number);

  // if (sap_acquisition != NULL)
  //    sap_acquisition->Destroy ();

  sap_acquisition = new SapAcquisition(*sap_location);

  sap_result = sap_acquisition->SetConfigFile(camera_format_file);
  // sap_result = sap_acquisition->Create ();

  return (sap_acquisition);
}

/**
 * Creates dalsa objects per the Dalsa API. 
 * @return  true of success.
 */
 
bool coreco::CreateObjects() {
  is_destroyed = false;

  // add transfer paths here
  if (m_Xfer) {
    // connect the acq directly to the host.
    SapXferPair acqToHostPair(m_Acq, m_Buffers, TRUE);
    acqToHostPair.SetCallbackInfo(XferCallback, this);
    m_Xfer->AddPair(acqToHostPair);
  }

  // Create acquisition object
  if (m_Acq && !*m_Acq && !m_Acq->Create()) {
    DestroyObjects();
    // throw ccd_exception("coreco::CreateObjects could not create Acq");
    return FALSE;
  }

  if (is_force_size) {
    int w2;
    int ntaps;

    w2 = sensor_width;

    m_Acq->GetParameter(CORACQ_PRM_TAPS, (void *)(&ntaps));

    // we mult by 2 because we have 8bit depth instead of 16bit...
    // real pixels are 16bit but grabber assumes 8bit.
    if (ntaps == 10) w2 = w2 * 2;

    m_Acq->SetParameter(CORACQ_PRM_CROP_HEIGHT, sensor_height, false);
    m_Acq->SetParameter(CORACQ_PRM_CROP_WIDTH, w2, false);

    m_Acq->SetParameter(CORACQ_PRM_SCALE_VERT, sensor_height, false);
    m_Acq->SetParameter(CORACQ_PRM_SCALE_HORZ, w2, false);

    m_Acq->SetParameter(CORACQ_PRM_VACTIVE, sensor_height, false);

    m_Acq->SetParameter(CORACQ_PRM_HACTIVE, w2 / ntaps, true);
  }

  // Create buffer object
  if (m_Buffers && !*m_Buffers && !m_Buffers->Create()) {
    char mm[128];
    DestroyObjects();
    sprintf(mm, "coreco::CreateObjects could not create SapBuffers %i %i ",
            sensor_height, sensor_width);
    ///  throw ccd_exception(mm);
    return FALSE;
  }

  // Create view object

  // Create transfer object
  if (m_Xfer != NULL && !*m_Xfer && !m_Xfer->Create()) {
    DestroyObjects();
    // throw ccd_exception("coreco::CreateObjects could not create Xfer");

    return FALSE;
  }

  if (m_Xfer && *m_Xfer) {
    m_Xfer->SetAutoEmpty(FALSE);
  }

  sap_buffer_count = m_Buffers->GetCount();
  sensor_width = m_Buffers->GetWidth();
  sensor_height = m_Buffers->GetHeight();

  return TRUE;
}

/**
 * Kills Dalsa API objects for shutting down grabber. 
 * @return  true if success. 
 */
 
bool coreco::DestroyObjects() {
  if (is_destroyed) return (true);

  if (m_Xfer) {
    if (*m_Xfer) {
      m_Xfer->Destroy();
    }
    m_Xfer->RemoveAllPairs();
  }

  // Destroy view object
  // if (m_View && *m_View) m_View->Destroy();

  // Destroy buffer object
  if (m_Buffers && *m_Buffers) m_Buffers->Destroy();

  // Destroy acquisition object
  if (m_Acq && *m_Acq) m_Acq->Destroy();

  // Delete all objects
  if (m_Xfer) delete m_Xfer;
  // if (m_ImageWnd)     delete m_ImageWnd;
  //    if (m_View)             delete m_View;
  if (m_Buffers) delete m_Buffers;
  if (m_Acq) delete m_Acq;
  is_destroyed = true;
  return TRUE;
}

/**
 *  destroys Dalsa API objects, no delete memory. 
 * @return  true if success. 
 */
 
bool coreco::DestroyObjectsNoDelete() {
  if (m_Xfer) {
    if (*m_Xfer) {
      m_Xfer->Destroy();
    }
    m_Xfer->RemoveAllPairs();
  }

  // destroy rtpro design

  // Destroy view object
  // if (m_View && *m_View) m_View->Destroy();

  // Destroy buffer object
  if (m_Buffers && *m_Buffers) m_Buffers->Destroy();

  // Destroy acquisition object
  if (m_Acq && *m_Acq) m_Acq->Destroy();

  // Delete all objects

  return TRUE;
}



/**
 * call this to abort frame grabbing 
 */
 
void coreco::abort() {
  if (m_Xfer) {
    m_Xfer->Abort();
  }

  // UpdateMenu();
}

/**
 * similar to abort above, Never used.  
 */
 
void coreco::freeze() {
  if (m_Xfer) {
    m_Xfer->Freeze();
  }

  //    UpdateMenu();
}

/**
 * Set into grab mode, where grabber gets any images from camera.  
 */
 
void coreco::grab() {
  if (m_Xfer) {
    //    m_statusWnd.SetWindowText("");
    m_Xfer->Grab();
  }

  // UpdateMenu();
}

/**
 * Grabs one frame then stops. Not used ususally.  
 */
 
void coreco::snap() {
  // m_statusWnd.SetWindowText("");
  if (m_Xfer) {
    m_Xfer->Snap();

    //     if (CAbortDlg(this, m_Xfer).DoModal() != IDOK)
    //       m_Xfer->Abort();
  }

  // UpdateMenu();
}

// inc missed frames counter
/**
 * Inc the missed frames counter. 
 */
 
void coreco::incMissedFrames(void) {
  missed_frames++;
  recent_missed_frames++;
}


/**
 * Get signal stat like pixel clock present etc.  
 */
 
void coreco::GetSignalStatus() {
  SapAcquisition::SignalStatus signalStatus;

  if (m_Acq && m_Acq->IsSignalStatusAvailable()) {
    if (m_Acq->GetSignalStatus(&signalStatus, SignalCallback, this))
      GetSignalStatus(signalStatus);
  }
}

/**
 *  get signal stat like clk present on grabber. 
 * @param   
 */
 
void coreco::GetSignalStatus(SapAcquisition::SignalStatus signalStatus) {
  m_IsSignalDetected = (signalStatus != SapAcquisition::SignalNone);
}

/**
 * set CameraLink pin. CL has 4 user pins to trigger camera. Sets on of the pins. String is
 *"CC0", "CC1", "CC2", "CC3", and 0 or 1 for val. 
 * @param   pinstr  C strign for pin name. "CC0" to CC3
 * @param   val 1 or 0 for on or off .
 */
 
void coreco::setPin(char *pinstr, int val) {
  int status, status2;
// char msg[255];
// char errbuff[1024];
// char infobuff[1024];

//    coreco_log.log("Doing Reset in Coreco Card");

#if USE_SAP

  hAcq = m_Acq->GetHandle(0);
  // sprintf(msg,"sap_acquisition->GetHandle %p",hAcq);
  // coreco_log.log(msg);

  //;instr  = "CC4" val is 0 or 1
  status = CorAcqSetCamIoControl(hAcq, pinstr, val);
// CorManGetStatusText(status,errbuff,1023,infobuff,1023);
// sprintf(msg,"CorAcqSetCamIoControl %i\d",status);
// coreco_log.log(msg);
// coreco_log.log(errbuff);
// coreco_log.log(infobuff);
#endif
}

#endif
