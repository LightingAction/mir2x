/*
 * =====================================================================================
 *
 *       Filename: indepmagic.cpp
 *        Created: 08/07/2017 21:31:24
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
#include "sdldevice.hpp"
#include "indepmagic.hpp"
#include "dbcomrecord.hpp"
#include "pngtexoffdb.hpp"

extern SDLDevice *g_SDLDevice;
extern PNGTexOffDB *g_MagicDB;

IndepMagic::IndepMagic(uint64_t nUID,
        int nMagicID,
        int nMagicParam,
        int nMagicStage,
        int nDirection,
        int nX,
        int nY,
        int nAimX,
        int nAimY,
        uint64_t nAimUID)
    : MagicBase(nMagicID, nMagicParam, nMagicStage)
    , m_UID(nUID)
    , m_Direction(nDirection)
    , m_X(nX)
    , m_Y(nY)
    , m_AimX(nAimX)
    , m_AimY(nAimY)
    , m_AimUID(nAimUID)
{}

IndepMagic::IndepMagic(uint64_t nUID,
        int nMagicID,
        int nMagicParam,
        int nSpeed,
        int nDirection,
        int nX,
        int nY,
        int nAimX,
        int nAimY)
    : IndepMagic(nUID,
            nMagicID,
            nMagicParam,
            nSpeed,
            nDirection,
            nX,
            nY,
            nAimX,
            nAimY,
            0)
{}

IndepMagic::IndepMagic(uint64_t nUID,
        int nMagicID,
        int nMagicParam,
        int nSpeed,
        int nDirection,
        int nX,
        int nY,
        uint64_t nAimUID)
    : IndepMagic(nUID,
            nMagicID,
            nMagicParam,
            nSpeed,
            nDirection,
            nX,
            nY,
            -1,
            -1,
            nAimUID)
{}

bool IndepMagic::Done() const
{
    if(!StageDone()){
        return false;
    }

    if(RefreshCache()){
        switch(m_CacheEntry->Stage){
            case EGS_DONE:
                {
                    return true;
                }
            default:
                {
                    return false;
                }
        }
    }else{
        // when we deref m_CacheEntry
        // we should call RefreshCache() first

        // when really done Update() will make current stage as EGS_NONE
        // then RefreshCache() makes m_CacheEntry as nullptr
        return true;
    }
}

void IndepMagic::Update(double fTime)
{
    // magic is driven by time, can't turn to DONE status without update
    // then only way to move the magic forward is to increase the accumulated time

    if(Done()){
        return;
    }

    m_AccuTime += fTime;
    ExecUpdateFunc();

    if(Done()){
        m_UpdateFunc.clear();
    }

    if(!StageDone()){
        return;
    }

    auto fnCheckStageValid = [this](int nNewStage) -> bool
    {
        if(auto &rstMR = DBCOM_MAGICRECORD(ID())){
            for(int nGfxEntryIndex = 1;; ++nGfxEntryIndex){
                if(auto &rstGfxEntry = rstMR.GetGfxEntry(nGfxEntryIndex)){
                    if(rstGfxEntry.Stage == nNewStage){
                        return true;
                    }
                }else{
                    break;
                }
            }
        }
        return false;
    };

    switch(Stage()){
        case EGS_START:
            {
                if(fnCheckStageValid(EGS_RUN)){
                    m_Stage = EGS_RUN;
                }else if(fnCheckStageValid(EGS_DONE)){
                    m_Stage = EGS_DONE;
                }else{
                    m_Stage = EGS_NONE;
                }
                break;
            }
        case EGS_RUN:
            {
                if(fnCheckStageValid(EGS_DONE)){
                    m_Stage = EGS_DONE;
                }else{
                    m_Stage = EGS_NONE;
                }
                break;
            }
        case EGS_DONE:
            {
                m_Stage = EGS_DONE;
                break;
            }
        default:
            {
                break;
            }
    }

    // clear the accumulated time
    // should I record the duration in total?
    m_AccuTime = 0.0;
}

void IndepMagic::Draw(int nViewX, int nViewY)
{
    if(!Done()){
        if(RefreshCache()){
            if(m_CacheEntry->GfxID >= 0){
                int nOffX = 0;
                int nOffY = 0;
                if(auto pTexture = g_MagicDB->Retrieve(m_CacheEntry->GfxID + Frame(), &nOffX, &nOffY)){
                    SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);
                    g_SDLDevice->DrawTexture(pTexture, DrawPX() - nViewX + nOffX, DrawPY() - nViewY + nOffY);
                }
            }
        }
    }
}

bool IndepMagic::DrawPLoc(int *pPX, int *pPY) const
{
    if(RefreshCache()){
        switch(m_CacheEntry->Type){
            case EGT_FIXED:
                {
                    if(pPX){ *pPX = AimX() * SYS_MAPGRIDXP; }
                    if(pPY){ *pPY = AimY() * SYS_MAPGRIDYP; }
                    return true;
                }
            default:
                {
                    break;
                }
        }
    }
    return false;
}
