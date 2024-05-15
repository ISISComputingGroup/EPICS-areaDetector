< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/pcowinApp.dbd")
pcowinApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "XF05IDD-ES{PCO:1}:")

# The port name for the detector
epicsEnvSet("PORT",   "PCO1")
# Larger queue size may be need to stream to disk at full camera speed, up tp 2000
epicsEnvSet("QSIZE",  "20")   
# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "2560")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "2160")
# The maximum number of time series points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
# Define NELEMENTS to be enough for a 2048x2048x3 (color) image
epicsEnvSet("NELEMENTS", "11059200")

# pcoConfig(const char* portName, int maxBuffers, size_t maxMemory)
pcoConfig("$(PORT)", 0, 0, 8)

# pcoApiConfig(const char* portName)
pcoApiConfig("$(PORT)")

# Asyn tracing
asynSetTraceIOMask($(PORT), 0, 2)
#asynSetTraceMask($(PORT), 0, 0xFF)
#asynSetTraceFile($(PORT), 0, "asynTrace.out")
#asynSetTraceInfoMask($(PORT), 0, 0xf)

dbLoadRecords("$(ADPCOWIN)/db/pco.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=0")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=1")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=2")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=3")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=4")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=5")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=6")
dbLoadRecords("$(ADPCOWIN)/db/pco_device_firmware.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),N=7")

# What other database files need to be loaded?

# Create a standard arrays plugin
NDStdArraysConfigure("Image1", 5, 0, "$(PORT)", 0, 0)
# Use this line for 8-bit data only
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=CHAR,NELEMENTS=$(NELEMENTS)")
# Use this line for 8-bit or 16-bit data
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int16,FTVL=SHORT,NELEMENTS=$(NELEMENTS)")
# Use this line for 8-bit, 16-bit, or 32-bit data
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int32,FTVL=LONG,NELEMENTS=$(NELEMENTS)")

# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
set_requestfile_path("$(ADPCOWIN)/pcowinApp/Db")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX)")
