errlogInit(20000)

< envPaths
#epicsThreadSleep(20)
dbLoadDatabase("$(TOP)/dbd/LambdaApp.dbd")
LambdaApp_registerRecordDeviceDriver(pdbbase) 


iocshLoad("ADLambda.iocsh", "PREFIX=LAMBDA1:, PORT=LAMBDA1")

set_requestfile_path("$(ADLAMBDA)/LambdaApp/Db")

##########
iocInit()
##########

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")
