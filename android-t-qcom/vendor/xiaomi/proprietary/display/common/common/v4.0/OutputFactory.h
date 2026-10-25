
#ifndef _OUTPUT_FACTORY_H_
#define _OUTPUT_FACTORY_H_

#include <utils/Singleton.h>
#include <utils/StrongPointer.h>
#include "OutputMethod.h"

namespace android {


class OutputFactory: public Singleton<OutputFactory>
{
public:
    OutputFactory();
    ~OutputFactory();
    OutputMethod* createProduct(unsigned int displayId, int featureId, char* model);
private:
    friend class Singleton<OutputFactory>;
};
}; //_OUTPUT_FACTORY_H_

#endif
