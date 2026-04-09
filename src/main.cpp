#include <iostream>
#include <string>
#include <filesystem>
#include "dicom_manager.h"
#include "dicom_converter.h"

namespace fs = std::filesystem;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <Input DICOM Folder> <Output NIfTI Folder>" << std::endl;
    std::cout << "      Or running in Interaction Mode" << std::endl;
    std::cout << "Example: " << progName << " D:\\DICOM\\Study1 D:\\NIfTI\\Output" << std::endl;
}

// 获取用户输入的路径
std::string getUserInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);

    // 移除首尾空格
    size_t start = input.find_first_not_of(" \t");
    size_t end = input.find_last_not_of(" \t");
    if (start == std::string::npos) return "";
    return input.substr(start, end - start + 1);
}

// 验证并获取文件夹路径
bool getFolderPath(const std::string& prompt, fs::path& outPath, bool mustExist = true) {
    while (true) {
        std::string input = getUserInput(prompt);

        if (input.empty()) {
            std::cout << "  Forbid empty input, please try angin! " << std::endl;
            continue;
        }

        // 处理引号（用户可能拖拽文件到控制台）
        if (input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }

        fs::path path(input);

        if (mustExist && !fs::exists(path)) {
            std::cout << "  Path does not exist" << path << std::endl;
            std::cout << "  Please input again! " << std::endl;
            continue;
        }

        if (mustExist && !fs::is_directory(path)) {
            std::cout << "  Invalid folder path" << path << std::endl;
            std::cout << "  Please input again! " << std::endl;
            continue;
        }

        outPath = path;
        return true;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== DICOM to NIfTI Converter (CLI) ===" << std::endl;
    std::cout << "Build Version: 0.0.2" << std::endl;
    std::cout << "dcm2niix: " << DCM2NIIX_EXE << std::endl;
    std::cout << "ANTs: " << ANTS_BIN_PATH << std::endl;
    std::cout << std::endl;

    fs::path inputFolder;
    fs::path outputFolder;

    // 检查命令行参数
    if (argc >= 3) {
        // 使用命令行参数
        inputFolder = argv[1];
        outputFolder = argv[2];

        if (!fs::exists(inputFolder)) {
            std::cerr << "Error: Folder does not exist! " << inputFolder << std::endl;
            return 1;
        }
    }
    else {
        // 进入交互模式
        std::cout << "=== Interaction Mode ===" << std::endl;
        std::cout << "Tips: Drag your folder into the window, and press <Enter>" << std::endl;
        std::cout << std::endl;

        // 获取输入文件夹
        if (!getFolderPath("Please input DICOM folder path: ", inputFolder, true)) {
            std::cerr << "Error：Input folder path is unavailable! " << std::endl;
            return 1;
        }

        // 获取输出文件夹
        std::cout << std::endl;
        std::cout << "NIfTI files will be save in Output folder" << std::endl;
        if (!getFolderPath("Please input Output folder path: ", outputFolder, false)) {
            std::cerr << "Error：Output folder path is unavailable!" << std::endl;
            return 1;
        }

        // 如果输出文件夹不存在，询问是否创建
        if (!fs::exists(outputFolder)) {
            std::cout << "Output folder does not exist, create it? (y/n): ";
            std::string confirm;
            std::getline(std::cin, confirm);

            if (confirm.empty() || confirm[0] == 'y' || confirm[0] == 'Y') {
                if (!fs::create_directories(outputFolder)) {
                    std::cerr << "错误：无法创建输出文件夹" << std::endl;
                    return 1;
                }
                std::cout << "已创建文件夹：" << outputFolder << std::endl;
            }
            else {
                std::cout << "操作已取消" << std::endl;
                return 0;
            }
        }

        std::cout << std::endl;
    }

    // 显示配置
    std::cout << "=== Configuration ===" << std::endl;
    std::cout << "Input folder: " << inputFolder << std::endl;
    std::cout << "Output folder: " << outputFolder << std::endl;
    std::cout << std::endl;

    // 确认开始
    if (argc < 3) {
        std::cout << "Press <Enter> to start conversion, or enter <q> to exit...";
        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm == "q" || confirm == "Q") {
            std::cout << "Canceled" << std::endl;
            return 0;
        }
    }

    // 1. 扫描 DICOM 文件
    std::cout << "[1/2] Scanning DICOM folder..." << std::endl;
    DICOMManager manager;
    if (!manager.scanDirectory(inputFolder)) {
        std::cerr << "Error: " << manager.getLastError() << std::endl;
        return 1;
    }

    const auto& seriesList = manager.getSeriesList();
    if (seriesList.empty()) {
        std::cerr << "Error: No DICOM series found" << std::endl;
        return 1;
    }

    std::cout << "\nfound " << seriesList.size() << " DICOM series:" << std::endl;

    for (size_t i = 0; i < seriesList.size(); ++i) {
        const auto& s = seriesList[i];
        std::cout << "  [" << (i + 1) << "] "
            << s.getDisplayName()
            << " - " << s.imageCount << " Pictures" << std::endl;
    }

    // 2. 转换每个系列
    std::cout << "\n[2/2] Start conversion..." << std::endl;
    Converter converter;

    size_t successCount = 0;
    for (size_t i = 0; i < seriesList.size(); ++i) {
        const auto& series = seriesList[i];

        std::cout << "\n[" << (i + 1) << "/" << seriesList.size() << "] "
            << "Conversion: " << series.getDisplayName() << std::endl;

        bool success = converter.convertSeries(
            series,
            outputFolder,
            [](int percent, const std::string& msg) {
                std::cout << "  Progress: " << msg << std::endl;
            },
            [](const std::string& error) {
                std::cerr << "  Error: " << error << std::endl;
            }
        );

        if (success) {
            std::cout << "  Conversion Successful! " << std::endl;
            successCount++;
        }
        else {
            std::cerr << "  Failed to conversion: " << converter.getLastError() << std::endl;
        }
    }

    // 3. 总结
    std::cout << "\n=== Conversion complete ===" << std::endl;
    std::cout << "Success: " << successCount << "/" << seriesList.size() << std::endl;
    std::cout << "Output Folder: " << outputFolder << std::endl;

    if (successCount > 0) {
        std::cout << "\nCreated files:" << std::endl;
        int fileCount = 0;
        for (const auto& entry : fs::directory_iterator(outputFolder)) {
            std::string ext = entry.path().extension().string();
            if (ext == ".gz" || ext == ".nii") {
                std::cout << "  - " << entry.path().filename().string() << std::endl;
                fileCount++;
            }
        }
        if (fileCount == 0) {
            std::cout << "  (no .nii or .nii.gz files found)" << std::endl;
        }
    }

    // 4. 交互模式下等待用户
    if (argc < 3) {
        std::cout << "\nPress any key to exit...";
        std::cin.get();
    }

    return (successCount == seriesList.size()) ? 0 : 1;
}