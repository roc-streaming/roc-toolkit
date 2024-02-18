/*
 * THIS FILE IS AUTO-GENERATED USING `channel_tables_gen.py'. DO NOT EDIT!
 */

#include "roc_audio/channel_tables.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

// Table of channel position names.
const ChannelPositionName ChanPositionNames[17] = {
    { "FL", ChanPos_FrontLeft },
    { "FLC", ChanPos_FrontLeftOfCenter },
    { "FC", ChanPos_FrontCenter },
    { "FRC", ChanPos_FrontRightOfCenter },
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
    {
        "none",
        ChanOrder_None,
        {
            ChanPos_Max,
        },
    },
    {
        "smpte",
        ChanOrder_Smpte,
        {
            ChanPos_FrontLeft,
            ChanPos_FrontRight,
            ChanPos_FrontCenter,
            ChanPos_LowFrequency,
            ChanPos_BackLeft,
            ChanPos_BackRight,
            ChanPos_FrontLeftOfCenter,
            ChanPos_FrontRightOfCenter,
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
    {
        "alsa",
        ChanOrder_Alsa,
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
const ChannelMapTable ChanMapTables[71] = {
    // 1.1-3c<>...
    {
        "1.1-3c<>1.1",
        ChanMask_Surround_1_1_3c,
        ChanMask_Surround_1_1,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 2.1<>...
    {
        "2.1<>1.1",
        ChanMask_Surround_2_1,
        ChanMask_Surround_1_1,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 3.1<>...
    {
        "3.1<>1.1",
        ChanMask_Surround_3_1,
        ChanMask_Surround_1_1,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "3.1<>2.1",
        ChanMask_Surround_3_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 3.1-3c<>...
    {
        "3.1-3c<>1.1-3c",
        ChanMask_Surround_3_1_3c,
        ChanMask_Surround_1_1_3c,
        {
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.7071068f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "3.1-3c<>3.1",
        ChanMask_Surround_3_1_3c,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 4.1<>...
    {
        "4.1<>2.1",
        ChanMask_Surround_4_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "4.1<>3.1",
        ChanMask_Surround_4_1,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.5000000f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.5000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 5.1<>...
    {
        "5.1<>3.1",
        ChanMask_Surround_5_1,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1<>4.1",
        ChanMask_Surround_5_1,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 5.1-3c<>...
    {
        "5.1-3c<>3.1-3c",
        ChanMask_Surround_5_1_3c,
        ChanMask_Surround_3_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_BackLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_BackRight, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1-3c<>5.1",
        ChanMask_Surround_5_1_3c,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 5.1.2<>...
    {
        "5.1.2<>3.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.2<>4.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.2<>5.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 5.1.2-3c<>...
    {
        "5.1.2-3c<>3.1-3c",
        ChanMask_Surround_5_1_2_3c,
        ChanMask_Surround_3_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.7071068f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_BackLeft, 0.7071068f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopMidLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_BackRight, 0.7071068f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopMidRight, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.2-3c<>5.1",
        ChanMask_Surround_5_1_2_3c,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.2-3c<>5.1-3c",
        ChanMask_Surround_5_1_2_3c,
        ChanMask_Surround_5_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopMidRight, 0.5000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.2-3c<>5.1.2",
        ChanMask_Surround_5_1_2_3c,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopMidLeft, 1.0000000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopMidRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 5.1.4<>...
    {
        "5.1.4<>3.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4<>4.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4<>5.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4<>5.1.2",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 5.1.4-3c<>...
    {
        "5.1.4-3c<>3.1-3c",
        ChanMask_Surround_5_1_4_3c,
        ChanMask_Surround_3_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.5000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_BackLeft, 0.7071068f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopBackLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_BackRight, 0.7071068f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopBackRight, 0.5000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4-3c<>5.1",
        ChanMask_Surround_5_1_4_3c,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4-3c<>5.1-3c",
        ChanMask_Surround_5_1_4_3c,
        ChanMask_Surround_5_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopFrontRight, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4-3c<>5.1.2-3c",
        ChanMask_Surround_5_1_4_3c,
        ChanMask_Surround_5_1_2_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "5.1.4-3c<>5.1.4",
        ChanMask_Surround_5_1_4_3c,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopFrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopFrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopBackLeft, 1.0000000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopBackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 6.1<>...
    {
        "6.1<>4.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "6.1<>5.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "6.1<>5.1.2",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_TopMidLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_TopMidLeft, ChanPos_BackCenter, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_TopMidRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_TopMidRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "6.1<>5.1.4",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_FrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_TopBackLeft, ChanPos_BackCenter, 0.7071068f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_TopBackRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 6.1-3c<>...
    {
        "6.1-3c<>5.1-3c",
        ChanMask_Surround_6_1_3c,
        ChanMask_Surround_5_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "6.1-3c<>5.1.2-3c",
        ChanMask_Surround_6_1_3c,
        ChanMask_Surround_5_1_2_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_TopMidLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_TopMidLeft, ChanPos_BackCenter, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_TopMidRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_TopMidRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "6.1-3c<>5.1.4-3c",
        ChanMask_Surround_6_1_3c,
        ChanMask_Surround_5_1_4_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackCenter, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackCenter, 0.7071068f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_FrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_TopBackLeft, ChanPos_BackCenter, 0.7071068f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_TopBackRight, ChanPos_BackCenter, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "6.1-3c<>6.1",
        ChanMask_Surround_6_1_3c,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackCenter, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 7.1<>...
    {
        "7.1<>5.1",
        ChanMask_Surround_7_1,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1<>5.1.2",
        ChanMask_Surround_7_1,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_TopMidLeft, ChanPos_BackLeft, 1.0000000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_TopMidRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1<>5.1.4",
        ChanMask_Surround_7_1,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_FrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_BackLeft, 1.0000000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1<>6.1",
        ChanMask_Surround_7_1,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 7.1-3c<>...
    {
        "7.1-3c<>5.1-3c",
        ChanMask_Surround_7_1_3c,
        ChanMask_Surround_5_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1-3c<>5.1.2-3c",
        ChanMask_Surround_7_1_3c,
        ChanMask_Surround_5_1_2_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_TopMidLeft, ChanPos_BackLeft, 1.0000000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_TopMidRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1-3c<>5.1.4-3c",
        ChanMask_Surround_7_1_3c,
        ChanMask_Surround_5_1_4_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_FrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_BackLeft, 1.0000000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1-3c<>6.1-3c",
        ChanMask_Surround_7_1_3c,
        ChanMask_Surround_6_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1-3c<>7.1",
        ChanMask_Surround_7_1_3c,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 7.1.2<>...
    {
        "7.1.2<>5.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2<>5.1.2",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopMidLeft, 1.0000000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopMidRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2<>5.1.4",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopMidLeft, 0.7071068f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopMidRight, 0.7071068f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopMidLeft, 0.7071068f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopMidRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2<>6.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_TopMidLeft, 0.7071068f },
            { ChanPos_BackCenter, ChanPos_TopMidRight, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2<>7.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_SideLeft, ChanPos_TopMidLeft, 0.7071068f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_SideRight, ChanPos_TopMidRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 7.1.2-3c<>...
    {
        "7.1.2-3c<>5.1-3c",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_5_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopMidRight, 0.5000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2-3c<>5.1.2-3c",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_5_1_2_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopMidLeft, 1.0000000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopMidRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2-3c<>5.1.4-3c",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_5_1_4_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopMidLeft, 0.7071068f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopMidRight, 0.7071068f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopMidLeft, 0.7071068f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopMidRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2-3c<>6.1-3c",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_6_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.5000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopMidLeft, 0.5000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopMidRight, 0.5000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.5000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopMidLeft, 0.5000000f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_TopMidLeft, 0.7071068f },
            { ChanPos_BackCenter, ChanPos_TopMidRight, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopMidRight, 0.5000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2-3c<>7.1",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_SideLeft, ChanPos_TopMidLeft, 0.7071068f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_SideRight, ChanPos_TopMidRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2-3c<>7.1-3c",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_7_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_SideLeft, ChanPos_TopMidLeft, 0.7071068f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_SideRight, ChanPos_TopMidRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.2-3c<>7.1.2",
        ChanMask_Surround_7_1_2_3c,
        ChanMask_Surround_7_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopMidLeft, 1.0000000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopMidRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 7.1.4<>...
    {
        "7.1.4<>5.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4<>5.1.2",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4<>5.1.4",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopFrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopFrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopBackLeft, 1.0000000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopBackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4<>6.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_TopBackLeft, 0.7071068f },
            { ChanPos_BackCenter, ChanPos_TopBackRight, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4<>7.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4<>7.1.2",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_7_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    // 7.1.4-3c<>...
    {
        "7.1.4-3c<>5.1-3c",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_5_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopFrontRight, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>5.1.2-3c",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_5_1_2_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>5.1.4-3c",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_5_1_4_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopFrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopFrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopBackLeft, 1.0000000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopBackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>6.1-3c",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_6_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopFrontRight, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // BL
            { ChanPos_BackLeft, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BC
            { ChanPos_BackCenter, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideLeft, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackCenter, ChanPos_TopBackLeft, 0.7071068f },
            { ChanPos_BackCenter, ChanPos_TopBackRight, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_SideRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>7.1",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>7.1-3c",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_7_1_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.7071068f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            { ChanPos_FrontLeftOfCenter, ChanPos_TopFrontLeft, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            { ChanPos_FrontRightOfCenter, ChanPos_TopFrontRight, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.7071068f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>7.1.2-3c",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_7_1_2_3c,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            // FLC
            { ChanPos_FrontLeftOfCenter, ChanPos_FrontLeftOfCenter, 1.0000000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            // FRC
            { ChanPos_FrontRightOfCenter, ChanPos_FrontRightOfCenter, 1.0000000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 0.7071068f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 0.7071068f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 0.7071068f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 0.7071068f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
    {
        "7.1.4-3c<>7.1.4",
        ChanMask_Surround_7_1_4_3c,
        ChanMask_Surround_7_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.0000000f },
            { ChanPos_FrontLeft, ChanPos_FrontLeftOfCenter, 0.7071068f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeftOfCenter, 0.7071068f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.0000000f },
            { ChanPos_FrontCenter, ChanPos_FrontRightOfCenter, 0.7071068f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.0000000f },
            { ChanPos_FrontRight, ChanPos_FrontRightOfCenter, 0.7071068f },
            // SL
            { ChanPos_SideLeft, ChanPos_SideLeft, 1.0000000f },
            // SR
            { ChanPos_SideRight, ChanPos_SideRight, 1.0000000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.0000000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.0000000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopFrontLeft, 1.0000000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopFrontRight, 1.0000000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopBackLeft, 1.0000000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopBackRight, 1.0000000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.0000000f },
        },
    },
};

} // namespace audio
} // namespace roc
