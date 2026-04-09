#include "dicom_converter.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <algorithm>

Converter::Converter() {}

// 辅助函数：将路径中的正斜杠转换为反斜杠
std::string normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
}

// 辅助函数：字符串转 wstring
std::wstring stringToWideString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}

bool Converter::convertSeries(
    const DICOMSeries& series,
    const fs::path& outputFolder,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    // 1. 确保输出目录存在
    if (!fs::exists(outputFolder)) {
        if (!fs::create_directories(outputFolder)) {
            lastError = "Cannot create output directory: " + outputFolder.string();
            if (onError) onError(lastError);
            return false;
        }
    }

    // 2. 获取并规范化 dcm2niix 路径 ⭐ 关键改动
    std::string exePath = normalizePath(DCM2NIIX_EXE);

    // 3. 生成输出文件名
    auto now = std::time(nullptr);
    char timeBuffer[100];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", std::localtime(&now));

    std::string fileName = series.modality + "_" +
        series.seriesNumber + "_" +
        timeBuffer;

    // 4. 规范化所有路径
    std::string outputDir = normalizePath(outputFolder.string());
    std::string inputDir = normalizePath(series.sourceFolder.string());

    // 5. 构建宽字符命令
    std::wstring wExePath = stringToWideString(exePath);
    std::wstring wOutputDir = stringToWideString(outputDir);
    std::wstring wFileName = stringToWideString(fileName);
    std::wstring wInputDir = stringToWideString(inputDir);

    std::wostringstream wCmd;
    wCmd << L"\"" << wExePath << L"\" "
        << L"-v 2 "                    // ⭐ 添加：详细输出
        << L"-o \"" << wOutputDir << L"\" "
        << L"-z y "
        << L"-f \"" << wFileName << L"\" "
        << L"-w 1 "
        << L"\"" << wInputDir << L"\"";

    std::wstring command = wCmd.str();

    // 6. 显示命令（用于调试）
    std::string displayCmd(command.begin(), command.end());
    std::cout << "Execute: " << displayCmd << std::endl;

    if (onProgress) onProgress(0, "Converting...");

    // 7. 使用 _wsystem 执行
    int result = _wsystem(command.c_str());

    if (result == 0) {
        if (onProgress) onProgress(100, "Conversion completed");
        return true;
    }
    else {
        lastError = "Conversion failed (Error code: " + std::to_string(result) + ")";
        std::cerr << "Error executing command" << std::endl;
        if (onError) onError(lastError);
        return false;
    }
}

std::string Converter::getLastError() const {
    return lastError;
}
