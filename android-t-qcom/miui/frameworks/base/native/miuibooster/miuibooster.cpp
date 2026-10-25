#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <malloc.h>
#include "miuibooster.h"

using namespace android;

namespace android {
   int MiuiBoosterUtils::get_soc_id(){
        int fd;
        int soc = -1;
        if (!access("/sys/devices/soc0/soc_id", F_OK)) {
           fd = open("/sys/devices/soc0/soc_id", O_RDONLY);
        } else {
           fd = open("/sys/devices/system/soc/soc0/id", O_RDONLY);
        }
        if (fd != -1)
        {
          char raw_buf[5];
          read(fd, raw_buf,4);
          raw_buf[4] = 0;
          soc = atoi(raw_buf);
          close(fd);
        }
        else
          close(fd);
        return soc;
   }

   void MiuiBoosterUtils::initCpuFreqLevelValue(){
        int soc_id = get_soc_id();
        pdbg("initCpuFreqLevel socid:%d", soc_id);
        switch(soc_id)
        {
            //SOCID_8998
            case 292:
                big_core_value = (int *)big_core_value_8998;
                little_core_value = (int *)little_core_value_8998;
                break;
            //SOCID_845
            case 321:
                big_core_value = (int *)big_core_value_845;
                little_core_value = (int *)little_core_value_845;
                break;
            //SOCID_636
            case 345:
                big_core_value = (int *)big_core_value_636;
                little_core_value = (int *)little_core_value_636;
                break;
            //SOCID_660
            case 317:
                big_core_value = (int *)big_core_value_660;
                little_core_value = (int *)little_core_value_660;
                break;
            //SOCID_8150
            case 339:
                big_core_value = (int *)big_core_value_8150;
                little_core_value = (int *)little_core_value_8150;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_8150;
                break;
            //SOCID_710
            case 360:
                big_core_value = (int *)big_core_value_710;
                little_core_value = (int *)little_core_value_710;
                break;
            //SOCID_625
            case 293:
                big_core_value = (int *)big_core_value_625;
                little_core_value = (int *)little_core_value_625;
                break;
            //SOCID_821
            case 305:
                big_core_value = (int *)big_core_value_821;
                little_core_value = (int *)little_core_value_821;
                break;
            //SOCID_450
            case 338:
                big_core_value = (int *)big_core_value_450;
                little_core_value = (int *)little_core_value_450;
                break;
            //SOCID_425
            case 303:
                big_core_value = (int *)big_core_value_425;
                little_core_value = (int *)little_core_value_425;
                break;
            //SOCID_712
            case 393:
                big_core_value = (int *)big_core_value_712;
                little_core_value = (int *)little_core_value_712;
                break;
            //SOCID_675
            case 355:
                big_core_value = (int *)big_core_value_675;
                little_core_value = (int *)little_core_value_675;
                break;
            //SOCID_632
            case 349:
                big_core_value = (int *)big_core_value_632;
                little_core_value = (int *)little_core_value_632;
                break;
            //SOCID_820
            case 246:
                big_core_value = (int *)big_core_value_820;
                little_core_value = (int *)little_core_value_820;
                break;
            //SOCID_730
            case 365:
                big_core_value = (int *)big_core_value_730;
                little_core_value = (int *)little_core_value_730;
                break;
            //SOCID_765g
            case 400:
                big_core_value = (int *)big_core_value_765g;
                little_core_value = (int *)little_core_value_765g;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_765g;
                break;
            //SOCID_439
            case 353:
                big_core_value = (int *)big_core_value_439;
                little_core_value = (int *)little_core_value_439;
                break;
            case 356:
                big_core_value = (int *)big_core_value_356;
                little_core_value = (int *)little_core_value_356;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_356;
                break;
            case 394:
                big_core_value = (int *)big_core_value_394;
                little_core_value = (int *)little_core_value_394;
                break;
            case 444:
                big_core_value = (int *)big_core_value_444;
                little_core_value = (int *)little_core_value_444;
                break;
            case 459:
                big_core_value = (int *)big_core_value_459;
                little_core_value = (int *)little_core_value_459;
                break;
            case 443:
                big_core_value = (int *)big_core_value_443;
                little_core_value = (int *)little_core_value_443;
                break;
            case 407:
                big_core_value = (int *)big_core_value_407;
                little_core_value = (int *)little_core_value_407;
                break;
            case 415:
                big_core_value = (int *)big_core_value_415;
                little_core_value = (int *)little_core_value_415;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_415;
                break;
            case 450:
                big_core_value = (int *)big_core_value_450_K9;
                little_core_value = (int *)little_core_value_450_K9;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_450_K9;
                break;
            case 475:
                big_core_value = (int *)big_core_value_475;
                little_core_value = (int *)little_core_value_475;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_475;
                break;
            case 518:
                big_core_value = (int *)big_core_value_518;
                little_core_value = (int *)little_core_value_518;
                break;
            case 507:
                big_core_value = (int *)big_core_value_507;
                little_core_value = (int *)little_core_value_507;
                break;
            case 454:
                big_core_value = (int *)big_core_value_454;
                little_core_value = (int *)little_core_value_454;
                break;
            case 457:
                big_core_value = (int *)big_core_value_457;
                little_core_value = (int *)little_core_value_457;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_457;
                break;
            case 530:
                big_core_value = (int *)big_core_value_530;
                little_core_value = (int *)little_core_value_530;
                has_prime_core = true;
                prime_core_value = (int *)prime_core_value_530;
                break;
                
            default:
                pwrn("Unsupported soc!!!");
                return;
        }
   }
   void MiuiBoosterUtils::initGpuFreqLevelValue(){
        int len = __system_property_get("vendor.gpu.available_frequencies", gpu_freq_buf);
        pwrn("gpu_freq_buf:%s", gpu_freq_buf);
        if (len <= 0) {
            return;
        }
        int count_gpu = 0;
        for (int i = 0; i < len; i++) {
            if (gpu_freq_buf[i] == ' ')
                ++count_gpu;
        }
        pwrn("count_gpu:%d", count_gpu);
        switch (count_gpu)
            {
                case 1:
                case 2:
                    gpu_freq_level[0] = 0;
                    gpu_freq_level[1] = 0;
                    gpu_freq_level[2] = 0;
                    break;
                case 3:
                case 4:
                    gpu_freq_level[0] = 0;
                    gpu_freq_level[1] = 1;
                    gpu_freq_level[2] = 2;
                    break;
                default:
                    gpu_freq_level[0] = 0;
                    gpu_freq_level[1] = (1+(1+count_gpu)/2)/2-1;
                    gpu_freq_level[2] = (1+count_gpu)/2-1;
            }
        pwrn("gpu_freq_level:%d,%d,%d", gpu_freq_level[0], gpu_freq_level[1], gpu_freq_level[2]);
   }

   void MiuiBoosterUtils::initDDRFreqLevelValue(){
        //Acquiring DDR available frequencies as string formal
        int fd;
        fd = open("/sys/class/devfreq/soc:qcom,cpu0-llcc-ddr-lat/available_frequencies", O_RDONLY);
        pwrn("fd : %d", fd);
        if (fd == -1)
            return;
        char ddr_freq_char[DDR_FREQ_CHAR_LEN];
        int len = read(fd, ddr_freq_char, DDR_FREQ_CHAR_LEN);
        close(fd);
        pwrn("len : %d", len);

        //Counting the number of DDR available frequencies
        int count_ddr = 0;
        for(int i = 0; i < len; i++){
            if(ddr_freq_char[i] == ' ')
               ++count_ddr;
        }
        ++count_ddr;
        pwrn("count : %d", count_ddr);

        //Split DDR available frequencies string and convert to int
        char ddr_freq_temp[DDR_FREQ_COUNT][PRE_PER_DDR_FREQ_LEN];
        for (int i = 0; i < DDR_FREQ_COUNT; i++){
            for (int j = 0 ;j < PRE_PER_DDR_FREQ_LEN; j++){
               ddr_freq_temp[i][j] = '\0';
            }
        }
        int ddr_freq[DDR_FREQ_COUNT];
        int j = 0;
        int k = 0;
        for (int i = 0; ddr_freq_char[i] != '\0'; i++){
            if (ddr_freq_char[i] != ' ' ){
               ddr_freq_temp[j][k++] = ddr_freq_char[i];
            }else {
                j++;
                k = 0;
            }
        }
        for (int i = 0; i <= j; i++){
            ddr_freq[i] = atoi(ddr_freq_temp[i]);
            pwrn("ddr_freq[%d] : %d\n", i, ddr_freq[i]);
        }

        ddr_freq_level[0] = ddr_freq[count_ddr - 1];
        ddr_freq_level[1] = ddr_freq[count_ddr - 2];
        ddr_freq_level[2] = ddr_freq[count_ddr - 3];
        pwrn ("ddr_freq_level: %d, %d, %d", ddr_freq_level[0], ddr_freq_level[1], ddr_freq_level[2]);
   }

     void MiuiBoosterUtils::initBindCoreValue(){
        int fd4 = open("/sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_max_freq", O_RDONLY);
        int fd6 = open("/sys/devices/system/cpu/cpu6/cpufreq/cpuinfo_max_freq", O_RDONLY);
        int fd7 = open("/sys/devices/system/cpu/cpu7/cpufreq/cpuinfo_max_freq", O_RDONLY);
        pwrn("fd4 : %d, fd6 : %d, fd7 : %d", fd4, fd6, fd7);
        if (fd4 == -1 || fd6 == -1 || fd7 == -1){
            perr("%s\n", strerror(errno));
            return;
        }

        char cpu4maxfreq[BIND_CORE_LEN];
        for (int i = 0; i < BIND_CORE_LEN; i++){
            cpu4maxfreq[i] = '\0';
        }
        char cpu6maxfreq[BIND_CORE_LEN];
        for (int i = 0; i < BIND_CORE_LEN; i++){
            cpu6maxfreq[i] = '\0';
        }
        char cpu7maxfreq[BIND_CORE_LEN];
        for (int i = 0; i < BIND_CORE_LEN; i++){
            cpu7maxfreq[i] = '\0';
        }

        int len4 = read(fd4, cpu4maxfreq, BIND_CORE_LEN);
        int len6 = read(fd6, cpu6maxfreq, BIND_CORE_LEN);
        int len7 = read(fd7, cpu7maxfreq, BIND_CORE_LEN);
        pwrn("len4 : %d, len6 : %d, len7 : %d", len4, len6, len7);
        close(fd4);
        close(fd6);
        close(fd7);

        Cpu4maxfreq = atoi(cpu4maxfreq);
        Cpu6maxfreq = atoi(cpu6maxfreq);
        Cpu7maxfreq = atoi(cpu7maxfreq);

        pwrn("Cpu4maxfreq = %d, Cpu6maxfreq = %d, Cpu7maxfreq = %d", Cpu4maxfreq, Cpu6maxfreq, Cpu7maxfreq);
     }

   void* MiuiBoosterUtils::get_qcopt_handle(){
       void *handle = NULL;

#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)
       #if defined __ANDROID_API_O__
       handle = android_load_sphal_library(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);
       #else
       handle = dlopen(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);
       #endif
       if (!handle) {
           pwrn("get_qcopt_handle Unable to open %s: %s\n", PERF_LOCK_LIB_FULL_NAME,
                   dlerror());
       }
#else
       #if defined __ANDROID_API_O__
       handle = android_load_sphal_library(PERFD_LIB, RTLD_NOW);
       #else
       handle = dlopen(PERFD_LIB, RTLD_NOW);
       #endif
       if (!handle) {
           pwrn("get_qcopt_handle Unable to open %s: %s\n", PERFD_LIB,
                   dlerror());
       }
#endif
       return handle;
   }

   MiuiBoosterUtils::MiuiBoosterUtils(){
        pwrn("MiuiBoosterUtils init ...");
        initBindCoreValue();
#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)
        if (spMtkPerf == NULL)
            spMtkPerf = IMtkPerf::tryGetService();
        if (spMtkPower == NULL)
            spMtkPower = IMtkPower::tryGetService();
        if (spMtkPerf == NULL)
            perr("Failed to get mtk Perf handle.\n");
        if (spMtkPower == NULL)
            perr("Failed to get mtk power handle.\n");

        initCpuFreqLevelValue_MTK();
        initGpuFreqLevelValue_MTK();
        initDDRFreqLevelValue_MTK();

#else   //qcom platform
        //perf
        qcopt_handle = get_qcopt_handle();
        if (!qcopt_handle) {
            pwrn("constructor unable to open %s: %s", PERFD_LIB, dlerror());
            return;
        } else {

            perf_lock_acq = (int (*)(unsigned long, int, int *, int))dlsym(qcopt_handle, "perf_lock_acq");
            if (!perf_lock_acq) {
                pwrn("couldn't get perf_lock_acq function handle.");
            }

            perf_lock_rel = (int (*)(unsigned long))dlsym(qcopt_handle, "perf_lock_rel");

            if (!perf_lock_rel) {
                pwrn("Unable to get perf_lock_rel function handle.");
            }
            perf_hint = (int (*)(int, char *, int, int))dlsym(qcopt_handle, "perf_hint");

            if (!perf_hint) {
                pwrn("Unable to get perf_hint function handle.");
            }
        }
        initCpuFreqLevelValue();
        initGpuFreqLevelValue();
        initDDRFreqLevelValue();
#endif
   }

   MiuiBoosterUtils::~MiuiBoosterUtils(){
        closeQTHandle();
   }

   int MiuiBoosterUtils::perf_hint_hc(int level , int duration){
        pwrn("perf_hint_hc start");
        int lock_handle = 0;

        if (duration < 0) {
            perr("duration < 0");
            return 0;
        }

        if (big_core_value == NULL || little_core_value == NULL) {
            perr("big_core_value or little_core_value is NULL");
            return 0;
        }


        if (has_prime_core && prime_core_value == NULL) {
            perr("has_prime_core is NULL");
            return 0;
        }

#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)
        if (spMtkPerf == NULL){
            spMtkPerf = IMtkPerf::tryGetService();
        }
        if (spMtkPerf != NULL) {
            android::hardware::hidl_vec<int32_t> resource_values = {0x00400000, big_core_value[level-1], 0x00400100, little_core_value[level-1]};
            lock_handle = spMtkPerf->perfLockAcquire(0, duration, resource_values, gettid());
        } else {
            perr("Unsupported spMtkPerf!!!");
        }
#else
        if (qcopt_handle) {
            // level 1, 2, 3,  level 1 = max
            pwrn("big_core_value[level-1]:%d", big_core_value[level-1]);
            pwrn("little_core_value[level-1]:%d", little_core_value[level-1]);
            int resource_values[6] = {0x40800000, big_core_value[level-1], 0x40800100, little_core_value[level-1]};
            int num_resources = sizeof(resource_values)/sizeof(resource_values[0]);
            if (has_prime_core) {
                pwrn("prime_core_value[level - 1]:%d", prime_core_value[level-1]);
                resource_values[4] = 0x40800200;
                resource_values[5] = prime_core_value[level-1];
            } else {
                num_resources -= 2;
            }

            if (perf_lock_acq) {
                lock_handle = perf_lock_acq(lock_handle, duration, resource_values, num_resources);
            }

            pdbg("perf_hint_hc end,lock_handle:%d",lock_handle);
        }
#endif
        return lock_handle;
   }


   int MiuiBoosterUtils::perf_hint_hg(int level , int duration){
        pwrn("perf_hint_hg start");

        if (duration < 0)
            return 0;

        int lock_handle = 0;
#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)
        if (spMtkPerf == NULL){
            spMtkPerf = IMtkPerf::tryGetService();
        }
        if (spMtkPerf != NULL) {
            android::hardware::hidl_vec<int32_t> resource_values = {0x00c00000, gpu_freq_level[level-1]};
            lock_handle = spMtkPerf->perfLockAcquire(0, duration, resource_values, gettid());
        }else {
            perr("Unsupported spMtkPerf!!!");
        }
#else
        if (qcopt_handle) {
            // level 1, 2, 3,  level 1 = max
            int resource_values[2] = {0X42804000, gpu_freq_level[level-1]};
            int num_resources = sizeof(resource_values)/sizeof(resource_values[0]);
            if (perf_lock_acq) {
                lock_handle = perf_lock_acq(lock_handle, duration, resource_values, num_resources);
            }
            pwrn("perf_hint_hg end,lock_handle:%d", lock_handle);
        }
#endif
        return lock_handle;
    }

   int MiuiBoosterUtils::perf_hint_hddr(int level , int duration){
        pwrn("perf_hint_hddr start");

        if (duration < 0)
            return 0;

        int lock_handle = 0;
#if (defined MTK_PLATFORM_P)|| (defined MTK_PLATFORM)
        if (spMtkPerf == NULL){
            spMtkPerf = IMtkPerf::tryGetService();
        }
        if (spMtkPerf != NULL) {
            android::hardware::hidl_vec<int32_t> resource_values = {0x01000000, ddr_freq_level[level-1]};
            lock_handle = spMtkPerf->perfLockAcquire(0, duration, resource_values, gettid());
            pwrn("lock_handle:%d\n", lock_handle);
        }else {
            perr("Unsupported spMtkPerf!!!");
        }

#else
        if (qcopt_handle) {
            // level 1, 2, 3,  level 1 = max
            int resource_values[4] = {0x43430000, ddr_freq_level[level-1], 0x43430100, ddr_freq_level[level-1]};
            int num_resources = sizeof(resource_values)/sizeof(resource_values[0]);
            if (perf_lock_acq) {
                lock_handle = perf_lock_acq(lock_handle, duration, resource_values, num_resources);
            }
            pwrn("perf_hint_hg end,lock_handle:%d", lock_handle);
        }
#endif
        return lock_handle;
   }

   int MiuiBoosterUtils::requestCpuHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp){
        pdbg("requestCpuHighFreq, scene:%d action:%d level:%d timeoutms:%d tid:%d timestamp:%d",
               scene, TOINT(action), level, timeoutms,
               tid, TOINT(timestamp/1000000L));

        if (level > CPU_FREQ_LEVELS || level < 0 || timeoutms <= 0)
            return -1;

        // limit max timeout for different level
        if (level == 1 && timeoutms > 10000){
            timeoutms = 10000;
        }else if (level == 2 && timeoutms > 5000){
            timeoutms = 5000;
        }else if (level == 3 && timeoutms > 1500){
            timeoutms = 1500;
        }

        pwrn("requestCpuHighFreq,final level:%d,final timeoutms:%d",level,timeoutms);

        perf_handle = perf_hint_hc(level, timeoutms);
        pwrn("perf_handle:%d",perf_handle);

        return perf_handle > 0 ? level : -1;
   }


   int MiuiBoosterUtils::requestGpuHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp){
        pwrn("requestGpuHighFreq, scene:%d action:%d level:%d timeoutms:%d tid:%d timestamp:%d",
               scene, TOINT(action), level, timeoutms,
               tid, TOINT(timestamp/1000000L));

        if (level > GPU_FREQ_LEVELS || level < 0 || timeoutms <= 0)
            return -1;

        if((gpu_freq_level[0] == 0) && (gpu_freq_level[1] == 0) && (gpu_freq_level[2] == 0))
            return -1;

        // limit max timeout for different level
        if (level == 1 && timeoutms > 10000){
            timeoutms = 10000;
        }else if (level == 2 && timeoutms > 5000){
            timeoutms = 5000;
        }else if (level == 3 && timeoutms > 1500){
            timeoutms = 1500;
        }

        pwrn("requestGpuHighFreq,final level:%d,final timeoutms:%d", level,timeoutms);

        perf_handle = perf_hint_hg(level, timeoutms);

        return perf_handle > 0 ? level : -1;
   }

  int MiuiBoosterUtils::requestIOPrefetch(String16& path){
        String8 filePath = (path) ? String8(path) : String8("");
        pwrn("requestIOPrefetch, filePath:%s", filePath.string());

        int fd = -1;
        int ret = -1;
        struct stat file_stat;

        fd = open(filePath.string(), O_RDONLY);
        if (fd == -1) {
            pwrn("Error %d: fail to open file!", errno);
            goto finish_preload;
        }

        if (fstat(fd, &file_stat) < 0) {
            pwrn("fail to get file stat!");
            goto finish_preload;
        }

        if (file_stat.st_size == 0) {
            pwrn("file size 0!");
            goto finish_preload;
        }

        //load each page by posix_fadvise here to avoid read_ahead
        //and decrease CPU usage of copy io content result to local viriable
        ret = posix_fadvise(fd, 0, file_stat.st_size, POSIX_FADV_WILLNEED);
        if (ret !=0) {
            pwrn("posix_fadvise failed[POSIX_FADV_WILLNEED]");
            goto finish_preload;
        }

        //after finish preload, mark file cache advise as normal
        ret = posix_fadvise(fd, 0, file_stat.st_size, POSIX_FADV_NORMAL);
        if (ret != 0) {
            pwrn("posix_fadvise failed[POSIX_FADV_NORMAL]");
        }

        finish_preload:
            pwrn("finish_preload");
            if (fd != -1) {
                close(fd);
            }

        return ret != 0 ? ret : MIUI_BOOSTER_OK;
        }

   int MiuiBoosterUtils::requestDDRHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp){
        pwrn("requestDDRHighFreq, scene:%d action:%d level:%d timeoutms:%d tid:%d timestamp:%d",
        scene, TOINT(action), level, timeoutms,
        tid, TOINT(timestamp/1000000L));

        if (level > DDR_FREQ_LEVELS || level < 0 || timeoutms <= 0){
            pwrn("error is here1");
            return -1;
         }
        if((ddr_freq_level[0] == 0) && (ddr_freq_level[1] == 0) && (ddr_freq_level[2] == 0)){
            pwrn("error is here2");
            return -1;
         }

         // limit max timeout for different level
        if (level == 1 && timeoutms > 10000){
            timeoutms = 10000;
        }else if (level == 2 && timeoutms > 5000){
            timeoutms = 5000;
        }else if (level == 3 && timeoutms > 1500){
            timeoutms = 1500;
        }

        pwrn("requestDDRHighFreq,final level:%d,final timeoutms:%d", level,timeoutms);

        perf_handle = perf_hint_hddr(level, timeoutms);

        return perf_handle > 0 ? level : -1;
   }

  int MiuiBoosterUtils::requestBindCore(int uid, int tid){
        pwrn("uid = %d, tid = %d", uid, tid);

        cpu_set_t mask;
        CPU_ZERO(&mask);
        bool isTwoBigCore = false;
        //4+4 4+3+1 6+2暂时不支持
        if ((Cpu4maxfreq == Cpu6maxfreq) && (Cpu4maxfreq == Cpu7maxfreq)){
            CPU_SET(7,&mask);
            CPU_SET(6,&mask);
            CPU_SET(5,&mask);
            CPU_SET(4,&mask);
        } else if (Cpu7maxfreq > Cpu6maxfreq){
            CPU_SET(4,&mask);
            CPU_SET(5,&mask);
            CPU_SET(6,&mask);
        } else {
            CPU_SET(6,&mask);
            CPU_SET(7,&mask);
            isTwoBigCore = true;
        }

        int ret = -2;
        if (!isTwoBigCore){
            ret = sched_setaffinity(tid, sizeof(cpu_set_t), &mask);
            if (ret < 0){
        //ret = -1 fail
               perr("set failed");
               perr("%s\n", strerror(errno));
            }
        } else {
            pwrn("6+2 not support!");
        }

        return ret != 0 ? ret : MIUI_BOOSTER_OK;
   }

   void MiuiBoosterUtils::release_request(int lock_handle){
#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)
        pwrn("perf_handle:%d", lock_handle);
        if (spMtkPerf != NULL && lock_handle > 0) {
            spMtkPerf->perfLockRelease(lock_handle, gettid());
            lock_handle = -1;
        }
#else
        if (qcopt_handle && perf_lock_rel && lock_handle > 0) {
            perf_lock_rel(lock_handle);
            lock_handle = -1;
        }
#endif
   }

   int MiuiBoosterUtils::cancelCpuHighFreq(int tid, int64_t timestamp){
        pdbg("cancelCpuHighFreq, tid:%d timestamp:%d",
               tid, TOINT(timestamp/1000000L));
        release_request(perf_handle);
        return 0;
   }
   int MiuiBoosterUtils::cancelGpuHighFreq(int tid, int64_t timestamp){
        pdbg("cancelGpuHighFreq, tid:%d timestamp:%d",
               tid, TOINT(timestamp/1000000L));
        release_request(perf_handle);
        return 0;
   }

   int MiuiBoosterUtils::cancelDDRHighFreq(int tid, int64_t timestamp){
        pdbg("cancelDDRHighFreq, tid:%d timestamp:%d",
               tid, TOINT(timestamp/1000000L));
        release_request(perf_handle);
        return 0;
    }

#if (defined MTK_PLATFORM_P) || (defined MTK_PLATFORM)

    char *MiuiBoosterUtils::get_mtk_cpu_model() {
        char *temp = NULL;
        char hardware[16];
        size_t sz = 0;

        char *cpumodel = (char *)malloc (sizeof (char) * 64);
       if (cpumodel == NULL){
            perr("cpumodel failed to allocate 64 bytes");
            return NULL;
        }

        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f == NULL) {
            perr("Can't open /proc/cpuinfo");
            free(cpumodel);
            return NULL;
        }
        bool isFindWare = false;
        do {
            ssize_t lsz = getline(&temp, &sz, f);
            if (lsz<0) break;
            if (strcspn(temp, "Hardware") == 0) {
                isFindWare = true;
                if (sscanf(temp, "%s\t: %[^\n]", hardware, cpumodel) == 2) {
                    break;
                }
            }
        } while (!feof (f));
        if (!isFindWare) {
          __system_property_get("ro.hardware", cpumodel);
        }
        free(temp);
        fclose(f);
        return cpumodel;
    }

    int MiuiBoosterUtils::get_soc_id_mtk() {
        char* mtk_cpu_model = get_mtk_cpu_model();
        int ret = 0;
        if (strcmp(mtk_cpu_model, "MT6765") == 0) {
            ret = 6765;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6765G") == 0) {
            ret = 6765;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6765V/WB") == 0) {
            ret = 6765;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6762V/CB") == 0) {
            ret = 6762;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6762G") == 0) {
            ret = 6762;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6762V/CN") == 0){
            ret = 67620;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6785") == 0){
            ret = 6785;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6785V/CC") == 0){
            ret = 6785;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6769V/CT") == 0){
            ret = 6769;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6769T") == 0){
            ret = 6769;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6769Z") == 0){
            ret = 6769;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6769V/CZ") == 0){
            ret = 6769;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6875") == 0){
            ret = 6875;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6785V/CD") == 0){
            ret = 6875;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6853T") == 0){
            ret = 6853;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6889Z/CZA") == 0){
            ret = 6889;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6891Z/CZA") == 0){
            ret = 6891;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6833V/ZA") == 0){
            ret = 6833;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6833") == 0){
            ret = 6833;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6833P") == 0){
            ret = 6833;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6853(ENG)") == 0){
            ret = 6853;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6853V/TNZA") == 0){
            ret = 6853;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "mt6983") == 0) {
            ret = 6983;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "mt6895") == 0) {
            ret = 6895;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6893Z/CZA") == 0){
            ret = 6893;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6893Z_D/CZA") == 0){
            ret = 6893;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6877V/TZA") == 0){
            ret = 6877;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "MT6781") == 0){
            ret = 6781;
            goto finish_get;
        }
        if (strcmp(mtk_cpu_model, "mt6789") == 0) {
            ret = 6789;
            goto finish_get;
        }
    finish_get:
        if (mtk_cpu_model != NULL) {
            delete mtk_cpu_model;
        }
        return ret;
    }

    void MiuiBoosterUtils::initCpuFreqLevelValue_MTK() {
        int soc_id = get_soc_id_mtk();
        switch(soc_id)
        {
            //MT6765
            case 6765:
                big_core_value = (int *)big_core_value_mt6765;
                little_core_value = (int *)little_core_value_mt6765;
                break;
            //MT6762V/CB
            case 6762:
                big_core_value = (int *)big_core_value_mt6762;
                little_core_value = (int *)little_core_value_mt6762;
                break;
            //MT6762V/CN
            case 67620:
                big_core_value = (int *)big_core_value_mt6762v;
                little_core_value = (int *)little_core_value_mt6762v;
                break;
            //MT6785
            case 6785:
                big_core_value = (int *)big_core_value_mt6785;
                little_core_value = (int *)little_core_value_mt6785;
                break;
            //MT6769
            case 6769:
                big_core_value = (int *)big_core_value_mt6769;
                little_core_value = (int *)little_core_value_mt6769;
                break;
            case 6875:
                big_core_value = (int *)big_core_value_mt6875;
                little_core_value = (int *)little_core_value_mt6875;
                break;
           case 6889:
                big_core_value = (int *)big_core_value_mt6889;
                little_core_value = (int *)little_core_value_mt6889;
                break;
           case 6853:
                big_core_value = (int *)big_core_value_mt6853;
                little_core_value = (int *)little_core_value_mt6853;
                break;
           case 6891:
                big_core_value = (int *)big_core_value_mt6891;
                little_core_value = (int *)little_core_value_mt6891;
                break;
           case 6833:
                big_core_value = (int *)big_core_value_mt6833;
                little_core_value = (int *)little_core_value_mt6833;
                break;
           case 6983:
                big_core_value = (int *)big_core_value_mt6983;
                little_core_value = (int *)little_core_value_mt6983;
                break;
           case 6895:
                big_core_value = (int *)big_core_value_mt6895;
                little_core_value = (int *)little_core_value_mt6895;
                break;
           case 6893:
                big_core_value = (int *)big_core_value_mt6893;
                little_core_value = (int *)little_core_value_mt6893;
                break;
           case 6877:
                big_core_value = (int *)big_core_value_mt6877;
                little_core_value = (int *)little_core_value_mt6877;
                break;
           case 6781:
                big_core_value = (int *)big_core_value_mt6781;
                little_core_value = (int *)little_core_value_mt6781;
                break;
           case 6789:
                big_core_value = (int *)big_core_value_mt6789;
                little_core_value = (int *)little_core_value_mt6789;
                break;
            default:
                perr("Unsupported mtk soc!!!");
                return;
        }
    }
    void MiuiBoosterUtils::initGpuFreqLevelValue_MTK(){
       pwrn("initGpuFreqLevelValue start!");
       if (spMtkPower == NULL) {
           perr("Failed to initGpuFreqLevelValue_MTK. spMtkPower is Null");
           return;
       }
      int count_gpu = spMtkPower->querySysInfo(5, 0);
       pwrn("count_gpu:%d",count_gpu);
       if(count_gpu <= 0){
           return;
       }
       switch(count_gpu)
        {
            case 1:
            case 2:
                gpu_freq_level[0] = 0;
                gpu_freq_level[1] = 0;
                gpu_freq_level[2] = 0;
                break;
            case 3:
            case 4:
                gpu_freq_level[0] = 0;
                gpu_freq_level[1] = 1;
                gpu_freq_level[2] = 2;
                break;
            default:
                gpu_freq_level[0] = 0;
                gpu_freq_level[1] = (1+(1+count_gpu)/2)/2-1;
                gpu_freq_level[2] = (1+count_gpu)/2-1;
                }
       pwrn("gpu_freq_level:%d,%d,%d", gpu_freq_level[0], gpu_freq_level[1], gpu_freq_level[2]);
       return;
   }

       void MiuiBoosterUtils::initDDRFreqLevelValue_MTK(){
       pwrn("initDDRFreqLevelValue start!");

       ddr_freq_level[0] = 0;
       ddr_freq_level[1] = 1;
       ddr_freq_level[2] = 2;
   }
#endif

    int MiuiBoosterUtils::requestUnifyCpuIOThreadCoreGpu(int scene, int64_t action, int cpulevel, int iolevel,
        vector<int> bindtids, int gpulevel, int timeoutms, int callertid, int64_t timestamp) {

        pdbg("requestUnifyCpuIOThreadCoreGpu, scene:%d action:%d cpulevel:%d iolevel:%d bindtids len:%d gpulevel: %d, timeoutms:%d callertid:%d timestamp:%d",
            scene, TOINT(action), cpulevel,iolevel,
            TOINT(bindtids.size()), gpulevel, timeoutms,
            callertid, TOINT(timestamp/1000000L));

        int ret = requestCpuHighFreq(scene, action, cpulevel, timeoutms, callertid, timestamp);

        int len = bindtids.size();
        for (int index = 0; index < len; index++) {
            pdbg("requestUnifyCpuIOThreadCore bindtids index=%d tid=%d", index, bindtids[index]);
            int rc = androidSetThreadPriority(bindtids[index], ANDROID_PRIORITY_FOREGROUND);
            if (rc != 0) {
                perr("androidSetThreadPriority rc = %d,  tid=%d ", rc, bindtids[index]);
                return MIUI_BOOSTER_FAILED_DEPENDENCY;
            }
        }

        if (ret > 0)
            return MIUI_BOOSTER_CPU_HIGH_FREQ | ret;
        else
            return MIUI_BOOSTER_FAILED_DEPENDENCY;
    }


    int MiuiBoosterUtils::cancelUnifyCpuIOThreadCoreGpu(int cancelcpu, int cancelio, int cancelthread, vector<int>bindtids,
        int cancelgpu, int callertid, int64_t timestamp) {

        pdbg("cancelUnifyCpuIOThreadCoreGpu, cancelcpu:%d cancelio:%d cancelthread:%d bindtids len:%d cancelgpu:%d, callertid:%d timestamp:%d",
            cancelcpu, cancelio, cancelthread,
            TOINT(bindtids.size()), cancelgpu, callertid,
            TOINT(timestamp/1000000L));

        release_request(perf_handle);

        int len = bindtids.size();
        for (int index = 0; index < len; index++){
            pdbg("cancelUnifyCpuIOThreadCore bindtids index=%d tid=%d", index, bindtids[index]);

            int rc = androidSetThreadPriority(bindtids[index], ANDROID_PRIORITY_NORMAL);
            if (rc != 0) {
                pdbg("androidSetThreadPriority rc = %d,  tid=%d ", rc, bindtids[index]);
                return MIUI_BOOSTER_FAILED_DEPENDENCY;
            }
        }
        return MIUI_BOOSTER_OK;
    }

    int MiuiBoosterUtils::requestThreadPriority(int scene, int64_t action, vector<int> bindtids,
        int timeoutms, int callertid, int64_t timestamp) {

        pdbg("requestThreadPriority, scene:%d action:%d bindtids len:%d timeoutms:%d callertid:%d timestamp:%d",
            scene, TOINT(action), TOINT(bindtids.size()),
            timeoutms, callertid, TOINT(timestamp/1000000L));

        int len = bindtids.size();
        for (int index = 0; index < len; index++){
            pdbg("requestThreadPriority bindtids index=%d tid=%d", index, bindtids[index]);

            int rc = androidSetThreadPriority(bindtids[index], ANDROID_PRIORITY_FOREGROUND);
            if (rc != 0) {
                pdbg("androidSetThreadPriority rc = %d,  tid=%d ", rc, bindtids[index]);
                return MIUI_BOOSTER_FAILED_DEPENDENCY;
            }
        }
        return MIUI_BOOSTER_OK;
    }


    int MiuiBoosterUtils::cancelThreadPriority(vector<int> bindtids, int callertid, int64_t timestamp) {
        pdbg("cancelThreadPriority, bindtids len:%d callertid:%d timestamp:%d",
             TOINT(bindtids.size()), callertid, TOINT(timestamp/1000000L));

        int len = bindtids.size();
        for (int index = 0; index < len; index++) {
            pdbg("cancelThreadPriority bindtids index=%d tid=%d", index, bindtids[index]);

            int rc = androidSetThreadPriority(bindtids[index], ANDROID_PRIORITY_NORMAL);
            if (rc != 0) {
                pdbg("androidSetThreadPriority rc = %d,  tid=%d ", rc, bindtids[index]);
                return MIUI_BOOSTER_FAILED_DEPENDENCY;
            }
        }
        return MIUI_BOOSTER_OK;
    }

    void MiuiBoosterUtils::closeQTHandle(){
        if (qcopt_handle) {
            if (dlclose(qcopt_handle))
                perr("Error occurred while closing qc-opt library.");
        }
    }

    int MiuiBoosterUtils::requestSetScene(String16& pkgName, int sceneId) {
        if (!sceneId_maxRefreshRate_map.count(sceneId)) {
            pwrn("MiuiBoosterUtils requestSetScene: %d not found", sceneId);
            return -1;
        }
        String8 package = (pkgName) ? String8(pkgName) : String8("");
        int maxRefreshRate = sceneId_maxRefreshRate_map.at(sceneId);
        pwrn("MiuiBoosterUtils requestSetScene: %s, scene: %d, maxRefreshRate: %d", package.string(), sceneId, maxRefreshRate);

        // binder call display service set max refresh rate
        const android::sp<android::IServiceManager> sm(android::defaultServiceManager());
        sp <IBinder> displayService = sm->getService(String16("display"));
        if (displayService == nullptr) {
            pwrn("MiuiBoosterUtils requestSetScene: display service not found");
            return -1;
        }
        Parcel data, reply;
        data.writeInterfaceToken(String16("android.view.android.hardware.display.IDisplayManager"));
        data.writeString16(pkgName);
        data.writeInt32(maxRefreshRate);
        int CODE_SET_SCENE_INFORMATION = 16777208;
        displayService->transact(CODE_SET_SCENE_INFORMATION, data, &reply);
        reply.readExceptionCode();

        return MIUI_BOOSTER_OK;
    }
}
