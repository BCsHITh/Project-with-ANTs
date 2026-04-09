#pragma once
#include <string>
#include <filesystem>
#include <functional>
#include "dicom_series.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

class Converter {
public:
    using ProgressCallback = std::function<void(int percent, const std::string& message)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    Converter();

    bool convertSeries(
        const DICOMSeries& series,
        const fs::path& outputFolder,
        ProgressCallback onProgress = nullptr,
        ErrorCallback onError = nullptr
    );

    std::string getLastError() const;  // ÉùÃ÷

private:
    std::string lastError;
};