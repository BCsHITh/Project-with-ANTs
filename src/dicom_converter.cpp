//#include "dicom_converter.h"
//#include <cstdlib>
//#include <ctime>
//#include <sstream>
//#include <iostream>
//#include <windows.h>
//#include <algorithm>
//
//Converter::Converter() {}
//
//std::string normalizePath(const std::string& path) {
//    std::string result = path;
//    std::replace(result.begin(), result.end(), '/', '\\');
//    return result;
//}
//
//std::wstring stringToWideString(const std::string& str) {
//    if (str.empty()) return L"";
//    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
//    std::wstring wstr(size_needed, 0);
//    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
//    return wstr;
//}
//
//bool Converter::convertSeries(
//    const DICOMSeries& series,
//    const fs::path& outputFolder,
//    ProgressCallback onProgress,
//    ErrorCallback onError)
//{
//    // 1. 验证输入文件夹
//    if (!fs::exists(series.sourceFolder)) {
//        lastError = "Input folder does not exist: " + series.sourceFolder.string();
//        if (onError) onError(lastError);
//        return false;
//    }
//
//    // 2. 检查文件夹中是否有文件
//    size_t fileCount = std::distance(
//        fs::directory_iterator(series.sourceFolder),
//        fs::directory_iterator()
//    );
//
//    if (fileCount == 0) {
//        lastError = "Input folder is empty: " + series.sourceFolder.string();
//        if (onError) onError(lastError);
//        return false;
//    }
//
//    std::cout << "  Found " << fileCount << " files in input folder" << std::endl;
//
//    // 3. 确保输出目录存在
//    if (!fs::exists(outputFolder)) {
//        if (!fs::create_directories(outputFolder)) {
//            lastError = "Cannot create output directory: " + outputFolder.string();
//            if (onError) onError(lastError);
//            return false;
//        }
//    }
//
//    // 4. 获取 dcm2niix 路径
//    std::string exePath = normalizePath(DCM2NIIX_EXE);
//
//    // 5. 生成输出文件名
//    auto now = std::time(nullptr);
//    char timeBuffer[100];
//    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", std::localtime(&now));
//
//    std::string fileName = series.modality + "_" +
//        series.seriesNumber + "_" +
//        timeBuffer;
//
//    // 6. 规范化路径
//    std::string outputDir = normalizePath(outputFolder.string());
//    std::string inputDir = normalizePath(series.sourceFolder.string());
//
//    // 7. 构建命令（使用 CreateProcess 代替 _wsystem，更可靠）
//    std::string cmdLine = "\"" + exePath + "\" " +
//        "-o \"" + outputDir + "\" " +
//        "-z y " +
//        "-f \"" + fileName + "\" " +
//        "-w 1 " +
//        "\"" + inputDir + "\"";
//
//    std::cout << "Execute: " << cmdLine << std::endl;
//
//    if (onProgress) onProgress(0, "Converting...");
//
//    // 8. 使用 CreateProcess 执行（比 _wsystem 更可靠）
//    STARTUPINFOA si = {};
//    si.cb = sizeof(si);
//    si.dwFlags = STARTF_USESHOWWINDOW;
//    si.wShowWindow = SW_HIDE;  // 隐藏窗口
//
//    PROCESS_INFORMATION pi = {};
//
//    // CreateProcess 需要可写的命令行缓冲区
//    std::vector<char> cmdBuffer(cmdLine.begin(), cmdLine.end());
//    cmdBuffer.push_back('\0');
//
//    BOOL success = CreateProcessA(
//        NULL,                    // 应用程序路径（从命令行解析）
//        cmdBuffer.data(),        // 命令行
//        NULL,                    // 进程安全属性
//        NULL,                    // 线程安全属性
//        FALSE,                   // 不继承句柄
//        CREATE_NO_WINDOW,        // 不创建窗口
//        NULL,                    // 使用父进程环境
//        NULL,                    // 使用父进程当前目录
//        &si,
//        &pi
//    );
//
//    if (!success) {
//        lastError = "Failed to start dcm2niix (Error: " + std::to_string(GetLastError()) + ")";
//        if (onError) onError(lastError);
//        return false;
//    }
//
//    // 9. 等待完成
//    WaitForSingleObject(pi.hProcess, INFINITE);
//
//    // 10. 获取退出码
//    DWORD exitCode;
//    GetExitCodeProcess(pi.hProcess, &exitCode);
//
//    CloseHandle(pi.hProcess);
//    CloseHandle(pi.hThread);
//
//    std::cout << "Exit code: " << exitCode << std::endl;
//
//    // 11. 验证输出文件是否生成
//    bool fileGenerated = false;
//    std::string expectedPrefix = fileName;
//
//    for (const auto& entry : fs::directory_iterator(outputFolder)) {
//        std::string filename = entry.path().filename().string();
//        if (filename.find(expectedPrefix) != std::string::npos &&
//            (filename.find(".nii.gz") != std::string::npos ||
//                filename.find(".nii") != std::string::npos)) {
//            fileGenerated = true;
//            std::cout << "  Generated: " << filename << std::endl;
//            break;
//        }
//    }
//
//    if (exitCode == 0 && fileGenerated) {
//        if (onProgress) onProgress(100, "Conversion completed");
//        return true;
//    }
//    else {
//        lastError = "Conversion failed (Exit code: " + std::to_string(exitCode) +
//            ", File generated: " + (fileGenerated ? "yes" : "no") + ")";
//        std::cerr << "Error: " << lastError << std::endl;
//        if (onError) onError(lastError);
//        return false;
//    }
//}
//
//std::string Converter::getLastError() const {
//    return lastError;
//}

#include "dicom_converter.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <algorithm>
#include <vector>

Converter::Converter() {}

Converter::~Converter() {}

std::string Converter::normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
}

std::wstring Converter::stringToWideString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}

std::string Converter::executeCommand(const std::string& cmd) {
    std::cout << "执行：" << cmd << std::endl;

    int result = std::system(cmd.c_str());

    if (result != 0) {
        lastError = "命令执行失败 (退出码：" + std::to_string(result) + ")";
    }

    return result == 0 ? "" : lastError;
}

bool Converter::convertSeries(
    const DICOMSeries& series,
    const fs::path& outputFolder,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    // ⭐ 调试输出
    std::cout << "    [Converter] 开始转换" << std::endl;
    std::cout << "    [Converter] 输入：" << series.sourceFolder << std::endl;
    std::cout << "    [Converter] 输出：" << outputFolder << std::endl;

    // 1. 验证输入文件夹
    if (!fs::exists(series.sourceFolder)) {
        lastError = "Input folder does not exist: " + series.sourceFolder.string();
        std::cout << "    [Converter] 错误：" << lastError << std::endl;
        if (onError) onError(lastError);
        return false;
    }

    // 2. 检查文件夹中是否有文件
    size_t fileCount = 0;
    try {
        fileCount = std::distance(
            fs::directory_iterator(series.sourceFolder),
            fs::directory_iterator()
        );
    }
    catch (const std::exception& e) {
        lastError = "Failed to read input folder: " + std::string(e.what());
        if (onError) onError(lastError);
        return false;
    }

    if (fileCount == 0) {
        lastError = "Input folder is empty: " + series.sourceFolder.string();
        std::cout << "    [Converter] 错误：" << lastError << std::endl;
        if (onError) onError(lastError);
        return false;
    }

    std::cout << "    [Converter] 找到 " << fileCount << " 个文件" << std::endl;

    // 3. 确保输出目录存在
    if (!fs::exists(outputFolder)) {
        try {
            if (!fs::create_directories(outputFolder)) {
                lastError = "Cannot create output directory: " + outputFolder.string();
                if (onError) onError(lastError);
                return false;
            }
            std::cout << "    [Converter] 已创建输出目录" << std::endl;
        }
        catch (const std::exception& e) {
            lastError = "Failed to create output directory: " + std::string(e.what());
            if (onError) onError(lastError);
            return false;
        }
    }

    // 4. 获取 dcm2niix 路径
    std::string exePath = normalizePath(DCM2NIIX_EXE);
    std::cout << "    [Converter] dcm2niix 路径：" << exePath << std::endl;

    if (!fs::exists(exePath)) {
        lastError = "dcm2niix.exe not found: " + exePath;
        std::cout << "    [Converter] 错误：" << lastError << std::endl;
        if (onError) onError(lastError);
        return false;
    }

    // 5. 生成输出文件名
    auto now = std::time(nullptr);
    char timeBuffer[100];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", std::localtime(&now));

    std::string fileName = series.modality + "_" +
        series.seriesNumber + "_" +
        timeBuffer;
    std::cout << "    [Converter] 输出文件名：" << fileName << std::endl;

    // 6. 规范化路径
    std::string outputDir = normalizePath(outputFolder.string());
    std::string inputDir = normalizePath(series.sourceFolder.string());

    // 7. 构建命令
    std::string cmdLine = "\"" + exePath + "\" " +
        "-o \"" + outputDir + "\" " +
        "-z y " +
        "-f \"" + fileName + "\" " +
        "-w 1 " +
        "\"" + inputDir + "\"";

    std::cout << "    [Converter] 执行命令：" << std::endl;
    std::cout << "      " << cmdLine << std::endl;

    if (onProgress) onProgress(0, "Converting...");

    // 8. 统计转换前的文件数
    size_t filesBefore = 0;
    try {
        filesBefore = std::distance(
            fs::directory_iterator(outputFolder),
            fs::directory_iterator()
        );
    }
    catch (...) {}

    // 9. 使用 CreateProcess 执行
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    std::vector<char> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back('\0');

    BOOL success = CreateProcessA(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        lastError = "Failed to start dcm2niix (Error: " + std::to_string(GetLastError()) + ")";
        std::cout << "    [Converter] 错误：" << lastError << std::endl;
        if (onError) onError(lastError);
        return false;
    }

    // 10. 等待完成（设置 5 分钟超时）
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 300000);

    // 11. 获取退出码
    DWORD exitCode = 1;
    if (waitResult == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exitCode);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "    [Converter] 退出码：" << exitCode << std::endl;
    std::cout << "    [Converter] 等待结果：" << (waitResult == WAIT_OBJECT_0 ? "完成" : "超时/错误") << std::endl;

    // 12. 统计转换后的文件数
    size_t filesAfter = 0;
    try {
        filesAfter = std::distance(
            fs::directory_iterator(outputFolder),
            fs::directory_iterator()
        );
    }
    catch (...) {}

    std::cout << "    [Converter] 文件数变化：" << filesBefore << " -> " << filesAfter << std::endl;

    // 13. 验证输出文件是否生成
    bool fileGenerated = false;
    std::string expectedPrefix = fileName;

    try {
        for (const auto& entry : fs::directory_iterator(outputFolder)) {
            std::string filename = entry.path().filename().string();
            if (filename.find(expectedPrefix) != std::string::npos &&
                (filename.find(".nii.gz") != std::string::npos ||
                    filename.find(".nii") != std::string::npos)) {
                fileGenerated = true;
                std::cout << "    [Converter] ✓ 生成文件：" << filename << std::endl;
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "    [Converter] 警告：无法读取输出目录：" << e.what() << std::endl;
    }

    // 14. 返回结果
    if (exitCode == 0 && fileGenerated) {
        if (onProgress) onProgress(100, "Conversion completed");
        std::cout << "    [Converter] ✓ 转换成功" << std::endl;
        return true;
    }
    else {
        lastError = "Conversion failed (Exit: " + std::to_string(exitCode) +
            ", Files: " + std::to_string(filesBefore) + "->" + std::to_string(filesAfter) +
            ", Generated: " + (fileGenerated ? "yes" : "no") + ")";
        std::cerr << "    [Converter] ✗ 错误：" << lastError << std::endl;
        if (onError) onError(lastError);
        return false;
    }
}

// ⭐ 新增：图像配准实现
bool Converter::registerImages(
    const std::string& fixedImage,
    const std::string& movingImage,
    const std::string& outputPrefix,
    RegistrationType regType,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    // 1. 验证输入文件
    if (!fs::exists(fixedImage)) {
        lastError = "固定图像不存在：" + fixedImage;
        if (onError) onError(lastError);
        return false;
    }

    if (!fs::exists(movingImage)) {
        lastError = "移动图像不存在：" + movingImage;
        if (onError) onError(lastError);
        return false;
    }

    // 2. 获取 ANTs 路径
    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        if (onError) onError(lastError);
        return false;
    }

    // 3. 根据配准类型选择命令
    std::string regCommand;
    std::string transformType;

    switch (regType) {
    case RegistrationType::Rigid:
        transformType = "Rigid";
        break;
    case RegistrationType::Affine:
        transformType = "Affine";
        break;
    case RegistrationType::SyN:
        transformType = "SyN";
        break;
    case RegistrationType::QuickRigid:
        transformType = "Rigid[0.1]";
        break;
    default:
        transformType = "Rigid";
    }

    // 4. 构建 antsRegistration 命令
    std::ostringstream cmd;
    cmd << "\"" << antsPath << "\\antsRegistration.exe\" "
        << "--dimensionality 3 "
        << "--float 0 "
        << "--output [" << outputPrefix << "," << outputPrefix << "_Warped.nii.gz] "
        << "--interpolation Linear "
        << "--winsorize-image-intensities [0.005,0.995] "
        << "--use-histogram-matching 0 "
        << "--initial-moving-transform [" << fixedImage << "," << movingImage << ",1] "
        << "--transform " << transformType << "[0.1] "
        << "--metric MI[" << fixedImage << "," << movingImage << ",1,32,Regular,0.25] "
        << "--convergence [1000x500x250x100,1e-6,10] "
        << "--shrink-factors 8x4x2x1 "
        << "--smoothing-sigmas 3x2x1x0vox";

    std::string command = cmd.str();

    if (onProgress) onProgress(0, "开始配准...");

    // 5. 执行命令
    std::cout << "配准命令：" << command << std::endl;

    // 使用 CreateProcess 执行（支持长命令）
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    std::vector<char> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back('\0');

    BOOL success = CreateProcessA(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        lastError = "无法启动配准进程";
        if (onError) onError(lastError);
        return false;
    }

    // 等待完成（配准可能需要较长时间）
    if (onProgress) onProgress(30, "配准中...");
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode == 0) {
        if (onProgress) onProgress(100, "配准完成");

        // 验证输出文件
        std::string warpedFile = outputPrefix + "_Warped.nii.gz";
        if (fs::exists(warpedFile)) {
            std::cout << "配准成功，输出：" << warpedFile << std::endl;
            return true;
        }
        else {
            lastError = "配准完成但未找到输出文件";
            return false;
        }
    }
    else {
        lastError = "配准失败 (退出码：" + std::to_string(exitCode) + ")";
        if (onError) onError(lastError);
        return false;
    }
}

std::string Converter::getLastError() const {
    return lastError;
}