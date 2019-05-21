#ifndef MWCALACSIZE_H
#define MWCALACSIZE_H

#include "LibMWVideoRender.h"

inline bool fmt_is_rgb(
    mw_colorformat_e t_fmt
    )
{
    switch (t_fmt) {
    case MW_RENDER_BGR24:
    case MW_RENDER_RGBA:
    case MW_RENDER_RGB10:
    case MW_RENDER_BGRA:
        return true;
    default:
        return false;
    }
}

inline bool fmt_is_packed(
    mw_colorformat_e t_fmt
    )
{
    switch (t_fmt) {
    case MW_RENDER_NV12:
    //case MWFOURCC_NV21:
    //case MWFOURCC_YV12:
    //case MWFOURCC_IYUV:
    case MW_RENDER_I420:
    case MW_RENDER_I422:
    //case MWFOURCC_YV16:
    case MW_RENDER_NV16:
    //case MWFOURCC_NV61:
    case MW_RENDER_P010:
    case MW_RENDER_P210:
        return false;
    default:
        return true;
    }
}

inline int fmt_get_bpp(
    mw_colorformat_e t_fmt
    )
{
    switch (t_fmt) {
    //case MWFOURCC_GREY:
    //case MWFOURCC_Y800:
    //case MWFOURCC_Y8:
    //    return 8;

    case MW_RENDER_I420:
    //case MWFOURCC_IYUV:
    //case MWFOURCC_YV12:
    case MW_RENDER_NV12:
    //case MWFOURCC_NV21:
        return 12;

    //case MWFOURCC_Y16:
    //case MWFOURCC_RGB15:
    //case MWFOURCC_BGR15:
    //case MWFOURCC_RGB16:
    //case MWFOURCC_BGR16:
    case MW_RENDER_YUY2:
    //case MWFOURCC_YUYV:
    case MW_RENDER_UYVY:
    //case MWFOURCC_YVYU:
    //case MWFOURCC_VYUY:
    case MW_RENDER_I422:
    //case MWFOURCC_YV16:
    //case MWFOURCC_NV16:
    //case MWFOURCC_NV61:
        return 16;

    //case MWFOURCC_IYU2:
    case MW_RENDER_V308:
    //case MWFOURCC_RGB24:
    case MW_RENDER_BGR24:
    //case MW_RENDER_P010:
        return 24;

    //case MWFOURCC_AYUV:
    //case MWFOURCC_UYVA:
    case MW_RENDER_V408:
    //case MWFOURCC_VYUA:
    case MW_RENDER_RGBA:
	case MW_RENDER_RGB10:
    case MW_RENDER_BGRA:
    //case MWFOURCC_ARGB:
    //case MWFOURCC_ABGR:
    case MW_RENDER_Y410:
    //case MWFOURCC_V410:
    //case MW_RENDER_P210:
        return 32;

    default:
        return 0;
    }
}

inline int fmt_calcminstride(
    mw_colorformat_e fmt,
    int cx,
    int dwAlign
    )
{
    bool bPacked = fmt_is_packed(fmt);

    int cbLine;

    if (bPacked) {
        int nBpp = fmt_get_bpp(fmt);
        cbLine = (cx * nBpp) / 8;
    }
    else {
        switch (fmt) {
        case MW_RENDER_P010:
        case MW_RENDER_P210:
            cbLine = cx * 2;
            break;
        default:
            cbLine = cx;
            break;
        }
    }

    return (cbLine + dwAlign - 1) & ~(dwAlign - 1);
}

inline int fmt_calcimagesize(
    mw_colorformat_e fmt,
    int cx,
    int cy,
    int cbStride
    )
{
    bool bPacked = fmt_is_packed(fmt);

    if (bPacked) {
        int nBpp = fmt_get_bpp(fmt);
        int cbLine = (cx * nBpp) / 8;
        if (cbStride < cbLine)
            return 0;

        return cbStride * cy;
    }
    else {
        if (cbStride < cx)
            return 0;

        switch (fmt) {
        case MW_RENDER_NV12:
        //case MWFOURCC_NV21:
        //case MWFOURCC_YV12:
        //case MWFOURCC_IYUV:
        case MW_RENDER_I420:
            if ((cbStride & 1) || (cy & 1))
                return 0;
            return cbStride * cy * 4 / 2;
        case MW_RENDER_I422:
        //case MWFOURCC_YV16:
        case MW_RENDER_NV16:
        //case MWFOURCC_NV61:
            if (cbStride & 1)
                return 0;
            return cbStride * cy * 2;
        case MW_RENDER_P010:
            if ((cbStride & 3) || (cy & 1))
                return 0;
            return cbStride * cy * 4 / 2;
        case MW_RENDER_P210:
            if (cbStride & 3)
                return 0;
            return cbStride * cy * 2;
        default:
            return 0;
        }
    }
}

#endif // MWCALACSIZE_H
