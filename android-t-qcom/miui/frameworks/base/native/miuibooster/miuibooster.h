#ifndef ANDROID_MIUIBOOSTER_H_H
#define ANDROID_MIUIBOOSTER_H_H

#include "util.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <log/log.h>
#include <utils/Log.h>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <sys/system_properties.h>
#include <cutils/properties.h>
#include <utils/AndroidThreads.h>
#include <utils/String16.h>
#include <vector>
#include <map>
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#define CPU_FREQ_LEVELS 3
#define GPU_FREQ_LEVELS 3
#define GPU_FREQ_LEN    100
#define DDR_FREQ_LEVELS 3
#define DDR_FREQ_CHAR_LEN  100
#define DDR_FREQ_COUNT  20
#define PRE_PER_DDR_FREQ_LEN 10
#define BIND_CORE_LEN 10

#define PERFD_LIB  "libqti-perfd-client.so"
#define PERF_LOCK_LIB_FULL_NAME  "libmtkperf_client_vendor.so"

#include <android/api-level.h>
#if defined __ANDROID_API_O__
#include <vndksupport/linker.h>
#endif

#if defined MTK_PLATFORM_P
#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPerf.h>
#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPower.h>
using namespace vendor::mediatek::hardware::mtkpower::V1_0;
//#include "mtkpower_types.h"
//#include "mtkperf_resource.h"
#endif
using namespace std;

const static int32_t MIUI_BOOSTER_OK = 0;
const static int32_t MIUI_BOOSTER_CPU_HIGH_FREQ = 1 << 3;// 1000，即8
const static int32_t MIUI_BOOSTER_GPU_HIGH_FREQ = 1 << 3;// 1000，即8

const static int32_t MIUI_BOOSTER_FAILED_DEPENDENCY = -10004;

//level 1, 2, 3,  level 1 = max
// 8998 C1 & D5
const static int big_core_value_8998[CPU_FREQ_LEVELS] = {0xFFF, 2100, 1600};
const static int little_core_value_8998[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
// 845 E1
const static int big_core_value_845[CPU_FREQ_LEVELS] = {0xFFF, 2200, 1900};
const static int little_core_value_845[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
// 8150 F1
const static int prime_core_value_8150[CPU_FREQ_LEVELS] = {0xFFF, 2600, 2200};
const static int big_core_value_8150[CPU_FREQ_LEVELS] = {0xFFF, 2200, 1900};
const static int little_core_value_8150[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
// 636 E7S & E4
const static int big_core_value_636[CPU_FREQ_LEVELS] = {0xFFF, 1800, 1600};
const static int little_core_value_636[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
//660 C8 & D2S & F7A
const static int big_core_value_660[CPU_FREQ_LEVELS] = {0xFFF, 1900, 1700};
const static int little_core_value_660[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1100};
// 710 E2
const static int big_core_value_710[CPU_FREQ_LEVELS] = {0xFFF, 2000, 1700};
const static int little_core_value_710[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1100};
// 625 D1S
const static int big_core_value_625[CPU_FREQ_LEVELS] = {0xFFF, 1800, 1600};
const static int little_core_value_625[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
// 821 A4 & A7 & A8 & B7
const static int big_core_value_821[CPU_FREQ_LEVELS] = {0xFFF, 2000, 1600};
const static int little_core_value_821[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
// 450 D1
const static int big_core_value_450[CPU_FREQ_LEVELS] = {0xFFF, 1800, 1600};
const static int little_core_value_450[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
// 425 C3B
const static int big_core_value_425[CPU_FREQ_LEVELS] = {0xFFF, 1400, 1200};
const static int little_core_value_425[CPU_FREQ_LEVELS] = {0xFFF, 1200, 1000};
// 712 F2
const static int big_core_value_712[CPU_FREQ_LEVELS] = {0xFFF, 2100, 1800};
const static int little_core_value_712[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1200};
//675 F7B
const static int big_core_value_675[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1300};
const static int little_core_value_675[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1200};
//632 F6 & F6L
const static int big_core_value_632[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1100};
const static int little_core_value_632[CPU_FREQ_LEVELS] = {0xFFF, 1500, 1000};
//820 A1
const static int big_core_value_820[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1500};
const static int little_core_value_820[CPU_FREQ_LEVELS] = {0xFFF, 1200, 1100};
//730 F10_730
const static int big_core_value_730[CPU_FREQ_LEVELS] = {0xFFF, 2100, 1900};
const static int little_core_value_730[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1500};
//765G(lito) G7A
const static int prime_core_value_765g[CPU_FREQ_LEVELS] = {0xFFF, 2100, 1700};
const static int big_core_value_765g[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1200};
const static int little_core_value_765g[CPU_FREQ_LEVELS] = {0xFFF, 1400, 1000};
//439 C3E/C3K/C3I
const static int big_core_value_439[CPU_FREQ_LEVELS] = {0xFFF, 1900, 1700};
const static int little_core_value_439[CPU_FREQ_LEVELS] = {0xFFF, 1300, 1100};
//356 J1/J2/J11/J1S
const static int prime_core_value_356[CPU_FREQ_LEVELS] = {0xFFF, 2700, 2600};
const static int big_core_value_356[CPU_FREQ_LEVELS] = {0xFFF, 2300, 2200};
const static int little_core_value_356[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1600};
//394 C3J/C3X
const static int big_core_value_394[CPU_FREQ_LEVELS] = {0xFFF, 1800, 1500};
const static int little_core_value_394[CPU_FREQ_LEVELS] = {0xFFF, 1600, 1400};
//444 J19S/J19SC
const static int big_core_value_444[CPU_FREQ_LEVELS] = {0xFFF, 1800, 1600};
const static int little_core_value_444[CPU_FREQ_LEVELS] = {0xFFF, 1600, 1400};
//459 J17
const static int big_core_value_459[CPU_FREQ_LEVELS] = {0xFFF, 2100, 2000};
const static int little_core_value_459[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1600};
//443 J6C
const static int big_core_value_443[CPU_FREQ_LEVELS] = {0xFFF, 2200, 2100};
const static int little_core_value_443[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1600};
//407 J6A1
const static int big_core_value_407[CPU_FREQ_LEVELS] = {0xFFF, 2100, 1900};
const static int little_core_value_407[CPU_FREQ_LEVELS] = {0xFFF, 1700, 1600};
//415 K2
const static int prime_core_value_415[CPU_FREQ_LEVELS] = {0xFFF, 2700, 2500};
const static int big_core_value_415[CPU_FREQ_LEVELS] = {0xFFF, 2350, 2200};
const static int little_core_value_415[CPU_FREQ_LEVELS] = {0xFFF, 1709, 1620};
//450 K9
const static int prime_core_value_450_K9[CPU_FREQ_LEVELS] = {0xFFF, 2361, 2169};
const static int big_core_value_450_K9[CPU_FREQ_LEVELS] = {0xFFF, 2131, 1900};
const static int little_core_value_450_K9[CPU_FREQ_LEVELS] = {0xFFF, 1804, 1670};
//475 K9B/k9D
const static int prime_core_value_475[CPU_FREQ_LEVELS] = {0xFFF, 2380, 2208};
const static int big_core_value_475[CPU_FREQ_LEVELS] = {0xFFF, 2131, 2054};
const static int little_core_value_475[CPU_FREQ_LEVELS] = {0xFFF, 1651, 1516};
//518 K7T
const static int big_core_value_518[CPU_FREQ_LEVELS] = {0xFFF, 2208, 1766};
const static int little_core_value_518[CPU_FREQ_LEVELS] = {0xFFF, 1804, 1516};
//507 K6S
const static int big_core_value_507[CPU_FREQ_LEVELS] = {0xFFF, 2054, 1900};
const static int little_core_value_507[CPU_FREQ_LEVELS] = {0xFFF, 1708, 1651};
//454 K19K
const static int big_core_value_454[CPU_FREQ_LEVELS] = {0xFFF, 1804, 1651};
const static int little_core_value_454[CPU_FREQ_LEVELS] = {0xFFF, 1708, 1574};
//457 L2/L3/L10
const static int prime_core_value_457[CPU_FREQ_LEVELS] = {0xFFF, 2726, 2515};
const static int big_core_value_457[CPU_FREQ_LEVELS] = {0xFFF, 2227, 1996};
const static int little_core_value_457[CPU_FREQ_LEVELS] = {0xFFF, 1574, 1363};
//530 L1/L2S/L3S/L18
const static int prime_core_value_530[CPU_FREQ_LEVELS] = {0xFFF, 2592, 2361};
const static int big_core_value_530[CPU_FREQ_LEVELS] = {0xFFF, 2342, 2112};
const static int little_core_value_530[CPU_FREQ_LEVELS] = {0xFFF, 1555, 1324};

#if ((defined MTK_PLATFORM_P) || (defined MTK_PLATFORM))
//MT6765 lotus F9/C3MN/C3MI/C3M
const static int big_core_value_mt6765[CPU_FREQ_LEVELS] = {2301000, 2215000, 2139000};
const static int little_core_value_mt6765[CPU_FREQ_LEVELS] = {1800000, 1682000, 1579000};
//MT6762 C3D/C3L
const static int big_core_value_mt6762[CPU_FREQ_LEVELS] = {2001000, 1961000, 1927000};
const static int little_core_value_mt6762[CPU_FREQ_LEVELS] = {1500000, 1429000, 1367000};
//MT6762V/CN C3C
const static int big_core_value_mt6762v[CPU_FREQ_LEVELS] = {2001000, 1961000, 1927000};
const static int little_core_value_mt6762v[CPU_FREQ_LEVELS] = {2001000, 1961000, 1927000};
//MT6785 G7
const static int big_core_value_mt6785[CPU_FREQ_LEVELS] = {2050000, 1986000, 1923000};
const static int little_core_value_mt6785[CPU_FREQ_LEVELS] = {2000000, 1933000, 1866000};
//MT6769 J19/J19P
const static int big_core_value_mt6769[CPU_FREQ_LEVELS] = {2000000, 1950000, 1900000};
const static int little_core_value_mt6769[CPU_FREQ_LEVELS] = {1800000, 1625000, 1500000};
//MT6875 J7A/J7B
const static int big_core_value_mt6875[CPU_FREQ_LEVELS] = {2600000, 2433000, 2266000};
const static int little_core_value_mt6875[CPU_FREQ_LEVELS] = {2000000, 1875000, 1812000};
//MT6853T J22
const static int big_core_value_mt6853[CPU_FREQ_LEVELS] = {2400000, 2306000, 2210000};
const static int little_core_value_mt6853[CPU_FREQ_LEVELS] = {2000000, 1916000, 1812000};
//MT6889Z/CZA J10
const static int big_core_value_mt6889[CPU_FREQ_LEVELS] = {2600000, 2529000, 2387000};
const static int little_core_value_mt6889[CPU_FREQ_LEVELS] = {2000000, 1895000, 1791000};
//MT6891Z/CZA K10A
const static int big_core_value_mt6891[CPU_FREQ_LEVELS] = {2600000, 2529000, 2387000};
const static int little_core_value_mt6891[CPU_FREQ_LEVELS] = {2000000, 1895000, 1791000};
//MT6833 K19/K16A
const static int big_core_value_mt6833[CPU_FREQ_LEVELS] = {2203000, 2087000, 1995000};
const static int little_core_value_mt6833[CPU_FREQ_LEVELS] = {2000000, 1916000, 1812000};
//MT6983 L11
const static int big_core_value_mt6983[CPU_FREQ_LEVELS] = {3050000, 2650000, 2350000};
const static int little_core_value_mt6983[CPU_FREQ_LEVELS] = {1800000, 1350000, 1000000};
//MT6895 L11A
const static int big_core_value_mt6895[CPU_FREQ_LEVELS] = {2850000, 2250000, 1800000};
const static int little_core_value_mt6895[CPU_FREQ_LEVELS] = {2000000, 1400000, 1050000};
//MT6893 K10 K11R
const static int big_core_value_mt6893[CPU_FREQ_LEVELS] = {2600000, 2354000, 1985000};
const static int little_core_value_mt6893[CPU_FREQ_LEVELS] = {2000000, 1800000, 1625000};
//MT6877 K16
const static int big_core_value_mt6877[CPU_FREQ_LEVELS] = {2500000, 2275000, 1900000};
const static int little_core_value_mt6877[CPU_FREQ_LEVELS] = {2000000, 1800000, 1600000};
//MT6781 K7S
const static int big_core_value_mt6781[CPU_FREQ_LEVELS] = {2050000, 1986000, 1923000};
const static int little_core_value_mt6781[CPU_FREQ_LEVELS] = {2000000, 1933000, 1866000};
//mt6789 L19A/C
const static int big_core_value_mt6789[CPU_FREQ_LEVELS] = {2200000, 1700000, 1400000};
const static int little_core_value_mt6789[CPU_FREQ_LEVELS] = {2000000, 1450000, 1200000};
//
#endif

namespace android {
class MiuiBoosterUtils{

public:
   int get_soc_id();
   int get_gpu_count();
   void initCpuFreqLevelValue();
   void initGpuFreqLevelValue();
   void initDDRFreqLevelValue();
   void initBindCoreValue();
   void *get_qcopt_handle();
   MiuiBoosterUtils();
   ~MiuiBoosterUtils();
   int perf_hint_hc(int level, int duration);
   int perf_hint_hg(int level, int duration);
   int perf_hint_hddr(int level , int duration);
   int requestCpuHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp);
   int requestGpuHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp);
   int requestDDRHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp);
   void release_request(int lock_handle);
   int cancelCpuHighFreq(int tid, int64_t timestamp);
   int cancelIOHighFreq(int tid, int64_t timestamp);
   int cancelDDRHighFreq(int tid, int64_t timestamp);
   int cancelGpuHighFreq(int tid, int64_t timestamp);
#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)
   char *get_mtk_cpu_model();
   int get_soc_id_mtk();
   void initCpuFreqLevelValue_MTK();
   void initGpuFreqLevelValue_MTK();
   void initDDRFreqLevelValue_MTK();
#endif
   int requestUnifyCpuIOThreadCoreGpu(int scene, int64_t action, int cpulevel, int iolevel,
        vector<int> bindtids, int gpulevel, int timeoutms, int callertid, int64_t timestamp);
   int cancelUnifyCpuIOThreadCoreGpu(int cancelcpu, int cancelio, int cancelthread, vector<int>bindtids,
        int cancelgpu, int callertid, int64_t timestamp);
   int requestThreadPriority(int scene, int64_t action, vector<int> bindtids,
        int timeoutms, int callertid, int64_t timestamp);
   int cancelThreadPriority(vector<int> bindtids, int callertid, int64_t timestamp);
   void closeQTHandle();
   int requestIOPrefetch(String16& filePath);
   int requestBindCore(int uid, int tid);
   int requestSetScene(String16& pkgName, int sceneId);

private:
   int *big_core_value;
   int *little_core_value;
   int *prime_core_value;
   char gpu_freq_buf[GPU_FREQ_LEN];
   int gpu_freq_level[GPU_FREQ_LEVELS];
   int ddr_freq_level[DDR_FREQ_LEVELS];
   bool has_prime_core = false;
   int Cpu4maxfreq = 0;
   int Cpu6maxfreq = 0;
   int Cpu7maxfreq = 0;
   // MIUI智能帧率场景ID和最大刷新率对照表
   map<int, int> sceneId_maxRefreshRate_map = {
      { 0, -1 },
      { 10101, 120 },
      { 10102, 120 },
      { 10103, 120 },
      { 10104, 60 },
      { 10105, 120},
      { 10201, 60 },
      { 10202, 60 },
      { 10203, 120 },
      { 20101, 60 },
      { 20102, 120 },
      { 20103, 120 },
      { 20201, 60 },
      { 20202, 120 },
      { 20203, 120 }
   };

#ifdef MTK_PLATFORM_P
   android::sp<IMtkPerf> spMtkPerf;
   android::sp<IMtkPower> spMtkPower;
#endif

   void *qcopt_handle;
   int perf_handle = 0;
   int (*perf_lock_acq)(unsigned long handle, int duration, int list[], int numArgs);
   int (*perf_lock_rel)(unsigned long);
   int (*perf_hint)(int, char *, int, int);
};
}
#endif //ANDROID_MIUIBOOSTER_H_H
