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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include "recovery_ui/device.h"
#include "minui/minui.h"
#include "otautil/paths.h"
#include "recovery_ui/ui.h"
#include "recovery_ui/screen_ui.h"
#include "recovery_ui/mi_screen_ui.h"
#include "install/install.h"
#include "install/fuse_install.h"
#include <cutils/properties.h>

#include "miutil/mi_battery.h"
#include "miutil/mi_system.h"
#include "install/mi_wipe.h"
#include "miutil/mi_utils.h"
#include "miutil/mi_verification.h"
#include "install/wipe_data.h"

#include <android-base/unique_fd.h>
#include <android-base/file.h>
#include "recovery_utils/roots.h"

#if defined(WITH_XXXHDPI)
#include "recovery_ui/mi_xxxhdpi.h"
#elif defined(WITH_XXHDPI)
#include "recovery_ui/mi_xxhdpi.h"
#elif defined(WITH_XHDPI)
#include "recovery_ui/mi_xhdpi.h"
#else
//#error "Not support dpi defined by PRODUCT_AAPT_CONFIG_SP."
#include "recovery_ui/mi_xxhdpi.h"
#endif

const char* updater_secure_msg = NULL;
const char* sideload_secure_msg = NULL;
const static bool DEBUG = false;
char loc_info[PROPERTY_VALUE_MAX] = { 0 };
std::string real_hwc = android::base::GetProperty("ro.boot.hwc", "");
std::string real_device = android::base::GetProperty("ro.product.device", "");

// Return the current time as a double (including fractions of a second).
static double now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

//====================================================================
// View
//====================================================================

View::View()
{
    Init();
}

void View::Init(void)
{
    mWidth = 0;
    mHeight = 0;
    mGravity = NONE_GRAVITY;
    mNormal = NULL;
    mSelected = NULL;
}

void View::Destroy(void)
{
    Init();
}

//====================================================================
// Menu
//====================================================================

MiuiMenu::MiuiMenu(void)
{
    Init();
}

void MiuiMenu::Init(void)
{
    mMenuItems = 0;
    mMenuStart = 0;
    mMenuSelected = 0;
    mCounts = 0;
}

void MiuiMenu::Destroy(void)
{
    mMenuItems = 0;
    mMenuStart = 0;
    mMenuSelected = 0;
    for (int i = 0; i < mCounts; ++i) {
        mChildren[i].Destroy();
    }
    mCounts = 0;
}

//====================================================================
// ProgressBar
//====================================================================

ProgressBar::ProgressBar(void)
{
    Init();
}

void ProgressBar::Init(void)
{
    mProgress = 0.0f;
    mProgressScopeStart = 0.0f;
    mProgressScopeSize = 0.0f;
    mProgressScopeTime = 0.0;
    mProgressScopeDuration = 0.0;
    mCounts = 0;
}

void ProgressBar::ComputeProgress(bool reset)
{
    float current_progress = mProgressScopeStart;
    if (current_progress <= mProgress && !reset) {
        mProgress = 0.0f;
        return;
    }
    mProgress = current_progress;
    // the progress is not accurate, in order not to let user waiting while
    // the progress is 100%, limit the progress value to below 1.0
    if (mProgress < 0.0f) {
        mProgress = 0.0f;
    } else if (mProgress > 1.0f) {
        mProgress = 1.0f;
    }
}

void ProgressBar::ComputeProgress(float portion, float seconds)
{
    mProgressScopeStart += mProgressScopeSize;
    if(portion < 0.0f) portion = 0.0f;
    else if(portion > 1.0f) portion = 1.0f;
    mProgressScopeSize = portion;
    mProgressScopeTime = now();
    mProgressScopeDuration = seconds;
    ComputeProgress(seconds != 0);
}

float ProgressBar::RelocateProgress() {
    return RelocateProgress(mProgress);
}

float ProgressBar::RelocateProgress(float progress) {
    if(mProgressScopeDuration == 0) {
      progress *= mProgressScopeSize;
      progress += mProgressScopeStart;
    }
    return progress;
}

void ProgressBar::Destroy(void)
{
    mProgress = 0.0f;
    mProgressScopeStart = 0.0f;
    mProgressScopeSize = 0.0f;
    mProgressScopeTime = 0.0f;
    mProgressScopeDuration = 0.0f;
    for (int i = 0; i < mCounts; ++i) {
        mChildren[i].Destroy();
    }
    mCounts = 0;
}

//====================================================================
// MiScreenRecoveryUI
//====================================================================

/* mImageNames and mImageSurfaces is 1-to-1 relationship */
const char* MiScreenRecoveryUI::mImageNames[IMAGE_COUNT] = {
#define IMAGE(c, normal, selected) normal, selected,
#include "recovery_ui/image.def"
#undef IMAGE
};

// There's only (at most) one of these objects, and global callbacks
// (for pthread_create, and the battery state system) need to find it,
// so use a global variable.
static MiScreenRecoveryUI* self = NULL;

MiScreenRecoveryUI::MiScreenRecoveryUI() :
        mShowCurrentMenu(false),
        mShowCurrentState(false),
        mShowCurrentProgress(false),
        mShowCurrentText(false)
{
    for (int i = 0; i < sizeof(mBackgroundIcon)/sizeof(mBackgroundIcon[0]); ++i) {
        mBackgroundIcon[i] = NULL;
    }
    self = this;
    memset(mImageSurfaces, 0, sizeof (GRSurface *) * IMAGE_COUNT);
    load_volume_table();
    pthread_mutex_init(&mUpdateMutex, NULL);
}

void MiScreenRecoveryUI::LoadLocalizedBitmap(const char* filename, GRSurface** surface)
{
    // during sideload update, if file 'last_locale' doesn't exists,then
    // locale will be NULL. After that we will judge if locale is
    // NULL in 'res_create_localized_alpha_surface' function, so we must
    // acquire locale's value before this judgement.
    if (this->locale_.empty()) {
        if (property_get("ro.product.locale", loc_info, NULL)) {
            // In order to compare with RECOVERY_SUPPORT_LOCALES's elements in resources.cpp,
            // change locale's style from "ab-CD" to "ab_CD".
            loc_info[2] = '_';
            this->locale_.assign(loc_info);
        }
    }

    int result = res_create_localized_alpha_surface(filename, locale_.c_str(), surface);
    if (result < 0) {
        LOG(ERROR) << "missing bitmap:" << filename << "(Code " << result << ")";
    }
}

void MiScreenRecoveryUI::SetStage(int current, int max) {
    /* Nothing to do. */
}

void MiScreenRecoveryUI::SetLocale(const std::string& new_locale)
{
    this->locale_ = new_locale;
    this->rtl_locale_ = false;

    if (!new_locale.empty()) {
        size_t underscore = new_locale.find('_');
        // lang has the language prefix prior to '_', or full string if '_' doesn't exist.
        std::string lang = new_locale.substr(0, underscore);

        // A bit cheesy: keep an explicit list of supported RTL languages.
        if (lang == "ar" ||  // Arabic
            lang == "fa" ||  // Persian (Farsi)
            lang == "he" ||  // Hebrew (new language code)
            lang == "iw" ||  // Hebrew (old language code)
            lang == "ur") {  // Urdu
            rtl_locale_ = true;
        }
    }
}

bool
MiScreenRecoveryUI::Init(const std::string& locale)
{
    SetLocale(locale);

    ScreenRecoveryUI::Init(locale);

    LoadLocalizedBitmap("installing_tip1", &backgroundText[INSTALLING_UPDATE]);
    LoadLocalizedBitmap("installing_tip2", &mUpdateTips);

    load_home_resources();

    set_screen_backlight(true);

    if(this->locale_ == "zh_CN") {
        mBackgroundIcon[BACKGROUND_STATE_CONNECT]   = mImageSurfaces[IMG_STATE_CONNECT_CN];
        mBackgroundIcon[BACKGROUND_STATE_UNCONNECT] = mImageSurfaces[IMG_STATE_UNCONNECT_CN];
    } else {
        mBackgroundIcon[BACKGROUND_STATE_CONNECT]   = mImageSurfaces[IMG_STATE_CONNECT_I18N];
        mBackgroundIcon[BACKGROUND_STATE_UNCONNECT] = mImageSurfaces[IMG_STATE_UNCONNECT_I18N];
    }
    mBackgroundIcon[BACKGROUND_STATE_FASTBOOTD] = mImageSurfaces[IMG_STATE_FASTBOOTD];

    mBatteryState = read_battery_state();

    pthread_create(&mFlushTid, NULL, flush_thread, NULL);
    return true;
}

void
MiScreenRecoveryUI::SetCurrentState(bool state) {
    mShowCurrentState = state;
}

void
MiScreenRecoveryUI::SetBackground(Icon icon)
{
    pthread_mutex_lock(&mUpdateMutex);
    //Only show log when recovery is being initialized.
    if (icon == NONE) {
        mShowCurrentMenu = false;
        draw_background_locked();
        draw_logo_locked();
        gr_flip();
    } else if (icon == INSTALLING_UPDATE) {
        mShowCurrentProgress = true;
        mShowCurrentMenu = false;
        update_screen_locked();
    } else if (icon == NO_COMMAND) {
        mShowCurrentMenu = true;
        update_screen_locked();
    }
    pthread_mutex_unlock(&mUpdateMutex);
}

void
MiScreenRecoveryUI::load_home_resources(void)
{
    load_common_resources("", 0, IMG_LOGO);
}

void
MiScreenRecoveryUI::load_common_resources(const char *language, int start, int end)
{
    for (int i = start; i <= end; ++i) {
        if (mImageNames[i] == NULL) {
            continue;
        }
        int ret = internal_res_create_display_surface(
            language, mImageNames[i], &mImageSurfaces[i]);
        if (ret < 0) {
            LOG(ERROR) << "missing bitmap:" << mImageNames[i] << "(Code " << ret << ")";
        }
    }
}

void*
MiScreenRecoveryUI::flush_thread(void *cookie)
{
    self->flush_loop();
    return NULL;
}

void
MiScreenRecoveryUI::flush_loop(void)
{
    double interval = 1.0 / kFlushFps;
    for (;;) {
        double start = now();
        {
            std::lock_guard<std::mutex> lg(updateMutex);
            bool redraw = false;
            mShowTextTimes += 1;
            struct battery_state new_state = read_battery_state();
            if (new_state.is_charging != mBatteryState.is_charging ||
                new_state.level != mBatteryState.level ||
                new_state.online != mBatteryState.online) {
                mBatteryState = new_state;
                if (mShowCurrentMenu) redraw = true;
            }

            // move the progress bar forward on timed intervals, if configured
            int duration = mCurrentProgress.GetProgressScopeDuration();
            if (mShowCurrentProgress && duration > 0) {
                double elapsed = now() - mCurrentProgress.GetProgressScopeTime();
                float p = 1.0 * elapsed / duration;
                if (p > 1.0) p = 1.0f;
                // current_progress represents for the percent of every single portion, which passed by ShowProgress(portion, seconds)
                float current_progress = p * mCurrentProgress.GetProgressScopeSize() + mCurrentProgress.GetProgressScopeStart();
                if (current_progress > 1.0) current_progress = 1.0f;
                if (current_progress > mCurrentProgress.GetProgress()) {
                    mCurrentProgress.SetProgress(current_progress);
                    redraw = true;
                }
            }
            if (redraw) update_screen_locked();
        }
        double end = now();
        // minimum of 20ms delay between frames
        double delay = interval - (end-start);
        if (delay < 0.02) delay = 0.02;
        usleep((long)(delay * 1000000));
    }
}

void
MiScreenRecoveryUI::StartMenu(const char* const * headers, const char* const * items,
                              int initial_selection) {
    /* Nothing to do. */
}

void MiScreenRecoveryUI::set_keytip_pos(void)
{
    KEYTIP_UPSIDE_SHIFT = gr_fb_height() - KEYTIP_DOWNSIDE_SHIFT - gr_get_height(mImageSurfaces[IMG_KEY_USAGE]);
}

void
MiScreenRecoveryUI::InitLanguageSpecificResource(const char *language)
{
    load_common_resources(language, IMG_MAIN_MENU, IMAGE_COUNT - 1);
    set_keytip_pos();
}

int
MiScreenRecoveryUI::GetMenuSelection(
    const char* id, int menu_only,
    int initial_selection, Device* device)
{
    // throw away keys pressed previously, so user doesn't
    // accidentally trigger menu items.
    FlushKeys();

    StartMenu(id, initial_selection);
    int selected = initial_selection;
    int chosen_item = -1;

    while (chosen_item < 0) {
        int key = WaitKey();
        int visible = IsTextVisible();

        if (key == -1) {   // ui_wait_key() timed out
            if (WasTextEverVisible()) {
                continue;
            } else {
                LOG(ERROR) << "timed out waiting for key input; rebooting.";
                EndMenu();
                return static_cast<int>(KeyError::TIMED_OUT); // XXX fixme
            }
        }

        int action = device->HandleMenuKey(key, visible);

        if (action < 0) {
            switch (action) {
                case Device::kHighlightUp:
                    --selected;
                    selected = SelectMenu(selected);
                    break;
                case Device::kHighlightDown:
                    ++selected;
                    selected = SelectMenu(selected);
                    break;
                case Device::kInvokeItem:
                    chosen_item = selected;
                    break;
                case Device::kNoAction:
                    break;
            }
        } else if (!menu_only) {
            chosen_item = action;
        }
    }

    EndMenu();
    printf("mi_screen: chosen_item[%d]\n", chosen_item);
    return chosen_item;
}

void MiScreenRecoveryUI::ShowAbnormalBootReason(int count, int index, int height) {
    if(mShowMismatch) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_VERSION_DISMATCH]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_VERSION_DISMATCH_SELECTED]);
    } else if (mShowNVCorrupted) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_NVDATA_CORRUPTED]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_NVDATA_CORRUPTED_SELECTED]);
    } else if (mHasRamdump) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_RAMDUMP_PRESENTED]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_RAMDUMP_PRESENTED_SELECTED]);
    } else if (mBootMonitor) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_BOOT_MONITOR]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_BOOT_MONITOR_SELECTED]);
    } else if (mEnableFileCryptoFailed) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_ENABLEFILECRYPTO_FAILED]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_ENABLEFILECRYPTO_FAILED_SELECTED]);
    } else if (mFsMgrMountAll) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_FS_MGR_MOUNT_ALL]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_FS_MGR_MOUNT_ALL_SELECTED]);
    } else if (mInitUser0Failed) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_INIT_USER0_FAILED]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_INIT_USER0_FAILED_SELECTED]);
    } else if (mRescueParty) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_RESCUEPARTY]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_RESCUEPARTY_SELECTED]);
    } else if (mSetPolicyFailed) {
        mCurrentMenu.SetCounts(count);
        mCurrentMenu.GetChild(index).SetHeight(height);
        mCurrentMenu.GetChild(index).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(index).SetNormal(&mImageSurfaces[IMG_SET_POLICY_FAILED]);
        mCurrentMenu.GetChild(index).SetSelected(&mImageSurfaces[IMG_SET_POLICY_FAILED_SELECTED]);
    }
}

void
MiScreenRecoveryUI::StartMenu(const char *id, int initial_selection)
{
    if (!strcmp(id, "menu_home")) {
        set_menu_home();
    } else if (!strcmp(id, "menu_reboot_single")) {
        set_menu_reboot_single();
    } else if (!strcmp(id, "menu_wipe_data_confirm")) {
        set_menu_common_confirm(IMG_WIPE_DATA_CONFIRM);
    } else if (!strcmp(id, "menu_wipe_data_result_ok")) {
        set_menu_common_result(IMG_WIPE_DATA_OK);
    } else if (!strcmp(id, "menu_wipe_data_result_fail")) {
        set_menu_common_result(IMG_WIPE_DATA_FAIL);
    } else if (!strcmp(id, "menu_wipe")) {
        set_menu_wipe();
    } else if (!strcmp(id, "menu_apply_ext_fail")) {
        set_menu_common_result(IMG_INSTALL_UPDATE_FAIL);
    } else if (!strcmp(id, "menu_apply_ext_no_file")) {
        set_menu_common_result(IMG_INSTALL_UPDATE_NO_FILE);
    } else if (!strcmp(id, "menu_apply_ext_verify_fail")) {
       set_menu_common_result(IMG_INSTALL_UPDATE_VERIFY_FAIL);
    } else if (!strcmp(id, "menu_apply_ext_corrupt")) {
        set_menu_common_result(IMG_INSTALL_UPDATE_FILE_CORRUPT);
    } else if (!strcmp(id, "menu_apply_ext_low_battery")) {
        set_menu_common_result(IMG_INSTALL_UPDATE_LOW_BATTERY);
    } else {
        LOG(ERROR) << "set:" << id << " menu layout error!";
    }

    pthread_mutex_lock(&mUpdateMutex);
    mShowCurrentMenu = true;
    mCurrentMenu.SetMenuSelected(initial_selection);
    update_screen_locked();
    pthread_mutex_unlock(&mUpdateMutex);
}

int
MiScreenRecoveryUI::SelectMenu(int sel) {
    int old_selected;

    pthread_mutex_lock(&mUpdateMutex);
    if (mShowCurrentMenu) {
        old_selected = mCurrentMenu.GetMenuSelected();
        mCurrentMenu.SetMenuSelected(sel);
        if (mCurrentMenu.GetMenuSelected() < 0) {
            mCurrentMenu.SetMenuSelected(0);
        }
        if (mCurrentMenu.GetMenuSelected() >= mCurrentMenu.GetMenuItems()) {
            mCurrentMenu.SetMenuSelected(mCurrentMenu.GetMenuItems() - 1);
        }
        sel = mCurrentMenu.GetMenuSelected();
        if (mCurrentMenu.GetMenuSelected() != old_selected) {
            update_screen_locked();
        }
    }
    pthread_mutex_unlock(&mUpdateMutex);
    return sel;
}

void
MiScreenRecoveryUI::EndMenu(void)
{
    pthread_mutex_lock(&mUpdateMutex);
    if (mShowCurrentMenu) {
        mShowCurrentMenu = false;
        mCurrentMenu.Destroy();
        update_screen_locked();
    }
    pthread_mutex_unlock(&mUpdateMutex);
}

//====================================================================
// Set Menu.
//====================================================================

void
MiScreenRecoveryUI::set_menu_home(void)
{
    mCurrentMenu.SetMenuItems(5);
    mCurrentMenu.SetMenuStart(3);
    mCurrentMenu.SetCounts(10);

    mCurrentMenu.GetChild(0).SetHeight(MENU_HOME_7_HEIGHT);
    mCurrentMenu.GetChild(0).SetGravity(View::BOTTOM_CENTER);
    mCurrentMenu.GetChild(0).SetNormal(&mImageSurfaces[IMG_MAIN_MENU]);
    mCurrentMenu.GetChild(0).SetSelected(&mImageSurfaces[IMG_MAIN_MENU_SELECTED]);

    mCurrentMenu.GetChild(1).SetHeight(MENU_HOME_8_HEIGHT);
    mCurrentMenu.GetChild(1).SetGravity(View::BOTTOM_CENTER);
#ifdef REDMI_DEVICE
    mCurrentMenu.GetChild(1).SetNormal(&mImageSurfaces[IMG_REDMITEXTLOGO]);
#else
    mCurrentMenu.GetChild(1).SetNormal(&mImageSurfaces[IMG_TEXTLOGO]);
#endif
    mCurrentMenu.GetChild(1).SetSelected(&mImageSurfaces[IMG_TEXTLOGO_SELECTED]);

    mCurrentMenu.GetChild(2).SetHeight(MENU_HOME_9_HEIGHT);
    mCurrentMenu.GetChild(2).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(2).SetNormal(NULL);
    mCurrentMenu.GetChild(2).SetSelected(NULL);

    mCurrentMenu.GetChild(3).SetHeight(MENU_HOME_1_HEIGHT);
    mCurrentMenu.GetChild(3).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(3).SetNormal(&mImageSurfaces[IMG_REBOOT]);
    mCurrentMenu.GetChild(3).SetSelected(&mImageSurfaces[IMG_REBOOT_SELECTED]);

// MIUI ADD: START for disable erase button
  if(!disable_format()) {
    mCurrentMenu.GetChild(4).SetHeight(MENU_HOME_2_HEIGHT);
    mCurrentMenu.GetChild(4).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(4).SetNormal(&mImageSurfaces[IMG_WIPE]);
    mCurrentMenu.GetChild(4).SetSelected(&mImageSurfaces[IMG_WIPE_SELECTED]);

    mCurrentMenu.GetChild(5).SetHeight(MENU_HOME_2_HEIGHT);
    mCurrentMenu.GetChild(5).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(5).SetNormal(&mImageSurfaces[IMG_MIASSISTANT]);
    mCurrentMenu.GetChild(5).SetSelected(&mImageSurfaces[IMG_MIASSISTANT_SELECTED]);

    mCurrentMenu.GetChild(6).SetHeight(MENU_HOME_2_HEIGHT);
    mCurrentMenu.GetChild(6).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(6).SetNormal(&mImageSurfaces[IMG_SAFEMODE]);
    mCurrentMenu.GetChild(6).SetSelected(&mImageSurfaces[IMG_SAFEMODE_SELECTED]);

    if (mShowMaintenance) {
        mCurrentMenu.GetChild(7).SetHeight(MENU_HOME_3_HEIGHT);
        mCurrentMenu.GetChild(7).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(7).SetNormal(&mImageSurfaces[IMG_MAINTENANCE_MODE]);
        mCurrentMenu.GetChild(7).SetSelected(&mImageSurfaces[IMG_MAINTENANCE_MODE_SELECTED]);
    } else {
        mCurrentMenu.GetChild(7).SetHeight(MENU_HOME_3_HEIGHT);
        mCurrentMenu.GetChild(7).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(7).SetNormal(NULL);
        mCurrentMenu.GetChild(7).SetSelected(NULL);
        mCurrentMenu.SetMenuItems(4);
    }
  } else {
    mCurrentMenu.GetChild(4).SetHeight(MENU_HOME_2_HEIGHT);
    mCurrentMenu.GetChild(4).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(4).SetNormal(&mImageSurfaces[IMG_MIASSISTANT]);
    mCurrentMenu.GetChild(4).SetSelected(&mImageSurfaces[IMG_MIASSISTANT_SELECTED]);

    mCurrentMenu.GetChild(5).SetHeight(MENU_HOME_2_HEIGHT);
    mCurrentMenu.GetChild(5).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(5).SetNormal(&mImageSurfaces[IMG_SAFEMODE]);
    mCurrentMenu.GetChild(5).SetSelected(&mImageSurfaces[IMG_SAFEMODE_SELECTED]);

    if (mShowMaintenance) {
        mCurrentMenu.GetChild(6).SetHeight(MENU_HOME_2_HEIGHT);
        mCurrentMenu.GetChild(6).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(6).SetNormal(&mImageSurfaces[IMG_MAINTENANCE_MODE]);
        mCurrentMenu.GetChild(6).SetSelected(&mImageSurfaces[IMG_MAINTENANCE_MODE_SELECTED]);
        mCurrentMenu.SetMenuItems(4);
    } else {
        mCurrentMenu.GetChild(6).SetHeight(MENU_HOME_2_HEIGHT);
        mCurrentMenu.GetChild(6).SetGravity(View::CENTER_HORIZONTAL);
        mCurrentMenu.GetChild(6).SetNormal(NULL);
        mCurrentMenu.GetChild(6).SetSelected(NULL);
        mCurrentMenu.SetMenuItems(3);
    }

    mCurrentMenu.GetChild(7).SetHeight(MENU_HOME_3_HEIGHT);
    mCurrentMenu.GetChild(7).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(7).SetNormal(NULL);
    mCurrentMenu.GetChild(7).SetSelected(NULL);
  }
  // END

    // aim to fix IMG_KEY_USAGE's position
    int height_sum = MENU_HOME_0_HEIGHT + MENU_HOME_1_HEIGHT + 3 * MENU_HOME_2_HEIGHT + MENU_HOME_3_HEIGHT;

    mCurrentMenu.GetChild(8).SetHeight(KEYTIP_UPSIDE_SHIFT - height_sum);
    mCurrentMenu.GetChild(8).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(8).SetNormal(NULL);
    mCurrentMenu.GetChild(8).SetSelected(NULL);

    mCurrentMenu.GetChild(9).SetHeight(MENU_HOME_5_HEIGHT);
    mCurrentMenu.GetChild(9).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(9).SetNormal(&mImageSurfaces[IMG_KEY_USAGE]);
    mCurrentMenu.GetChild(9).SetSelected(&mImageSurfaces[IMG_KEY_USAGE_SELECTED]);

    ShowAbnormalBootReason(11, 10, MENU_HOME_6_HEIGHT);
}

void
MiScreenRecoveryUI::set_menu_reboot_single(void)
{
    mCurrentMenu.SetMenuItems(2);
    mCurrentMenu.SetMenuStart(1);
    mCurrentMenu.SetCounts(5);

    mCurrentMenu.GetChild(0).SetHeight(MENU_REBOOT_SINGLE_0_HEIGHT);
    mCurrentMenu.GetChild(0).SetGravity(View::CENTER);
    mCurrentMenu.GetChild(0).SetNormal(&mImageSurfaces[IMG_REBOOT_MENU]);
    mCurrentMenu.GetChild(0).SetSelected(&mImageSurfaces[IMG_REBOOT_MENU_SELECTED]);

    mCurrentMenu.GetChild(1).SetHeight(MENU_REBOOT_SINGLE_1_HEIGHT);
    mCurrentMenu.GetChild(1).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(1).SetNormal(&mImageSurfaces[IMG_REBOOT_SYSTEM]);
    mCurrentMenu.GetChild(1).SetSelected(&mImageSurfaces[IMG_REBOOT_SYSTEM_SELECTED]);

    mCurrentMenu.GetChild(2).SetHeight(MENU_REBOOT_SINGLE_2_HEIGHT);
    mCurrentMenu.GetChild(2).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(2).SetNormal(&mImageSurfaces[IMG_BACK_MAIN]);
    mCurrentMenu.GetChild(2).SetSelected(&mImageSurfaces[IMG_BACK_MAIN_SELECTED]);

    // aim to fix IMG_KEY_USAGE's position
    int height_sum = MENU_REBOOT_SINGLE_0_HEIGHT + MENU_REBOOT_SINGLE_1_HEIGHT + MENU_REBOOT_SINGLE_2_HEIGHT;
    mCurrentMenu.GetChild(3).SetHeight(KEYTIP_UPSIDE_SHIFT - height_sum);
    mCurrentMenu.GetChild(3).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(3).SetNormal(NULL);
    mCurrentMenu.GetChild(3).SetSelected(NULL);

    mCurrentMenu.GetChild(4).SetHeight(MENU_REBOOT_SINGLE_3_HEIGHT);
    mCurrentMenu.GetChild(4).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(4).SetNormal(&mImageSurfaces[IMG_KEY_USAGE]);
    mCurrentMenu.GetChild(4).SetSelected(&mImageSurfaces[IMG_KEY_USAGE_SELECTED]);

    ShowAbnormalBootReason(6, 5, MENU_REBOOT_SINGLE_4_HEIGHT);
}

void
MiScreenRecoveryUI::set_menu_wipe(void)
{
    int count = 0;

    mCurrentMenu.GetChild(count).SetHeight(MENU_WIPE_0_HEIGHT);
    mCurrentMenu.GetChild(count).SetGravity(View::CENTER);
    mCurrentMenu.GetChild(count).SetNormal(&mImageSurfaces[IMG_WIPE_MENU]);
    mCurrentMenu.GetChild(count++).SetSelected(&mImageSurfaces[IMG_WIPE_MENU_SELECTED]);

    mCurrentMenu.GetChild(count).SetHeight(MENU_WIPE_1_HEIGHT);
    mCurrentMenu.GetChild(count).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(count).SetNormal(&mImageSurfaces[IMG_BACK_MAIN]);
    mCurrentMenu.GetChild(count++).SetSelected(&mImageSurfaces[IMG_BACK_MAIN_SELECTED]);

    mCurrentMenu.GetChild(count).SetHeight(MENU_WIPE_2_HEIGHT);
    mCurrentMenu.GetChild(count).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(count).SetNormal(&mImageSurfaces[IMG_WIPE_DATA]);
    mCurrentMenu.GetChild(count++).SetSelected(&mImageSurfaces[IMG_WIPE_DATA_SELECTED]);

    // aim to fix IMG_KEY_USAGE's position
    int height_sum = MENU_WIPE_0_HEIGHT + MENU_WIPE_1_HEIGHT + MENU_WIPE_2_HEIGHT;
    mCurrentMenu.GetChild(count).SetHeight(KEYTIP_UPSIDE_SHIFT - height_sum);
    mCurrentMenu.GetChild(count).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(count).SetNormal(NULL);
    mCurrentMenu.GetChild(count++).SetSelected(NULL);

    mCurrentMenu.GetChild(count).SetHeight(MENU_WIPE_6_HEIGHT);
    mCurrentMenu.GetChild(count).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(count).SetNormal(&mImageSurfaces[IMG_KEY_USAGE]);
    mCurrentMenu.GetChild(count++).SetSelected(&mImageSurfaces[IMG_KEY_USAGE_SELECTED]);

    ShowAbnormalBootReason(count, count, MENU_WIPE_7_HEIGHT);
    if (mShowMismatch || mShowNVCorrupted || mHasRamdump ||
        mBootMonitor || mEnableFileCryptoFailed || mFsMgrMountAll ||
        mInitUser0Failed || mRescueParty || mSetPolicyFailed) {
        count ++;
    }

    mCurrentMenu.SetMenuStart(1);
    mCurrentMenu.SetMenuItems(2);
    mCurrentMenu.SetCounts(count);
}

void
MiScreenRecoveryUI::set_menu_common_confirm(ImageElement img)
{
    mCurrentMenu.SetMenuItems(2);
    mCurrentMenu.SetMenuStart(1);
    mCurrentMenu.SetCounts(5);

    mCurrentMenu.GetChild(0).SetHeight(MENU_COMMON_CONFIRM_0_HEIGHT);
    mCurrentMenu.GetChild(0).SetGravity(View::CENTER);
    mCurrentMenu.GetChild(0).SetNormal(&mImageSurfaces[img]);
    mCurrentMenu.GetChild(0).SetSelected(&mImageSurfaces[img + 1]);

    mCurrentMenu.GetChild(1).SetHeight(MENU_COMMON_CONFIRM_1_HEIGHT);
    mCurrentMenu.GetChild(1).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(1).SetNormal(&mImageSurfaces[IMG_CHOICE_NO]);
    mCurrentMenu.GetChild(1).SetSelected(&mImageSurfaces[IMG_CHOICE_NO_SELECTED]);

    mCurrentMenu.GetChild(2).SetHeight(MENU_COMMON_CONFIRM_2_HEIGHT);
    mCurrentMenu.GetChild(2).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(2).SetNormal(&mImageSurfaces[IMG_CHOICE_YES]);
    mCurrentMenu.GetChild(2).SetSelected(&mImageSurfaces[IMG_CHOICE_YES_SELECTED]);

    // aim to fix IMG_KEY_USAGE's position
    int height_sum = MENU_COMMON_CONFIRM_0_HEIGHT + MENU_COMMON_CONFIRM_1_HEIGHT + MENU_COMMON_CONFIRM_2_HEIGHT;
    mCurrentMenu.GetChild(3).SetHeight(KEYTIP_UPSIDE_SHIFT - height_sum);
    mCurrentMenu.GetChild(3).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(3).SetNormal(NULL);
    mCurrentMenu.GetChild(3).SetSelected(NULL);

    mCurrentMenu.GetChild(4).SetHeight(MENU_COMMON_CONFIRM_3_HEIGHT);
    mCurrentMenu.GetChild(4).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(4).SetNormal(&mImageSurfaces[IMG_KEY_USAGE]);
    mCurrentMenu.GetChild(4).SetSelected(&mImageSurfaces[IMG_KEY_USAGE_SELECTED]);

    ShowAbnormalBootReason(6, 5, MENU_COMMON_CONFIRM_4_HEIGHT);
}

void
MiScreenRecoveryUI::set_menu_common_result(ImageElement img)
{
    mCurrentMenu.SetMenuItems(1);
    mCurrentMenu.SetMenuStart(1);
    mCurrentMenu.SetCounts(4);

    mCurrentMenu.GetChild(0).SetHeight(MENU_COMMON_RESULT_0_HEIGHT);
    mCurrentMenu.GetChild(0).SetGravity(View::CENTER);
    mCurrentMenu.GetChild(0).SetNormal(&mImageSurfaces[img]);
    mCurrentMenu.GetChild(0).SetSelected(&mImageSurfaces[img + 1]);

    mCurrentMenu.GetChild(1).SetHeight(MENU_COMMON_RESULT_1_HEIGHT );
    mCurrentMenu.GetChild(1).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(1).SetNormal(&mImageSurfaces[IMG_BACK_MAIN]);
    mCurrentMenu.GetChild(1).SetSelected(&mImageSurfaces[IMG_BACK_MAIN_SELECTED]);

    // aim to fix IMG_KEY_USAGE's position
    int height_sum = MENU_COMMON_RESULT_0_HEIGHT + MENU_COMMON_RESULT_1_HEIGHT;
    mCurrentMenu.GetChild(2).SetHeight(KEYTIP_UPSIDE_SHIFT - height_sum);
    mCurrentMenu.GetChild(2).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(2).SetNormal(NULL);
    mCurrentMenu.GetChild(2).SetSelected(NULL);

    mCurrentMenu.GetChild(3).SetHeight(MENU_COMMON_RESULT_3_HEIGHT);
    mCurrentMenu.GetChild(3).SetGravity(View::CENTER_HORIZONTAL);
    mCurrentMenu.GetChild(3).SetNormal(&mImageSurfaces[IMG_KEY_USAGE]);
    mCurrentMenu.GetChild(3).SetSelected(&mImageSurfaces[IMG_KEY_USAGE_SELECTED]);

    ShowAbnormalBootReason(5, 4, MENU_COMMON_RESULT_4_HEIGHT);
}

//====================================================================
// Draw Screen
//====================================================================

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with updateMutex locked.
void
MiScreenRecoveryUI::update_screen_locked(void)
{
    draw_screen_locked();
    gr_flip();
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with updateMutex locked.
void
MiScreenRecoveryUI::draw_screen_locked(void)
{
    draw_background_locked();
    if (mShowCurrentMenu) {
        if (mBatteryState.level < (mBatteryState.is_charging
                    ? BATTERY_LEVEL_CHARGING_LOW : BATTERY_LEVEL_LOW)) {
            draw_battery_locked();
        } else if (mShowCurrentState) {
            draw_state_locked();
        } else {
            draw_menu_locked();
        }
    } else if (mShowCurrentProgress) {
        draw_progress_locked();
    }
}

void
MiScreenRecoveryUI::draw_background_locked(void)
{
    gr_color(0, 0, 0, 255);
    gr_clear();
}

void
MiScreenRecoveryUI::draw_logo_locked(void)
{
    int width, height;
#ifdef REDMI_DEVICE
    GRSurface* surface = mImageSurfaces[IMG_REDMILOGO];
#else
    GRSurface* surface = mImageSurfaces[IMG_LOGO];
#endif
    if (real_device == "beryllium") {
        surface = mImageSurfaces[IMG_POCOPHONE];
        if (real_hwc == "INDIA") {
            surface = mImageSurfaces[IMG_POCO];
        }
    }
    width = gr_get_width(surface);
    height = gr_get_height(surface);
    int xoff = (gr_fb_width() - width) / 2;
    int yoff = (gr_fb_height() - height - LOGO_VIEW_OVERSCAN_HEIGHT) / 2;
    gr_blit(surface, 0, 0, width, height, xoff, yoff);
}

void
MiScreenRecoveryUI::draw_textlogo_locked(void)
{
    int width, height;

#ifdef REDMI_DEVICE
    GRSurface* surface = mImageSurfaces[IMG_REDMITEXTLOGO];
#else
    GRSurface* surface = mImageSurfaces[IMG_TEXTLOGO];
#endif
    width = gr_get_width(surface);
    height = gr_get_height(surface);
    gr_blit(surface, 0, 0, width, height, 0, 0);
}

void
MiScreenRecoveryUI::draw_battery_locked(void)
{
    int width, height;
    GRSurface* surface = mImageSurfaces[IMG_BATTERY_PROMPT];
    width = gr_get_width(surface);
    height = gr_get_height(surface);
    int xoff = (gr_fb_width() - width) / 2;
    gr_blit(surface, 0, 0, width, height, xoff, BATTERY_PROMPT_HEIGHT);
}

void
MiScreenRecoveryUI::draw_menu_locked(void)
{
    int y = 0;
    for (int i = 0; i < mCurrentMenu.GetCounts(); ++i) {
        if ((mCurrentMenu.GetMenuItems() != 0) &&
           (i == mCurrentMenu.GetMenuSelected() + mCurrentMenu.GetMenuStart())) {
            draw_view_locked(mCurrentMenu.GetChild(i), 0, y, true);
        } else {
            draw_view_locked(mCurrentMenu.GetChild(i), 0, y, false);
        }
        y += mCurrentMenu.GetChild(i).GetHeight();
    }
}

void
MiScreenRecoveryUI::draw_state_locked(void)
{
    GRSurface * surface;
    if (fastbootd_logo_enabled_) { // show Fastbootd BG
       surface = mBackgroundIcon[BACKGROUND_STATE_FASTBOOTD];
    }
    else { // show MI-Assistant BG
    if (mBatteryState.online) {
       surface = mBackgroundIcon[BACKGROUND_STATE_CONNECT];
   } else {
       surface = mBackgroundIcon[BACKGROUND_STATE_UNCONNECT];
   }
   }

   int width = gr_get_width(surface);
   int height = gr_get_height(surface);
   int xoff = (gr_fb_width() - width) / 2;      // align left and right
   int yoff = (gr_fb_height() - height) / 2;    // align top and bottom
   gr_blit(surface, 0, 0, width, height, xoff, yoff);
}

// draw the view on the screen, should only be called with update_mutex locked
void
MiScreenRecoveryUI::draw_view_locked(View &view, int x, int y, bool selected)
{
    GRSurface** surface = selected ? view.GetSelected() : view.GetNormal();
    if (surface == NULL) {
        return;
    }

    int width = gr_get_width(*surface);
    int height = gr_get_height(*surface);

    int xoff = 0, yoff = 0;

    compute_image_offset(view, &xoff, &yoff);
    gr_blit(*surface, 0, 0, width, height, x + xoff, y + yoff);
}

void
MiScreenRecoveryUI::compute_image_offset(View &view, int *xoff, int *yoff)
{
    GRSurface** surface = view.GetNormal();
    if (surface == NULL) {
        return;
    }

    int width = gr_get_width(*surface);
    int height = gr_get_height(*surface);

    view.SetWidth(gr_fb_width());

    switch (view.GetGravity()) {
        case View::TOP:
            *xoff = *yoff = 0;
            break;

        case View::CENTER_HORIZONTAL:
            *xoff = (view.GetWidth() - width) / 2;
            *yoff = 0;
            break;

        case View::CENTER_VERTICAL:
            *xoff = 0;
            *yoff = (view.GetHeight() - height) / 2;
            break;

        case View::CENTER:
            *xoff = (view.GetWidth() - width) / 2;
            *yoff = (view.GetHeight() - height) / 2;
            break;

        case View::BOTTOM:
            *xoff = 0;
            *yoff = view.GetHeight() - height;
            break;

        case View::BOTTOM_CENTER:
            *xoff = (view.GetWidth() - width) / 2;
            *yoff = view.GetHeight() - height - 63;
            break;

        default:
            break;
    }
}

//====================================================================
// Draw ProgressBar
//====================================================================

void
MiScreenRecoveryUI::set_progress_common_start()
{
    mCurrentProgress.SetCounts(3);

    mCurrentProgress.GetChild(0).SetHeight(PROGRESS_COMMON__1_HEIGHT);
    mCurrentProgress.GetChild(0).SetGravity(View::CENTER);
    mCurrentProgress.GetChild(0).SetNormal(NULL);
    mCurrentProgress.GetChild(0).SetSelected(NULL);

    mCurrentProgress.GetChild(1).SetHeight(PROGRESS_COMMON__2_HEIGHT);
    mCurrentProgress.GetChild(1).SetGravity(View::CENTER);
    mCurrentProgress.GetChild(1).SetNormal(&mImageSurfaces[IMG_PROGRESS_BAR]);
    mCurrentProgress.GetChild(1).SetSelected(&mImageSurfaces[IMG_PROGRESS_BAR_SELECTED]);

    mCurrentProgress.GetChild(2).SetHeight(0);
    mCurrentProgress.GetChild(2).SetGravity(View::CENTER);
    mCurrentProgress.GetChild(2).SetNormal(NULL);
    mCurrentProgress.GetChild(2).SetSelected(NULL);
}

void
MiScreenRecoveryUI::StartProgress(float portion, float seconds)
{
    std::lock_guard<std::mutex> lg(updateMutex);
    mCurrentProgress.Init();
    set_progress_common_start();
    mCurrentProgress.ComputeProgress(portion, seconds);
    mShowCurrentProgress = true;
    mShowCurrentMenu = false;
}

void
MiScreenRecoveryUI::ShowProgress(float portion, float seconds)
{
    std::lock_guard<std::mutex> lg(updateMutex);
    mCurrentProgress.ComputeProgress(portion, seconds);
    //printf("current progress %f, progress start %f\n", mCurrentProgress.GetProgress(), mCurrentProgress.GetProgressScopeStart());
    mShowCurrentProgress = true;
    mShowCurrentMenu = false;
    if(seconds == 0) return;
    update_screen_locked();
}

void
MiScreenRecoveryUI::SetProgress(float fraction)
{
    int duration = mCurrentProgress.GetProgressScopeDuration();
    if(DEBUG && duration == 0) {
        printf("SetProgress: fraction %f current_progress %f\n", mCurrentProgress.RelocateProgress(fraction), mCurrentProgress.RelocateProgress());
    }
    if(duration == 0 && (mCurrentProgress.RelocateProgress(fraction) - mCurrentProgress.RelocateProgress() < 0.002)) return;
    std::lock_guard<std::mutex> lg(updateMutex);
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    if(duration == 0) {
      mCurrentProgress.SetProgress(fraction);
      update_screen_locked();
      return;
    }
    if (mShowCurrentProgress && fraction > mCurrentProgress.GetProgress()) {
        // Skip updates that aren't visibly different.
        int width = gr_get_width(mImageSurfaces[IMG_PROGRESS_BAR]);
        float scale = width * mCurrentProgress.GetProgressScopeSize();
        if ((int)(mCurrentProgress.GetProgress() * scale) != (int)(fraction * scale)) {
            mCurrentProgress.SetProgress(fraction);
            update_screen_locked();
        }
    }
}

void
MiScreenRecoveryUI::EndProgress(void)
{
    std::lock_guard<std::mutex> lg(updateMutex);
    if (mShowCurrentProgress) {
        mShowCurrentProgress = false;
        mCurrentProgress.Destroy();
        update_screen_locked();
    }
}

void
MiScreenRecoveryUI::SetProgressType(ProgressType type)
{
    /* Nothing to do. */
}

void
MiScreenRecoveryUI::Print(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    fputs(buf, stdout);
}

void
MiScreenRecoveryUI::PrintOnScreenOnly(const char *fmt, ...)
{
    /* Nothing to do. */
}

// MIUI ADD: START
void
MiScreenRecoveryUI::SafeCloseDisplayDevice(void) {
    gr_exit();
}
// END

// text log
void
MiScreenRecoveryUI::ShowText(bool visible)
{
    /* Nothing to do */
}

bool
MiScreenRecoveryUI::IsTextVisible(void)
{
    return true; // Don't show text
}

bool
MiScreenRecoveryUI::WasTextEverVisible(void)
{
    return false;
}

void
MiScreenRecoveryUI::KeyLongPress(int)
{
    /* Nothing to do */
}

void
MiScreenRecoveryUI::draw_progress_locked(void)
{
    int x = 0;
    int y = 0;

    if (mCurrentProgress.GetCounts() == 0) return;

    // draw percentage
    y += mCurrentProgress.GetChild(0).GetHeight();
    draw_percent_locked(x, y);

    // draw progressbar
    y += mCurrentProgress.GetChild(1).GetHeight();
    draw_progressbar_locked(mCurrentProgress.GetChild(1), x, y);

    // draw title
    if (mShowCurrentText) {
        y += mCurrentProgress.GetChild(2).GetHeight();
        draw_text_locked(x, y);
    }
}

void
MiScreenRecoveryUI::draw_percent_locked(int x, int y)
{
    float progress = mCurrentProgress.RelocateProgress();
    if(DEBUG)
        printf("progress %f %f\n", progress, mCurrentProgress.GetProgressScopeStart());

    int progress_d = (int)(progress * 0.9999 * 10000);
    int a = progress_d % 10000 / 1000;
    int b = progress_d % 1000 / 100;
    int c = progress_d % 100 / 10;
    int d = progress_d % 10;

    GRSurface *surface1 = mImageSurfaces[a * 2 + IMG_PERCENT_DIGIT_0];
    GRSurface *surface2 = mImageSurfaces[b * 2 + IMG_PERCENT_DIGIT_0];
    GRSurface *surface3 = mImageSurfaces[IMG_PERCENT_POINT];
    GRSurface *surface4 = mImageSurfaces[c * 2 + IMG_PERCENT_DIGIT_0];
    GRSurface *surface5 = mImageSurfaces[d * 2 + IMG_PERCENT_DIGIT_0];
    GRSurface *surface6 = mImageSurfaces[IMG_PERCENT_SYMBOL];

    int width = gr_get_width(surface1);
    int height = gr_get_height(surface1);

    int point_width = gr_get_width(surface3);
    int point_height = gr_get_height(surface3);

    int percent_width = gr_get_width(surface6);
    int percent_height = gr_get_height(surface6);

    int xoff = 0;

    if (a == 0) {
        xoff = (gr_fb_width() - 4 * width - point_width) / 2;
        gr_blit(surface2, 0, 0, width, height, xoff, y);
        gr_blit(surface3, 0, 0, point_width, point_height, xoff + width, y);
        gr_blit(surface4, 0, 0, width, height, xoff + width + point_width, y);
        gr_blit(surface5, 0, 0, width, height, xoff + 2 * width + point_width, y);
        gr_blit(surface6, 0, 0, percent_width, percent_height, xoff + 3 * width + point_width, y);
    } else {
        xoff = (gr_fb_width() - 5 * width - point_width) / 2;
        gr_blit(surface1, 0, 0, width, height, xoff, y);
        gr_blit(surface2, 0, 0, width, height, xoff + width, y);
        gr_blit(surface3, 0, 0, point_width, point_height, xoff + 2 * width, y);
        gr_blit(surface4, 0, 0, width, height, xoff + 2 * width + point_width, y);
        gr_blit(surface5, 0, 0, width, height, xoff + 3 * width + point_width, y);
        gr_blit(surface6, 0, 0, percent_width, percent_height, xoff + width * 4 + point_width, y);
    }
}

void
MiScreenRecoveryUI::draw_progressbar_locked(View &view, int x, int y)
{
    GRSurface **normal = view.GetNormal();
    if (normal == NULL) {
        LOG(ERROR) << __FUNCTION__ << ":" << __LINE__ << ": failed to get surface";
        return;
    }

    int width = gr_get_width(*normal);
    int height = gr_get_height(*normal);

    int xoff = (gr_fb_width() - width) / 2;

    gr_blit(*normal, 0, 0, width, height, xoff, y);

    GRSurface **selected = view.GetSelected();
    if (selected == NULL) {
        LOG(ERROR) << __FUNCTION__ << ":" << __LINE__ << ": failed to get surface";
        return;
    }
    float progress = mCurrentProgress.RelocateProgress();
    int rate = (int)(progress * gr_get_width(*selected));
    if(DEBUG)
        printf("rate pos: %f\n", progress);
    if (rate >= 0) {
        gr_blit(*selected, 0, 0, rate, height, xoff, y);
    }
}

void
MiScreenRecoveryUI::draw_text_locked(int x, int y)
{
    gr_color(255, 255, 255, 255);
    GRSurface* text_surface = backgroundText[INSTALLING_UPDATE];
    if ((mShowTextTimes / 200) % 2 == 1) {
        text_surface = mUpdateTips;
    }

    int textWidth = gr_get_width(text_surface);
    [[maybe_unused]]int textHeight = gr_get_height(text_surface);

    int textX = (gr_fb_width() - textWidth) / 2;
    int textY = y;

    gr_texticon(textX, textY, text_surface);
}

//====================================================================
// Sub Menu
//====================================================================

// 1 - reboot; 0 - no-reboot.
int
MiScreenRecoveryUI::SubMenuReboot(Device *device)
{
    int chosen_item;
#ifdef WITH_DUAL_SYSTEM
    int system_id = get_current_system_id();
    chosen_item = GetMenuSelection("menu_reboot_double", 0, system_id, device);
    switch (chosen_item) {
        case static_cast<int>(KeyError::TIMED_OUT):
        case 0:
        case 1:
            set_reboot_message(chosen_item);
            sync();
            return 1;
        case 2: return 0;
    }
#else
    chosen_item = GetMenuSelection("menu_reboot_single", 0, 0, device);
    switch (chosen_item) {
        case static_cast<int>(KeyError::TIMED_OUT):
        case 0: {
            set_reboot_message(0);
            sync();
            return 1;
        }
        case 1: return 0;
    }
#endif /* WITH_DUAL_SYSTEM */

    return 0;
}

int
MiScreenRecoveryUI::SubMenuApplyExt(Device *device, const char *verification_msg,
    const char *update_package, bool should_wipe_cache, int retry_count, bool install_with_fuse)
{
    int status = INSTALL_SUCCESS;
    struct battery_state state = read_battery_state();

#if defined(WITH_STORAGE_INT)
    const char* kUpdateZipPath = "/storage_int/update.zip";
#else
    const char* kUpdateZipPath = "/data/media/0/update.zip";
#endif /* WITH_STORAGE_INT */

    if (retry_count == 0 && state.level < (state.is_charging ? BATTERY_LEVEL_CHARGING_LOW : BATTERY_LEVEL_LOW)) {
        LOG(ERROR) << "low battery:" << state.level;
        GetMenuSelection("menu_apply_ext_low_battery", 0, 0, device);
        return INSTALL_ERROR;
    }

    if (update_package == NULL) {
        LOG(ERROR) << "Update package is NULL.";
        return INSTALL_NONE;
    }

    if (verification_msg == NULL) {
        LOG(ERROR) << "Verification msg is NULL.";
        return INSTALL_VERIFY_FAILURE;
    }

    record_package_name();

    // Configure to install update progress.
    bool wipe_cache = false;
    mShowCurrentText = true;
    StartProgress(0.0f, kUpdateDuration);
    updater_secure_msg = verification_msg;
    printf("SubMenuApplyExt install package\n");
    set_cpu_governor(CONFIGURATION);


    bool should_use_fuse = false;
    if (!SetupPackageMount(update_package, &should_use_fuse)) {
      LOG(INFO) << "Failed to set up the package access, skipping installation";
      status = INSTALL_ERROR;
    } else if (install_with_fuse || should_use_fuse) {
      LOG(INFO) << "Installing package " << update_package << " with fuse";
      status = InstallWithFuseFromPath(update_package, this);
    } else if (auto memory_package = Package::CreateMemoryPackage(
              update_package,
              std::bind(&RecoveryUI::SetProgress, this, std::placeholders::_1));
            memory_package != nullptr) {
      status = InstallPackage(memory_package.get(), update_package, should_wipe_cache,
                              retry_count, this);
    } else {
      // We may fail to memory map the package on 32 bit builds for packages with 2GiB+ size.
      // In such cases, we will try to install the package with fuse. This is not the default
      // installation method because it introduces a layer of indirection from the kernel space.
      LOG(WARNING) << "Failed to memory map package " << update_package
                   << "; falling back to install with fuse";
      status = InstallWithFuseFromPath(update_package, this);
    }
    set_cpu_governor(DEFAULT);
    SetProgress(1.0f);
    mShowCurrentText = false;
    EndProgress();

    switch (status) {
        case INSTALL_SUCCESS:
            /* On success install, we force current system to the newly
             * installed 1st system. This should be what the user wants.
             * Else user may go to 2nd system and things may go wrong.
             */
            set_reboot_message(0);
            sync();
            break;
        case INSTALL_ERROR:
            GetMenuSelection("menu_apply_ext_fail", 0, 0, device);
            break;
        case INSTALL_CORRUPT:
            GetMenuSelection("menu_apply_ext_corrupt", 0, 0, device);
            break;
        case INSTALL_NO_FILE:
            GetMenuSelection("menu_apply_ext_no_file", 0, 0, device);
            break;
        case INSTALL_VERIFY_FAILURE:
            // if Verify failed, don't show information and reboot directly.
//          GetMenuSelection("menu_apply_ext_verify_fail", 0, 0, device);
            break;
    }

    return status;
}

int
MiScreenRecoveryUI::SubMenuWipe()
{
    int ret = 0;
    int chosen_item = GetMenuSelection("menu_wipe", 0, 0, this->GetDevice());
    switch (chosen_item) {
        case MENU_WIPE_ITEM_WIPE_DATA:
            chosen_item = GetMenuSelection("menu_wipe_data_confirm", 0, 0, this->GetDevice());
            if (chosen_item == CHOICE_YES) {
                android::base::SetProperty("vendor.wipecache.menu.confirmed", "yes");
                ret = SubMenuWipeCmdFormatData();
                if (ret == 0) {
                    chosen_item = GetMenuSelection(
                        "menu_wipe_data_result_ok", 0, 0, this->GetDevice());
                } else {
                    chosen_item = GetMenuSelection(
                        "menu_wipe_data_result_fail", 0, 0, this->GetDevice());
                }
            }
            return 0;
        case MENU_WIPE_ITEM_BACK_MAIN:
            return 0;
        case static_cast<int>(KeyError::TIMED_OUT):
            return static_cast<int>(KeyError::TIMED_OUT);
        default:
            LOG(ERROR) << "Choosen to menu_wipe item error";
            return -3;
    }
}

int
MiScreenRecoveryUI::SubMenuWipeCmdWipeCache(void)
{
    int ret = 0;

    StartProgress(1.0f, kWipeDuration);
    SetEnableReboot(false);
    ret = internal_erase_cache(this);
    SetEnableReboot(true);
    SetProgress(1.0f);
    EndProgress();

    return ret;
}

int
MiScreenRecoveryUI::SubMenuWipeCmdWipeData(bool convert_fbe)
{
    int ret = 0;
    StartProgress(1.0f, kWipeDuration);
    SetEnableReboot(false);
    ret = internal_erase_data_cache(this->GetDevice(), convert_fbe);
    if (ret == INSTALL_SUCCESS && should_wipe_efs_while_wipe_data()) {
        ret |= internal_wipe_efs();
    }
    SetEnableReboot(true);
    SetProgress(1.0f);
    EndProgress();

    return ret;
}

int
MiScreenRecoveryUI::SubMenuWipeCmdWipeEfs(void)
{
    int ret = 0;

    StartProgress(1.0f, kWipeDuration);
    SetEnableReboot(false);
    ret = internal_wipe_efs();
    SetEnableReboot(true);
    SetProgress(1.0f);
    EndProgress();

    return ret;
}

int
MiScreenRecoveryUI::SubMenuWipeCmdFormatData(void)
{
    int ret = 0;

    StartProgress(1.0f, kWipeDuration);
    SetEnableReboot(false);
    ret = internal_erase_data_cache(this->GetDevice(), false);
    if (ret == INSTALL_SUCCESS && should_wipe_efs_while_wipe_data()) {
        ret |= internal_wipe_efs();
    }
    SetEnableReboot(true);
    SetProgress(1.0f);
    EndProgress();

    return ret;
}

int
MiScreenRecoveryUI::SubMenuWipeCmdFormatStorage(void)
{
    int ret = 0;

    StartProgress(1.0f, kWipeDuration);
    SetEnableReboot(false);
    ret = internal_erase_storage(this);
    SetEnableReboot(true);
    SetProgress(1.0f);
    EndProgress();

    return ret;
}

int
MiScreenRecoveryUI::SubMenuWipeCmdWipePartial(void)
{
    int ret = 0;

    StartProgress(1.0f, kWipeDuration);
    SetEnableReboot(false);
    ret = internal_wipe_data_partial_erase_cache(this);
    if (ret == INSTALL_SUCCESS && should_wipe_efs_while_wipe_data()) {
        ret |= internal_wipe_efs();
    }
    SetEnableReboot(true);
    SetProgress(1.0f);
    EndProgress();

    return ret;
}

int
MiScreenRecoveryUI::StartInstallPackage(const char* path, bool* wipe_cache, const char* install_file,
    bool needs_mount, char* verification_msg, bool sideload)
{
    int result = 0;

    mShowCurrentText = true;
    StartProgress(0.0f, kSideloadDuration);
    SetEnableReboot(false);
    if (needs_mount) {
        updater_secure_msg = verification_msg;
    } else {
        sideload_secure_msg = verification_msg;
    }
    set_cpu_governor(CONFIGURATION);
    if(!sideload) {
      result = InstallWithFuseFromPath(path, this, 0, install_file, needs_mount);
    } else {
      auto package =
              Package::CreateFilePackage(path,
                                         std::bind(&RecoveryUI::SetProgress, this, std::placeholders::_1));
      result = InstallPackage(package.get(), path, false, 0, this, install_file, needs_mount);
    }
    set_cpu_governor(DEFAULT);
    SetEnableReboot(true);
    SetProgress(1.0f);
    mShowCurrentText = false;
    EndProgress();

    return result;
}
