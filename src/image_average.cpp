#include "image_average.h"
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <vector>

ImageAverager::ImageAverager() {}

std::string ImageAverager::executeCommand(const std::string& cmd) {
    std::cout << "  执行：" << cmd << std::endl;

    int result = std::system(cmd.c_str());

    if (result != 0) {
        lastError = "命令执行失败 (退出码：" + std::to_string(result) + ")";
        return lastError;
    }

    return "";
}

std::vector<std::string> ImageAverager::findNiftiFiles(const fs::path& folder) {
    std::vector<std::string> files;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find(".nii.gz") != std::string::npos ||
                filename.find(".nii") != std::string::npos) {
                // 排除平均图像
                if (filename.find("average") == std::string::npos) {
                    files.push_back(entry.path().string());
                }
            }
        }
    }

    // 排序以保证一致性
    std::sort(files.begin(), files.end());

    return files;
}

bool ImageAverager::registerToReference(const std::string& fixed,
    const std::string& moving,
    const std::string& outputPrefix,
    const std::string& transformType) {
    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        return false;
    }

    std::string cmd = "\"" + antsPath + "\\antsRegistration.exe\" "
        "--dimensionality 3 "
        "--float 0 "
        "--output [" + outputPrefix + "," + outputPrefix + "_Warped.nii.gz] "
        "--interpolation Linear "
        "--winsorize-image-intensities [0.005,0.995] "
        "--use-histogram-matching 0 "
        "--initial-moving-transform [" + fixed + "," + moving + ",1] "
        "--transform " + transformType + "[0.1] "
        "--metric MI[" + fixed + "," + moving + ",1,32,Regular,0.25] "
        "--convergence [1000x500x250x100,1e-6,10] "
        "--shrink-factors 8x4x2x1 "
        "--smoothing-sigmas 3x2x1x0vox";

    return executeCommand(cmd).empty();
}

bool ImageAverager::averageImages(const std::vector<std::string>& inputImages,
    const std::string& outputFile) {
    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        return false;
    }

    // 构建输入文件列表
    std::string inputList;
    for (size_t i = 0; i < inputImages.size(); ++i) {
        inputList += inputImages[i];
        if (i < inputImages.size() - 1) {
            inputList += " ";
        }
    }

    std::string cmd = "\"" + antsPath + "\\AverageImages.exe\" 3 " + outputFile + " 0 " + inputList;

    return executeCommand(cmd).empty();
}

bool ImageAverager::applyTransform(const std::string& reference,
    const std::string& moving,
    const std::string& transformFile,
    const std::string& outputFile,
    InterpolationType interpType) {
    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        return false;
    }
    std::string interpStr = interpolationToString(interpType);

    std::string cmd = "\"" + antsPath + "\\antsApplyTransforms.exe\" "
        "--dimensionality 3 "
        "--input " + moving + " "
        "--reference " + reference + " "
        "--transform " + transformFile + " "
        "--output " + outputFile + " "
        "--interpolation " + interpStr;

    return executeCommand(cmd).empty();
}

bool ImageAverager::denoiseImage(const std::string& input,
    const std::string& output) {
    std::string antsPath = ANTS_BIN_PATH;
    if (antsPath.empty()) {
        lastError = "ANTs 路径未配置";
        return false;
    }

    std::string cmd = "\"" + antsPath + "\\DenoiseImage.exe\" " + input + " " + output;

    return executeCommand(cmd).empty();
}

bool ImageAverager::average(const AverageConfig& config) {
    std::cout << "=== 图像平均化处理 ===" << std::endl;
    std::cout << "输入目录：" << config.inputFolder << std::endl;
    std::cout << "输出目录：" << config.outputFolder << std::endl;
    std::cout << "进行配准：" << (config.enableRegistration ? "是" : "否") << std::endl;  // ⭐
    std::cout << "去噪：" << (config.enableDenoise ? "是" : "否") << std::endl;
    std::cout << std::endl;

    // 1. 查找所有 NIfTI 文件
    std::cout << "[1/" << (config.enableRegistration ? config.maxIterations : 1) << "] "
        << "扫描 NIfTI 文件..." << std::endl;
    std::vector<std::string> imageFiles = findNiftiFiles(config.inputFolder);

    if (imageFiles.empty()) {
        lastError = "未找到 NIfTI 文件";
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }

    std::cout << "  找到 " << imageFiles.size() << " 个图像文件" << std::endl;

    if (imageFiles.size() < 2) {
        lastError = "至少需要 2 个图像文件进行平均";
        std::cerr << "错误：" << lastError << std::endl;
        return false;
    }

    // 确保输出目录存在
    if (!fs::exists(config.outputFolder)) {
        if (!fs::create_directories(config.outputFolder)) {
            lastError = "无法创建输出目录";
            return false;
        }
    }

    std::vector<std::string> imagesToAverage;

    // ⭐ 根据是否启用配准选择不同流程
    if (!config.enableRegistration) {
        // === 模式 1：直接平均（已配准的图像）===
        std::cout << std::endl;
        std::cout << "[1/1] 直接平均已配准的图像..." << std::endl;

        // 使用所有找到的图像
        imagesToAverage = imageFiles;

        std::cout << "  将平均 " << imagesToAverage.size() << " 个图像" << std::endl;

        // 计算平均图像
        std::string averageFile = config.outputFolder + "/" + config.outputPrefix +
            "_average.nii.gz";

        if (!averageImages(imagesToAverage, averageFile)) {
            std::cerr << "  错误：平均图像生成失败：" << lastError << std::endl;
            return false;
        }

        std::cout << "  ✓ 平均图像已保存：" << averageFile << std::endl;

        // 可选去噪
        if (config.enableDenoise) {
            std::cout << "  对平均图像去噪..." << std::endl;
            std::string dnFile = config.outputFolder + "/" + config.outputPrefix +
                "_average_dn.nii.gz";
            if (denoiseImage(averageFile, dnFile)) {
                std::cout << "  ✓ 去噪图像已保存：" << dnFile << std::endl;
            }
            else {
                std::cerr << "  警告：去噪失败：" << lastError << std::endl;
            }
        }

        std::cout << std::endl;
        std::cout << "=== 图像平均化完成 ===" << std::endl;
        std::cout << "最终平均图像：" << averageFile << std::endl;
        std::cout << "处理图像数：" << imageFiles.size() << std::endl;

        return true;
    }

    // === 模式 2：迭代配准 + 平均（未配准的图像）===
    std::string referenceImage = imageFiles[0];
    std::vector<std::string> warpedImages;

    // 迭代配准和平均
    for (int iteration = 0; iteration < config.maxIterations; ++iteration) {
        std::cout << std::endl;
        std::cout << "========== 迭代 " << (iteration + 1) << " ==========" << std::endl;

        std::vector<std::string> currentImages;
        std::string currentReference;

        if (iteration == 0) {
            // 第一轮：配准到第一张图像
            std::cout << "[" << (iteration + 1) << "/" << config.maxIterations << "] "
                << "配准到参考图像..." << std::endl;
            currentReference = referenceImage;

            // 第一张图像本身
            currentImages.push_back(referenceImage);

            // 配准其他图像
            for (size_t i = 1; i < imageFiles.size(); ++i) {
                std::cout << "  [" << i << "/" << (imageFiles.size() - 1) << "] "
                    << fs::path(imageFiles[i]).filename().string() << "... ";

                std::string outputPrefix = config.outputFolder + "/iter" +
                    std::to_string(iteration) + "_" +
                    std::to_string(i);

                bool success = registerToReference(
                    currentReference,
                    imageFiles[i],
                    outputPrefix,
                    config.regType1
                );

                if (success) {
                    std::string warpedFile = outputPrefix + "_Warped.nii.gz";
                    if (fs::exists(warpedFile)) {
                        currentImages.push_back(warpedFile);
                        warpedImages.push_back(warpedFile);
                        std::cout << "✓" << std::endl;
                    }
                    else {
                        std::cout << "✗ (未生成输出文件)" << std::endl;
                    }
                }
                else {
                    std::cout << "✗ (" << lastError << ")" << std::endl;
                }
            }
        }
        else {
            // 后续轮次：配准到平均图像
            std::cout << "[" << (iteration + 1) << "/" << config.maxIterations << "] "
                << "配准到平均图像..." << std::endl;

            // 上一轮的平均图像
            currentReference = config.outputFolder + "/average_iter" +
                std::to_string(iteration - 1) + ".nii.gz";

            if (!fs::exists(currentReference)) {
                lastError = "上一轮平均图像不存在";
                return false;
            }

            // 重新配准所有原始图像到平均图像
            for (size_t i = 0; i < imageFiles.size(); ++i) {
                std::cout << "  [" << (i + 1) << "/" << imageFiles.size() << "] "
                    << fs::path(imageFiles[i]).filename().string() << "... ";

                std::string outputPrefix = config.outputFolder + "/iter" +
                    std::to_string(iteration) + "_" +
                    std::to_string(i);

                bool success = registerToReference(
                    currentReference,
                    imageFiles[i],
                    outputPrefix,
                    config.regType2
                );

                if (success) {
                    std::string warpedFile = outputPrefix + "_Warped.nii.gz";
                    if (fs::exists(warpedFile)) {
                        currentImages.push_back(warpedFile);
                        std::cout << "✓" << std::endl;
                    }
                    else {
                        std::cout << "✗ (未生成输出文件)" << std::endl;
                    }
                }
                else {
                    std::cout << "✗ (" << lastError << ")" << std::endl;
                }
            }
        }

        // 计算平均图像
        std::cout << std::endl;
        std::cout << "  计算平均图像..." << std::endl;

        std::string averageFile = config.outputFolder + "/average_iter" +
            std::to_string(iteration) + ".nii.gz";

        if (!averageImages(currentImages, averageFile)) {
            std::cerr << "  警告：平均图像生成失败：" << lastError << std::endl;
        }
        else {
            std::cout << "  ✓ 平均图像已保存：" << averageFile << std::endl;
        }
    }

    // 最终处理
    std::cout << std::endl;
    std::cout << "========== 最终处理 ==========" << std::endl;

    std::string finalAverage = config.outputFolder + "/" + config.outputPrefix +
        "_average.nii.gz";

    // 使用最后一轮的平均图像
    std::string lastAverage = config.outputFolder + "/average_iter" +
        std::to_string(config.maxIterations - 1) + ".nii.gz";

    if (fs::exists(lastAverage)) {
        // 可选去噪
        if (config.enableDenoise) {
            std::cout << "对平均图像去噪..." << std::endl;
            std::string dnFile = config.outputFolder + "/" + config.outputPrefix +
                "_average_dn.nii.gz";
            if (denoiseImage(lastAverage, dnFile)) {
                std::cout << "  ✓ 去噪图像已保存：" << dnFile << std::endl;
                finalAverage = dnFile;
            }
            else {
                std::cerr << "  警告：去噪失败：" << lastError << std::endl;
            }
        }
        else {
            // 直接复制
            fs::copy_file(lastAverage, finalAverage, fs::copy_options::overwrite_existing);
        }

        std::cout << std::endl;
        std::cout << "=== 图像平均化完成 ===" << std::endl;
        std::cout << "最终平均图像：" << finalAverage << std::endl;
        std::cout << "处理图像数：" << imageFiles.size() << std::endl;
        std::cout << "配准轮次：" << config.maxIterations << std::endl;

        return true;
    }
    else {
        lastError = "最终平均图像不存在";
        return false;
    }
}
