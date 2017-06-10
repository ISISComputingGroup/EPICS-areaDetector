import copy

from iocbuilder import Device, AutoSubstitution
from iocbuilder.arginfo import *

from iocbuilder.modules.asyn import Asyn, AsynPort
from iocbuilder.modules.ADCore import ADCore, NDPluginBaseTemplate, includesTemplates, makeTemplateInstance

class AdPython(Device):
    '''Library dependencies for adPython'''
    Dependencies = (ADCore,)
    # Device attributes
    LibFileList = ['adPython']
    DbdFileList = ['adPythonPlugin']
    AutoInstantiate = True

@includesTemplates(NDPluginBaseTemplate)
class _adPythonBase(AutoSubstitution):
    '''This plugin Works out the area and tip of a sample'''
    TemplateFile = "adPythonPlugin.template"
    
class adPythonPlugin(AsynPort):
    """This plugin creates an adPython object"""
    # This tells xmlbuilder to use PORT instead of name as the row ID
    UniqueName = "PORT"

    _SpecificTemplate = _adPythonBase
    Dependencies = (AdPython,)

    def __init__(self, classname, PORT, NDARRAY_PORT, QUEUE = 5, BLOCK = 0, NDARRAY_ADDR = 0, BUFFERS = 50, MEMORY = 0, **args):
        # Init the superclass (AsynPort)
        self.__super.__init__(PORT)
        # Update the attributes of self from the commandline args
        self.__dict__.update(locals())
        # Make an instance of our template
        makeTemplateInstance(self._SpecificTemplate, locals(), args)
        # Init the python classname specific class
        class _tmp(AutoSubstitution):
            ModuleName = adPythonPlugin.ModuleName
            TrueName = "_adPython%s" % classname
            TemplateFile = "adPython%s.template" % classname
        _tmpargs = copy.deepcopy(args)
        _tmpargs['PORT'] = PORT
        _tmp(**filter_dict(_tmpargs, _tmp.ArgInfo.Names()))
        # Store the args
        self.filename = "$(ADPYTHON)/adPythonApp/scripts/adPython%s.py" % classname
        self.Configure = 'adPythonPluginConfigure'

    def Initialise(self):
        print '# %(Configure)s(portName, filename, classname, queueSize, '\
            'blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, ' \
            'maxMemory)' % self.__dict__
        print '%(Configure)s("%(PORT)s", "%(filename)s", "%(classname)s", %(QUEUE)d, ' \
            '%(BLOCK)d, "%(NDARRAY_PORT)s", %(NDARRAY_ADDR)s, %(BUFFERS)d, ' \
            '%(MEMORY)d)' % self.__dict__

    # __init__ arguments
    ArgInfo = _SpecificTemplate.ArgInfo + makeArgInfo(__init__,
        classname = Choice('Predefined python class to use', [
            "Morph", "Focus", "Template", "BarCode", "Transfer", "Mitegen",
            "Circle", "DataMatrix", "Gaussian2DFitter", "PowerMean",
            "MxSampleDetect","Rotate"]),
        PORT = Simple('Port name for the plugin', str),
        QUEUE = Simple('Input array queue size', int),
        BLOCK = Simple('Blocking callbacks?', int),
        NDARRAY_PORT = Ident('Input array port', AsynPort),
        NDARRAY_ADDR = Simple('Input array port address', int),
        BUFFERS = Simple('Maximum number of NDArray buffers to be created for '
            'plugin callbacks', int),
        MEMORY = Simple('Max memory to allocate, should be maxw*maxh*nbuffer '
            'for driver and all attached plugins', int))


