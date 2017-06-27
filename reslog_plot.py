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

def Spectrum(s):
    Ftest = scipy.fftpack.fft( s )
    n = round(s.shape[0]/2)
    xf = np.linspace(0.0, 1.0, n)
    return xf, np.abs(Ftest[0:n])

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
    fsig = list(map(lambda x: np.max([x, 10**-32]), np.abs(scipy.fftpack.fft(data[0]))))
    f_abs = data[1]
    f_phase = data[2]


    plt.subplot(311)
    plt.plot( 20*np.log10(fsig))
    plt.subplot(312)
    plt.plot(f_abs[0:round(f_abs.shape[0]/2)])
    plt.subplot(313)
    plt.plot(f_phase[0:round(f_phase.shape[0]/2)])
    plt.show()
    