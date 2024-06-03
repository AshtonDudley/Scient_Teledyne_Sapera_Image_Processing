#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <atomic>
#include "SapClassBasic.h"

#include "SapMyProcessing.h"





// Function to find a camera device and load its settings from a .cff file.
// Returns a SapAcqDevice object representing the camera if found.
std::unique_ptr<SapAcqDevice> getDeviceFromFile(const std::string& configFilepath) {

    char serverName[CORSERVER_MAX_STRLEN];
    char serialNumberName[2048];

    // Check if the we recive a vaild .cff file
    
    
    const int serverCount = SapManager::GetServerCount();
    for (int i = 0; i < serverCount; i++) {
        if (SapManager::GetResourceCount(i, SapManager::ResourceAcqDevice) != 0) {
            SapManager::GetServerName(i, serverName, sizeof(serverName));

            SapLocation loc(serverName, i);
            auto camera = std::make_unique<SapAcqDevice>(serverName);

            camera->SetConfigFile(configFilepath.c_str());

            if (camera->Create()) {
                std::cout << "[DEBUG] Camera created." << std::endl;
                return camera;
                }
            }
        }
    throw std::runtime_error("[ERROR] Camera config file \"" + configFilepath + "\" was not found.");
}



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
                        std::cout << "Camera created." << std::endl;
                        return camera;
                    }
                }
            }
            else {
                std::cout << "Camera not created." << std::endl;
                // No need to call Destroy here, unique_ptr will take care of deallocating memory
            }
        }
    }
    throw std::runtime_error("Camera \"" + sn + "\" was not found.");
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

        int bufferHeight = buffer->GetHeight(); // Debug Statements
        int bufferWidth = buffer->GetWidth();   // Debug Statements

        std::cout << "[DEBUG] Buffer Height: " << bufferHeight << std::endl;    // Debug Statements
        std::cout << "[DEBUG] Buffer Width: " << bufferWidth << std::endl;      // Debug Statements

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

void snap(std::unique_ptr<SapAcqDevice>& camera) {
    int maxFrameCount = 1;
    
    TransferContext context;

    std::unique_ptr<SapBuffer> buffer = std::make_unique<SapBufferWithTrash>(maxFrameCount, camera.get());
    std::unique_ptr<SapTransfer> transfer = std::make_unique<SapAcqDeviceToBuf>(camera.get(), buffer.get(), transferCallback, &context);
    context.processing = std::make_shared<SapMyProcessing>(buffer.get(), processingCallback, &context);

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

        int bufferHeight = buffer->GetHeight(); // Debug Statements
        int bufferWidth = buffer->GetWidth();   // Debug Statements

        std::cout << "[DEBUG] Buffer Height: " << bufferHeight << std::endl;    // Debug Statements
        std::cout << "[DEBUG] Buffer Width: " << bufferWidth << std::endl;      // Debug Statements

        transfer->SetAutoEmpty(false);
        context.processing->SetAutoEmpty(true);
        context.processing->Init();

        // while (context.frameGrabCount < maxFrameCount);
        for (int i = 0; i < 10; i++) {
            transfer->Snap(maxFrameCount);
            transfer->Freeze();
            if (!transfer->Wait(5000)) throw std::runtime_error("Failed to stop grab.");
            while (context.frameProcessingCount < maxFrameCount);
        }
        

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
        
        
        // std::string serialNumber = "H2657500";      // Camera serial number.
        // auto camera = getDeviceBySN(serialNumber);
        
        auto camera = getDeviceFromFile("C:\\Program Files\\Teledyne DALSA\\Sapera\\CamFiles\\User\\T_Linea2-C4096-7um_Custom_1_Custom_1.ccf");
        // grab(camera);  // Start the grab and process procedure.
        for (int i = 0; i < 1; i++) {
            snap(camera);
        }
        
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;  // Exit with error code.
    }
    return 0;  // Successful execution.
}
