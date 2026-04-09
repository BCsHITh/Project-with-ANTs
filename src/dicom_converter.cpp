#include "dicom_converter.h"
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

Converter::Converter() {}

bool Converter::convertSeries(
    const DICOMSeries& series,
    const fs::path& outputFolder,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    if (!fs::exists(outputFolder)) {
        if (!fs::create_directories(outputFolder)) {
            lastError = "Unable to create output directory:" + outputFolder.string();
            if (onError) onError(lastError);
            return false;
        }
    }

    std::string exePath = DCM2NIIX_EXE;

    auto now = std::time(nullptr);
    char timeBuffer[100];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", std::localtime(&now));

    std::string fileName = series.modality + "_" +
        series.seriesNumber + "_" +
        timeBuffer;

    std::ostringstream cmd;
    cmd << "\"" << exePath << "\" "
        << "-o \"" << outputFolder.string() << "\" "
        << "-z y "
        << "-f \"" << fileName << "\" "
        << "-w 1 "
        << "\"" << series.sourceFolder.string() << "\"";

    std::string command = cmd.str();
    std::cout << "Perform: " << command << std::endl;

    if (onProgress) onProgress(0, "Converting...");

    int result = std::system(command.c_str());

    if (result == 0) {
        if (onProgress) onProgress(100, "Conversion Complete");
        return true;
    }
    else {
        lastError = "Conversion Failed (Error code:" + std::to_string(result) + ")";
        if (onError) onError(lastError);
        return false;
    }
}

std::string Converter::getLastError() const {  // 实现
    return lastError;
}