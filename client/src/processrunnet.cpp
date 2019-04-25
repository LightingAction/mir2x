/*
 * =====================================================================================
 *
 *       Filename: processrunnet.cpp
 *        Created: 08/31/2015 03:43:46
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

#include <memory>
#include <cstring>

#include "log.hpp"
#include "client.hpp"
#include "dbcomid.hpp"
#include "monster.hpp"
#include "uidfunc.hpp"
#include "sysconst.hpp"
#include "pngtexdb.hpp"
#include "sdldevice.hpp"
#include "sdldevice.hpp"
#include "processrun.hpp"
#include "dbcomrecord.hpp"

// we get all needed initialization info for init the process run
void ProcessRun::Net_LOGINOK(const uint8_t *pBuf, size_t nLen)
{
    if(pBuf && nLen && (nLen == sizeof(SMLoginOK))){
        SMLoginOK stSMLOK;
        std::memcpy(&stSMLOK, pBuf, nLen);

        uint64_t nUID     = stSMLOK.UID;
        uint32_t nDBID    = stSMLOK.DBID;
        bool     bGender  = stSMLOK.Male;
        uint32_t nMapID   = stSMLOK.MapID;
        uint32_t nDressID = 0;

        int nX = stSMLOK.X;
        int nY = stSMLOK.Y;
        int nDirection = stSMLOK.Direction;

        LoadMap(nMapID);

        m_MyHeroUID = nUID;
        m_CreatureList[nUID] = std::make_shared<MyHero>(nUID, nDBID, bGender, nDressID, this, ActionStand(nX, nY, nDirection));

        CenterMyHero();
    }
}

void ProcessRun::Net_ACTION(const uint8_t *pBuf, size_t)
{
    SMAction stSMA;
    std::memcpy(&stSMA, pBuf, sizeof(stSMA));

    ActionNode stAction
    {
        stSMA.Action,
        stSMA.Speed,
        stSMA.Direction,
        stSMA.X,
        stSMA.Y,
        stSMA.AimX,
        stSMA.AimY,
        stSMA.AimUID,
        stSMA.ActionParam,
    };

    if(stSMA.MapID != MapID()){
        if(stSMA.UID != GetMyHero()->UID()){
            return;
        }

        // detected map switch for myhero
        // need to do map switch and parse current action

        LoadMap(stSMA.MapID);

        auto nUID       = GetMyHero()->UID();
        auto nDBID      = GetMyHero()->DBID();
        auto bGender    = GetMyHero()->Gender();
        auto nDress     = GetMyHero()->Dress();
        auto nDirection = GetMyHero()->CurrMotion().Direction;

        auto nX = stSMA.X;
        auto nY = stSMA.Y;

        m_UIDPending.clear();

        ClearCreature();
        m_CreatureList[m_MyHeroUID] = std::make_shared<MyHero>(nUID, nDBID, bGender, nDress, this, ActionStand(nX, nY, nDirection));

        CenterMyHero();
        GetMyHero()->ParseAction(stAction);
        return;
    }

    // map doesn't change
    // action from an existing charobject for current processrun

    if(auto pCreature = RetrieveUID(stSMA.UID)){
        // shouldn't accept ACTION_SPAWN
        // we shouldn't have spawn action after co created
        condcheck(stSMA.Action != ACTION_SPAWN);

        pCreature->ParseAction(stAction);
        switch(stAction.Action){
            case ACTION_SPACEMOVE2:
                {
                    if(stSMA.UID == m_MyHeroUID){
                        CenterMyHero();
                    }
                    return;
                }
            default:
                {
                    return;
                }
        }
        return;
    }

    // map doesn't change
    // action from an non-existing charobject, may need query

    switch(UIDFunc::GetUIDType(stSMA.UID)){
        case UID_PLY:
            {
                // do query only for player
                // can't create new player based on action information
                QueryCORecord(stSMA.UID);
                return;
            }
        case UID_MON:
            {
                switch(stSMA.Action){
                    case ACTION_SPAWN:
                        {
                            OnActionSpawn(stSMA.UID, stAction);
                            return;
                        }
                    default:
                        {
                            switch(UIDFunc::GetMonsterID(stSMA.UID)){
                                case DBCOM_MONSTERID(u8"变异骷髅"):
                                    {
                                        if(UIDPending(stSMA.UID)){
                                            return;
                                        }
                                        break;
                                    }
                            }

                            if(auto pMonster = Monster::CreateMonster(stSMA.UID, this, stAction)){
                                m_CreatureList[stSMA.UID].reset(pMonster);
                            }
                            return;
                        }
                }
                return;
            }
        default:
            {
                return;
            }
    }
}

void ProcessRun::Net_CORECORD(const uint8_t *pBuf, size_t)
{
    SMCORecord stSMCOR;
    std::memcpy(&stSMCOR, pBuf, sizeof(stSMCOR));

    if(stSMCOR.Action.MapID != MapID()){
        return;
    }

    ActionNode stAction
    {
        stSMCOR.Action.Action,
        stSMCOR.Action.Speed,
        stSMCOR.Action.Direction,
        stSMCOR.Action.X,
        stSMCOR.Action.Y,
        stSMCOR.Action.AimX,
        stSMCOR.Action.AimY,
        stSMCOR.Action.AimUID,
        stSMCOR.Action.ActionParam,
    };

    if(auto p = m_CreatureList.find(stSMCOR.Action.UID); p != m_CreatureList.end()){
        p->second->ParseAction(stAction);
        return;
    }

    switch(stSMCOR.COType){
        case CREATURE_MONSTER:
            {
                if(auto pMonster = Monster::CreateMonster(stSMCOR.Action.UID, this, stAction)){
                    m_CreatureList[stSMCOR.Action.UID].reset(pMonster);
                }
                break;
            }
        case CREATURE_PLAYER:
            {
                m_CreatureList[stSMCOR.Action.UID] = std::make_shared<Hero>(stSMCOR.Action.UID, stSMCOR.Player.DBID, true, 0, this, stAction);
                break;
            }
        default:
            {
                break;
            }
    }
}

void ProcessRun::Net_UPDATEHP(const uint8_t *pBuf, size_t)
{
    SMUpdateHP stSMUHP;
    std::memcpy(&stSMUHP, pBuf, sizeof(stSMUHP));

    if(stSMUHP.MapID == MapID()){
        if(auto p = RetrieveUID(stSMUHP.UID)){
            p->UpdateHP(stSMUHP.HP, stSMUHP.HPMax);
        }
    }
}

void ProcessRun::Net_NOTIFYDEAD(const uint8_t *pBuf, size_t)
{
    SMNotifyDead stSMND;
    std::memcpy(&stSMND, pBuf, sizeof(stSMND));

    if(auto p = RetrieveUID(stSMND.UID)){
        p->ParseAction(ActionDie(p->X(), p->Y(), p->CurrMotion().Direction, true));
    }
}

void ProcessRun::Net_DEADFADEOUT(const uint8_t *pBuf, size_t)
{
    SMDeadFadeOut stSMDFO;
    std::memcpy(&stSMDFO, pBuf, sizeof(stSMDFO));

    if(stSMDFO.MapID == MapID()){
        if(auto p = RetrieveUID(stSMDFO.UID)){
            p->DeadFadeOut();
        }
    }
}

void ProcessRun::Net_EXP(const uint8_t *pBuf, size_t)
{
    SMExp stSME;
    std::memcpy(&stSME, pBuf, sizeof(stSME));

    if(stSME.Exp){
        AddOPLog(OUTPORT_CONTROLBOARD, 2, "", u8"你获得了经验值%d", (int)(stSME.Exp));
    }
}

void ProcessRun::Net_MISS(const uint8_t *pBuf, size_t)
{
    SMMiss stSMM;
    std::memcpy(&stSMM, pBuf, sizeof(stSMM));

    if(auto p = RetrieveUID(stSMM.UID)){
        int nX = p->X() * SYS_MAPGRIDXP + SYS_MAPGRIDXP / 2 - 20;
        int nY = p->Y() * SYS_MAPGRIDYP - SYS_MAPGRIDYP * 1;
        AddAscendStr(ASCENDSTR_MISS, 0, nX, nY);
    }
}

void ProcessRun::Net_SHOWDROPITEM(const uint8_t *pBuf, size_t)
{
    SMShowDropItem stSMSDI;
    std::memcpy(&stSMSDI, pBuf, sizeof(stSMSDI));

    ClearGroundItem(stSMSDI.X, stSMSDI.Y);
    for(size_t nIndex = 0; nIndex < std::extent<decltype(stSMSDI.IDList)>::value; ++nIndex){
        CommonItem stCommonItem(stSMSDI.IDList[nIndex].ID, stSMSDI.IDList[nIndex].DBID);
        if(stCommonItem){
            AddGroundItem(stCommonItem, stSMSDI.X, stSMSDI.Y);
        }else{
            break;
        }
    }
}

void ProcessRun::Net_FIREMAGIC(const uint8_t *pBuf, size_t)
{
    SMFireMagic stSMFM;
    std::memcpy(&stSMFM, pBuf, sizeof(stSMFM));

    if(auto &rstMR = DBCOM_MAGICRECORD(stSMFM.Magic)){
        AddOPLog(OUTPORT_CONTROLBOARD, 2, "", u8"使用魔法: %s", rstMR.Name);

        switch(stSMFM.Magic){
            case DBCOM_MAGICID(u8"魔法盾"):
                {
                    if(auto stEntry = rstMR.GetGfxEntry(u8"开始")){
                        if(auto pCreature = RetrieveUID(stSMFM.UID)){
                            pCreature->AddAttachMagic(stSMFM.Magic, 0, stEntry.Stage);
                        }
                        return;
                    }
                }
        }

        const GfxEntry *pEntry = nullptr;
        if(stSMFM.UID != GetMyHero()->UID()){
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"启动")); }
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"开始")); }
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"运行")); }
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"结束")); }
        }else{
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"开始")); }
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"运行")); }
            if(!(pEntry && *pEntry)){ pEntry = &(rstMR.GetGfxEntry(u8"结束")); }
        }

        if(pEntry && *pEntry){
            switch(pEntry->Type){
                case EGT_BOUND:
                    {
                        if(auto pCreature = RetrieveUID(stSMFM.AimUID)){
                            pCreature->AddAttachMagic(stSMFM.Magic, 0, pEntry->Stage);
                        }
                        break;
                    }
                case EGT_FIXED:
                    {
                        m_IndepMagicList.emplace_back(std::make_shared<IndepMagic>
                        (
                            stSMFM.UID,
                            stSMFM.Magic,
                            stSMFM.MagicParam,
                            pEntry->Stage,
                            stSMFM.Direction,
                            stSMFM.X,
                            stSMFM.Y,
                            stSMFM.AimX,
                            stSMFM.AimY,
                            stSMFM.AimUID
                        ));
                        break;
                    }
                case EGT_SHOOT:
                    {
                        break;
                    }
                case EGT_FOLLOW:
                    {
                        break;
                    }
                default:
                    {
                        break;
                    }
            }
        }
    }
}

void ProcessRun::Net_OFFLINE(const uint8_t *pBuf, size_t)
{
    SMOffline stSMO;
    std::memcpy(&stSMO, pBuf, sizeof(stSMO));

    if(stSMO.MapID == MapID()){
        if(auto pCreature = RetrieveUID(stSMO.UID)){
            pCreature->AddAttachMagic(DBCOM_MAGICID(u8"瞬息移动"), 0, EGS_INIT);
        }
    }
}

void ProcessRun::Net_PICKUPOK(const uint8_t *pBuf, size_t)
{
    SMPickUpOK stSMPUOK;
    std::memcpy(&stSMPUOK, pBuf, sizeof(stSMPUOK));

    GetMyHero()->GetInvPack().Add(stSMPUOK.ID);
    RemoveGroundItem(CommonItem(stSMPUOK.ID, 0), stSMPUOK.X, stSMPUOK.Y);
    AddOPLog(OUTPORT_CONTROLBOARD, 2, "", u8"捡起%s于坐标(%d, %d)", DBCOM_ITEMRECORD(stSMPUOK.ID).Name, (int)(stSMPUOK.X), (int)(stSMPUOK.Y));
}

void ProcessRun::Net_GOLD(const uint8_t *pBuf, size_t)
{
    SMGold stSMG;
    std::memcpy(&stSMG, pBuf, sizeof(stSMG));
    GetMyHero()->SetGold(stSMG.Gold);
}
