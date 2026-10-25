/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_

#include <string>
#include <map>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

using std::map;
using std::string;
using std::make_pair;
using std::pair;

namespace android {

class XmlParser {
public:
    XmlParser();
    ~XmlParser();
    int init();
    int deinit();
    int ParseLine(const char *input, const char *delim, char *tokens[],
                            const unsigned int max_token, unsigned int *count);
    int parseXml(const string fName);
    bool IsRoot(const xmlChar* root);
    int getXmlValue(const string key,char *p ,int len);
private:
    map<string,string> mNodeCT;
};

};
#endif //_XML_PARSER_H_

