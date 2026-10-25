/*****************************************************************************
Copyright (c) 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#include "ImsConfigCommon.h"

std::map<std::string, std::string> g_configValues;
uint32_t userData = 1001;
std::string userProvidedAppId = "";

void loadProperties()
{
  std::string line;
  std::ifstream fileStream;
  fileStream.open(PROPERTIES_FILE, std::ifstream::in);
  while (std::getline(fileStream, line))
  {
    std::istringstream is_line(line);
    std::string key;
    if (std::getline(is_line, key, '='))
    {
      std::string value = "";
      if (key[0] == '#')
        continue;

      if (std::getline(is_line, value))
      {
        std::cout << "key [" << key << "] value[" << value << "]\n";
        ALOGI("key [%s], value[%s]", key.c_str(), value.c_str());
        g_configValues[key] = value;
      }
    }
  }
  fileStream.close();
}

int main(int argc, char** argv)
{
  //loading properties
  cout << "Please double check you have a carrier config xml file and a properties.txt file";
  cout << " in the same location as this test binary" << endl;
  loadProperties();
  ::testing::InitGoogleTest(&argc, argv);

  for (int i = 0; i < argc; i++) {
    std::string tempArg(argv[i]);
    if(tempArg.find("--appId=") != std::string::npos) {
      tempArg.replace(0,sizeof("--appId=")-1, "");
      userProvidedAppId = tempArg;
    }
  }

  int status = RUN_ALL_TESTS();
  ALOGE("Test result with status=%d", status);
  return status;
}
