# our base class requires numpy, so make sure it's on the path here
# this step is only needed if numpy is an egg installed multi-version
try:
    from pkg_resources import require
    require("numpy")
except:
    pass

import imp, os, logging, numpy

logging.basicConfig(format='%(asctime)s %(levelname)8s %(name)8s %(filename)s:%(lineno)d: %(message)s', level=logging.INFO)

# define a helper function that imports a python filename and returns an 
# instance of classname which is contained in it
def makePyInst(portname, filename, classname):
    # Create a logger associated with this portname
    log = logging.getLogger(portname)
    log.setLevel(logging.INFO) 
    log.info("Creating %s:%s with portname %s", 
        os.path.basename(filename), classname, portname)
    try:
        # This dance is needed to load a file explicitly from a filename
        f = open(filename)
        pymodule, ext = os.path.splitext(os.path.basename(filename))
        AdPythonPlugin.log = log        
        mod = imp.load_module(pymodule, f, filename, (ext, 'U', 1))
        f.close()
        # Get classname ref from this module and make an instance of it
        inst = getattr(mod, classname)()
        # Call paramChanged it might do some useful setup
        inst.paramChanged()
        return inst
    except:
        # Log the exception in the logger as the C caller will throw away the
        # exception text
        log.exception("Creating %s:%s threw exception", filename, classname)
        raise

class AdPythonPlugin(object):   
    # Will be our param dict
    _params = None
    # Will be our logger when used in conjunction with makePyInst()
    log = logging.getLogger("Offline")
    
    # init our param dict
    def __init__(self, params={}):
        self._params = dict(params)
        # self.log is the logger associated with AdPythonPlugin, copy it
        # and define it as the logger just for this instance...
        self.log = self.log

    # get a param value
    def __getitem__(self, param):
        return self._params[param]

    # set a param value 
    def __setitem__(self, param, value):
        assert param in self, "Param %s not in param lib" % param
        self._params[param] = value
 
    # see if param is supported
    def __contains__(self, param):
        return param in self._params
 
    # length of param dict
    def __len__(self):
        return len(self._params)

    # for if we want to print the dict 
    def __repr__(self):
        return repr(self._params)

    # iter
    def __iter__(self):
        return iter(self._params)

    # called when parameter list changes
    def _paramChanged(self):
        try:
            return self.paramChanged()
        except:
            # Log the exception in the logger as the C caller will throw away 
            # the exception text        
            self.log.exception("Error calling paramChanged()")
            raise
    
    # called when a new array is generated
    def _processArray(self, arr, attr):
        try:
            # Tell numpy that it does not own the data in arr, so it is read only
            # This should really be done at the C layer, but it's much easier here!        
            arr.flags.writeable = False            
            return self.processArray(arr, attr)
        except:
            # Log the exception in the logger as the C caller will throw away 
            # the exception text        
            self.log.exception("Error calling processArray()")
            raise
        
    # called when run offline
    def runOffline(self, **ranges):
        from adPythonOffline import AdPythonOffline
        AdPythonOffline(self, **ranges)

if __name__=="__main__":
    # If run from the command line, assume we want the location of the numpy lib
    print numpy.get_include()
