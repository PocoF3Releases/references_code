#include <cstdint>
#include <string>

#include <json/json.h>

namespace aidl::vendor::xiaomi::hardware::misight {
namespace JsonUtil {
    bool ConvertStrToJson(const std::string& jsonStr, Json::Value &jsonObj);
    std::string ConvertJsonToStr(Json::Value paramJson);
}
} // namespace aidl::vendor::xiaomi::hardware::misight