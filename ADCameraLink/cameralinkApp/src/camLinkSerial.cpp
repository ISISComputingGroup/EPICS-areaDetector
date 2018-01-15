 
/**
 * asyn Driver for Serial port on Camera Link grabber. Functions like standard rs232 port in ASyn.
 *
 *@author Timothy Madden
 *@date 2013
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <cantProceed.h>

#include <iocsh.h>
#include <epicsExit.h>

#include <asynPortDriver.h>

#include <epicsExport.h>
#include "camLinkSerial.h"

static const char *driverName = "camLinkSerial";

/**
 *	for setting key val pairs to serial port. 
 * 	clearpipe optino for reading out all garbage in the serial port .
 *  @param pasynUser    standard asyn user obj.
 *  @param key          C string for key.
 *  @param value        C string for valuie.
 */

 asynStatus camLinkSerial::writeOption(asynUser *pasynUser, const char *key,
                                      const char *value) {
  lock();

  std::string _key(key);
  std::string _value(value);
  option_pairs[_key] = _value;
  unlock();

  try {
    if (std::string(key) == "clearpipe")
      serial_port->clearPipe();
    else
      optionsToSerial();

  } catch (ccd_exception err) {
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
	  "camLinkSerial::writeOption Exception %s\n", err.err_mess());

  }  // catch
  catch (...) {
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,"camLinkSerial::writeOption Unknown Exception \n");

  }  // catch

  return (asynSuccess);
}

/**
 *  Set options to serial port.
 */

void camLinkSerial::optionsToSerial(void) {
  int baud;
  int parity;
  int nbits;
  int nstop;
  int rdtimeout;

  serial_port->setPortName((char *)option_pairs["name"].c_str());

  baud = atoi(option_pairs["baud"].c_str());
  nbits = atoi(option_pairs["bits"].c_str());
  rdtimeout = atoi(option_pairs["rdtimeout"].c_str());

  if (option_pairs["parity"] == "none")
    parity = 0;
  else
    parity = 1;

  nstop = atoi(option_pairs["stop"].c_str());

  if (option_pairs["open"] == "open")
    serial_port->open(baud, parity, nbits, nstop, rdtimeout);
  else
    serial_port->close();

  // from asynDriver serial docs
}

/**
 *  Read options from serial port. 
 * @param   pasynUser   Standard asyn User obj.
 * @param   key         C string key. name of option. Set by caller.
 * @param   value       c strign value of opton. Returned.
 * @param   maxChars    max num chars to put into value.
 */

asynStatus camLinkSerial::readOption(asynUser *pasynUser, const char *key,
                                     char *value, int maxChars) {
  lock();
  std::string _key(key);
  std::string &_value = option_pairs[_key];

  if (strlen(_value.c_str()) < maxChars) strcpy(value, _value.c_str());

  unlock();

  return (asynError);
}


/**
 * Called by asyn write Octet.writes the octet of bytes to the serial port. 
 * @param  pasynUser
 *  @param  value  a string.
 * @param   nchars  len of string, max.
 * @param   nActial  actuial len of string. 
 * 
 */

asynStatus camLinkSerial::writeOctet(asynUser *pasynUser, const char *value,
                                     size_t nChars, size_t *nActual) {
  int addr = 0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *functionName = "writeOctet";

  // virtual void write(unsigned char *buffer, int length)=0;
  serial_port->write((unsigned char *)value, nChars);

  *nActual = nChars;

  return status;
}

/**
 * 
 * reads the serial port back to epics driver. 
 * @param pasynUser     asyn User obj.
 * @param value         memory in which to put data from ser port.
 * @param maxChars      size of mem value
 * @param nActual       actual chars read into value
 * @param eomReason     set to 0
 * @return  always return asynSucess. 
 */

asynStatus camLinkSerial::readOctet(asynUser *pasynUser, char *value,
                                    size_t maxChars, size_t *nActual,
                                    int *eomReason) {
  // virtual void read(unsigned char *buffer, int length)=0;

  serial_port->read((unsigned char *)value, maxChars);
  *nActual = maxChars;
  *eomReason = 0;

  return (asynSuccess);
}

/**
 *  Flushes the serial port. 
 * @param pasynUser asyn uiser. Not actually needed....
 * @return  asynSuccess always returned.
 */

asynStatus camLinkSerial::flushOctet(asynUser *pasynUser) {
  serial_port->flush();
  return (asynSuccess);
}

/**
 * reports on driver.  
 */

void camLinkSerial::report(FILE *fp, int details) {
  asynPortDriver::report(fp, details);
}

/**
 * constructor for camLinkSerial. 
 * @param   portName        Port name of this asyn driver.
 * @param   comportname     name of com port, like "COM1" Some grabbers want this. Others dont.
 * @param   maxAddr         Set to 0. 
 * @param   interfaceMask   asyn interface mask bits
 * @param   interruptMask   asyn interrupt mask bits
 * @param   asynFlags       asyn flag bits
 * @param   autoConnect     1 for autoconnect ON. We use 1.
 * @param   priority        Use 50. Sets thread priority from 0 to 100
 * @param   stackSize       Use 0. Automatic stack size.
 */

camLinkSerial::camLinkSerial(const char *portName, const char *comportname,
                             int maxAddr, int interfaceMask, int interruptMask,
                             int asynFlags, int autoConnect, int priority,
                             int stackSize)
    :

      asynPortDriver(
          portName, maxAddr, NUM_CLS_PARAMS,
          interfaceMask | asynOctetMask | asynDrvUserMask |
              asynOptionMask,  // interfaceMask,
          interruptMask | asynOctetMask | asynOptionMask,  // interruptMask,
          asynFlags,
          autoConnect, priority, stackSize),

      option_pairs(),
	  lf("camLinkSerial_logfile.txt")

{


  #ifdef LOGFILE_USE_ASYN
  lf.setAsynUser(pasynUserSelf);
  #endif
  
#ifdef USE_SAP
  serial_port = new cl_com_port((char *)comportname,&lf);
#elif USE_SISW
  serial_port = new siswSerialPort(0,&lf);
#else
  serial_port = new sw_com_port((char *)comportname,&lf);

#endif

  /*
   *  add the option interface
   */

  option_pairs["name"] = "COM2";

  // open or close

  option_pairs["open"] = "close";

  // from asynDriver serial docs

  option_pairs["baud"] = "9600";
  option_pairs["bits"] = "8";
  option_pairs["parity"] = "none";
  option_pairs["stop"] = "1";
  option_pairs["clocal"] = "N";
  option_pairs["crtscts"] = "N";
  option_pairs["ixon"] = "N";
  option_pairs["ixoff"] = "N";
  option_pairs["ixany"] = "N";

  option_pairs["rdtimeout"] = "500";
}

extern "C" {
/*
 * Configure and register a generic serial device
 * @param   portName        asyn Driver port name
 * @param   comportname     name like COM1 for grabbers that want it. 
 */
int drvCamlinkSerialConfigure(char *portName, char *comportname) {
  camLinkSerial *a = new camLinkSerial(portName, comportname,
                                       0,  // maxAddr
                                       0,  // interfacemash
                                       0,  // interruptmash
                                       1,  // asynflags  1 for can block,
                                       1,  // auytoconnect
                                       50,  // proity
                                       100);  // stachsize

  return (0);
}

/*
 * IOC shell command registration
 */
static const iocshArg drvCamlinkSerialConfigureArg0 = {"port name",
                                                       iocshArgString};
static const iocshArg drvCamlinkSerialConfigureArg1 = {"tty name",
                                                       iocshArgString};

static const iocshArg *drvCamlinkSerialConfigureArgs[] = {
    &drvCamlinkSerialConfigureArg0, &drvCamlinkSerialConfigureArg1};

static const iocshFuncDef drvCamlinkSerialConfigureFuncDef = {
    "drvCamlinkSerialConfigure", 2, drvCamlinkSerialConfigureArgs};

static void drvCamlinkSerialConfigureCallFunc(const iocshArgBuf *args) {
  drvCamlinkSerialConfigure(args[0].sval, args[1].sval);
}

/*
 * This routine is called before multitasking has started, so there's
 * no race condition in the test/set of firstTime.
 */
static void drvCamlinkSerialRegisterCommands(void) {
  static int firstTime = 1;
  if (firstTime) {
    iocshRegister(&drvCamlinkSerialConfigureFuncDef,
                  drvCamlinkSerialConfigureCallFunc);
    firstTime = 0;
  }
}
epicsExportRegistrar(drvCamlinkSerialRegisterCommands);
}
