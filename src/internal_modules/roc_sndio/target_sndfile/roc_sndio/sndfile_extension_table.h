#pragma once

#ifndef ROC_SNDIO_SNDFILE_FILEMAP_H_
#define ROC_SNDIO_SNDFILE_FILEMAP_H_

#include "sndfile.h"

//! Sndfile driver map.
struct FileMap {
    //! SF_FORMAT ID corresponding to the enum value in sndfile.h
    int format_id;
    //! Name of driver mapped to SF_FORMAT
    const char* driver_name;
    //! File extension associatied with driver and SF_FORMAT if it exists.
    const char* file_extension;
}; static FileMap file_type_map[5]= {
    { SF_FORMAT_MAT4, "mat4", NULL}, { SF_FORMAT_MAT5, "mat5", NULL}, { SF_FORMAT_WAV, "wav", "wav" }, 
    { SF_FORMAT_NIST, "nist", NULL }, { SF_FORMAT_WAVEX, "wavex", NULL }
};

#endif