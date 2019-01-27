#pragma once

#include "CStreamer.h"

class SimStreamer: public CStreamer
{
    bool m_showBig;
public:
    SimStreamer(SOCKET aClient, bool showBig);

    virtual void    streamImage();
};
