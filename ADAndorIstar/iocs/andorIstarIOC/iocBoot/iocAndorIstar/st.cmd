< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/andorIstarApp.dbd")
andorIstarApp_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("PREFIX", "13ANDOR1:")
epicsEnvSet("PORT",   "ANDOR")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "1024")
epicsEnvSet("YSIZE",  "255")
epicsEnvSet("NCHANS", "1024")

# andorIstarConfig(const char *portName, 
#                  const char *installPath, 
#                  int shamrockID,
#                  int maxBuffers,
#                  size_t maxMemory, 
#                  int priority, 
#                  int stackSize)
andorIstarConfig("$(PORT)", "C:/Program Files (x86)/Andor iStar/Drivers", 0, 0, 0, 0, 0)

dbLoadRecords("$(ADCORE)/db/ADBase.template",           "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=10")
dbLoadRecords("$(ADCORE)/db/NDFile.template",           "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=10")
dbLoadRecords("$(ADANDORISTAR)/db/andorIstar.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=10")

# Create a standard arrays plugin
NDStdArraysConfigure("Image1", 5, 0, "$(PORT)", 0, 0)
dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=10,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")
#
# Make NELEMENTS in the following be a little bigger than 2048*2048
#
# Use the following command for 16-bit images.  This can be used for 16-bit detector as long as accumulate mode would not result in 16-bit overflow
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=10,TYPE=Int16,FTVL=SHORT,NELEMENTS=522240")
#
# Load all other plugins using commonPlugins.cmd
# (Gabriele Salvato) uncomment this !!! < $(ADCORE)/iocBoot/commonPlugins.cmd
#
set_requestfile_path("$(ADANDORISTAR)/andorApp/Db")
#
#asynSetTraceMask("$(PORT)",0,3)
#asynSetTraceIOMask("$(PORT)",0,4)
#
iocInit()
#
# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX)")
#asynSetTraceMask($(PORT), 0, 255)
