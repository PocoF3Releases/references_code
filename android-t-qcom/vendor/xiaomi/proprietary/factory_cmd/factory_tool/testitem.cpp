#include <iostream>
#include <regex>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "testitem.h"
#ifdef ANDROID
#include <cutils/properties.h>
#endif
using namespace tinyxml2;

void CTestStep::setTestType(const char* s)
{
    if (strncmp(s, "file", 4) == 0)
        testType = TestType::tFile;
    else if (strncmp(s, "reg", 3) == 0)
        testType = TestType::tReg;
    else if (strncmp(s, "exec", 4) == 0)
        testType = TestType::tExecute;
    else if (strncmp(s, "prop", 4) == 0)
        testType = TestType::tProp;
    else
        testType = TestType::tFile; // Set default type to FILE
}

void CTestStep::setAccessType(const char* s)
{
    if (strncmp(s, "read", 4) == 0)
        accessType = AccessType::acRead;
    else if (strncmp(s, "write", 5) == 0)
        accessType = AccessType::acWrite;
    else
        accessType = AccessType::acRead;    // Set default type to READ

    if (testType == TestType::tExecute)
        accessType = AccessType::acExecute;
}

void CTestStep::setJudgeType(const char* s)
{
    // Judge type not specified
    if (!s)
        return;

    if (strncmp(s, "regex", 5) == 0)
        judgeType = JudgeType::jRegex;
    else if (strncmp(s, "greater", 7) == 0)
        judgeType = JudgeType::jGreater;
    else if (strncmp(s, "less", 4) == 0)
        judgeType = JudgeType::jLess;
    else if (strncmp(s, "equal", 5) == 0)
        judgeType = JudgeType::jEqual;
    else if (strncmp(s, "show", 4) == 0)
        judgeType = JudgeType::jShow;
    else if (strncmp(s, "ignore", 6) == 0)
        judgeType = JudgeType::jIgnore;
}

int CTestStep::parseElement(XMLElement* elem)
{
    XMLElement* el = elem->FirstChildElement();

    if (verbose)
        std::cout << "Step (" << index << "):" << std::endl;

    while (el) {
        const char* name = el->Name();
        if (strncmp(name, "type", 4) == 0) {
            setTestType(el->GetText());
            if (verbose)
                std::cout << "Type: " << el->GetText() << std::endl;
        } else if (strncmp(name, "addr", 4) == 0) {
            addr = el->GetText();
            if (el->Attribute("width"))
                reg_width = std::atoi(el->Attribute("width"));
            if (verbose) {
                std::cout << "Addr: " << el->GetText() << std::endl;
                if (reg_width)
                    std::cout << "Width: " << reg_width << std::endl;
            }
        } else if (strncmp(name, "operation", 9) == 0) {
            setAccessType(el->GetText());
            if (el->Attribute("delay"))
                cmd_delay = std::atoi(el->Attribute("delay"));

            if (verbose) {
                std::cout << "Oper: " << el->GetText() << std::endl;
                if (cmd_delay)
                    std::cout << "delay: " << cmd_delay << "ms" << std::endl;
            }
        } else if (strncmp(name, "value", 5) == 0) {
            value = el->GetText();
            const char *t = el->Attribute("type");
            setJudgeType(t);
            if (verbose) {
                std::cout << "Value: " << value << std::endl;
                std::cout << "Judge: " << (t ? t : "none") << std::endl << std::endl;
            }
        } else if (strncmp(name, "trim", 4) == 0) {
            if (strncmp(el->GetText(), "true", 4) == 0) {
                trim = true;
            } else {
                trim = false;
            }

            if (verbose) {
                std::cout << "Trim: " << el->GetText() << std::endl;
            }
        }

        el = el->NextSiblingElement();
    }

    return 0;
}

void CTestStep::accessFile()
{
    if (accessType == AccessType::acRead) {
        char temp[MAX_BUF_SIZE] = { 0 };
        FILE* fp = fopen(addr.c_str(), "r");
        if (!fp) {
            std::cerr << "Error opening file " + addr << std::endl;
            return;
        }

        fseek(fp, 0, SEEK_SET);
        fread(temp, MAX_BUF_SIZE - 1, 1, fp);
        fclose(fp);

        Result = temp;

        if (trim) {
            Trim();
        }

        if (verbose) {
            std::cout << "Read from file " + addr << std::endl;
            std::cout << "Result: " + Result << std::endl;
        }
    } else if (accessType == AccessType::acWrite) {
        FILE* fp = fopen(addr.c_str(), "w");
        if (!fp) {
            std::cerr << "Error opening file " + addr << std::endl;
            return;
        }

        fseek(fp, 0, SEEK_SET);
        fwrite(value.c_str(), value.size(), 1, fp);
        fclose(fp);

        if (verbose) {
            std::cout << "Write to file " + addr << std::endl;
            std::cout << "Data: " + value << std::endl;
        }
    } else {
        std::cerr << "Invalid access type!" << std::endl;
    }
}

void CTestStep::accessReg()
{
    reg_intf_access_rawmem data;
    int fd;

    fd = open(REG_INTF_FILE, O_RDWR);
    if (fd < 0) {
        std::cerr << "Cannot open file " << REG_INTF_FILE << std::endl;
        return;
    }

    memset(&data, 0, sizeof(reg_intf_access_rawmem));
    data.p = strtoul(addr.c_str(), NULL, 0);
    if (reg_width == 4 || reg_width == 2 || reg_width == 1)
        data.width = reg_width;
    else
        data.width = 4;

    if (accessType == AccessType::acRead) {
        std::stringstream ss;
        ioctl(fd, REG_INTF_READ_MEM, &data);
        ss << "0x" << std::setfill('0') << std::setw(2 * data.width) << std::hex << data.val;
        Result = ss.str();

        if (trim) {
            Trim();
        }

    } else if (accessType == AccessType::acWrite) {
        data.val = strtoul(value.c_str(), NULL, 0);
        ioctl(fd, REG_INTF_WRITE_MEM, &data);
    } else {
        std::cerr << "Invalid access type!" << std::endl;
    }

    close(fd);
}

#ifdef ANDROID
void CTestStep::accessProp()
{
    if (accessType == AccessType::acRead) {
        char prop[PROP_VALUE_MAX] = { 0 };
        property_get(addr.c_str(), prop, "0");

        Result = prop;

        if (trim) {
            Trim();
        }

        if (verbose) {
            std::cout << "Read from property " + addr << std::endl;
            std::cout << "Result: " + Result << std::endl;
        }
    } else if (accessType == AccessType::acWrite) {
        property_set(addr.c_str(), value.c_str());

        if (verbose) {
            std::cout << "Write to property " + addr << std::endl;
            std::cout << "Data: " + value << std::endl;
        }
    } else {
        std::cerr << "Invalid access type!" << std::endl;
    }
}
#else
void CTestStep::accessProp()
{
    std::cerr << "Properties only used for Android!!" << std::endl;
}
#endif

void CTestStep::executeCmd()
{
    FILE* fp = NULL;
    char buf[MAX_BUF_SIZE] = { 0 };

    if (accessType != AccessType::acExecute) {
        std::cerr << "Invalid access type!" << std::endl;
        return;
    }

    if (verbose)
        std::cout << "Execute command: " + addr << std::endl;

    // Redirect stderr to stdout
    addr += " 2>&1";

    fp = popen(addr.c_str(), "r");
    if (!fp) {
        std::cerr << "Failed to run command: " + addr << std::endl;
        return;
    }

    fread(buf, MAX_BUF_SIZE - 1, 1, fp);
    fclose(fp);

    Result = buf;

    if (trim) {
        Trim();
    }

    if (verbose)
        std::cout << "Result: " + Result << std::endl;
}

void CTestStep::Trim()
{
    Result.erase(Result.find_last_not_of(" \n\r\t") + 1);
}

void CTestStep::perform()
{
    if (verbose)
        std::cout << "Step " << index << std::endl;

    switch (testType) {
        case TestType::tFile:
            accessFile();
            break;

        case TestType::tReg:
            accessReg();
            break;

        case TestType::tExecute:
            executeCmd();
            break;

        case TestType::tProp:
            accessProp();
            break;
    }

    if (cmd_delay)
        usleep(cmd_delay * 1000);
}

bool CTestStep::checkResult()
{
    bool ret = false;
    int res, val;

    // No need to check result for write operation
    if (accessType == AccessType::acWrite)
        return true;

    // when the judgeType is "jIgnore", we ignore the result output of the current step.
    if (judgeType == JudgeType::jIgnore)
        return true;

    switch (judgeType) {
        case JudgeType::jGreater:
            res = std::stoi(Result);
            val = std::stoi(value);
            ret = (res > val) ? true : false;
            break;

        case JudgeType::jLess:
            res = std::stoi(Result);
            val = std::stoi(value);
            ret = (res < val) ? true : false;
            break;

        case JudgeType::jEqual:
            ret = (Result.compare(value) == 0) ? true : false;
            break;

        case JudgeType::jRegex:
            ret = std::regex_match(Result, std::regex(value));
            break;

        case JudgeType::jShow:
        default:
            std::cout << Result << std::endl;
            ret = true;
            break;
    }

    // Output PASS/FAIL with any judge types
    if (judgeType != JudgeType::jShow) {
        if (ret)
            std::cout << "PASS" << std::endl;
        else
            std::cout << "FAIL" << std::endl;
    }

    return ret;
}

CTestItem::CTestItem() : verbose(false)
{
    stepHead = std::make_shared<CTestStep>();
}

int CTestItem::parseElement(XMLElement* elem)
{
    int stepIndex = 1;

    description = elem->FirstChildElement("Description")->GetText();
    if (verbose)
        std::cout << "Description: '" + description + '"' << std::endl;

    XMLElement* elemStep = elem->FirstChildElement("Step");
    auto step = stepHead;
    while (elemStep) {
        auto next = std::make_shared<CTestStep>();
        next->setVerbose(verbose);
        next->setIndex(stepIndex++);
        next->parseElement(elemStep);

        step->setNextStep(std::move(next));
        step = step->getNextStep();
        elemStep = elemStep->NextSiblingElement("Step");
    }

    return 0;
}

int CTestItem::performTest()
{
    auto step = stepHead->getNextStep();

    if (verbose)
        std::cout << "Start testing item: " + description << std::endl;

    while (step != nullptr) {
        step->perform();
        step->checkResult();

        step = step->getNextStep();
    }

    if (verbose)
        std::cout << "Item " + description + " test finished" << std::endl;

    return 0;
}

CTestCases::CTestCases() : verbose(false)
{
    itemHead = std::make_shared<CTestItem>();
}

int CTestCases::parseFile(std::string file)
{
    XMLDocument doc;
    XMLError err = doc.LoadFile(file.c_str());
    if (err != XML_SUCCESS) {
        std::cerr << "Error loading XML file " + file << std::endl;
        return -1;
    }

    XMLElement* elemRoot = doc.RootElement();
    description = elemRoot->FirstChildElement("Description")->GetText();

    if (verbose) {
        std::cout << "=================================================================" << std::endl;
        std::cout << "Start parsing file '" + file + "'"<< std::endl;
        std::cout << "-----------------------------------------------------------------" << std::endl;
        std::cout << "Test item '" + description + "' detected!" << std::endl;
    }

    XMLElement* elemItem = elemRoot->FirstChildElement("Item");
    auto item = itemHead;
    while (elemItem) {
        auto next = std::make_shared<CTestItem>();
        next->setVerbose(verbose);
        next->parseElement(elemItem);

        item->setNextItem(std::move(next));
        item = item->getNextItem();
        elemItem = elemItem->NextSiblingElement("Item");
    }

    if (verbose) {
        std::cout << "-----------------------------------------------------------------" << std::endl;
        std::cout << "Parsing finished" << std::endl;
        std::cout << "=================================================================" << std::endl << std::endl;
    }

    return 0;
}

int CTestCases::performTest(std::string desc = "")
{
    auto item = itemHead->getNextItem();

    if (verbose) {
        std::cout << "=================================================================" << std::endl;
        std::cout << "Start testing cases: '" + description + "'" << std::endl;
        std::cout << "-----------------------------------------------------------------" << std::endl;
    }

    while (item != nullptr) {
        if ((desc.empty()) || (item->getDesc().compare(desc) == 0)) {
            item->performTest();
        }

        item = item->getNextItem();
    }

    if (verbose) {
        std::cout << "-----------------------------------------------------------------" << std::endl;
        std::cout << "Cases '" + description + "' test finished" << std::endl;
        std::cout << "=================================================================" << std::endl;
    }

    return 0;
}
