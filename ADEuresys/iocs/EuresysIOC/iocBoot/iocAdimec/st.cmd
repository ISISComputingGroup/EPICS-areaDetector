< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/EuresysApp.dbd")
EuresysApp_registerRecordDeviceDriver(pdbbase) 

# Use this line for a specific board
epicsEnvSet("BOARD_ID", "0")

# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "4096")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "3072")

epicsEnvSet("GENICAM_DB_FILE", "$(ADGENICAM)/db/Adimec_Q12A180CXP_1_1_5.template")

< ../st_base.cmd
