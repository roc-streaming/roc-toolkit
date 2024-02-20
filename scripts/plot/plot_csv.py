#!/usr/bin/env python3

import fileinput
import numpy
import pylab
import sys

text = []
columns = []

for line in fileinput.input(encoding='utf-8'):
    line = line.strip()
    if line.startswith('#'):
        columns = line.replace('#', '').split(',')
        continue
    text.append(line)

data = numpy.genfromtxt(text, dtype=float, delimiter=',')

if len(data.shape) == 1:
    data = data[:,numpy.newaxis]

if not columns:
    columns = [None] * data.shape[1]

fig, axs = pylab.subplots(len(columns), 1, sharex=True)

if len(columns) == 1:
    axs = [axs]

for col_idx, col_name in enumerate(columns):
    axs[col_idx].ticklabel_format(style='plain')
    if col_name:
        axs[col_idx].set_title(col_name)
    axs[col_idx].grid()
    axs[col_idx].plot(data[:,col_idx], '-*')

pylab.show()
