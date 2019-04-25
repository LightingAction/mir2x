/*
 * =====================================================================================
 *
 *       Filename: controlboard.hpp
 *        Created: 08/21/2016 04:12:57
 *    Description: main control pannel for running client
 *                 try support dynamically allocated control board
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

#pragma once
#include <cstdint>
#include <functional>

#include "log.hpp"
#include "widget.hpp"
#include "pngtexdb.hpp"
#include "sdldevice.hpp"
#include "inputline.hpp"
#include "tritexbutton.hpp"
#include "linebrowserboard.hpp"

class ProcessRun;
class ControlBoard: public Widget
{
    private:
        ProcessRun *m_ProcessRun;

    private:
        TritexButton m_ButtonClose;
        TritexButton m_ButtonMinize;
        TritexButton m_ButtonInventory;

    private:
        InputLine        m_CmdLine;
        LabelBoard       m_LocBoard;
        LineBrowserBoard m_LogBoard;

    public:
        ControlBoard(
                int,            // x
                int,            // y
                int,            // screen width
                ProcessRun *,   // 
                Widget *,       //
                bool);          //

    public:
        ~ControlBoard() = default;

    public:
        void DrawEx(int, int, int, int, int, int);

    public:
        void Update(double);
        bool ProcessEvent(const SDL_Event &, bool *);

    public:
        void InputLineDone();

    public:
        void AddLog(int, const char *);

    public:
        bool CheckMyHeroMoved();
};
