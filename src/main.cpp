#include <iostream>
#include <string>
#include <filesystem>
#include "dicom_manager.h"
#include "dicom_converter.h"

namespace fs = std::filesystem;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <Input DICOM Folder> <Output NIfTI Folder>" << std::endl;
    std::cout << "      Or enter Interaction Mode" << std::endl;
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
            std::cout << "  Input Empty, please try again! " << std::endl;
            continue;
        }

        // 处理引号（用户可能拖拽文件到控制台）
        if (input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }

        fs::path path(input);

        if (mustExist && !fs::exists(path)) {
            std::cout << "  The Path does not exist: " << path << std::endl;
            std::cout << "  Please try again! " << std::endl;
            continue;
        }

        if (mustExist && !fs::is_directory(path)) {
            std::cout << "  The Path is not unavailable" << path << std::endl;
            std::cout << "  Please try again! " << std::endl;
            continue;
        }

        outPath = path;
        return true;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== DICOM to NIfTI Converter (CLI) ===" << std::endl;
    std::cout << "Build Version：0.0.1" << std::endl;
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
            std::cerr << "Error: Input folder does not exist" << inputFolder << std::endl;
            return 1;
        }
    }
    else {
        // 进入交互模式
        std::cout << "=== Interaction Mode ===" << std::endl;
        std::cout << "Tip: You can directly drag the folder into the window and press Enter" << std::endl;
        std::cout << std::endl;

        // 获取输入文件夹
        if (!getFolderPath("Please enter DICOM Folder Path:  ", inputFolder, true)) {
            std::cerr << "Error: Unable to obtain input folder path" << std::endl;
            return 1;
        }

        // 获取输出文件夹
        std::cout << std::endl;
        std::cout << "The output folder will be used to save the converted NIfTI file" << std::endl;
        if (!getFolderPath("Please enter output Folder Path: ", outputFolder, false)) {
            std::cerr << "Error: Unable to obtain input folder path" << std::endl;
            return 1;
        }

        // 如果输出文件夹不存在，询问是否创建
        if (!fs::exists(outputFolder)) {
            std::cout << "The output folder does not exist, do you want to create it? (y/n): ";
            std::string confirm;
            std::getline(std::cin, confirm);

            if (confirm.empty() || confirm[0] == 'y' || confirm[0] == 'Y') {
                if (!fs::create_directories(outputFolder)) {
                    std::cerr << "Error: Unable to create output folder" << std::endl;
                    return 1;
                }
                std::cout << "Folder is created" << outputFolder << std::endl;
            }
            else {
                std::cout << "Operation canceled" << std::endl;
                return 0;
            }
        }

        std::cout << std::endl;
    }

    // 显示配置
    std::cout << "=== Configuration ===" << std::endl;
    std::cout << "Input folder:  " << inputFolder << std::endl;
    std::cout << "Output folder:  " << outputFolder << std::endl;
    std::cout << std::endl;

    // 确认开始
    if (argc < 3) {
        std::cout << "Press enter to start the conversion, or q to exit ...";
        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm == "q" || confirm == "Q") {
            std::cout << "Canceled" << std::endl;
            return 0;
        }
    }

    // 1. 扫描 DICOM 文件
    std::cout << "[1/2] Scaning DICOM folder ..." << std::endl;
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
    std::cout << "\n[2/2] Start Conversion" << std::endl;
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
            std::cout << "  Conversion successful" << std::endl;
            successCount++;
        }
        else {
            std::cerr << "  Failed!" << converter.getLastError() << std::endl;
        }
    }

    // 3. 总结
    std::cout << "\n=== Conversion Completed ===" << std::endl;
    std::cout << "Success: " << successCount << "/" << seriesList.size() << std::endl;
    std::cout << "Output directory: " << outputFolder << std::endl;

    if (successCount > 0) {
        std::cout << "\nGenerated files: " << std::endl;
        int fileCount = 0;
        for (const auto& entry : fs::directory_iterator(outputFolder)) {
            std::string ext = entry.path().extension().string();
            if (ext == ".gz" || ext == ".nii") {
                std::cout << "  - " << entry.path().filename().string() << std::endl;
                fileCount++;
            }
        }
        if (fileCount == 0) {
            std::cout << "  (.nii or .nii.gz file not found)" << std::endl;
        }
    }

    // 4. 交互模式下等待用户
    if (argc < 3) {
        std::cout << "\nPress any key to exit ...";
        std::cin.get();
    }

    return (successCount == seriesList.size()) ? 0 : 1;
}