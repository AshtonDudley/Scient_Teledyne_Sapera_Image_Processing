Version 1: 
Commited on 12-04-2024
Software Workflow:
This application is designed to manage the acquisition, processing, and saving of images from cameras using the SapClassBasic library, currently it saves each frame as a tiff file, however the goal is to stack the frames to create a continuous image and save it in a tiff file. Here's an overview of the program's workflow:

1. Device Detection
The getDeviceBySN() function is responsible for locating a camera device using its serial number. The function iterates over available server resources, attempting to create and configure a SapAcqDevice object for each server. If a device with the matching serial number is found, it is returned. If no such device is found, an exception is thrown.

Key Steps:
Enumerate all servers and check each for acquisition device resources.
For each potential device, create a SapAcqDevice, configure it, and compare its serial number.
Return the device if a match is found; otherwise, throw a runtime error indicating the device was not found.

2. Custom Image Processing
The SapMyProcessing class, derived from SapProcessing, handles custom image processing tasks.

Responsibilities:
Overriding the Run() method to implement the specific image processing logic needed for your application.
Ensuring buffer readiness and performing image processing if conditions are met.
Saving the processed image in TIFF format.
3. Image Acquisition and Buffer Management
The grab() function sets up the environment for image grabbing and processing using the identified camera device.

Workflow:
Retrieve the camera device by serial number.
Create buffer and transfer objects for handling image data.
Configure and initiate image acquisition and processing using callbacks to manage events.

4. Callback Functions
Callback functions are crucial for responding to image acquisition and processing events dynamically.

Functions:
transferCallback(): Called on each image transfer event to manage frame counting and initiate processing if the image data is valid.
processingCallback(): Invoked after each processing event to update the count of processed frames.
5. Main Execution Flow
The main() function is the entry point of the application, which starts the image capture and processing operations.

Execution Details:
Initiates the grab() function with a predefined camera serial number.
Implements exception handling to manage errors and ensure clean exits with appropriate exit codes.
