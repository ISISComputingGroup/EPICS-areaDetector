< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/EuresysApp.dbd")
EuresysApp_registerRecordDeviceDriver(pdbbase) 

# Use this line for a specific board
epicsEnvSet("BOARD_ID", "0")

# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "1920")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "1080")

epicsEnvSet("GENICAM_DB_FILE", "$(ADGENICAM)/db/Mikrotron_CXP_MC206xS11.template")

< ../st_base.cmd
