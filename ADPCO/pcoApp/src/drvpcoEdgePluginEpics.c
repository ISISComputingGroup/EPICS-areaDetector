/**
 * drvsimPluginEpics.c
 *
 *
 * @author Madden
 *
 * @date March 25, 2009
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iocsh.h>
#include <epicsExport.h>

#include "drvpcoEdgePlugin.h"


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"drvpcoEdgePluginConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvpcoEdgePluginConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

void pcoEdgePluginRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(pcoEdgePluginRegister);
