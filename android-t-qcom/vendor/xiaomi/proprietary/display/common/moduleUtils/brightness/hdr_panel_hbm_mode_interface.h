/*
* Copyright (c) 2021 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef __HDR_PANEL_HBM_MODE_INTERFACE__
#define __HDR_PANEL_HBM_MODE_INTERFACE__

#include <stdint.h>
#include <string>

class PanelHBMModeInterface
{
 public:
    virtual ~PanelHBMModeInterface() {}
    virtual bool IsHBMMode() = 0;
    virtual float GetNitsforHBM() = 0;
};

class PanelHBMModeFactoryIntf
{
 public:
    virtual ~PanelHBMModeFactoryIntf() {}
    virtual PanelHBMModeInterface* CreatePanelHBMModeIntf(const char*) = 0;
    virtual void DestroyPanelHBMModeIntf(PanelHBMModeInterface*) = 0;
};

/* the only export function to return PanelHBMModeFactoryIntf pointer */
extern "C" PanelHBMModeFactoryIntf* GetPanelHBMModeFactoryIntf();

#endif /* __HDR_PANEL_HBM_MODE_INTERFACE__ */
