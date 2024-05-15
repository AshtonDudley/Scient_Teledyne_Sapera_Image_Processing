#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <atomic>
#include "SapClassBasic.h"


// Function to find a camera device by its serial number.
// Returns a SapAcqDevice object representing the camera if found.
std::unique_ptr<SapAcqDevice> getDeviceBySN(const std::string& sn) {
    char serverName[CORSERVER_MAX_STRLEN];
    char serialNumberName[2048];
    const int serverCount = SapManager::GetServerCount();

    for (int i = 0; i < serverCount; i++) {
        if (SapManager::GetResourceCount(i, SapManager::ResourceAcqDevice) != 0) {
            SapManager::GetServerName(i, serverName, sizeof(serverName));
            
            auto camera = std::make_unique<SapAcqDevice>(serverName);
            if (camera->Create()) {
                int featureCount;
                if (camera->GetFeatureCount(&featureCount) && featureCount > 0) {
                    if (camera->GetFeatureValue("DeviceID", serialNumberName, sizeof(serialNumberName)) && serialNumberName == sn) {
                        std::cerr << "Camera created." << std::endl;
                        return camera;
                    }
                }
            }
            else {
                std::cerr << "Camera not created." << std::endl;
                // No need to call Destroy here, unique_ptr will take care of deallocating memory
            }
        }
    }
    throw std::runtime_error("Camera \"" + sn + "\" was not found.");
}


// Custom processing class derived from SapProcessing.
class SapMyProcessing : public SapProcessing
{
public:
    SapMyProcessing(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext);

    virtual ~SapMyProcessing();

protected:
    virtual BOOL Run();  // Custom processing logic to be executed.
};

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
        std::cerr << "Buffer is not ready for processing." << std::endl;
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
        std::cout << "Image successfully saved to " << filename << std::endl;
    }
    else {
        std::cerr << "Failed to save image to " << filename << std::endl;
        return FALSE;
    }

    return TRUE;
}

// Structure to hold context information for callbacks.
struct TransferContext {
    std::atomic_int frameGrabCount = 0;  // Count of frames grabbed.
    std::atomic_int frameProcessingCount = 0;  // Count of frames processed.
    std::shared_ptr<SapMyProcessing> processing;  // Shared pointer to the custom processing object.
};

// Callback function for transfer events.
void transferCallback(SapXferCallbackInfo* info) {
    auto context = (TransferContext*)info->GetContext();

    context->frameGrabCount++;
    if (!info->IsTrash()) {  // Check if the buffer is not a trash buffer.
        context->processing->ExecuteNext();  // Execute processing for the current frame.
    }
}

// Callback function called after processing is done.
void processingCallback(SapProCallbackInfo* info) {
    auto context = (TransferContext*)info->GetContext();

    context->frameProcessingCount++;  // Increment the processed frame count.
}

// Main function to initiate grabbing and processing of images from the camera.
void grab(std::unique_ptr<SapAcqDevice> &camera) {
    const int maxFrameCount = 10;
    TransferContext context;

    

    std::unique_ptr<SapBuffer> buffer = std::make_unique<SapBufferWithTrash>(maxFrameCount, camera.get());
    std::unique_ptr<SapTransfer> transfer = std::make_unique<SapAcqDeviceToBuf>(camera.get(), buffer.get(), transferCallback, &context);
    context.processing = std::make_shared<SapMyProcessing>(buffer.get(), processingCallback, &context);
    
    bool rcHeight= buffer->SetHeight(32);
    bool rcWidth = buffer->SetWidth(1024);

    int bufferHeight = buffer->GetHeight(); // Debug Statements
    int bufferWidth = buffer->GetWidth();   // Debug Statements


    std::cout << "Buffer Height: " << bufferHeight << std::endl;    // Debug Statements
    std::cout << "Buffer Width: " << bufferWidth << std::endl;      // Debug Statements
    std::cout << "Height RC: " << rcHeight << std::endl;      // Debug Statements
    std::cout << "Width RC: " << rcWidth << std::endl;      // Debug Statements

    auto cleanup = [&]() {
        if (context.processing) context.processing->Destroy();
        if (transfer) transfer->Destroy();
        if (buffer) buffer->Destroy();
        if (camera) camera->Destroy();  // Now it's safe to directly call Destroy
    };

    try {
        if (!buffer->Create()) throw std::runtime_error("Failed to create buffer object.");
        if (!transfer->Create()) throw std::runtime_error("Failed to create transfer object.");
        if (!context.processing->Create()) throw std::runtime_error("Failed to create processing object.");

    
        transfer->SetAutoEmpty(false);
        context.processing->SetAutoEmpty(true);
        context.processing->Init();

        transfer->Grab();
        while (context.frameGrabCount < maxFrameCount);
        transfer->Freeze();
        if (!transfer->Wait(5000)) throw std::runtime_error("Failed to stop grab.");
        while (context.frameProcessingCount < maxFrameCount);

        cleanup();
    }
    catch (...) {
        cleanup();
        throw;
    }
}


// Entry point of the application.
int main() {
    try {
        std::string serialNumber = "H2657500";      // Camera serial number.
        auto camera = getDeviceBySN(serialNumber);
        grab(camera);  // Start the grab and process procedure.

    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;  // Exit with error code.
    }
    return 0;  // Successful execution.
}
