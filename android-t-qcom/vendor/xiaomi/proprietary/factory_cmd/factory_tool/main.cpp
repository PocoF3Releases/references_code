#include <iostream>
#include <string.h>
#include <getopt.h>
#include "testitem.h"

void showUsage(const char* program_name)
{
    fprintf(stdout, "Factory test tool\n");
    fprintf(stdout, "Author: Taojun(taojun@xiaomi.com)\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Usage: %s [options]\n", program_name);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "--file -f              Specify XML file of test items\n");
    fprintf(stdout, "--item -i              Specify the test item, must be defined in XML file\n");
    fprintf(stdout, "--verbose -v           Show verbose logs\n");
    fprintf(stdout, "--help -h              Show usage\n");
}

int main(int argc, const char* argv[])
{
    CTestCases tc;
    std::string sFileName;
    std::string sItemName;
    bool bShowHelp = false;
    int opt = 0;
    int opt_index = 0;

    struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"item", required_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc,
                              (char * const *)argv,
                              "f:i:hv",
                              long_options,
                              &opt_index)) != -1) {
        switch(opt) {
            case 'f':
                sFileName = optarg;
                break;
            case 'i':
                sItemName = optarg;
                break;
            case 'v':
                tc.setVerbose(true);
                break;
            case 'h':
            default:
                bShowHelp = true;
                break;
        }
    }

    if (bShowHelp || sFileName.empty()) {
        showUsage(argv[0]);
        exit(0);
    }

    tc.parseFile(sFileName);
    tc.performTest(sItemName);

    return 0;
}