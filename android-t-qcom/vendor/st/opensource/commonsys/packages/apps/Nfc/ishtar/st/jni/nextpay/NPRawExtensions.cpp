#include "NPRawExtensions.h"
#include "json.h"
#include "alog.h"
#include "rawProbe.h"

void init(Json::Value *nonce, denonce *d);

void init(Json::Value *tag, mftag *t);

void release(denonce *d);

void structToValue(denonce *d, Json::Value *nonce);

void structToValue(ntprobe *probe_p, Json::Value *probe);


string rawProbe(string data) {
    LOGD("rawProbe: data = %s", data.c_str());
    denonce nonce_t = {};
    mftag tag_t = {};
    ntprobe ntprobe_t = {};
    Json::Reader reader;
    Json::Value root;

    Json::Value result;
    int resultCode = -1;

    if (reader.parse(data, root)) {
        try {
            int version = root["version"].asInt();
            int modeValue = root["mode"].asInt();
            char mode = 'd';
            if (modeValue == 1) {
                mode = 'r';
            }
            int probeSectorIndex = root["probeSectorIndex"].asInt();
            bool dumpKeyA = root["dumpKeyA"].asBool();
            LOGV("NPRawExtensions param version:%d, mode:%c, sectorIndex:%d", version, mode, probeSectorIndex);
            Json::Value tag = root["tag"];
            init(&tag, &tag_t);
            Json::Value nonce = root["nonce"];
            init(&nonce, &nonce_t);
            resultCode = mf_enhanced_auth(tag_t.e_sector_index, probeSectorIndex, &tag_t, &nonce_t, mode, dumpKeyA, &ntprobe_t);
            LOGD("rawProbe mf_enhanced_auth resultCode: %d", resultCode);
            Json::Value resultNonce;
            structToValue(&nonce_t, &resultNonce);
            result["nonce"] = resultNonce;
            Json::Value resultProbe;
            structToValue(&ntprobe_t, &resultProbe);
            result["probe"] = resultProbe;
            release(&nonce_t);
        } catch (exception e) {
            LOGE("rawProbe exception: %s", e.what());
        }
    }
    result["resultCode"] = resultCode;
    result["version"] = 1;
    string jsonResult = result.toStyledString();
    LOGD("rawProbe result: %s", jsonResult.c_str());
    return jsonResult;
}

void init(Json::Value *nonce, denonce *d) {
    const int distancesNum = (*nonce)["distancesNumber"].asInt();
    d->median = (*nonce)["median"].asInt();
    d->deviation = (*nonce)["deviation"].asInt();
    d->num_distances = distancesNum;
    d->distances = new uint32_t[distancesNum]{0};
    Json::Value distances = (*nonce)["distances"];
    LOGD("rawProbe: nonce distances size = %d", distances.size());
    for (uint32_t i = 0; i < distances.size(); i++) {
        d->distances[i] = distances[i].asInt();
    }
}

void init(Json::Value *tag, mftag *t) {
    t->authuid = (*tag)["uid"].asInt();
    t->e_sector_index = (*tag)["eSector"].asInt();
    t->num_sectors = NR_TRAILERS_1k;
    Json::Value sectorsValue = (*tag)["sectors"];
    if (sectorsValue.empty()) {
        return;
    }
    Json::Value eSectorValue = sectorsValue[t->e_sector_index];
    sector *eSector = t->sectors + t->e_sector_index;
    (*eSector).foundKeyA = eSectorValue["foundKeyA"].asBool();
    (*eSector).foundKeyB = eSectorValue["foundKeyB"].asBool();
    Json::Value keyAValue = eSectorValue["keyA"];
    if (!keyAValue.empty()) {
        for (uint32_t i = 0; i < keyAValue.size(); i++) {
            (*eSector).KeyA[i] = (uint8_t) keyAValue[i].asInt();
        }
    }
    Json::Value keyBValue = eSectorValue["keyB"];
    if (!keyBValue.empty()) {
        for (uint32_t i = 0; i < keyBValue.size(); i++) {
            (*eSector).KeyB[i] = (uint8_t) keyBValue[i].asInt();
        }
    }
    for (int i = 0; i < t->num_sectors; i++) {
        t->sectors[i].trailer = (i + 1) * 4 - 1;
    }
}

void structToValue(denonce *d, Json::Value *nonce) {
    (*nonce)["distancesNumber"] = d->num_distances;
    (*nonce)["median"] = d->median;
    (*nonce)["deviation"] = d->deviation;
    LOGD("rawProbe: struct nonce distances size = %d", d->num_distances);
    for (uint32_t i = 0; i < d->num_distances; i++) {
        (*nonce)["distances"][i] = d->distances[i];
    }
}

void structToValue(ntprobe *probe_p, Json::Value *probe) {
    (*probe)["median"] = probe_p->median;
    (*probe)["deviation"] = probe_p->deviation;
    (*probe)["nt"] = probe_p->Nt;
    (*probe)["ntE"] = probe_p->NtE;
    int parityCount = (sizeof(probe_p->parity) / sizeof(probe_p->parity[0]));
    for (int i = 0; i < parityCount; i++) {
        (*probe)["parity"][i] = probe_p->parity[i];
    }
}

void release(denonce *d) {
    delete d->distances;
}
