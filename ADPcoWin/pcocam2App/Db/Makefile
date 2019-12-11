TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# In a Diamond Support Module, install
# templates like this
#
DB += pco.template
DB += pco_gangconnection.template
DB += pco_gangserver.template
DB += PCO_dio_logic.template
DB += PCO_dio_sim.template
DB += pcocam2_detail.template

#----------------------------------------------------
# In a Diamond IOC Application, build db files from
# template files like this
#
#DB += pcocamfull.db

#USES_TEMPLATE += $(MOTOR)/db/motor.template
#USES_TEMPLATE += $(MOTOR)/db/basic_motor.template

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
#
#pcocamfull.db: ../pcocamfull.substitutions
#	msi -I.. -I$(ASYN)/db -I$(AREADETECTOR)/db -I$(EPICS_BASE)/db -S$? <$(DEV_NULL) > $@

