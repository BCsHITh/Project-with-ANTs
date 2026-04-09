#include "dicom_converter.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <algorithm>

Converter::Converter() {}

std::string normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
}

std::wstring stringToWideString(const std::string& str) {
    if (str.empty()) return L"";
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
    // 1. 验证输入文件夹
    if (!fs::exists(series.sourceFolder)) {
        lastError = "Input folder does not exist: " + series.sourceFolder.string();
        if (onError) onError(lastError);
        return false;
    }

    // 2. 检查文件夹中是否有文件
    size_t fileCount = std::distance(
        fs::directory_iterator(series.sourceFolder),
        fs::directory_iterator()
    );

    if (fileCount == 0) {
        lastError = "Input folder is empty: " + series.sourceFolder.string();
        if (onError) onError(lastError);
        return false;
    }

    std::cout << "  Found " << fileCount << " files in input folder" << std::endl;

    // 3. 确保输出目录存在
    if (!fs::exists(outputFolder)) {
        if (!fs::create_directories(outputFolder)) {
            lastError = "Cannot create output directory: " + outputFolder.string();
            if (onError) onError(lastError);
            return false;
        }
    }

    // 4. 获取 dcm2niix 路径
    std::string exePath = normalizePath(DCM2NIIX_EXE);

    // 5. 生成输出文件名
    auto now = std::time(nullptr);
    char timeBuffer[100];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", std::localtime(&now));

    std::string fileName = series.modality + "_" +
        series.seriesNumber + "_" +
        timeBuffer;

    // 6. 规范化路径
    std::string outputDir = normalizePath(outputFolder.string());
    std::string inputDir = normalizePath(series.sourceFolder.string());

    // 7. 构建命令（使用 CreateProcess 代替 _wsystem，更可靠）
    std::string cmdLine = "\"" + exePath + "\" " +
        "-o \"" + outputDir + "\" " +
        "-z y " +
        "-f \"" + fileName + "\" " +
        "-w 1 " +
        "\"" + inputDir + "\"";

    std::cout << "Execute: " << cmdLine << std::endl;

    if (onProgress) onProgress(0, "Converting...");

    // 8. 使用 CreateProcess 执行（比 _wsystem 更可靠）
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏窗口

    PROCESS_INFORMATION pi = {};

    // CreateProcess 需要可写的命令行缓冲区
    std::vector<char> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back('\0');

    BOOL success = CreateProcessA(
        NULL,                    // 应用程序路径（从命令行解析）
        cmdBuffer.data(),        // 命令行
        NULL,                    // 进程安全属性
        NULL,                    // 线程安全属性
        FALSE,                   // 不继承句柄
        CREATE_NO_WINDOW,        // 不创建窗口
        NULL,                    // 使用父进程环境
        NULL,                    // 使用父进程当前目录
        &si,
        &pi
    );

    if (!success) {
        lastError = "Failed to start dcm2niix (Error: " + std::to_string(GetLastError()) + ")";
        if (onError) onError(lastError);
        return false;
    }

    // 9. 等待完成
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 10. 获取退出码
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "Exit code: " << exitCode << std::endl;

    // 11. 验证输出文件是否生成
    bool fileGenerated = false;
    std::string expectedPrefix = fileName;

    for (const auto& entry : fs::directory_iterator(outputFolder)) {
        std::string filename = entry.path().filename().string();
        if (filename.find(expectedPrefix) != std::string::npos &&
            (filename.find(".nii.gz") != std::string::npos ||
                filename.find(".nii") != std::string::npos)) {
            fileGenerated = true;
            std::cout << "  Generated: " << filename << std::endl;
            break;
        }
    }

    if (exitCode == 0 && fileGenerated) {
        if (onProgress) onProgress(100, "Conversion completed");
        return true;
    }
    else {
        lastError = "Conversion failed (Exit code: " + std::to_string(exitCode) +
            ", File generated: " + (fileGenerated ? "yes" : "no") + ")";
        std::cerr << "Error: " << lastError << std::endl;
        if (onError) onError(lastError);
        return false;
    }
}

std::string Converter::getLastError() const {
    return lastError;
}