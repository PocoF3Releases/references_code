/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RECOVERY_MI_SCREEN_UI_H
#define RECOVERY_MI_SCREEN_UI_H

#include "recovery_ui/device.h"
#include "recovery_ui/screen_ui.h"
#include "miutil/mi_battery.h"

class View {
  public:
    View(void);

    enum GravityElement {
        NONE_GRAVITY, TOP, CENTER_HORIZONTAL, CENTER_VERTICAL, CENTER, BOTTOM, BOTTOM_CENTER
    };

    void SetWidth(int width) { mWidth = width; }
    int GetWidth(void) { return mWidth; }

    void SetHeight(int height) { mHeight = height; }
    int GetHeight(void) { return mHeight; }

    void SetGravity(GravityElement gravity) { mGravity = gravity; }
    GravityElement GetGravity(void) { return mGravity; }

    void SetNormal(GRSurface** surface) { mNormal = surface; }
    GRSurface** GetNormal(void) { return mNormal; }

    void SetSelected(GRSurface** surface) { mSelected = surface; }
    GRSurface** GetSelected(void) { return mSelected; }

    void Init(void);

    void Destroy(void);

  private:
    // Width of this view, mostly the screen width
    int mWidth;

    // Height of this view
    int mHeight;

    // Describes how the child views are positioned.
    GravityElement mGravity;

    // The normal image surface for this view,
    // for an empty view, the surface is null.
    GRSurface** mNormal;

    // The selected image surface for this view,
    // for an empty view, the surface is null.
    GRSurface** mSelected;
};

class MiuiMenu {
  public:
    MiuiMenu(void);

    void SetMenuItems(int items) { mMenuItems = items; }
    int  GetMenuItems(void) { return mMenuItems; }

    void SetMenuStart(int start) { mMenuStart = start; }
    int  GetMenuStart(void) { return mMenuStart; }

    void SetMenuSelected(int selected) { mMenuSelected = selected; }
    int  GetMenuSelected(void) { return mMenuSelected; }

    void SetCounts(int counts) { mCounts = counts; }
    int  GetCounts(void) { return mCounts; }

    void  SetChild(View view, int index) { mChildren[index] = view; }
    View& GetChild(int index) { return mChildren[index]; }

    void Init(void);

    void Destroy(void);

  private:
    // How many menu items
    int mMenuItems;

    // The position of first menu item in the views array
    int mMenuStart;

    // The selected menu item index( 0 <= menu_sel < menu_items)
    int mMenuSelected;

    // How many child views
    int mCounts;

    // The max views of a view group.
    static const int kMaxChildViews = 12;

    // Array of child view in y-axis order
    View mChildren[kMaxChildViews];
};

class ProgressBar {
  public:
    ProgressBar(void);

    void SetProgress(float progress) { mProgress = progress; }
    float GetProgress(void) { return mProgress; }

    void SetProgressScopeStart(float start) { mProgressScopeStart = start; }
    float GetProgressScopeStart(void) { return mProgressScopeStart; }

    void SetProgressScopeSize(float size) { mProgressScopeSize = size; }
    float GetProgressScopeSize(void) { return mProgressScopeSize; }

    void SetProgressScopeTime(time_t time) { mProgressScopeTime = time; }
    time_t GetProgressScopeTime(void) { return mProgressScopeTime; }

    void SetProgressScopeDuration(time_t duration) { mProgressScopeDuration = duration; }
    time_t GetProgressScopeDuration(void) { return mProgressScopeDuration; }

    void SetCounts(int counts) { mCounts = counts; }
    int GetCounts(void) { return mCounts; }

    void  SetChild(View view, int index) { mChildren[index] = view; }
    View& GetChild(int index) { return mChildren[index]; }

    void Init(void);

    float RelocateProgress();
    float RelocateProgress(float progress);

    void ComputeProgress(bool reset);

    void ComputeProgress(float portion, float seconds);

    void Destroy(void);

  private:
    // The progress of current progressbar
    float mProgress;
    float mProgressScopeStart;
    float mProgressScopeSize;

    double mProgressScopeTime;
    double mProgressScopeDuration;

    // How many child views
    int mCounts;

    // The max views of a view group.
    static const int kMaxChildViews = 10;

    // Array of child view in y-axis order
    View mChildren[kMaxChildViews];
};

class MiScreenRecoveryUI : public ScreenRecoveryUI {
  public:
    MiScreenRecoveryUI(void);

    // enumeration constant definition for all the images
    enum ImageElement {
#define IMAGE(con, normal, selected) con, con##_SELECTED,
#include "recovery_ui/image.def"
#undef IMAGE /* IMAGE */
        IMAGE_COUNT
    };

    bool Init(const std::string& locale);

    void SetBackground(Icon icon);
    void SetCurrentState(bool state);

    void InitLanguageSpecificResource(const char *language);
    /**
      * Prompt a menu to show information and wait user selection.
      *   if not menu_only return handle result.
      * Deprecated in the future, use ShowMenu in ScreenRecovery instead
      *  @param id                   the menu id
      *  @param initial_selection    the initial position of icon
      */
    int GetMenuSelection(const char* id, int menu_only,
                         int initial_selection,
                         Device* device);
    void StartMenu(const char* const * headers, const char* const * items,
                   int initial_selection); // Nothing to do.
    void StartMenu(const char *id, int initial_selection);
    int  SelectMenu(int sel);
    void EndMenu(void);

    void StartProgress(float portion, float seconds);
    void SetProgressType(ProgressType type);
    void SetProgress(float fraction);
    void ShowProgress(float portion, float seconds);
    void EndProgress(void);

    void SetStage(int current, int max);
    void SetLocale(const std::string& new_locale);

    void Print(const char *fmt, ...); // __attribute__((format(printf, 1, 2)));
    void PrintOnScreenOnly(const char *fmt, ...); // __attribute__((format(printf, 1, 2)));

    // MIUI ADD:
    // Need to close display device when recovery reboot or shutdown.
    void SafeCloseDisplayDevice(void);

    // text log
    void ShowText(bool visible);
    bool IsTextVisible(void);
    bool WasTextEverVisible(void);

    void KeyLongPress(int);

    // sub menus
    int SubMenuReboot(Device *device);
    int SubMenuApplyExt(Device *device, const char *verification_msg,
                    const char *update_package, bool should_wipe_cache, int retry_count, bool install_with_fuse = false);
    int SubMenuWipe();
    int SubMenuWipeCmdWipeCache(void);
    int SubMenuWipeCmdWipeData(bool convert_fbe);
    void SetDevice(Device* device) {device_ = std::unique_ptr<Device>(device);};
    Device* GetDevice() { return device_.get();};
    int SubMenuWipeCmdFormatData(void);
    int SubMenuWipeCmdFormatStorage(void);
    int SubMenuWipeCmdWipeEfs(void);
    void ShowAbnormalBootReason(int count, int index, int height);
    /**
      * wipe data and cache partition but skip some directories.
      * the skiped directories are defined in internal_wipe_data_partial_erase_cache();
      * usually called by "1217" factoryreset code.
      *
      * @retval   install status type of result
    */
    int SubMenuWipeCmdWipePartial(void);
    int StartInstallPackage(const char* path, bool* wipe_cache, const char* install_file,
                    bool needs_mount, char* verification_msg, bool sideload = false);

    // visiablize resources if version dismatch
    void SetVersionMismatch(bool is_mismatch) {
        mShowMismatch = is_mismatch;
    };

    // visiablize image if NV data is corrupted
    void SetNVdataCorrupted(bool is_NVcorrupted) {
        mShowNVCorrupted = is_NVcorrupted;
    };

    // visiablize image if support maintenance mode
    void SetShowMaintenanceMode(bool is_supportMaintenanceMode) {
        mShowMaintenance = is_supportMaintenanceMode;
    };

    // visiablize image if boot reason is BootMonitor
    void SetBootMonitor(bool is_bootmonitor) {
        mBootMonitor = is_bootmonitor;
    };

    // visiablize image if boot reason is enablefilecrypto_failed
    void SetEnableFileCryptoFailed(bool is_enablefilecryptofailed) {
        mEnableFileCryptoFailed = is_enablefilecryptofailed;
    };

    // visiablize image if boot reason is fs_mgr_mount_all
    void SetFsMgrMountAll(bool is_fsmgrmountall) {
        mFsMgrMountAll = is_fsmgrmountall;
    };

    // visiablize image if boot reason is init_user0_failed
    void SetInitUser0Failed(bool is_inituser0failed) {
        mInitUser0Failed = is_inituser0failed;
    };

    // visiablize image if boot reason is RescueParty
    void SetRescueParty(bool is_rescueparty) {
        mRescueParty = is_rescueparty;
    };

    // visiablize image if boot reason is set_policy_failed
    void SetPolicyFailed(bool is_setpolicyfailed) {
        mSetPolicyFailed = is_setpolicyfailed;
    };

    // visiablize image if Ramdump partition presents
    void SetRamdumpPresented(bool has) {
        mHasRamdump = has;
    };

    void set_keytip_pos(void);

    enum MenuConfirmItem {
        CHOICE_NO, CHOICE_YES
    };

    enum MenuWipeItem {
        MENU_WIPE_ITEM_BACK_MAIN,
        MENU_WIPE_ITEM_WIPE_DATA
    };

    enum BackgroundIcon {
        BACKGROUND_STATE_CONNECT=0,
        BACKGROUND_STATE_UNCONNECT,
        BACKGROUND_STATE_FASTBOOTD,
        BACKGROUND_STATE_MAX,
    };

  private:
    // About menu
    std::unique_ptr<Device> device_;
    MiuiMenu mCurrentMenu;
    bool mShowCurrentMenu;
    bool mShowCurrentState;
    bool mShowMismatch = false;
    bool mShowNVCorrupted = false;
    bool mShowMaintenance = false;
    bool mHasRamdump = false;

    bool mBootMonitor = false;
    bool mEnableFileCryptoFailed = false;
    bool mFsMgrMountAll = false;
    bool mInitUser0Failed = false;
    bool mRescueParty = false;
    bool mSetPolicyFailed = false;
    // About progressbar
    ProgressBar mCurrentProgress;
    bool mShowCurrentProgress;
    // About text
    long mShowTextTimes = 0;
    bool mShowCurrentText;
    pthread_mutex_t mUpdateMutex;

    /* mImageNames and mImageSurfaces is 1-to-1 relationship */
    GRSurface* mImageSurfaces[IMAGE_COUNT];
    static const char* mImageNames[IMAGE_COUNT];

    static const int kFlushFps = 30;
    static constexpr double kWipeDuration = 100.0;
    static constexpr double kUpdateDuration = 600.0;
    static constexpr double kSideloadDuration = 660.0;
    int KEYTIP_UPSIDE_SHIFT;

    struct battery_state mBatteryState;

    pthread_t mFlushTid;

    // About background
    GRSurface* mBackgroundIcon[BACKGROUND_STATE_MAX];
    GRSurface* backgroundText[5];
    GRSurface* mUpdateTips;

    // ======================================================
    // private methods.
    static void* flush_thread(void *cookie);
    void flush_loop(void);

    void load_home_resources(void);
    void load_common_resources(const char *language, int start, int end);
    void LoadLocalizedBitmap(const char* filename, GRSurface** surface);

    // About menu.
    void set_menu_home(void);
    void set_menu_reboot_single(void);
    //void set_menu_reboot_double(int current_system_id);
    void set_menu_wipe(void);
    void set_menu_common_result(ImageElement img);
    void set_menu_common_confirm(ImageElement img);

    void update_screen_locked(void);

    void draw_background_locked(void);
    void draw_screen_locked(void);
    void draw_logo_locked(void);
    void draw_textlogo_locked(void);
    void draw_battery_locked(void);
    void draw_menu_locked(void);
    void draw_state_locked(void);
    void draw_view_locked(View &view, int x, int y, bool selected);

    void compute_image_offset(View &view, int *xoff, int *yoff);

    // About progress
    void set_progress_common_start();

    void draw_progress_locked(void);
    void draw_percent_locked(int x, int y);
    void draw_progressbar_locked(View &view, int x, int y);
    void draw_text_locked(int x, int y);
};

#endif /* RECOVERY_MI_SCREEN_UI_H */
