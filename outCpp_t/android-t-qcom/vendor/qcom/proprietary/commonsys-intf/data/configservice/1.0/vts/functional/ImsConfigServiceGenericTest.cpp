/*****************************************************************************
Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#include "ImsConfigServiceGenericTest.h"

using ::vendor::qti::ims::configservice::V1_0::StatusCode;

std::map<std::string, std::string> ImsConfigVtsTest::g_configValues;
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
        ImsConfigVtsTest::g_configValues[key] = value;
      }
    }
  }
  fileStream.close();
}

void ImsConfigServiceGenericTest::SetUp()
  {
    ALOGI("%s - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener();
    if(factory != nullptr)
    {
      for(auto it : g_configValues)
      {
        std::cout << "Setup g_configValues [" << it.first << "] value[" << it.second << "]\n";
        ALOGI("Setup g_configValues[%s][%s]", it.first.c_str(), it.second.c_str() );
      }
      int testI = atoi(g_configValues[slot_id_key].c_str());
      ALOGI("Setup testI[%d]", testI);
      std::cout << "Setup testI[" << testI <<"]";
      factory->createConfigService(atoi(g_configValues[slot_id_key].c_str()),
                                    mListener,
                                   [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                                   status = _status;
                                   pService = config;
                                   /* end Lamda Func*/ } );

    }

}

int main(int argc, char** argv)
{
  //loading properties
  cout << "Please double check you have a carrier config xml file and a properties.txt file";
  cout << " in the same location as this test binary" << endl;
  loadProperties();
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGE("Test result with status=%d", status);
  return status;
}
