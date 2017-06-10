#!/usr/bin/env dls-python

from pkg_resources import require
require("fit_lib == 1.3")
require("scipy == 0.10.1")
    
from adPythonPlugin import AdPythonPlugin
import logging, numpy
from fit_lib import fit_lib
import scipy.ndimage

class Gaussian2DFitter(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        self.log.setLevel(logging.DEBUG) 
        # Make inputs and ouptuts list
        params = dict(PeakHeight = 1, 
                      OriginX = 2, 
                      OriginY = 3,  
                      Baseline = 3., 
                      SigmaX = 1.0,
                      SigmaY = 2.0,
                      Angle = 3.0,
                      Error = 2.0,
                      FitWindowSize = 3, 
                      FitThinning = 5,
                      Maxiter = 20,
                      FitStatus = "",
                      OverlayROI = 1,
                      OverlayElipse = 1,
                      OverlayCross = 1,
                      OutputType = 1,
                      )
        AdPythonPlugin.__init__(self, params)
        
    #def paramChanged(self):
        # one of our input parameters has changed
        # just log it for now, do nothing.
     #   self.log.debug("Parameter has been changed %s", self)
        

    def processArray(self, arr, attr={}):        
        # Called when the plugin gets a new array
        # arr is a numpy array
        # attr is an attribute dictionary that will be attached to the array
        
        # Convert the array to a float so that we do not overflow during processing.
        arr2 = numpy.float_(arr)
        # Run a median filter over the image to remove the spikes due to dead pixels.
        arr2 = scipy.ndimage.median_filter(arr2, size = 3)
        try:
        
            fit, error, results = fit_lib.doFit2dGaussian(
             arr2, thinning=(self["FitThinning"], self["FitThinning"]), #self.FitThinning
             window_size = self["FitWindowSize"], maxiter = self["Maxiter"], #self.FitWindowSize   self.maxiter
             ROI = None, gamma = (0, 255), ##[[150, 150],[100, 100]]
             extra_data = True)
             # fit outputs in terms of ABC we want sigma x, sigma y and angle.
            s_x, s_y, th = fit_lib.convert_abc(*fit[4:7])
        except fit_lib.levmar.FitError:
            self["FitStatus"] = "Fit error"
        else:
            self["FitStatus"] = "Fit OK"
            # Write out to the EDM output parameters.
            self["Baseline"] = float(fit[0])
            self["PeakHeight"] = int(fit[1])
            self["OriginX"] = int(fit[2])
            self["OriginY"] = int(fit[3])
            self["SigmaX"] = s_x
            self["SigmaY"] = s_y
            self["Angle"] = th
            self["Error"] = float(error)
            
            if self["OutputType"] == 1:
                # create the model output and take a difference to the original data.
                grid = fit_lib.flatten_grid(fit_lib.create_grid(arr.shape))
                arr = arr2 - fit_lib.Gaussian2d(fit, grid).reshape(arr.shape)
                arr = numpy.uint8(arr)
         
        # Add the annotations
            def plot_ab_axis(image, orig_x, orig_y, theta, ax_size = 70, col = 256):
                '''Creates image overlay for crosshairs.'''
                theta = -theta * numpy.pi /180 # converting to radians
                # Create an array of zeros the same size as the original image.
                overlay_cross = numpy.zeros_like(image)
                #Draw cross pixel by pixel by setting each pixel to 256 (i.e. white)
                for axs in range(0,ax_size):
                    ulimb = (orig_x + axs * numpy.cos(theta), orig_y + axs * numpy.sin(theta))
                    llimb = (orig_x - axs * numpy.cos(theta), orig_y - axs * numpy.sin(theta))
                    ulima = (orig_x + axs * numpy.sin(theta), orig_y - axs * numpy.cos(theta))
                    llima = (orig_x - axs * numpy.sin(theta), orig_y + axs * numpy.cos(theta))
                    overlay_cross[ulimb] = col
                    overlay_cross[llimb] = col
                    overlay_cross[ulima] = col
                    overlay_cross[llima] = col
                return overlay_cross
                
            def plot_elipse(image, orig_x, orig_y, sig_x, sig_y, theta, col):
                '''Plots an elipse on the given axis of interest.'''
                # Create an array of zeros the same size as the original image.
                overlay_elipse = numpy.zeros_like(image)
                ex_vec = numpy.arange(-1,1, 0.01) * sig_x
                ey_vec = numpy.sqrt(numpy.square(sig_y) * (1.- ( numpy.square(ex_vec) / numpy.square(sig_x))))
                ex_vec = numpy.hstack([ex_vec, -ex_vec])
                ey_vec = numpy.hstack([ey_vec, -ey_vec])
                theta = theta * numpy.pi /180 # converting to radians
                # converting to r, theta and adding additional theta term
                r = numpy.sqrt(ex_vec*ex_vec + ey_vec*ey_vec)
                t = numpy.arctan(ey_vec/ex_vec) - theta
                # Converting back to [x,y]
                x_len = len(ex_vec)
                x_seg = numpy.floor(x_len/2)
                ex_vec[:x_seg] =  r[:x_seg] * numpy.cos(t[:x_seg])
                ey_vec[:x_seg] =  r[:x_seg] * numpy.sin(t[:x_seg])
                ex_vec[x_seg:] = -r[x_seg:] * numpy.cos(t[x_seg:])
                ey_vec[x_seg:] = -r[x_seg:] * numpy.sin(t[x_seg:])
                # Moving the origin
                ex_vec = ex_vec + orig_x
                ey_vec = ey_vec + orig_y
                point_list = zip(ex_vec,ey_vec)
                for nf in point_list:
                    overlay_elipse[nf] = col
                return overlay_elipse
    
            def plot_ROI(image, results):
                '''Plots a box showing the region of interest used for the fit.'''
                # Create an array of zeros the same size as the original image.
                overlay_ROI = numpy.zeros_like(image)
                for ns in range(int(results.origin[1]), int(results.origin[1]) + results.extent[1]):
                    overlay_ROI[(int(results.origin[0]),ns)] = 255
                    overlay_ROI[(int(results.origin[0]) + results.extent[0]-1, ns)] = 255
                for nt in range(int(results.origin[0]), int(results.origin[0]) + results.extent[0]):
                    overlay_ROI[(nt, int(results.origin[1]))] = 255
                    overlay_ROI[(nt, int(results.origin[1]) + results.extent[1]-1)] = 255
                return overlay_ROI
                
            def apply_overlay(image, overlay):
                # Preferentially sets the pixel value to the overaly value if the overlay is not zero.
                out = numpy.where(overlay == 0, image, overlay)
                return out
    
            if self["OverlayCross"] == 1:
                ol_cross = plot_ab_axis(arr, fit[2], fit[3], th, ax_size = 20, col=255)
                arr = apply_overlay(arr, ol_cross)
            
            if self["OverlayElipse"] == 1:            
                ol_elipse = plot_elipse(arr, fit[2], fit[3], s_x, s_y, th, 255)
                arr = apply_overlay(arr, ol_elipse)
           
            if self["OverlayROI"] == 1: 
                ol_ROI = plot_ROI(arr, results)           
                arr = apply_overlay(arr, ol_ROI)
                
             
            # Write the attibute array which will be attached to the output array.
            #Note that we convert from the numpy
            # uint64 type to a python integer as we only handle python integers,
            # doubles and strings in the C code for now
            # Fitter results
            for param in self:
                attr[param] = self[param]
            # Write something to the logs
            self.log.debug("Array processed, baseline: %f, peak height: %d, origin x: %d, origin y: %d, sigma x: %f, sigma y: %f, angle: %f, error: %f, output: %d", self["Baseline"], self["PeakHeight"], self["OriginX"], self["OriginY"], self["SigmaX"],self["SigmaY"],self["Angle"], self["Error"], self["OutputType"])    
        # return the resultant array.
        return arr

if __name__=="__main__":
    Gaussian2DFitter().runOffline(
        int1=256,            # This has range 0..255
        int2=500,        # This has range 0..255
        int3=500,        # This has range 0..255
        double1=(0,30,0.01), # This has range 0, 0.01, 0.02 ... 30
        double2=(0,30,0.01), # This has range 0, 0.01, 0.02 ... 30
        double3=(0,360,0.1)) # This has range 0, 0.1, 0.002 ... 360
    #     PeakHeight = 256,
    #     OriginX = (-500, 500,1), 
    #     OriginY = (-500,500,1),  
    #     Baseline = (-256, 256,1), 
    #     SigmaX = 30,
    #     SigmaY = 30, 
    #     Angle = (-360, 360, 1),
     #    Error = 500);
