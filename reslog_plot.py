#! env python3

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import scipy.signal
import scipy.fftpack
import math
import sys
from optparse import OptionParser

#######################################################################################################################

def Spectrum(s, low=0, high=1.0):
    Ftest = scipy.fftpack.fft( s )
    n = round(s.shape[0]/2)
    xf = np.linspace(low, high, n)
    return xf, 20*np.log10(np.abs(Ftest[0:n])+10**-60)

#######################################################################################################################

if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option( "-f", "--flog", action="store", 
                        type="string", dest="file_log",
                        help="Log file path")
    (options, args) = parser.parse_args()


    if not options.file_log:
    	print('Define path to the log!')
    	sys.exit(-1)

    data = pd.read_csv(options.file_log, header=None)
    data = data.as_matrix()
    
    plt.axis( [0, 2.0, -250, 100])
    x, y = Spectrum(data[0])
    plt.plot( x, y, '-o')
    x, y = Spectrum(data[1], high=2.0)
    plt.plot(x, y, '-x')
    plt.grid()
    plt.show()
    # fsig = data[0]
    # f_abs = data[1]
    # f_phase = data[2]


    # plt.plot(*Spectrum(fsig))
    # n = round(f_abs.shape[0]/2)
    # plt.plot( np.linspace(0,2,n,), f_abs[0:n])
    # plt.grid()
    # plt.show()
    