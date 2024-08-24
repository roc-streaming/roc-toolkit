#! /usr/bin/env python3
import os
import os.path
import subprocess

def format_name(encoding, endian):
    return 'PcmSubformat_' + \
        encoding['pcm_encoding'] + '_' + endian['pcm_endian']

def format_array(array, maxlen=8, indent=1):
    if len(array) > maxlen:
        ret = "{\n"
        while array:
            ret += " " * (indent * 2 + 1)
            for n in range(maxlen):
                if array:
                    ret += " %s," % array.pop(0)
            ret += "\n"
        ret += " " * (indent * 2)
        ret += "}"
    else:
        ret = "{ "
        ret += ", ".join(map(str, array))
        ret += " }"

    return ret

def format_blob(blob):
    return format_array(["0x%02x" % b for b in blob], maxlen=12)

def format_samples(samples):
    return format_array(["%+.8ff" % s for s in samples], maxlen=4)

def read_samples(datfile):
    samples = []

    with open(datfile) as fp:
        for line in fp.read().splitlines():
            line = line.strip()
            if line.startswith(';'):
                continue
            time, value = map(float, line.split())
            samples.append(value)

    return samples

def synthesize_samples(datfile):
    cmd = ' '.join(map(str, [
        'sox', '-V0', '-R',
        '--null',
        '--rate', 48000,
        '--channels', 1,
        datfile,
        'synth', '0.005', 'sin', '1000',
        ]))
    subprocess.call(cmd, shell=True)

def generate_blob(encoding, endian, datfile, rawfile):
    cmd = ' '.join(map(str, [
        'sox', '-V0', '-R',
        datfile,
        '--type', 'raw',
        '--encoding', encoding['sox_encoding'],
        '--bits', encoding['depth'],
        '--endian', endian['sox_endian'],
        rawfile,
        ]))
    subprocess.call(cmd, shell=True)

def generate_header(encoding, endian, name, datfile, rawfile, hdrfile):
    with open(rawfile, 'rb') as fp:
        rawdata = fp.read()

    samples = read_samples(datfile)

    guard = 'ROC_AUDIO_TEST_SAMPLES_' + hdrfile.upper().replace('.', '_') + '_'

    with open(hdrfile, 'w') as fp:
        fp.write(f'''
/*
 * THIS FILE IS AUTO-GENERATED USING `generate_samples.py'
 */

#ifndef {guard}
#define {guard}

#include "test_samples/sample_info.h"

namespace roc {{
namespace audio {{
namespace test {{

static SampleInfo sample_{name} = {{
  /* name */ "{name}",

  /* format */ {format_name(encoding, endian)},

  /* num_samples */ {len(samples)},
  /* samples     */ {format_samples(samples)},

  /* num_bytes */ {len(rawdata)},
  /* bytes     */ {format_blob(rawdata)}
}};

}} // namespace test
}} // namespace audio
}} // namespace roc

#endif // {guard}
'''.lstrip())

encodings = [
    {
        'pcm_encoding': 'SInt8',
        'sox_encoding': 'signed-integer',
        'depth': 8,
    },
    {
        'pcm_encoding': 'UInt8',
        'sox_encoding': 'unsigned-integer',
        'depth': 8,
    },
    {
        'pcm_encoding': 'SInt16',
        'sox_encoding': 'signed-integer',
        'depth': 16,
    },
    {
        'pcm_encoding': 'UInt16',
        'sox_encoding': 'unsigned-integer',
        'depth': 16,
    },
    {
        'pcm_encoding': 'SInt24',
        'sox_encoding': 'signed-integer',
        'depth': 24,
    },
    {
        'pcm_encoding': 'UInt24',
        'sox_encoding': 'unsigned-integer',
        'depth': 24,
    },
    {
        'pcm_encoding': 'SInt32',
        'sox_encoding': 'signed-integer',
        'depth': 32,
    },
    {
        'pcm_encoding': 'UInt32',
        'sox_encoding': 'unsigned-integer',
        'depth': 32,
    },
    {
        'pcm_encoding': 'Float32',
        'sox_encoding': 'floating-point',
        'depth': 32,
    },
]

endians = [
    {
        'pcm_endian': 'Be',
        'sox_endian': 'big',
    },
    {
        'pcm_endian': 'Le',
        'sox_endian': 'little',
    },
]

os.chdir(os.path.dirname(os.path.abspath(__file__)))

datfile = 'original_samples.dat'

print(f'generating {datfile}')
synthesize_samples(datfile)

for encoding in encodings:
    for endian in endians:
        output = '_'.join([
            'pcm',
            encoding['pcm_encoding'].lower(),
            endian['pcm_endian'][0].lower() + 'e',
            ])

        print(f'generating {output}')

        rawfile = f'{output}.blob'
        hdrfile = f'{output}.h'

        generate_blob(encoding, endian, datfile, rawfile)
        generate_header(encoding, endian, output, datfile, rawfile, hdrfile)
