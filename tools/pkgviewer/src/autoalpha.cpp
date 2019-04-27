/*
 * =====================================================================================
 *
 *       Filename: autoalpha.cpp
 *        Created: 02/12/2019 06:24:47
 *    Description: 
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include "strfunc.hpp"
#include "autoalpha.hpp"

void CalcPixelAutoAlpha(uint32_t *pData, size_t nDataLen)
{
    if(!(pData && nDataLen)){
        throw std::invalid_argument(str_fflprintf(": invalid buffer: (%p, %zu)", pData, nDataLen));
    }

    for(size_t nIndex = 0; nIndex < nDataLen; ++nIndex){
        uint8_t a = ((pData[nIndex] & 0XFF000000) >> 24);

        if(a == 0){
            continue;
        }

        uint8_t r = ((pData[nIndex] & 0X00FF0000) >> 16);
        uint8_t g = ((pData[nIndex] & 0X0000FF00) >>  8);
        uint8_t b = ((pData[nIndex] & 0X000000FF) >>  0);

        a = std::max<uint8_t>({r, g, b});

        r = std::lround(1.0 * r * 255.0 / a);
        g = std::lround(1.0 * g * 255.0 / a);
        b = std::lround(1.0 * b * 255.0 / a);

        pData[nIndex] = ((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b);
    }
}

void CalcShadowRemovalAlpha(uint32_t *pData, size_t nWidth, size_t nHeight, uint32_t nShadowColor)
{
    if(!(pData && nWidth && nHeight)){
        throw std::invalid_argument(str_fflprintf(": invalid buffer: (%p, %zu, %zu)", pData, nWidth, nHeight));
    }

    if(nWidth < 3 || nHeight < 3){
        return;
    }

    // -1: normal
    //  0: transparent
    //  1: has shadow color, but can be noise points
    //  2: shadow points for sure

    std::vector<int> stvMark(nWidth * nHeight);
    auto fnGetMarkRef = [nWidth, &stvMark](size_t nX, size_t nY) -> int &
    {
        return stvMark.at(nY * nWidth + nX);
    };

    for(size_t nX = 0; nX < nWidth; ++nX){
        for(size_t nY = 0; nY < nHeight; ++nY){
            if(uint32_t nPixel = pData[nY * nWidth + nX]; nPixel & 0XFF000000){
                // alpha not 0
                // can be shadow pixels, noise points or normal points
                if(false
                        || (nPixel & 0X00FFFFFF) == 0X00080000
                        || (nPixel & 0X00FFFFFF) == 0X00000800
                        || (nPixel & 0X00FFFFFF) == 0X00000008){
                    fnGetMarkRef(nX, nY) = 1;
                }else{
                    fnGetMarkRef(nX, nY) = -1;
                }
            }else{
                // alpha is 0
                // can be grids between shadow pixels
                fnGetMarkRef(nX, nY) = 0;
            }
        }
    }

    // mark the 100% sure shadow points, strict step
    // can fail the edge of shadow area, shadow area too thin also fails

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(fnGetMarkRef(nX, nY) == 1){
                if(true
                        && fnGetMarkRef(nX - 1, nY - 1) > 0
                        && fnGetMarkRef(nX + 1, nY - 1) > 0
                        && fnGetMarkRef(nX - 1, nY + 1) > 0
                        && fnGetMarkRef(nX + 1, nY + 1) > 0

                        && fnGetMarkRef(nX, nY - 1) == 0
                        && fnGetMarkRef(nX, nY + 1) == 0
                        && fnGetMarkRef(nX - 1, nY) == 0
                        && fnGetMarkRef(nX + 1, nY) == 0){

                    // check neighbors around it
                    // mark as shadow points for 100% sure
                    fnGetMarkRef(nX, nY) = 2;
                }
            }
        }
    }

    // after previous step, if a pixel is 1, then it must be a shadow pixel
    // now add back edge points

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(fnGetMarkRef(nX, nY) == 2){
                if(auto &nMark = fnGetMarkRef(nX - 1, nY - 1); nMark == 1){ nMark = 2; }
                if(auto &nMark = fnGetMarkRef(nX + 1, nY - 1); nMark == 1){ nMark = 2; }
                if(auto &nMark = fnGetMarkRef(nX - 1, nY + 1); nMark == 1){ nMark = 2; }
                if(auto &nMark = fnGetMarkRef(nX + 1, nY + 1); nMark == 1){ nMark = 2; }
            }
        }
    }

    // fill those 0's inside 1's
    // edges can be filled, but area too thin can't be filled

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(fnGetMarkRef(nX, nY) == 0){
                if(false
                        || (fnGetMarkRef(nX - 1, nY) == 2 && fnGetMarkRef(nX + 1, nY) == 2)
                        || (fnGetMarkRef(nX, nY - 1) == 2 && fnGetMarkRef(nX, nY + 1) == 2)){
                    fnGetMarkRef(nX, nY) = 2;
                }
            }
        }
    }

    // done
    // for pixel marked as 1 or 2, assign given color
    // also assign for pixels marked as 1, because I don't want extra process for areas too thin

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(fnGetMarkRef(nX, nY) > 0){
                pData[nY * nWidth + nX] = nShadowColor;
            }
        }
    }
}
