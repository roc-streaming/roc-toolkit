#!env python3
#
# usage:
# roc-test-audio -v -g resampler -n upscale_downscale_mono |& \
#  ./scripts/plots/plot_resampler_test_dump.py
#
import pylab
import numpy
import re
import fileinput

orig = []
proc = []

for line in fileinput.input():
    m = re.search('roc_test: dump (\S+) (\S+)', line)
    if m:
        orig.append(float(m.group(1)))
        proc.append(float(m.group(2)))

orig = numpy.array(orig)
proc = numpy.array(proc)

_, px = pylab.subplots(2, 1, sharex=True)

px[0].plot(orig, '.-', label='orig')
px[0].plot(proc, '.-', label='proc')
px[0].legend()
px[0].grid()

px[1].plot(numpy.abs(orig-proc), '.-', label='abs(diff)')
px[1].legend()
px[1].grid()

pylab.show()
