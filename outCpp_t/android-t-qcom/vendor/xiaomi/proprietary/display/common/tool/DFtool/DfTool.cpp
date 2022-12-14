#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <wait.h>
#include <unistd.h>
#include <utils/Log.h>
#include <errno.h>
#include <getopt.h>
#include <cutils/properties.h>
#include <string>
#include <map>
#include "DfTool.h"
#include "mi_stc_service.h"
#include "mi_clstc_service.h"

using namespace android;
using snapdragoncolor::IMiStcService;
using clstc::IMiClstcService;
using android::Parcel;
using android::sp;
using android::IServiceManager;

int dispatchClstc(uint32_t command, const Parcel* inParcel, Parcel* outParcel)
{
    sp<IMiClstcService> imiclstc;
    imiclstc = interface_cast<IMiClstcService>(defaultServiceManager()->getService(String16("miclstcservice")));
    if (imiclstc.get())
        imiclstc->dispatch(command, inParcel, outParcel);
    else {
        printf("Failed to acquire miclstcservice\n");
        return -1;
    }
    return 0;
}

int dispatch(uint32_t command, const Parcel* inParcel, Parcel* outParcel)
{
    sp<IMiStcService> imistcservice;
    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    if (imistcservice.get())
        imistcservice->dispatch(command, inParcel, outParcel);
    else {
        printf("Failed to acquire display.mistcservice\n");
        return -1;
    }
    return 0;
}

int main(int argc, char** argv){
        char* cmd;
        int gamma;
        android::Parcel data;
        android::Parcel outData;
        if (!strcmp("-l", argv[1])) {
        cmd = (char*)malloc(100*sizeof(char));
        memset(cmd, 0, 100*sizeof(char));
        sprintf(cmd, "service call DisplayFeatureControl 99 i32 0 i32 9 i32 0 i32 %d", atoi(argv[2]));
        system("setprop sys.displayfeature.entry.enable true");
        system("setprop vendor.displayfeature.entry.enable true");
        system(cmd);
        memset(cmd, 0, 100*sizeof(char));
        sprintf(cmd, "vndservice call DisplayFeatureControl 99 i32 0 i32 9 i32 0 i32 %d", atoi(argv[2]));
        system(cmd);
        free(cmd);
        } else if (!strcmp("-d", argv[1])) {
            cmd = (char*)malloc(100*sizeof(char));
            memset(cmd, 0, 100*sizeof(char));
            sprintf(cmd, "vndservice call DisplayFeatureControl 100 i32 0 i32 30 i32 0 i32 %d", atoi(argv[2]));
            system(cmd);
            free(cmd);
        } else if (!strcmp("-debug", argv[1])) {
            if (argc < 4 || argc > 6) {
                printf("Invalid Parameters!\n");
                return 0;
            }
            cmd = (char*) malloc (100 * sizeof(char));
            memset(cmd, 0, 100 * sizeof(char));
            if (argc == 4) {
                sprintf(cmd, "vndservice call DisplayFeatureControl 100 i32 0 i32 %d i32 %d i32 255", atoi(argv[2]), atoi(argv[3]));
            } else if (argc == 5) {
                sprintf(cmd, "vndservice call DisplayFeatureControl 100 i32 0 i32 %d i32 %d i32 %d", atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
            } else {
                sprintf(cmd, "vndservice call DisplayFeatureControl 100 i32 %d i32 %d i32 %d i32 %d", atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
            }
            system(cmd);
            free(cmd);
        } else if (!strcmp("-p", argv[1])) {
            if (argc != 3 && argc != 4 && argc != 5 && argc != 6 && argc != 11 && argc != 12 &&
                argc != 18 && argc != 19) {
                printf("Invalid input");
                return -1;
            }
            int base;
            if (argc == 3 || argc == 4) {
                data.writeInt32(atoi(argv[2])); //Enable
                if (argc == 3)
                    data.writeInt32(0); //DisplayId
                else
                    data.writeInt32(atoi(argv[3])); //DisplayId
            } else if (argc == 5 || argc == 6) {
                data.writeInt32(1); //Enable
                if (argc == 5) {
                    data.writeInt32(0); //DisplayId
                    base = 2;
                } else {
                    data.writeInt32(atoi(argv[2])); //DisplayId
                    base = 3;
                }
                for (int i = 0; i < 15; i++) {
                    if (i%5 == 0) {
                        data.writeDouble(atof(argv[base + i/5]));
                    } else {
                        data.writeDouble(0);
                    }
                }
                data.writeDouble(1);
            } else if (argc == 11 || argc == 12) {
                data.writeInt32(1); //Enable
                if (argc == 11) {
                    data.writeInt32(0); //DisplayId
                    base = 2;
                } else {
                    data.writeInt32(atoi(argv[2])); //DisplayId
                    base = 3;
                }
                for (int i = 0; i < 15; i++) {
                    if (i < 11 && (i % 4 != 3)) {
                        data.writeDouble(atof(argv[base + i - i/4]));
                    } else {
                        data.writeDouble(0);
                    }
                }
                data.writeDouble(1);
            } else if (argc == 18 || argc == 19) {
                data.writeInt32(1); //Enable
                if (argc == 18) {
                    data.writeInt32(0); //DisplayId
                    base = 2;
                } else {
                    data.writeInt32(atoi(argv[2])); //DisplayId
                    base = 3;
                }
                for (int i = 0; i < 16; i++) {
                    data.writeDouble(atof(argv[base + i]));
                }
            }
            dispatch(IMiStcService::SET_PCC, &data, NULL);
        } else if (!strcmp("-m", argv[1])) {
            if (argc != 3) {
                 printf("Invalid input\n");
                 return -1;
            }
            cmd = (char*)malloc(100*sizeof(char));
            memset(cmd, 0, 100*sizeof(char));
            sprintf(cmd, "service call SurfaceFlinger 1023 i32 %d", atoi(argv[2]));
        } else if (!strcmp("-g", argv[1])) {
            if (argc != 7 && argc != 8) {
                printf("Invalid input\n");
                return -1;
            }
            int type = atoi(argv[5]);
            data.writeInt32(atoi(argv[2])); //Enable
            data.writeInt32(atoi(argv[3])); //DisplayId
            data.writeInt32(atoi(argv[4])); //dimming

            gamma = (int)(atof(argv[6]) * 100);
            data.writeInt32(gamma);
            if (argc == 8)
                data.writeInt32(atoi(argv[7]));

            if (type == 0) {
                dispatch(IMiStcService::SET_IGC, &data, NULL);
            } else if (type == 1) {
                dispatch(IMiStcService::SET_GC, &data, NULL);
            }
        } else if (!strcmp("-hsv", argv[1])) {
            if (argc!= 10) {
                printf("Invalid input\n");
                return -1;
            }
            data.writeInt32(atoi(argv[2])); //Enable
            data.writeInt32(atoi(argv[3])); //DisplayId
            data.writeInt32(atoi(argv[4])); //Dimming
            data.writeInt32(atoi(argv[5])); //Hue
            data.writeFloat(atof(argv[6])); //Saturation
            data.writeFloat(atof(argv[7])); //Value
            data.writeFloat(atof(argv[8])); //Contrast
            data.writeFloat(atof(argv[9])); //Sat_thresh
            dispatch(IMiStcService::SET_PA, &data, NULL);
        } else if (!strcmp("tap", argv[1]) || !strcmp("swipe", argv[1])) {
            touchInput(argc, &argv[0]);
        } else if (!strcmp("-lut", argv[1])) {
            if (argc != 7) {
                printf("Invalid input\n");
                return -1;
            }
            data.writeInt32(atoi(argv[2])); //Enable
            data.writeInt32(atoi(argv[3])); //DisplayId
            data.writeInt32(atoi(argv[4])); //Dimming
            data.writeInt32(atoi(argv[5])); //re-load lut files
            data.writeInt32(atoi(argv[6])); //lut file index
            dispatch(IMiStcService::SET_LUT, &data, NULL);
        } else if (!strcmp("-c", argv[1])) {
            if (!strcmp("-p", argv[2])) {
                if (argc != 14 && argc != 5) {
                    printf("Invalid input\n");
                    return -1;
                }
                data.writeInt32(atoi(argv[3])); // layer id
                int enable = atoi(argv[4]);
                if ((enable && argc != 14) || (!enable && argc != 5)) {
                    printf("Invalid input\n");
                    return -1;
                }
                data.writeInt32(enable);
                if (enable) {
                    for (int i = 0; i < 9; i++)
                        data.writeDouble(atof(argv[i + 5]));
                }
                dispatchClstc(IMiClstcService::SET_PCC, &data, NULL);
            } else if (!strcmp("-lut", argv[2])) {
                if (argc != 6 && argc != 5) {
                    printf("Invalid input\n");
                    return -1;
                }
                data.writeInt32(atoi(argv[3])); // layer id
                int enable = atoi(argv[4]);
                if ((enable && argc != 6) || (!enable && argc != 5)) {
                    printf("Invalid input\n");
                    return -1;
                }
                data.writeInt32(enable);
                if (enable) {
                    data.writeUint32(atoi(argv[5]));
                }
                dispatchClstc(IMiClstcService::SET_LUT, &data, NULL);
            } else if (!strcmp("-g", argv[2])) {
                if (argc != 7 && argc != 6) {
                    printf("Invalid input\n");
                    return -1;
                }
                data.writeInt32(atoi(argv[3])); // layer id
                int enable = atoi(argv[4]);
                if ((enable && argc != 7) || (!enable && argc != 6)) {
                    printf("Invalid input\n");
                    return -1;
                }
                data.writeInt32(enable);
                int type = atoi(argv[5]); // type 0 IGC, type 1 GC
                if (enable) {
                    data.writeUint32(atoi(argv[6])); // gamma file id
                }
                if (type == 0) {
                    dispatchClstc(IMiClstcService::SET_IGC, &data, NULL);
                } else if (type == 1) {
                    dispatchClstc(IMiClstcService::SET_GC, &data, NULL);
                } else {
                    printf("Invalid gamma type, IGC:0, GC:1\n");
                }
            }
        } else if (!strcmp("--show-layer", argv[1])) {
            dispatchClstc(IMiClstcService::SHOW_LAYER_INFO, NULL, &outData);
            int num = outData.readInt32();
            for (int i = 0; i < num; i++)
                printf("ID[%d]:%s, type:%s\n", outData.readInt32(), outData.readCString(), outData.readCString());
        } else if (!strcmp("-h", argv[1])) {
            printf("-l  Enable, DisplayId, Switch displayfeature log print level. 0 will print all logs.\n");
            printf("CLSTC cmds:\n");
            printf("--show-layer : show the ID and name of all layers managed by clstc\n");
            printf("-c -p layerId, enable, (3x3 pcc coeffs) : set clstc pcc for a layer.\n");
            printf("-c -lut layerId, enable, lutId: set clstc lut from file ClstcLut(lutId).txt for a layer.\n");
            printf("-c -g layerId, enable, type, gammaId: set clstc gamma from file for a layer. IGC: 0, GC: 1\n");
            printf("\n");
            printf("STC cmds:\n");
            printf("-p  (Enable), (DisplayId), 3x1, 3x3 or 4x4 matrix : Set Pcc for specified display\n");
            printf("-m  Enable, DisplayId, Set Mode By RenderIntent Id.\n");
            printf("-g  Enable, DisplayId, dimming(GC only), Set Gamma, 0 for IGC and 1 for GC, with target gamma followed.\n");
            printf("-d n Dump DSPP outputs, n means dump frame numbers\n");
            printf("-hsv Enable, DisplayId, dimming, Hue(-180~180), Saturation(-0.5~0.5), Value(-255.0~255.0), Contrast(-1~1), sat_thresh(0~1)\n");
            printf("-dither  Enable, DisplayId, strength.\n");
            printf("-lut Enable, DisplayId, dimming, re-load lut files congit, lut file index in vendor/etc/\n");
            printf("LUT file table:\n");
            printf("index: 0,  srgb_d65\n");
            printf("index: 1,  dcip3_d65\n");
            printf("index: 2,  srgb_d75\n");
            printf("index: 3,  dcip3_d75\n");
            printf("index: 4,  vivid\n");
            printf("index: 5,  hdr\n");
            printf("index: 6,  retro\n");
            printf("index: 7,  colorless\n");
            printf("index: 8,  warm\n");
            printf("index: 9,  cool\n");
            printf("index: 10, lime\n");
            printf("index: 11, jaze\n");
            printf("index: 12, fresh\n");
            printf("index: 13, pink\n");
            printf("index: 14, machine\n");
            printf("index: 15, nature_srgb\n");
            printf("index: 16, nature_dcip3\n");
            printf("-debug  (displayId), mode, value, (cookie), Conrtol the switch of DisplayFeature, displayId and cookie default value 0, can be omitted\n");
            touchInput(1, &argv[1]);
        } else if (!strcmp("-dither", argv[1])) {
            if (argc!= 5) {
                printf("Invalid input\n");
                return -1;
            }
            data.writeInt32(atoi(argv[2])); //Enable
            data.writeInt32(atoi(argv[3])); //DisplayId
            data.writeInt32(atoi(argv[4])); //Strength
            dispatch(IMiStcService::SET_IGC_DITHER, &data, NULL);
        } else {
            printf("Invalid Parameters!\n");
        }
        return 0;
}
