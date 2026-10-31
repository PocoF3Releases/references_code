#include <memory>
#include <string>
#include <sys/ioctl.h>
#include "tinyxml2.h"
using namespace tinyxml2;

#define MAX_BUF_SIZE 4096
#define REG_INTF_FILE "/dev/mi_reg_intf"

typedef struct _reg_intf_access_rawmem
{
    unsigned int p;
    int width;
    unsigned int val;
} reg_intf_access_rawmem;

#define REG_INTF_MAGIC 'M'
#define REG_INTF_PRIVATE 168
#define REG_INTF_WRITE_MEM \
        _IOW(REG_INTF_MAGIC, REG_INTF_PRIVATE + 1, reg_intf_access_rawmem)
#define REG_INTF_READ_MEM \
        _IOWR(REG_INTF_MAGIC, REG_INTF_PRIVATE + 2, reg_intf_access_rawmem)

class CTestStep
{
    private:
        enum class TestType { tReg, tFile, tExecute, tProp };
        enum class AccessType { acWrite, acRead, acExecute };
        enum class JudgeType { jShow, jRegex, jGreater, jLess, jEqual, jIgnore, jNone };
        std::string addr;
        TestType testType;
        AccessType accessType;
        JudgeType judgeType = JudgeType::jNone;
        std::string Result;
        std::string value;
        std::shared_ptr<CTestStep> nextStep = nullptr;
        bool verbose = false;
        bool trim = false;
        int index;
        int reg_width = 0;
        int cmd_delay = 0;

        void accessFile();
        void accessReg();
        void accessProp();
        void executeCmd();
        void Trim();

    public:
        CTestStep() = default;
        ~CTestStep() = default;
        std::shared_ptr<CTestStep> getNextStep() { return nextStep; }
        template <typename T>
        void setNextStep(T&& step) { nextStep = std::forward<T>(step); }
        void setTestType(TestType t) { testType = t; }
        void setTestType(const char* s);
        void setAccessType(AccessType t) { accessType = t; }
        void setAccessType(const char* s);
        void setJudgeType(JudgeType t) { judgeType = t; }
        void setJudgeType(const char* s);
        std::string getResult() { return Result; }
        int parseElement(XMLElement* elem);
        void perform();
        bool checkResult();
        void setVerbose(bool v) { verbose = v; }
        void setIndex(int i) { index = i; }
};

class CTestItem
{
    private:
        std::shared_ptr<CTestStep> stepHead;
        std::shared_ptr<CTestItem> nextItem = nullptr;
        std::string description;
        bool verbose;

    public:
        CTestItem();
        ~CTestItem() = default;
        int parseElement(XMLElement* elem);
        std::shared_ptr<CTestItem> getNextItem() { return nextItem; }
        template <typename T>
        void setNextItem(T&& item) { nextItem = std::forward<T>(item); }
        std::string getDesc() { return description; }
        int performTest();
        void setVerbose(bool v) { verbose = v; }
};

class CTestCases
{
    private:
        std::string description;
        std::shared_ptr<CTestItem> itemHead;
        bool verbose;

    public:
        CTestCases();
        ~CTestCases() = default;
        int parseFile(std::string file);
        int performTest(std::string desc);
        void setVerbose(bool v) { verbose = v; }
        std::string getDesc() { return description; }
};
