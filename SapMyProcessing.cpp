
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <atomic>
#include "SapClassBasic.h"


#include "SapMyProcessing.h"



SapMyProcessing::SapMyProcessing(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext)
    : SapProcessing(pBuffers, pCallback, pContext) {}

SapMyProcessing::~SapMyProcessing() {
    if (m_bInitOK) Destroy();  // Ensure proper cleanup.
}

// Implementation of the custom processing logic.
BOOL SapMyProcessing::Run() {
    int proIndex = GetIndex();  // Get the current buffer index.

    SapBuffer::State state;

    // Check if the buffer is ready for processing.
    if (!m_pBuffers->GetState(proIndex, &state) || state != SapBuffer::StateFull) {
        std::cout << "Buffer is not ready for processing." << std::endl;
        return FALSE;
    }

    void* pData = nullptr;
    m_pBuffers->GetAddress(proIndex, &pData);  // Access the data in the buffer.
    int dataSize = 0;
    m_pBuffers->GetSpaceUsed(proIndex, &dataSize);  // Get the size of the data in the buffer.

    // Construct a filename for saving the image.
    std::string filename = "SavedImage_" + std::to_string(proIndex) + ".tiff";
    const char* options = "-format tiff -compression lzw";  // Specify TIFF format and compression.

    // Save the buffer content to a file.
    if (m_pBuffers->Save(filename.c_str(), options, proIndex)) {
        std::cout << "[OUTPUT] Image successfully saved to " << filename << std::endl;
    }
    else {
        std::cout << "[ERROR] Failed to save image to " << filename << std::endl;
        return FALSE;
    }

    return TRUE;
}