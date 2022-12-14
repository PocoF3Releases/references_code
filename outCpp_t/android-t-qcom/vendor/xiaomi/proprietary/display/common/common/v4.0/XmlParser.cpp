

#include <cutils/log.h>

#include "XmlParser.h"

#define LOG_TAG "XMLParser"

#define XML_LOG ALOGV


namespace android {

XmlParser::XmlParser()
{}
XmlParser::~XmlParser()
{}


int XmlParser::init()
{

    return 0;
}

int XmlParser::deinit()
{

    return 0;
}

bool XmlParser::IsRoot(const xmlChar* root)
{
    bool ret = false;
    if (!xmlStrcmp(root, BAD_CAST ("root"))) {
        ret = true;
    }
    return ret;
}

int XmlParser::ParseLine(const char *input, const char *delim, char *tokens[],
                        const unsigned int max_token, unsigned int *count)
{
    char *tmp_token = NULL;
    char *temp_ptr;
    unsigned int index = 0;
    if (!input) {
        return -1;
    }
    tmp_token = strtok_r(const_cast<char *>(input), delim, &temp_ptr);
    while (tmp_token && index < max_token) {
        tokens[index++] = tmp_token;
        tmp_token = strtok_r(NULL, delim, &temp_ptr);
    }
    *count = index;

    return 0;
}

int XmlParser::getXmlValue(const string key,char *p ,int len)
{
    int ret = 0;
    map<string,string>::iterator   it;
    if(mNodeCT.empty())
        return -1;
    it = mNodeCT.find(key);
    if(it == mNodeCT.end()) {
        XML_LOG("No find %s",key.c_str());
        ret = -1;
    } else {
        if(len>it->second.size()+1)
            strcpy(p,it->second.c_str());
        else
            ret = -1;
    }
    return ret;
}

int XmlParser::parseXml(const string fName)
{
    xmlDocPtr doc;
    xmlNodePtr rootNode;
    unsigned int token_count = 0;
    const unsigned int max_count = 256;
    char *tokens[max_count] = {NULL};
    int num = 0;
    int nodeNum = 0;
    char *nodeName[max_count] = {NULL};
    char *paramName[max_count] = {NULL};
    char ParamName[256];

    if (access(fName.c_str(), F_OK) < 0) {
        ALOGE("Could not access the XML file at %s\n", fName.c_str());
        return -1;
    }
    doc = xmlReadFile(fName.c_str(), "UTF-8", XML_PARSE_RECOVER);
    if(!doc) {
        XML_LOG("XML Document not parsed successfully\n");
        return -1;
    }
    rootNode = xmlDocGetRootElement(doc);
    if(!rootNode) {
        XML_LOG("Empty document\n");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return -1;
    }

    // Confirm the root-element of the tree
    if(!IsRoot(rootNode->name)) {
        XML_LOG("Document of the wrong type, root node != root");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return -1;
    }

    xmlNodePtr pFirst = rootNode->xmlChildrenNode;
    while(NULL != pFirst)
    {
        XML_LOG("first type:%d name:%s\n",pFirst->type,pFirst->name);
        if(pFirst->type != XML_ELEMENT_NODE) {
            pFirst = pFirst->next;
            continue;
        }

        if(!xmlStrcmp(pFirst->name, (const xmlChar *)("color"))) {
            xmlNodePtr pSecond = pFirst->xmlChildrenNode;
            while(NULL != pSecond) {
                XML_LOG("second type:%d name:%s\n",pSecond->type,pSecond->name);
                if(pSecond->type != XML_ELEMENT_NODE) {
                    pSecond = pSecond->next;
                    continue;
                }
                if(!xmlStrcmp(pSecond->name, (const xmlChar *)("NodeTree"))) {
                    xmlNodePtr pThird = pSecond->xmlChildrenNode;
                    xmlChar* value= NULL;
                    while(NULL != pThird) {
                        XML_LOG("third type:%d name:%s\n",pThird->type,pThird->name);
                        if(pThird->type != XML_ELEMENT_NODE) {
                            pThird = pThird->next;
                            continue;
                        }
                        if(!xmlStrcmp(pThird->name, (const xmlChar *)("NodeName"))) {
                            value = xmlNodeGetContent(pThird);
                            string key((char *)pThird->name);
                            string values((char *)value);
                            mNodeCT.insert(pair<string,string>(key,values));
                            if (!ParseLine((char *)value, " \n", tokens, max_count, &token_count)) {
                                nodeNum = token_count;
                                for (int i=0;i<nodeNum;i++) {
                                    nodeName[i] = (char*)malloc(256*sizeof(char));
                                    memset(nodeName[i], 0, 256*sizeof(char));
                                    strcpy(nodeName[i], (char *)tokens[i]);
                                }
                            } else {
                                nodeNum = 0;
                            }
                            if (value != NULL) {
                                xmlFree(value);
                                value = NULL;
                            }
                        }
                        pThird = pThird->next;
                    }
                }
                for (int j = 0; j < nodeNum; j++) {
                    if (nodeName[j] != NULL) {
                        pSecond = pSecond->next;
                        while(pSecond->type != XML_ELEMENT_NODE) {
                            pSecond = pSecond->next;
                            continue;
                        }
                        if(!xmlStrcmp(pSecond->name, (const xmlChar *)nodeName[j])) {
                            xmlNodePtr pThird = pSecond->xmlChildrenNode;
                            xmlChar* value= NULL;
                            memset(ParamName, 0, 256);
                            strcat(ParamName, nodeName[j]);
                            strcat(ParamName, "ParamName");
                            while(NULL != pThird) {
                                XML_LOG("third type:%d name:%s\n",pThird->type,pThird->name);
                                if(pThird->type != XML_ELEMENT_NODE) {
                                    pThird = pThird->next;
                                    continue;
                                }

                                if(!xmlStrcmp(pThird->name, (const xmlChar *)ParamName)) {
                                    value = xmlNodeGetContent(pThird);
                                    string key((char *)pThird->name);
                                    string values((char *)value);
                                    mNodeCT.insert(pair<string,string>(key,values));
                                    if (!ParseLine((char *)value, " \n", tokens, max_count, &token_count)) {
                                        num = token_count;
                                        for (int i=0;i<num;i++) {
                                            paramName[i] = (char*)malloc(256*sizeof(char));
                                            memset(paramName[i], 0, 256*sizeof(char));
                                            strcpy(paramName[i], (char *)tokens[i]);
                                        }
                                    } else {
                                        num = 0;
                                    }
                                    if (value != NULL) {
                                        xmlFree(value);
                                        value = NULL;
                                    }
                                }
                                for (int i = 0; i < num; i++) {
                                    if (paramName[i] != NULL) {
                                        pThird = pThird->next;
                                        while(pThird != NULL) {
                                            if (pThird->type != XML_ELEMENT_NODE) {
                                                pThird = pThird->next;
                                                continue;
                                            } else {
                                                break;
                                            }
                                        }
                                        if (pThird != NULL) {
                                            if(!xmlStrcmp(pThird->name, (const xmlChar *)paramName[i])) {
                                                value = xmlNodeGetContent(pThird);
                                                //XML_LOG("\n%s-->%d\n", pThird->name, atoi((char *)value));
                                                string key((char *)pThird->name);
                                                string values((char *)value);
                                                mNodeCT.insert(pair<string,string>(key,values));
                                                if (value != NULL) {
                                                    xmlFree(value);
                                                    value = NULL;
                                                }
                                                if (paramName[i] != NULL) {
                                                    free(paramName[i]);
                                                    paramName[i] = NULL;
                                                }
                                            }
                                        } else
                                            break;
                                    }
                                }
                                pThird = pThird->next;
                            }
                        }
                    }else {
                        break;
                    }
                    if (nodeName[j] != NULL) {
                        free(nodeName[j]);
                        nodeName[j] = NULL;
                    }
                }
                pSecond = pSecond->next;
            }
        }
        pFirst = pFirst->next;
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    ALOGD("map test\n");
    map<string,string>::iterator   it;
    for(it=mNodeCT.begin();it!=mNodeCT.end(); it++)
    {
        ALOGD("%s %s",it->first.c_str(),it->second.c_str());
    }

    return 0;
}

}

