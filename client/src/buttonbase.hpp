/*
 * =====================================================================================
 *
 *       Filename: buttonbase.hpp
 *        Created: 08/25/2016 04:12:57
 *    Description:
 *              
 *              basic button class to handle event logic only
 *              1. no draw
 *              2. no texture id field
 *
 *              I support two callbacks only: off->on and on->click
 *              this class ask user to configure whether the on->click is triggered
 *              at the PRESS or RELEASE event.
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

#include "widget.hpp"
#include "pngtexdb.hpp"
#include "sdldevice.hpp"

class ButtonBase: public Widget
{
    public:
        enum ButtonState: int
        {
            BUTTON_OFF     = 0,
            BUTTON_OVER    = 1,
            BUTTON_PRESSED = 2,
        };

    protected:
        int m_State;

    protected:
        bool m_OnClickDone;

    protected:
        int m_Offset[3][2];

    protected:
        std::function<void()> m_OnOver;
        std::function<void()> m_OnClick;
        
    public:
        ButtonBase(
                int nX,
                int nY,
                int nW,
                int nH,

                const std::function<void()> &fnOnOver  = [](){},
                const std::function<void()> &fnOnClick = [](){},

                int nOffXOnOver  = 0,
                int nOffYOnOver  = 0,
                int nOffXOnClick = 0,
                int nOffYOnClick = 0,

                bool    bOnClickDone = true,
                Widget *pWidget      = nullptr,
                bool    bFreeWidget  = false)
            : Widget(nX, nY, nW, nH, pWidget, bFreeWidget)
            , m_State(BUTTON_OFF)
            , m_OnClickDone(bOnClickDone)
            , m_Offset
              {
                  {0            , 0           },
                  {nOffXOnOver  , nOffYOnOver },
                  {nOffXOnClick , nOffYOnClick},
              }
            , m_OnOver (fnOnOver)
            , m_OnClick(fnOnClick)
        {
            // we don't fail even if x, y, w, h are invalid
            // because derived class could reset it in its constructor
        }

    public:
        virtual ~ButtonBase() = default;

    public:
        bool ProcessEvent(const SDL_Event &, bool *);

    protected:
        int OffX() const
        {
            return m_Offset[State()][0];
        }

        int OffY() const
        {
            return m_Offset[State()][1];
        }

        int State() const
        {
            return m_State;
        }
};
