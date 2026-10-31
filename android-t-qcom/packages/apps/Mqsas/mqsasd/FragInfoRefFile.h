#ifndef FRAG_INFO_REF_FILE
#define FRAG_INFO_REF_FILE

#include <string>
#include <vector>

static std::string refFileArr[] = {
    "/data/media/0",
    "/data/system/theme",
    "/data/system/theme/compatibility-v10",
};

std::vector<std::string> refFiles(
        refFileArr,
        refFileArr + sizeof(refFileArr) / sizeof(std::string)
);

#endif // FRAG_INFO_REF_FILE
