< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/andorIstarApp.dbd")
andorIstarApp_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("PREFIX", "$(MYPVPREFIX)13ANDOR1:")
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
#andorIstarConfig("$(PORT)", "C:\Program Files (x86)\Andor iStar", 0, 0, 0, 0, 0)
andorIstarConfig("$(PORT)", "C:\\Program Files (x86)\\Andor iStar", 0, 0, 0, 0, 0)

dbLoadRecords("$(ADCORE)/db/ADBase.template",           "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=10")
dbLoadRecords("$(ADCORE)/db/NDFile.template",           "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=10")
dbLoadRecords("$(ADANDORISTAR)/db/andorIstar.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=10")

# Create a standard arrays plugin
NDStdArraysConfigure("Image1", 5, 0, "$(PORT)", 0, 0)
dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=10,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

# (Gabriele Salvato)
# Create an autofocus plugin
NDFocusMetricsConfigure("FOCUS1", 1, 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX), R=Focus1:, PORT=FOCUS1, ADDR=0, TIMEOUT=1, NDARRAY_PORT=$(PORT), NDARRAY_ADDR=0")
dbLoadRecords("$(ADCORE)/db/NDFocusMetrics.template", "P=$(PREFIX), R=Focus1:, PORT=FOCUS1, ADDR=0, TIMEOUT=1")
# (Gabriele Salvato) end


#
# Make NELEMENTS in the following be a little bigger than 2048*2048
#
# Use the following command for 16-bit images.  This can be used for 16-bit detector as long as accumulate mode would not result in 16-bit overflow
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=10,TYPE=Int16,FTVL=SHORT,NELEMENTS=522240")

#
# Load all other plugins using commonPlugins.cmd
# (Gabriele Salvato) uncomment this !!! < $(ADCORE)/iocBoot/commonPlugins.cmd
#

# specify where request files come from
set_requestfile_path("$(TOP)/iocBoot/$(IOC)", "")
set_requestfile_path("$(TOP)/iocBoot/$(IOC)", "autosave")
set_requestfile_path("$(ADANDORISTAR)", "andorIstarApp/Db")
set_requestfile_path("$(ADCORE)", "ADApp/Db")

# Is the path needed in Windows format ????
set_savefile_path("C:\Instrument\Apps\EPICS\support\areaDetector\master\ADAndorIstar\iocs\andorIstarIOC\iocBoot\iocAndorIstar\autosave")

# Again: is the path needed in Windows format ????
set_pass1_restoreFile("C:\Instrument\Apps\EPICS\support\areaDetector\master\ADAndorIstar\iocs\andorIstarIOC\iocBoot\iocAndorIstar\autosave\auto_settings.sav", "P=$(PREFIX)")
#

#asynSetTraceMask("$(PORT)",0,3)
#asynSetTraceIOMask("$(PORT)",0,4)
#
iocInit()
#
# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX)")
#asynSetTraceMask($(PORT), 0, 255)
