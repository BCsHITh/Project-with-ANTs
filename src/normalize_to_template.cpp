#include "normalize_to_template.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <vector>

TemplateNormalizer::TemplateNormalizer(){}

std::string TemplateNormalizer::executeCommand(const std::string& cmd) {
    std::cout << "  执行命令..." << std::endl;
    if (verbose) {
        std::cout << "    " << cmd << std::endl;
    }

    int result = std::system(cmd.c_str());

    if (result != 0) {
        lastError = "命令执行失败 (退出码：" + std::to_string(result) + ")";
        return lastError;
    }

    return "";
}

std::vector<std::string> TemplateNormalizer::findBoldFiles(const fs::path& folder) {
    std::vector<std::string> files;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find(".nii.gz") != std::string::npos ||
                filename.find(".nii") != std::string::npos) {
                // 排除已经处理过的文件
                if (filename.find("MNI") == std::string::npos &&
                    filename.find("normalized") == std::string::npos) {
                    files.push_back(entry.path().string());
                }
            }
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

//bool TemplateNormalizer::registerT1ToTemplate(const std::string& t1w,
//    const std::string& templateImg,
//    const std::string& outputPrefix,
//    std::string& affineTransform,
//    std::string& warpTransform) {
//
//    std::string t1wNorm = t1w;
//    std::string templateNorm = templateImg;
//    std::string outputPrefixNorm = outputPrefix;
//
//    std::replace(t1wNorm.begin(), t1wNorm.end(), '/', '\\');
//    std::replace(templateNorm.begin(), templateNorm.end(), '/', '\\');
//    std::replace(outputPrefixNorm.begin(), outputPrefixNorm.end(), '/', '\\');
//
//    std::cout << "[1/2] T1w 到模板的 SyN 配准..." << std::endl;
//
//    std::string antsPath = ANTS_BIN_PATH;
//    if (antsPath.empty()) {
//        lastError = "ANTs 路径未配置";
//        return false;
//    }
//
//    // 验证输入文件
//    if (!fs::exists(t1w)) {
//        lastError = "T1w 图像不存在：" + t1w;
//        return false;
//    }
//
//    if (!fs::exists(templateImg)) {
//        lastError = "模板图像不存在：" + templateImg;
//        return false;
//    }
//
//    // 构建 SyN 配准命令
//    std::string cmd = "\"" + antsPath + "\\antsRegistrationSyN.sh\" "
//        "-d 3 "
//        "-f \"" + templateImg + "\" "
//        "-m \"" + t1w + "\" "
//        "-o \"" + outputPrefix + "\" "
//        "-t s";  // s = SyN (slow but accurate)
//
//    std::cout << "  T1w: " << t1w << std::endl;
//    std::cout << "  模板：" << templateImg << std::endl;
//    std::cout << "  输出前缀：" << outputPrefix << std::endl;
//
//    if (!executeCommand(cmd).empty()) {
//        return false;
//    }
//
//    // 生成的变换文件
//    affineTransform = outputPrefix + "0GenericAffine.mat";
//    warpTransform = outputPrefix + "1Warp.nii.gz";
//
//    // 验证变换文件是否生成
//    if (!fs::exists(affineTransform)) {
//        lastError = "仿射变换文件未生成：" + affineTransform;
//        return false;
//    }
//
//    if (!fs::exists(warpTransform)) {
//        lastError = "形变场文件未生成：" + warpTransform;
//        return false;
//    }
//
//    std::cout << "  ✓ 仿射矩阵：" << affineTransform << std::endl;
//    std::cout << "  ✓ 形变场：" << warpTransform << std::endl;
//
//    return true;
//}

bool TemplateNormalizer::registerT1ToTemplate(const std::string& t1w,
    const std::string& templateImg,
    const std::string& outputPrefix,
    std::string& affineTransform,
    std::string& warpTransform) {
    std::cout << "[1/2] T1w 到模板的 SyN 配准..." << std::endl;

    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        return false;
    }

    // 验证输入文件
    if (!fs::exists(t1w)) {
        lastError = "T1w 图像不存在：" + t1w;
        return false;
    }

    if (!fs::exists(templateImg)) {
        lastError = "模板图像不存在：" + templateImg;
        return false;
    }

    // ⭐ 修复：使用 antsRegistration.exe 而不是 .sh 脚本
    // 构建完整的 SyN 配准命令
    std::ostringstream cmd;
    cmd << "\"" << antsPath << "\\antsRegistration.exe\" "
        << "--dimensionality 3 "
        << "--float 0 "
        << "--output [" << outputPrefix << "," << outputPrefix << "Warped.nii.gz] "
        << "--interpolation Linear "
        << "--winsorize-image-intensities [0.005,0.995] "
        << "--use-histogram-matching 0 "
        << "--initial-moving-transform [" << templateImg << "," << t1w << ",1] "
        // 第一阶段：刚性配准
        << "--transform Rigid[0.1] "
        << "--metric MI[" << templateImg << "," << t1w << ",1,32,Regular,0.25] "
        << "--convergence [1000x500x250x100,1e-6,10] "
        << "--shrink-factors 8x4x2x1 "
        << "--smoothing-sigmas 3x2x1x0vox "
        // 第二阶段：仿射配准
        << "--transform Affine[0.1] "
        << "--metric MI[" << templateImg << "," << t1w << ",1,32,Regular,0.25] "
        << "--convergence [1000x500x250x100,1e-6,10] "
        << "--shrink-factors 8x4x2x1 "
        << "--smoothing-sigmas 3x2x1x0vox "
        // 第三阶段：SyN 非线性配准
        << "--transform SyN[0.1,3,0] "
        << "--metric CC[" << templateImg << "," << t1w << ",1,4] "
        << "--convergence [100x70x50x20,1e-6,10] "
        << "--shrink-factors 8x4x2x1 "
        << "--smoothing-sigmas 3x2x1x0vox";

    std::string command = cmd.str();

    std::cout << "  T1w: " << t1w << std::endl;
    std::cout << "  模板：" << templateImg << std::endl;
    std::cout << "  输出前缀：" << outputPrefix << std::endl;
    std::cout << "  执行 SyN 配准命令..." << std::endl;

    // 使用 CreateProcess 执行（支持长命令）
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

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
        lastError = "无法启动配准进程 (Error: " + std::to_string(GetLastError()) + ")";
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    // 等待完成（SyN 配准可能需要较长时间）
    std::cout << "  配准中...（这可能需要几分钟）" << std::endl;
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "  退出码：" << exitCode << std::endl;

    if (exitCode != 0) {
        lastError = "SyN 配准失败 (退出码：" + std::to_string(exitCode) + ")";
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    affineTransform = outputPrefix + "0GenericAffine.mat";
    warpTransform = outputPrefix + "1Warp.nii.gz";  // ⭐ 使用 1Warp 而不是 1InverseWarp

    // 验证变换文件是否生成
    if (!fs::exists(affineTransform)) {
        lastError = "仿射变换文件未生成：" + affineTransform;
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    if (!fs::exists(warpTransform)) {
        lastError = "形变场文件未生成：" + warpTransform;
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    std::cout << "  ✓ 仿射矩阵：" << affineTransform << std::endl;
    std::cout << "  ✓ 形变场：" << warpTransform << std::endl;

    return true;
}

bool TemplateNormalizer::applyTransformToBold(const std::string& boldImage,
    const std::string& t1wImage,
    const std::string& templateImage,
    const std::string& affineTransform,
    const std::string& warpTransform,
    const std::string& outputFile) {
    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        return false;
    }

    // 验证输入
    if (!fs::exists(boldImage)) {
        lastError = "BOLD 图像不存在：" + boldImage;
        return false;
    }

    if (!fs::exists(t1wImage)) {
        lastError = "T1w 图像不存在：" + t1wImage;
        return false;
    }

    if (!fs::exists(affineTransform)) {
        lastError = "仿射变换文件不存在：" + affineTransform;
        return false;
    }

    if (!fs::exists(warpTransform)) {
        lastError = "形变场文件不存在：" + warpTransform;
        return false;
    }

    //// 构建应用变换命令
    //// 注意：需要先从 BOLD 空间到 T1w 空间，再从 T1w 到模板空间
    //std::string cmd = "\"" + antsPath + "\\antsApplyTransforms.exe\" "
    //    "--dimensionality 4 "  // BOLD 通常是 4D
    //    "--input \"" + boldImage + "\" "
    //    "--reference \"" + templateImage + "\" "
    //    "--transform \"" + warpTransform + "\" "
    //    "--transform \"" + affineTransform + "\" "
    //    "--output \"" + outputFile + "\" "
    //    "--interpolation Linear "
    //    "--verbose 1";
    // ⭐ 修复：构建应用变换命令
    // 注意：变换应用顺序是从右到左（先仿射，后非线性）
    // BOLD 已经和 T1w 对齐了（通过之前的配准），所以只需要应用 T1w→模板的变换
    std::string cmd = "\"" + antsPath + "\\antsApplyTransforms.exe\" "
        "--dimensionality 4 "  // BOLD 通常是 4D
        "--input \"" + boldImage + "\" "
        "--reference \"" + templateImage + "\" "
        "--transform \"" + warpTransform + "\" "  // ⭐ 先应用非线性形变
        "--transform \"" + affineTransform + "\" "  // ⭐ 再应用仿射
        "--output \"" + outputFile + "\" "
        "--interpolation Linear "
        "--verbose 0";  // 关闭详细输出以减少日志


    std::cout << "  BOLD: " << fs::path(boldImage).filename().string() << std::endl;
    std::cout << "  输出：" << fs::path(outputFile).filename().string() << std::endl;

    /*if (!executeCommand(cmd).empty()) {
        return false;
    }*/
    // 使用 CreateProcess 执行
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
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
        lastError = "无法启动 antsApplyTransforms (Error: " + std::to_string(GetLastError()) + ")";
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    // 等待完成
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        lastError = "应用变换失败 (退出码：" + std::to_string(exitCode) + ")";
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    // 验证输出
    if (!fs::exists(outputFile)) {
        lastError = "标准化后的 BOLD 文件未生成：" + outputFile;
        std::cerr << "  错误：" << lastError << std::endl;
        return false;
    }

    std::cout << "  ✓ 标准化完成" << std::endl;
    return true;
}

bool TemplateNormalizer::normalize(const NormalizeConfig& config) {
    verbose = config.verbose;  // ⭐ 从配置中获取 verbose 设置
    std::cout << "=== 空间标准化到模板空间 ===" << std::endl;
    std::cout << "使用 SyN 非线性配准" << std::endl;
    std::cout << std::endl;

    // 1. 验证输入
    std::cout << "[准备] 验证输入文件..." << std::endl;

    if (!fs::exists(config.t1wImage)) {
        lastError = "T1w 图像不存在：" + config.t1wImage;
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }
    std::cout << "  ✓ T1w: " << config.t1wImage << std::endl;

    if (!fs::exists(config.templateImage)) {
        lastError = "模板图像不存在：" + config.templateImage;
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }
    std::cout << "  ✓ 模板：" << config.templateImage << std::endl;

    if (!fs::exists(config.boldFolder)) {
        lastError = "BOLD 文件夹不存在：" + config.boldFolder;
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }
    std::cout << "  ✓ BOLD 文件夹：" << config.boldFolder << std::endl;

    // 确保输出目录存在
    if (!fs::exists(config.outputFolder)) {
        if (!fs::create_directories(config.outputFolder)) {
            lastError = "无法创建输出目录";
            std::cerr << "错误：" << lastError << std::endl;
            return false;
        }
    }
    std::cout << "  ✓ 输出目录：" << config.outputFolder << std::endl;

    // 2. T1w 到模板的配准
    std::cout << std::endl;
    // ⭐ 修复：使用 fs::path 构建路径（自动处理斜杠）
    fs::path outputPath = fs::path(config.outputFolder) / config.outputPrefix;
    std::string outputPrefix = outputPath.string();

    // ⭐ 规范化路径（统一为反斜杠）
    std::replace(outputPrefix.begin(), outputPrefix.end(), '/', '\\');
    std::string affineTransform, warpTransform;

    if (!registerT1ToTemplate(config.t1wImage,
        config.templateImage,
        outputPrefix,
        affineTransform,
        warpTransform)) {
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }

    // 3. 查找所有 BOLD 文件
    std::cout << std::endl;
    std::cout << "[2/2] 查找 BOLD 文件..." << std::endl;
    std::vector<std::string> boldFiles = findBoldFiles(config.boldFolder);

    if (boldFiles.empty()) {
        lastError = "未找到 BOLD 文件";
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }

    std::cout << "  找到 " << boldFiles.size() << " 个 BOLD 文件" << std::endl;

    // 4. 对每个 BOLD 文件应用变换
    std::cout << std::endl;
    std::cout << "应用变换到 BOLD 序列..." << std::endl;

    int successCount = 0;
    int failCount = 0;

    for (size_t i = 0; i < boldFiles.size(); ++i) {
        const std::string& boldFile = boldFiles[i];

        std::cout << "\n[" << (i + 1) << "/" << boldFiles.size() << "] ";
        std::cout << fs::path(boldFile).filename().string() << std::endl;

        // 构建输出文件名
        std::string outputName = config.outputPrefix + "_" +
            fs::path(boldFile).stem().string() +
            "_MNI.nii.gz";
        // ⭐ 修复：使用 fs::path 构建输出路径
        fs::path outputFile = fs::path(config.outputFolder) / outputName;
        std::string outputFilePath = outputFile.string();
        std::replace(outputFilePath.begin(), outputFilePath.end(), '/', '\\');

        if (applyTransformToBold(boldFile,
            config.t1wImage,
            config.templateImage,
            affineTransform,
            warpTransform,
            outputFilePath)) {
            successCount++;
        }
        else {
            std::cerr << "  ✗ 失败：" << lastError << std::endl;
            failCount++;
        }
    }

    // 5. 总结
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "=== 空间标准化完成 ===" << std::endl;
    std::cout << "成功：" << successCount << "/" << boldFiles.size() << " 个 BOLD 文件" << std::endl;
    if (failCount > 0) {
        std::cout << "失败：" << failCount << " 个 BOLD 文件" << std::endl;
    }
    std::cout << "输出目录：" << config.outputFolder << std::endl;
    std::cout << "保存的变换文件：" << std::endl;
    std::cout << "  - 仿射矩阵：" << affineTransform << std::endl;
    std::cout << "  - 形变场：" << warpTransform << std::endl;
    std::cout << "========================================" << std::endl;

    return (failCount == 0);
}