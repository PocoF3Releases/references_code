#include "PicHDRAlgo.h"
#define LOG_TAG "PicHDRAlgo"
using namespace std;
namespace  android {

PicHDRAlgo::PicHDRAlgo() {
    Init(kMiImplPrimary);
}

void PicHDRAlgo::Init(int disp_id)

{
    ALOGD("PicHDRAlgo init");
    char path[256];
    std::vector<struct rgb_entry> rgb;
    FILE *fp = NULL;
    int ret = 0;
    char panel_name[255];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;
    char p[4096];

    string file_name = disp_id == kMiImplPrimary ? PRIM_PANEL_INFO_PATH : SEC_PANEL_INFO_PATH;
    mParseXml[disp_id] = new MiParseXml();

    if (mParseXml[disp_id].get()) {
        mParseXml[disp_id]->getPanelName(file_name.c_str(),panel_name);
    } else {
        ALOGE("Can not access parse xml");
        return;
    }
    //parse 3d lut info
    sprintf(path, "/odm/etc/disp%d/%s/clstc/PicHDRLchIn.txt",disp_id, panel_name);
    if (access(path, F_OK|R_OK) < 0) {
        ALOGE("can not access file %s", path);
        return;
    }
    ALOGD("reading %s", path);
    if (access(path, F_OK|R_OK) < 0) {
        ALOGE("can not access file %s", path);
        return;
    }
    ret = readLutData(path);
    if (ret < 0) {
        ALOGE("Failed to parse calib lut srgb file");
        return;
    }
    string key("Rp");
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Rp.push_back(atof(tokens[i]));
            ALOGD("Rp[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Gp";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Gp.push_back(atof(tokens[i]));
            ALOGD("Gp[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Bp";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Bp.push_back(atof(tokens[i]));
            ALOGD("Bp[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Wp";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Wp.push_back(atof(tokens[i]));
            ALOGD("Wp[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "WYp";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        WYp = atof(p);
        ALOGD("WYp: %f",atof(p));
    }

    key = "Rc";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Rc.push_back(atof(tokens[i]));
            ALOGD("Rc[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Gc";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Gc.push_back(atof(tokens[i]));
            ALOGD("Gc[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Bc";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Bc.push_back(atof(tokens[i]));
            ALOGD("Bc[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Wc";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Wc.push_back(atof(tokens[i]));
            ALOGD("Wc[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "WYc";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        WYc = atof(p);
        ALOGD("WYc: %f",atof(p));
    }

    key = "LthLeft";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        Lth_left = atof(p);
        ALOGD("Lth_left: %f",atof(p));
    }
    key = "LthRight";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        Lth_right = atof(p);
        ALOGD("Lth_right: %f",atof(p));
    }
    key = "LrStartMinDay";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        Lr_start_min_day = atof(p);
        ALOGD("Lr_start_min_day: %f",atof(p));
    }
    key = "LrStartMaxDay";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        Lr_start_max_day = atof(p);
        ALOGD("Lr_start_max_day: %f",atof(p));
    }
    key = "LrStartMinNight";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        Lr_start_min_day = atof(p);
        ALOGD("Lr_start_min_night: %f",atof(p));
    }
    key = "LrStartMaxNight";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        Lr_start_max_day = atof(p);
        ALOGD("Lr_start_max_night: %f",atof(p));
    }
    key = "AdrcLeft";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        adrc_left = atof(p);
        ALOGD("adrc_left: %f",atof(p));
    }
    key = "AdrcMid";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        adrc_mid = atof(p);
        ALOGD("adrc_mid: %f",atof(p));
    }
    key = "AdrcRight";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        adrc_right = atof(p);
        ALOGD("adrc_right: %f",atof(p));
    }
    key = "LuxIndexThLeft";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        lux_index_th_left = atof(p);
        ALOGD("lux_index_th_left: %f",atof(p));
    }
    key = "LuxIndexThRight";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        lux_index_th_right = atof(p);
        ALOGD("lux_index_th_right: %f",atof(p));
    }
    key = "SlopLeft";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        slope_left = atof(p);
        ALOGD("slope_left: %f",atof(p));
    }
    key = "SlopRight";
    ret = mParseXml[disp_id]->parseXml(disp_id, key, p, sizeof(p));
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        slope_right = atof(p);
        ALOGD("slope_right: %f",atof(p));
    }
    key = "HTuningRange";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            H_tuning_range.push_back(atoi(tokens[i]));
            ALOGD("H_tuning_range[%d]:[%d %d]", i, H_tuning_range[i]);
        }
    }
    key = "Cp";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Cp.push_back(atof(tokens[i]));
            ALOGD("Cp[%d]:%f",i, atof(tokens[i]));
        }
    }
    key = "Lp";
    ret = mParseXml[disp_id]->parseXml(disp_id, key,p,tokens,&count);
    if(ret != 0) {
        ALOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            Lp.push_back(atof(tokens[i]));
            ALOGD("Lp[%d]:%f",i, atof(tokens[i]));
        }
    }
}

int PicHDRAlgo::readLutData(char* path)
{
    FILE *fp = NULL;
    int cnt = 0;
    int ret = 0;
    int entries = pow(17,3);
    int temp[3];
    fp = fopen(path, "r");
    if (fp == NULL) {
        ALOGE("open file error %s", path);
        return -1;
    }
    while (fscanf(fp, "%d %d %d", &temp[0], &temp[1], &temp[2]) != EOF) {
        LCH_in[cnt][0] = static_cast<double>(temp[0]);
        LCH_in[cnt][1] = static_cast<double>(temp[1]);
        LCH_in[cnt][2] = static_cast<double>(temp[2]);
        cnt++;
        if (cnt > entries) {
            ALOGE("Invalid lut file, need %d, receive %d", entries, cnt);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    if (cnt != entries) {
         ALOGE("Invalid Lut file %s, line num %d, need %d", path, cnt, entries);
         return -1;
    }
    return ret;
}

void PicHDRAlgo::H2C_init() {
	for (int k = 0; k < 360; k++) {
		H2C[k] = 1;
	}
}

void PicHDRAlgo::H2L_init() {
	for (int k = 0; k < 360; k++) {
		H2L[k] = 1;
	}
}

double* PicHDRAlgo::scene_judge(double lux_index) {
	double* Lr_start_range = new double[2]();
	if (lux_index < lux_index_th_left) {
		Lr_start_range[0] = Lr_start_min_day;
		Lr_start_range[1] = Lr_start_max_day;
		return Lr_start_range;
	}
	else if (lux_index > lux_index_th_right) {
		Lr_start_range[0] = Lr_start_min_night;
		Lr_start_range[1] = Lr_start_max_night;
		return Lr_start_range;
	}
	else {
		Lr_start_range[0] = (Lr_start_min_day - Lr_start_min_night) * (lux_index - lux_index_th_left) / (lux_index_th_left - lux_index_th_right) + Lr_start_min_day;
		Lr_start_range[1] = (Lr_start_max_day - Lr_start_max_night) * (lux_index - lux_index_th_left) / (lux_index_th_left - lux_index_th_right) + Lr_start_max_day;
		return Lr_start_range;
	}
}

double PicHDRAlgo::Slope_cal(double lux_index) {
	double slope_val;
	if (lux_index < lux_index_th_left) {
		slope_val = slope_left;
	}
	else if (lux_index > lux_index_th_right) {
		slope_val = slope_right;
	}
	else {
		slope_val = (lux_index - lux_index_th_left) * (slope_right - slope_left) / (lux_index_th_right - lux_index_th_left) + slope_left;
	}
	return slope_val;
}

void PicHDRAlgo::C_tuning(int * H_range, double Cp, double Lp, int trans) {
	for (int k = H_range[0]; k < H_range[1] + 1; k++) {
			H2C[k] = Cp;
            H2L[k] = Lp;
	}
	for (int k = H_range[0] - trans; k < H_range[0]; k++) {
		if (k < 0) {
			H2C[k + 360] = (H2C[H_range[0]] - H2C[(H_range[0] - trans) % 360]) * (k - (H_range[0] - trans) % 360) / trans + H2C[(H_range[0] - trans) % 360];
			H2L[k + 360] = (H2L[H_range[0]] - H2L[(H_range[0] - trans) % 360]) * (k - (H_range[0] - trans) % 360) / trans + H2L[(H_range[0] - trans) % 360];
		}
		else {
			H2C[k] = (H2C[H_range[0]] - H2C[(H_range[0] - trans) % 360]) * (k - H_range[0]) / trans + H2C[H_range[0]];
			H2L[k] = (H2L[H_range[0]] - H2L[(H_range[0] - trans) % 360]) * (k - H_range[0]) / trans + H2L[H_range[0]];
		}
	}
	for (int k = H_range[1] + 1; k < H_range[1] + trans + 1; k++) {
		if (k > 359) {
			H2C[k - 360] = (H2C[(H_range[1] + trans) % 360] - H2C[H_range[1]]) * (k - (H_range[1] + trans) % 360) / trans + H2C[(H_range[1] + trans) % 360];
			H2L[k - 360] = (H2L[(H_range[1] + trans) % 360] - H2L[H_range[1]]) * (k - (H_range[1] + trans) % 360) / trans + H2L[(H_range[1] + trans) % 360];
		}
		else {
			H2C[k] = (H2C[(H_range[1] + trans) % 360] - H2C[H_range[1]]) * (k - H_range[1]) / trans + H2C[H_range[1]];
			H2L[k] = (H2L[(H_range[1] + trans) % 360] - H2L[H_range[1]]) * (k - H_range[1]) / trans + H2L[H_range[1]];
		}
	}
};

double PicHDRAlgo::Lth_cal(double adrc) {
	double Lth = 0.0;
	if (adrc <= adrc_mid) {
		Lth = (Lth_right - Lth_left) * (adrc - adrc_left) / (adrc_mid - adrc_left) + Lth_left;
	}
	else {
		Lth = Lth_right;
	}
	return Lth;
};

double* PicHDRAlgo::Lr_bound_cal(double adrc, double* Lr_start_range, double slope) {
	double* Lr_bound = new double[2]();
	double Lr_start_min = Lr_start_range[0];
	double Lr_start_max = Lr_start_range[1];
	if (adrc < adrc_mid) {
		Lr_bound[0] = (Lr_start_max - Lr_start_min) * (adrc - adrc_left) / (adrc_mid - adrc_left) + Lr_start_min;
	}
	else {
		Lr_bound[0] = Lr_start_max;
	}
	Lr_bound[1] = Lr_bound[0] * slope + 1 - slope;
	return Lr_bound;
};

double** PicHDRAlgo::get_inv(double** mat) {
	double adj[3][3] = {};
	double det_mat = mat[0][0] * mat[1][1] * mat[2][2] + mat[0][1] * mat[1][2] * mat[2][0] + mat[0][2] * mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1] * mat[0][2] - mat[0][0] * mat[2][1] * mat[1][2] - mat[1][0] * mat[0][1] * mat[2][2];
	for (int r = 0; r < 3; r++) {
		for (int c = 0; c < 3; c++) {
			double det[4] = {};
			int k = 0;
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					if (i != r && j != c) {
						det[k] = mat[i][j];
						k++;
					}
				}
			}
			double det_val = det[0] * det[3] - det[1] * det[2];
			adj[c][r] = pow(-1, r + c) * det_val;
		}
	}
	double** inv = new double* [3];
	for (int x = 0; x < 3; x++) {
		inv[x] = new double[3];
		for (int y = 0; y < 3; y++) {
			inv[x][y] = adj[x][y] / det_mat;
		}
	}
	return inv;
};

double** PicHDRAlgo::RGB2XYZ_gen(double R[2], double G[2], double B[2], double W[2]) {
	double** mat_XYZ = new double*[3];
	for (int k = 0; k < 3; k++) {
		mat_XYZ[k] = new double[3];
		if (k == 0) {
			mat_XYZ[k][0] = R[0];
			mat_XYZ[k][1] = G[0];
			mat_XYZ[k][2] = B[0];
		}
		else if (k == 1) {
			mat_XYZ[k][0] = R[1];
			mat_XYZ[k][1] = G[1];
			mat_XYZ[k][2] = B[1];
		}
		else {
			mat_XYZ[k][0] = 1 - R[0] - R[1];
			mat_XYZ[k][1] = 1 - G[0] - G[1];
			mat_XYZ[k][2] = 1 - B[0] - B[1];
		}
	}
	double w[3] = { W[0] / W[1],1.0,(1 - W[0] - W[1]) / W[1] };
	double w1[3][3] = { {0,0,0},{0,0,0} ,{0,0,0} };
	double** mat_XYZ_I = get_inv(mat_XYZ);
	for (int k = 0; k < 3; k++) {
		w1[k][k] = mat_XYZ_I[k][0] * w[0] + mat_XYZ_I[k][1] * w[1] + mat_XYZ_I[k][2] * w[2];
	}
	double** RGB2XYZ = new double* [3];
	for (int i = 0; i < 3; i++) {
		RGB2XYZ[i] = new double[3];
		for (int j = 0; j < 3; j++) {
			RGB2XYZ[i][j] = mat_XYZ[i][j] * w1[j][j];
		}
	}
	return RGB2XYZ;
};

double* PicHDRAlgo::RGB2XYZ_cal(int R, int G, int B, double** RGB2XYZ, double WY) {
	double Rd = double(R) / 1023;
	double Gd = double(G) / 1023;
	double Bd = double(B) / 1023;
	double* XYZ = new double[3]();
	XYZ[0] = (Rd * RGB2XYZ[0][0] + Gd * RGB2XYZ[0][1] + Bd * RGB2XYZ[0][2]) * WY;
	XYZ[1] = (Rd * RGB2XYZ[1][0] + Gd * RGB2XYZ[1][1] + Bd * RGB2XYZ[1][2]) * WY;
	XYZ[2] = (Rd * RGB2XYZ[2][0] + Gd * RGB2XYZ[2][1] + Bd * RGB2XYZ[2][2]) * WY;
	return XYZ;
};

int* PicHDRAlgo::XYZ2RGB_cal(double XYZ[3], double** XYZ2RGB, double WY) {
	double X = XYZ[0] / WY;
	double Y = XYZ[1] / WY;
	double Z = XYZ[2] / WY;
	int* RGB = new int[3]();
	double R = (X * XYZ2RGB[0][0] + Y * XYZ2RGB[0][1] + Z * XYZ2RGB[0][2]) * 1023;
	double G = (X * XYZ2RGB[1][0] + Y * XYZ2RGB[1][1] + Z * XYZ2RGB[1][2]) * 1023;
	double B = (X * XYZ2RGB[2][0] + Y * XYZ2RGB[2][1] + Z * XYZ2RGB[2][2]) * 1023;
	double max_val = max(RGB[0], max(RGB[1], RGB[2]));
	if (max_val > 1023) {
		RGB[0] = int(R / max_val * 1023);
		RGB[1] = int(G / max_val * 1023);
		RGB[2] = int(B / max_val * 1023);
	}
	else {
		RGB[0] = int(R);
		RGB[1] = int(G);
		RGB[2] = int(B);
	}
	return RGB;
};

double PicHDRAlgo::f(double x) {
	double fx = 0.0;
	if (x > pow(6.0 / 29, 3)) {
		fx = pow(x, 1.0 / 3);
	}
	else {
		fx = (841.0 / 108) * x + 4.0 / 29;
	}
	return fx;
};

double* PicHDRAlgo::xylv2XYZ_cal(double Wxy[2], double WY) {
	double* XYZ = new double[3]();
	XYZ[0] = Wxy[0] * WY / Wxy[1];
	XYZ[1] = WY;
	XYZ[2] = WY / Wxy[1] - WY - XYZ[0];
	return XYZ;
}

double* PicHDRAlgo::XYZ2Lab_cal(double XYZ[3], double XYZW[3]) {
	double* Lab = new double[3]();
	Lab[0] = 116 * f(XYZ[1] / XYZW[1]) - 16;
	Lab[1] = 500 * (f(XYZ[0] / XYZW[0]) - f(XYZ[1] / XYZW[1]));
	Lab[2] = 200 * (f(XYZ[1] / XYZW[1]) - f(XYZ[2] / XYZW[2]));
	return Lab;
}

double* PicHDRAlgo::Lab2LCH_cal(double Lab[3]) {
	double* LCH = new double[3]();
	LCH[0] = Lab[0];
	LCH[1] = sqrt(pow(Lab[1], 2) + pow(Lab[2], 2));
	LCH[2] = fmod((atan2(Lab[2], Lab[1]) * 180 / PI + 360), 360.0);
	return LCH;
}

double* PicHDRAlgo::LCH2Lab_cal(double LCH[3]) {
	double* Lab = new double[3]();
	Lab[0] = LCH[0];
	Lab[1] = LCH[1] * cos(LCH[2] * PI / 180);
	Lab[2] = LCH[1] * sin(LCH[2] * PI / 180);
	return Lab;
}

double* PicHDRAlgo::Lab2XYZ_cal(double Lab[3], double XYZW[3]) {
	double fy = (Lab[0] + 16) / 116;
	double fx = fy + Lab[1] / 500;
	double fz = fy - Lab[2] / 200;
	double* XYZ = new double[3]();
	if (fy > 6.0 / 29) {
		XYZ[1] = pow(fy, 3) * XYZW[1];
	}
	else {
		XYZ[1] = (fy - 16.0 / 116) * 3 * pow(6.0 / 29, 2) * XYZW[1];
	}
	if (fx > 6.0 / 29) {
		XYZ[0] = pow(fx, 3) * XYZW[0];
	}
	else {
		XYZ[0] = (fx - 16.0 / 116) * 3 * pow(6.0 / 29, 2) * XYZW[0];
	}
	if (fz > 6.0 / 29) {
		XYZ[2] = pow(fz, 3) * XYZW[2];
	}
	else {
		XYZ[2] = (fz - 16.0 / 116) * 3 * pow(6.0 / 29, 2) * XYZW[2];
	}
	return XYZ;
}

int PicHDRAlgo::calcHDRLut(struct rgb_entry *lutInfo) {
	//Initialize Parameters
	double** RGB2XYZ_in = RGB2XYZ_gen(Rp.data(), Gp.data(), Bp.data(), Wp.data());
	double** XYZ2RGB_in = get_inv(RGB2XYZ_in);
	double* XYZW_in = xylv2XYZ_cal(Wp.data(), WYp);

	double** RGB2XYZ_out = RGB2XYZ_gen(Rc.data(), Gc.data(), Bc.data(), Wc.data());
	double** XYZ2RGB_out = get_inv(RGB2XYZ_out);
	double* XYZW_out = xylv2XYZ_cal(Wc.data(), WYc);

	double Lth = Lth_cal(mAdrc);
	double* Lr_start_range = scene_judge(mLuxIndex);
	double slope = Slope_cal(mLuxIndex);
	double* Lr_bound = Lr_bound_cal(mAdrc, Lr_start_range, slope);
	double Lr_left = Lr_bound[0];
	double Lr_right = Lr_bound[1];

	H2C_init();
    H2L_init();

     int len = H_tuning_range.size() / 2;
        for (int idx = 0; idx < len; idx++) {
            C_tuning(&H_tuning_range[idx * 2], Cp[idx], Lp[idx], 2);
        }

    int lut_vals[4913][3] = {};
	int index = 0;//Temp LUT saving

	//Go through LUT
    string line;
	for (int i = 0; i < 4913; i++) {
        //add start
        // double* XYZ_in = RGB2XYZ_cal(lutInfo[i].in.r, lutInfo[i].in.g, lutInfo[i].in.b, RGB2XYZ_in, WYp);
		// double* Lab_in = XYZ2Lab_cal(XYZ_in, XYZW_in);
		// double* LCH_in = Lab2LCH_cal(Lab_in);
        //add end
		double* XYZ_out = RGB2XYZ_cal(lutInfo[i].out.r, lutInfo[i].out.g, lutInfo[i].out.b, RGB2XYZ_out, WYc);
		double* Lab_out = XYZ2Lab_cal(XYZ_out, XYZW_out);
		double* LCH_out = Lab2LCH_cal(Lab_out);
       
        LCH_out[1] *= H2C[int(LCH_in[i][2])];

	    double Lr = 0.0;
		if (LCH_in[i][0] < Lth) {
			Lr = LCH_in[i][0] * (Lr_right - Lr_left) / Lth + Lr_left;
		}
		else if (LCH_in[i][0] < 95) {
			Lr = (1 - Lr_right) * (LCH_in[i][0] - 95) / (95 - Lth) + 1;
		}
		else{
			Lr = 1;
		}

		double* Lab_final = LCH2Lab_cal(LCH_out);
		Lab_final[0] *= Lr;
        /*Lab_final[0] *= (1 - Lr) * (1 - H2L[int(LCH_in[i][2])]) + Lr;*/
        //ALOGD("calcHDRLut  ,Lab_final[0] = %f ,Lr = %f, LCH_in[i][2] = %d, int(LCH_in[i][2])=%f, H2L[int(LCH_in[i][2])] = %f,H2C[int(LCH_in[2])] = %f"
        //, (1 - Lr) * (1 - H2L[int(LCH_in[i][2])]) + Lr, Lr, int(LCH_in[i][2]), H2L[int(LCH_in[i][2])], H2C[int(LCH_in[i][2])]);
		double* XYZ_final = Lab2XYZ_cal(Lab_final, XYZW_out);
		int* RGB_final = XYZ2RGB_cal(XYZ_final, XYZ2RGB_out, WYc);

		RGB_final[0] = max(RGB_final[0], 0);
		RGB_final[1] = max(RGB_final[1], 0);
		RGB_final[2] = max(RGB_final[2], 0);
		lut_vals[index][0] = RGB_final[0];
		lut_vals[index][1] = RGB_final[1];
		lut_vals[index][2] = RGB_final[2];
		index++;
	}

	for (int i = 0; i < 4913; i++) {
                lutInfo[i].out.r = lut_vals[i][0];
                lutInfo[i].out.g = lut_vals[i][1];
                lutInfo[i].out.b = lut_vals[i][2];
	}
	return 0;
}

void PicHDRAlgo::SetExif(double Adrc, double LuxIndex)
{
    mAdrc = Adrc/100;
    mLuxIndex = LuxIndex;
}
} //namespace clstc
