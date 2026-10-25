#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <utils/Log.h>
#include "ParamManager2.h"
#include <stdio.h>
#include <fcntl.h>
#define LOG_TAG "DisplayFeatureHal"
namespace android {
using namespace std;
ParamManager::ParamManager()
{
    mInit = false;
}

ParamManager::~ParamManager()
{
}

bool ParamManager::Init() {
    bool ret = false;
    XMLDocument doc;
    XMLError err = XML_SUCCESS;
    XMLError err_bak = XML_SUCCESS;
    err = doc.LoadFile(ParamFileName.c_str());
    if(XML_SUCCESS != err) {
        BackupXMLFile(1);
        err_bak = doc.LoadFile(ParamFileName.c_str());
        if(XML_SUCCESS != err_bak) {
            if (XML_ERROR_FILE_NOT_FOUND == err)
                ALOGE("file %s not exist", ParamFileName.c_str());
            else {
                ALOGE("load file %s error %d", ParamFileName.c_str(), err);
                if (remove(ParamFileName.c_str()) == 0)
                    ALOGI("file remove success");
                else
                    ALOGE("file remove failed");
            }

            map<string, string> _Map;
            for (int i = EXPERT_NODE_ENABLE; i < EXPERT_NODE_MAX; i++) {
                _Map[ExpertNode[i].c_str()]    = "0";
            }
            _Map[ExpertNode[EXPERT_NODE_R].c_str()]   = "255";
            _Map[ExpertNode[EXPERT_NODE_G].c_str()]   = "255";
            _Map[ExpertNode[EXPERT_NODE_B].c_str()]   = "255";
            _Map[ExpertNode[EXPERT_NODE_GAMMA].c_str()]   = "220";
            char ExpertOption_value_get[100] = {0};
            ret = CreateXMLFile(_Map);
            if (ret == false) {
                return ret;
            }
        }
        ALOGD("%s load file failed but backup success", __func__);
    }
    mInit = true;
    return true;
}

bool ParamManager::CreateXMLFile(const map<string, string> &T_Map)
{
    XMLDocument *pDoc = nullptr;
    XMLDeclaration *declaration = nullptr;
    XMLElement *root = nullptr;
    XMLElement* pNode = nullptr;
	XMLElement* Param = nullptr;
	XMLText* value = nullptr;

    pDoc = new XMLDocument();
    if(nullptr == pDoc) {
        ALOGE("%s fail pDoc null", __func__);
        return false;
    }
    // Add declaration
    declaration = pDoc->NewDeclaration();
    if (nullptr == declaration) {
        ALOGE("%s fail declaration null", __func__);
        return false;
    }
    pDoc->InsertFirstChild(declaration);
    // Add Root node
    root = pDoc->NewElement("root");
    if (nullptr == root) {
        ALOGE("%s fail root null", __func__);
        return false;
    }
    pDoc->InsertEndChild(root);
    pNode = pDoc->NewElement(ExpertNode[EXPERT_NODE_NAME].c_str());
    if (nullptr == pNode) {
        ALOGE("%s fail ExpertNode null", __func__);
        return false;
    }
    root->InsertEndChild(pNode);
    for (auto it = T_Map.begin(); it != T_Map.end(); ++it)
    {
        Param = pDoc->NewElement(it->first.c_str());
        value = pDoc->NewText(it->second.c_str());
        Param->InsertEndChild(value);
        pNode->InsertEndChild(Param);
    }

    pDoc->SaveFile(ParamFileName.c_str());
    delete pDoc;
    return true;
}

int ParamManager::getParam(const string Node, const string Param)
{
    int ret = 0;
    if (!mInit) {
        Init();
    }
    XMLDocument *pDoc = nullptr;
    XMLElement* pRoot = nullptr;
    XMLElement* pNode = nullptr;
    XMLElement* pParam = nullptr;

    pDoc = new XMLDocument();
    if (nullptr == pDoc)
    {
        ALOGE("%s fail pDoc null", __func__);
        return ret;
    }
    ret = pDoc->LoadFile(ParamFileName.c_str());
    if (ret != 0) {
        ALOGE("Load xml file %s failed", ParamFileName.c_str());
        BackupXMLFile(1);
        Init();
        return -1;
    }
    pRoot = pDoc->RootElement();
    if (nullptr == pRoot) {
        ALOGE("%s fail pRoot null", __func__);
        return -1;
    }
    pNode = pRoot->FirstChildElement(Node.c_str());
    if (nullptr == pNode) {
        ALOGE("%s fail pNode null", __func__);
        return -1;
    }
    pParam = pNode->FirstChildElement(Param.c_str());
    if (nullptr == pParam) {
        ALOGE("%s fail pParam null", __func__);
        return -1;
    }
    ret = atoi(pParam->GetText());
    delete pDoc;
    ALOGD("%s Node %s, case %s, value %d", __func__, Node.c_str(), Param.c_str(), ret);
    return ret;
}

bool ParamManager::setParam(const string Node, const string Param, int value)
{
    int ret = 0;
    if (!mInit) {
        Init();
    }
    XMLDocument *pDoc = nullptr;
    XMLElement* pRoot = nullptr;
    XMLElement* pNode = nullptr;
    XMLElement* pParam = nullptr;

    pDoc = new XMLDocument();
    if (nullptr == pDoc)
    {
        ALOGE("%s fail pDoc null", __func__);
        return ret;
    }
    ret = pDoc->LoadFile(ParamFileName.c_str());
    if (ret != 0) {
        ALOGE("Load xml file %s failed", ParamFileName.c_str());
        BackupXMLFile(1);
        Init();
        return -1;
    }
    pRoot = pDoc->RootElement();
    if (nullptr == pRoot) {
        ALOGE("%s fail pRoot null", __func__);
        return -1;
    }
    pNode = pRoot->FirstChildElement(Node.c_str());
    if (nullptr == pNode) {
        ALOGE("%s fail pNode null", __func__);
        return -1;
    }
    pParam = pNode->FirstChildElement(Param.c_str());
    if (nullptr == pParam) {
        ALOGE("%s fail pParam null", __func__);
        return -1;
    }
    pParam->SetText(value);
    pDoc->SaveFile(ParamFileName.c_str());
    delete pDoc;
    syncParam();
    ALOGV("%s Node %s, case %s, value %d", __func__, Node.c_str(), Param.c_str(), value);
    return ret;
}

bool ParamManager::BackupXMLFile(int status)
{
	int ret = 0;
	std::ifstream in;
	std::ofstream out;
	string SourceFile = ParamFileName;
	string DescFile = BakParamFileName;
	if(status != 0)
	{
		SourceFile = BakParamFileName;
		DescFile = ParamFileName;
	}
	in.open(SourceFile.c_str(), std::ios::binary);
	if (in.fail()) {
		ALOGE("%s,Open file %s failed", __func__,ParamFileName.c_str());
		in.close();
		out.close();
		return -1;
	}
	out.open(DescFile.c_str(), std::ios::binary);
	if (out.fail()) {
		ALOGE("%s,Open file %s failed", __func__,BakParamFileName.c_str());
		in.close();
		out.close();
		return -1;
	} else {
		out << in.rdbuf();
		out.close();
		in.close();
		return ret;
	}
}

bool ParamManager::syncParam()
{
	int ret = 0;
	FILE *fp = NULL;
	fp=fopen(ParamFileName.c_str(),"r");
	if(fp == NULL) {
		ALOGE("%s fail to open file", __func__);
		return -1;
	}
	int fd = -1;
	fd = fileno(fp);
	if(fd == -1)
	{
	    ALOGE("%s fp to fd error", __func__);
            fflush(fp);
            fclose(fp);
	    return -1;
	}
	fflush(fp);
	fsync(fd);
	fclose(fp);
	return ret;
}

}
