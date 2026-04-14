#pragma once
#include <string>
#include <filesystem>
#include <functional>
#include "dicom_series.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

enum class RegistrationType {
    Rigid,        // 刚性配准
    Affine,       // 仿射配准
    SyN,          // 非线性配准
    QuickRigid    // 快速刚性配准
};

class Converter {
public:
    using ProgressCallback = std::function<void(int percent, const std::string& message)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    Converter();
    ~Converter();

    bool convertSeries(
        const DICOMSeries& series,
        const fs::path& outputFolder,
        ProgressCallback onProgress = nullptr,
        ErrorCallback onError = nullptr
    );

    bool registerImages(
        const std::string& fixedImage,      // 固定图像
        const std::string& movingImage,     // 移动图像
        const std::string& outputPrefix,    // 输出前缀
        RegistrationType regType = RegistrationType::Rigid,
        ProgressCallback onProgress = nullptr,
        ErrorCallback onError = nullptr
    );

    std::string getLastError() const;  // 声明

private:
    std::string lastError;

    std::string normalizePath(const std::string& path);
    std::wstring stringToWideString(const std::string& str);
    std::string executeCommand(const std::string& cmd);
};