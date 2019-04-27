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

// #include <cstdio>
// #include <cinttypes>
void CalcShadowRemovalAlpha(uint32_t *pData, size_t nWidth, size_t nHeight, uint32_t nCheckMask, uint32_t nCheckShadow, uint32_t nShadowColor)
{
    if(!(pData && nWidth && nHeight)){
        throw std::invalid_argument(str_fflprintf(": invalid buffer: (%p, %zu, %zu)", pData, nWidth, nHeight));
    }

    if(nWidth < 3 || nHeight < 3){
        return;
    }

    // for(size_t nX = 0; nX < nWidth; ++nX){
    //     for(size_t nY = 0; nY < nHeight; ++nY){
    //         std::printf("%08" PRIX32 "\n", pData[nY * nWidth + nX] & nCheckMask);
    //     }
    // }

    std::vector<int> stBuf(nWidth * nHeight);
    for(size_t nX = 0; nX < nWidth; ++nX){
        for(size_t nY = 0; nY < nHeight; ++nY){
            stBuf[nY * nWidth + nX] = (int)((pData[nY * nWidth + nX] & nCheckMask) == (nCheckShadow & nCheckMask));
        }
    }

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(stBuf[nY * nWidth + nX]){
                if(false
                        || !stBuf[(nY - 1) * nWidth + (nX - 1)]
                        || !stBuf[(nY - 1) * nWidth + (nX + 1)]
                        || !stBuf[(nY + 1) * nWidth + (nX - 1)]
                        || !stBuf[(nY + 1) * nWidth + (nX + 1)]){
                    stBuf[nY * nWidth + nX] = 0;
                    continue;
                }

                if(false
                        || stBuf[(nY - 1) * nWidth + nX]
                        || stBuf[(nY + 1) * nWidth + nX]
                        || stBuf[nY * nWidth + (nX - 1)]
                        || stBuf[nY * nWidth + (nX + 1)]){
                    stBuf[nY * nWidth + nX] = 0;
                    continue;
                }
            }
        }
    }

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(!stBuf[nY * nWidth + nX]){
                if(false
                        || (stBuf[nY * nWidth + (nX - 1)] && stBuf[nY * nWidth + (nX + 1)])
                        || (stBuf[(nY - 1) * nWidth + nX] && stBuf[(nY + 1) * nWidth + nX])){
                    stBuf[nY * nWidth + nX] = 1;
                }
            }
        }
    }

    for(size_t nX = 1; nX < nWidth - 1; ++nX){
        for(size_t nY = 1; nY < nHeight - 1; ++nY){
            if(stBuf[nY * nWidth + nX]){
                pData[nY * nWidth + nX] = nShadowColor;
            }
        }
    }
}
