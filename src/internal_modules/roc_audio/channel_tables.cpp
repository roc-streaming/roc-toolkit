/*
 * THIS FILE IS AUTO-GENERATED USING `channel_tables_gen.py'. DO NOT EDIT!
 */

#include "roc_audio/channel_tables.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

// Table of channel position names.
const ChannelPositionName ChanPositionNames[15] = {
    { "FL", ChanPos_FrontLeft },
    { "FC", ChanPos_FrontCenter },
    { "FR", ChanPos_FrontRight },
    { "SL", ChanPos_SideLeft },
    { "SR", ChanPos_SideRight },
    { "BL", ChanPos_BackLeft },
    { "BC", ChanPos_BackCenter },
    { "BR", ChanPos_BackRight },
    { "TFL", ChanPos_TopFrontLeft },
    { "TFR", ChanPos_TopFrontRight },
    { "TML", ChanPos_TopMidLeft },
    { "TMR", ChanPos_TopMidRight },
    { "TBL", ChanPos_TopBackLeft },
    { "TBR", ChanPos_TopBackRight },
    { "LFE", ChanPos_LowFrequency },
};

// Table of channel mask names.
const ChannelMaskName ChanMaskNames[17] = {
    { "mono", ChanMask_Surround_Mono },
    { "stereo", ChanMask_Surround_Stereo },
    { "surround2.1", ChanMask_Surround_2_1 },
    { "surround3.0", ChanMask_Surround_3_0 },
    { "surround3.1", ChanMask_Surround_3_1 },
    { "surround4.0", ChanMask_Surround_4_0 },
    { "surround4.1", ChanMask_Surround_4_1 },
    { "surround5.0", ChanMask_Surround_5_0 },
    { "surround5.1", ChanMask_Surround_5_1 },
    { "surround5.1.2", ChanMask_Surround_5_1_2 },
    { "surround5.1.4", ChanMask_Surround_5_1_4 },
    { "surround6.0", ChanMask_Surround_6_0 },
    { "surround6.1", ChanMask_Surround_6_1 },
    { "surround7.0", ChanMask_Surround_7_0 },
    { "surround7.1", ChanMask_Surround_7_1 },
    { "surround7.1.2", ChanMask_Surround_7_1_2 },
    { "surround7.1.4", ChanMask_Surround_7_1_4 },
};

// Table of channel orders.
const ChannelOrderTable ChanOrderTables[3] = {
    // ChanOrder_None
    {
        {
            ChanPos_Max,
        },
    },
    // ChanOrder_Smpte
    {
        {
            ChanPos_FrontLeft,
            ChanPos_FrontRight,
            ChanPos_FrontCenter,
            ChanPos_LowFrequency,
            ChanPos_BackLeft,
            ChanPos_BackRight,
            ChanPos_BackCenter,
            ChanPos_SideLeft,
            ChanPos_SideRight,
            ChanPos_TopFrontLeft,
            ChanPos_TopFrontRight,
            ChanPos_TopMidLeft,
            ChanPos_TopMidRight,
            ChanPos_TopBackLeft,
            ChanPos_TopBackRight,
            ChanPos_Max,
        },
    },
    // ChanOrder_Alsa
    {
        {
            ChanPos_FrontLeft,
            ChanPos_FrontRight,
            ChanPos_BackLeft,
            ChanPos_BackRight,
            ChanPos_FrontCenter,
            ChanPos_LowFrequency,
            ChanPos_SideLeft,
            ChanPos_SideRight,
            ChanPos_BackCenter,
            ChanPos_Max,
        },
    },
};

// Table of channel mappings.
const ChannelMapTable ChanMapTables[40] = {
    // 2.1->...
    {
        "2.1->1.0",
        ChanMask_Surround_2_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
        },
    },
    // 3.1->...
    {
        "3.1->1.0",
        ChanMask_Surround_3_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
        },
    },
    {
        "3.1->2.1",
        ChanMask_Surround_3_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 4.1->...
    {
        "4.1->1.0",
        ChanMask_Surround_4_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
        },
    },
    {
        "4.1->2.1",
        ChanMask_Surround_4_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "4.1->3.1",
        ChanMask_Surround_4_1,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 5.1.2->...
    {
        "5.1.2->1.0",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidRight, 0.500f },
        },
    },
    {
        "5.1.2->2.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.2->3.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.2->4.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.2->5.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 5.1.4->...
    {
        "5.1.4->1.0",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopBackLeft, 0.354f },
            { ChanPos_FrontCenter, ChanPos_TopBackRight, 0.354f },
        },
    },
    {
        "5.1.4->2.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->3.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->4.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->5.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->5.1.2",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.707f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 6.1->...
    {
        "6.1->1.0",
        ChanMask_Surround_6_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackCenter, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
        },
    },
    {
        "6.1->2.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackCenter, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackCenter, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->3.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackCenter, 0.500f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackCenter, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->4.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->5.1.2",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_BackCenter, 0.707f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_BackRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_BackCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->5.1.4",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.707f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_FrontLeft, 1.000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_FrontRight, 1.000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_TopBackLeft, ChanPos_BackCenter, 0.707f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_TopBackRight, ChanPos_BackCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 7.1.2->...
    {
        "7.1.2->1.0",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SideLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SideRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidRight, 0.500f },
        },
    },
    {
        "7.1.2->2.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SideLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SideRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->3.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SideLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SideRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->4.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->5.1.2",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopMidLeft, 1.000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopMidRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->5.1.4",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopMidLeft, 0.707f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopMidRight, 0.707f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopMidLeft, 0.707f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->6.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.707f },
            // BC
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.000f },
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.000f },
            { ChanPos_BackCenter, ChanPos_TopMidLeft, 0.707f },
            { ChanPos_BackCenter, ChanPos_TopMidRight, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->7.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_SideLeft, ChanPos_TopMidLeft, 0.707f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.000f },
            { ChanPos_SideRight, ChanPos_TopMidRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 7.1.4->...
    {
        "7.1.4->1.0",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SideLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SideRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopBackLeft, 0.354f },
            { ChanPos_FrontCenter, ChanPos_TopBackRight, 0.354f },
        },
    },
    {
        "7.1.4->2.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SideLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SideRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->3.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SideLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SideRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->4.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->5.1.2",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.707f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->5.1.4",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopFrontLeft, 1.000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopFrontRight, 1.000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopBackLeft, 1.000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopBackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->6.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BC
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.000f },
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.000f },
            { ChanPos_BackCenter, ChanPos_TopBackLeft, 0.707f },
            { ChanPos_BackCenter, ChanPos_TopBackRight, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->7.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->7.1.2",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_7_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.707f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
};

} // namespace audio
} // namespace roc
