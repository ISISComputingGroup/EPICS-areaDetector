
from iocbuilder import Device, AutoSubstitution, Architecture
from iocbuilder.arginfo import *

from iocbuilder.modules.ADCore import ADCore, ADBaseTemplate, includesTemplates, makeTemplateInstance
from iocbuilder.modules.asyn import AsynPort

@includesTemplates(ADBaseTemplate)

class _pcocam2(AutoSubstitution):
    TemplateFile="pco.template"

class pcocam2(AsynPort):
    """Create a PCO camera detector"""
    Dependencies = (ADCore,)
    _SpecificTemplate = _pcocam2
    UniqueName = "PORT"

    def __init__(self, PORT, BUFFERS=50, MEMORY=-1, **args):
        self.__super.__init__(PORT)
        self.__dict__.update(locals())
        makeTemplateInstance(self._SpecificTemplate, locals(), args)

    # __init__ arguments
    ArgInfo = _SpecificTemplate.ArgInfo + makeArgInfo(__init__,
        PORT = Simple('Port name for the detector', str),
        BUFFERS = Simple('Maximum number of NDArray buffers to be created', int),
        MEMORY  = Simple('Max memory to allocate', int))
    LibFileList = ['pcocam2']
    DbdFileList = ['pcocam2Support']
    SysLibFileList = []
    MakefileStringList = []
    epics_host_arch = Architecture()
    # For any windows architecture, install the pcocam libraries
    # and configure the required linker flags
    if epics_host_arch.find('win') >= 0:
        LibFileList += ['SC2_DLG', 'SC2_Cam','PCO_CDLG','Pco_conv' ]
        SysLibFileList += ['windowscodecs', 'Comdlg32', 'Winspool', 'Comctl32', 'nafxcw']
        DbdFileList += ['pcocam2HardwareSupport']
        if epics_host_arch.find('debug') >= 0:
            MakefileStringList += ['%(ioc_name)s_LDFLAGS_WIN32 += /NOD:nafxcwd.lib /NOD:nafxcw.lib /NOD:libcmt']
        else:
            MakefileStringList += ['%(ioc_name)s_LDFLAGS_WIN32 += /NOD:nafxcwd.lib /NOD:nafxcw.lib']

    def Initialise(self):
        print '# pcoConfig(portName, buffers, memory)'
        print 'pcoConfig("%(PORT)s", %(BUFFERS)d, %(MEMORY)d)' % self.__dict__
        if self.epics_host_arch.find('win') >= 0:
            print '# pcoApiConfig(portName)'
            print 'pcoApiConfig("%(PORT)s")' % self.__dict__
        else:
            print '# simulationApiConfig(portName)'
            print 'simulationApiConfig("%(PORT)s")' % self.__dict__

class _pcocam2GangServer(AutoSubstitution):
    TemplateFile = "pco_gangserver.template"

class pcocam2GangServer(Device):
    _SpecificTemplate = _pcocam2GangServer
    AutoInstantiate = True

    def __init__(self, LISTENINGTCPPORT, PORT, **args):
        self.__super.__init__()
        self.__dict__.update(locals())
        makeTemplateInstance(self._SpecificTemplate, locals(), args)

    ArgInfo = _SpecificTemplate.ArgInfo + makeArgInfo(
        __init__,
        PORT = Ident('The asyn port name of the detector driver', pcocam2),
        LISTENINGTCPPORT = Simple('A TCP port number for the server to listen on', int))

    def Initialise(self):
        print '# gangServerConfig(portName, listeningTcpPort)'
        print 'gangServerConfig("%(PORT)s", %(LISTENINGTCPPORT)d)' % self.__dict__

class _pcocam2GangClient(AutoSubstitution):
    TemplateFile = "pco_gangconnection.template"

class pcocam2GangClient(Device):
    _SpecificTemplate = _pcocam2GangClient
    AutoInstantiate = True

    def __init__(self, SERVERIP, SERVERTCPPORT, PORT, **args):
        self.__super.__init__()
        self.__dict__.update(locals())
        makeTemplateInstance(self._SpecificTemplate, locals(), args)

    ArgInfo = _SpecificTemplate.ArgInfo + makeArgInfo(
        __init__,
        PORT = Ident('The asyn port name of the detector driver', pcocam2),
        SERVERIP = Simple('The IP address of the gang server', str),
        SERVERTCPPORT = Simple('A TCP port number of the gang server', int))

    def Initialise(self):
        print '# gangConnectionConfig(portName, serverIp, serverTcpPort)'
        print 'gangConnectionConfig("%(PORT)s", "%(SERVERIP)s", %(SERVERTCPPORT)d)' % self.__dict__

