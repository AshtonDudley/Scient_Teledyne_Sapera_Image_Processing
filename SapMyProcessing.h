#pragma once
#include <SapProcessing.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <atomic>
#include "SapClassBasic.h"


// Custom processing class derived from SapProcessing.
class SapMyProcessing : public SapProcessing
{
public:
    SapMyProcessing(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext);

    virtual ~SapMyProcessing();

protected:
    virtual BOOL Run();  // Custom processing logic to be executed.
};