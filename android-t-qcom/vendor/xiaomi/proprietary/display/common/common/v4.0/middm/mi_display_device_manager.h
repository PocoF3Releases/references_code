#ifndef _MI_DISPLAY_DEVICE_MANAGER_H_
#define _MI_DISPLAY_DEVICE_MANAGER_H_

#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/Singleton.h>
#include <mi_disp.h>

namespace android {

class MiDisplayDeviceManager : public Singleton<MiDisplayDeviceManager>
{
public:
    MiDisplayDeviceManager();
    ~MiDisplayDeviceManager();

    int setFeature(struct disp_feature_req *feature_req);
    int setDozeBrightness(struct disp_doze_brightness_req *doze_req);
    int getDozeBrightness(struct disp_doze_brightness_req *doze_req);
    int getPanelInfo(struct disp_panel_info *panel_info_req);
    int getWPInfo(struct disp_wp_info *wp_info_req);
    int getFPS(struct disp_fps_info *fps_info_req);
    int registerEvent(struct disp_event_req *event_req);
    int deRegisterEvent(struct disp_event_req *event_req);
    int writeDsiCmd(struct disp_dsi_cmd_req* dsi_cmd_req);

private:
    int mMiDeviceFd;
};
}; // namespace android
#endif
