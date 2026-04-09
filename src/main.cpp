#include <iostream>
#include <string>
#include <filesystem>
#include "dicom_manager.h"
#include "dicom_converter.h"

void setupConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

namespace fs = std::filesystem;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <Input DICOM Folder> <Output NIfTI Folder>" << std::endl;
    std::cout << "      Or running in Interaction Mode" << std::endl;
    std::cout << "Example: " << progName << " D:\\DICOM\\Study1 D:\\NIfTI\\Output" << std::endl;
}

int runBatchMode();


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


int main(int argc, char* argv[])
{
    setupConsole();

    std::cout << "=== DICOM to NIfTI Converter (CLI) ===" << std::endl;
    std::cout << "Build Version: 0.0.4" << std::endl;
    std::cout << "dcm2niix: " << DCM2NIIX_EXE << std::endl;
    std::cout << "ANTs: " << ANTS_BIN_PATH << std::endl;
    std::cout << std::endl;

    std::cout << "请选择模式：" << std::endl;
    std::cout << "  1. 单个文件夹转换" << std::endl;
    std::cout << "  2. 批量转换（递归处理所有子文件夹）" << std::endl;
    std::cout << std::endl;

    int mode = 1;
    if (argc < 3) {
        std::cout << "请输入模式 (1 或 2，默认 1): ";
        std::string modeInput;
        std::getline(std::cin, modeInput);
        if (!modeInput.empty() && modeInput[0] == '2') {
            mode = 2;
        }
    }

    fs::path inputFolder;
    fs::path outputFolder;

    if (mode == 1) {
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
                std::string filename = entry.path().filename().string();
                std::string ext = entry.path().extension().string();
                bool isNifti = (ext == ".gz" && filename.find(".nii.gz") != std::string::npos) ||
                    (ext == ".nii");

                if (isNifti) {
                    std::cout << "  - " << filename << std::endl;
                    fileCount++;
                }

            }
            if (fileCount == 0) {
                // 尝试显示所有文件用于调试
                std::cout << "  (no .nii file found, show all files to debug): " << std::endl;
                for (const auto& entry : fs::directory_iterator(outputFolder)) {
                    std::cout << "  - " << entry.path().filename().string() << std::endl;
                }
            }
        }

        // 4. 交互模式下等待用户
        if (argc < 3) {
            std::cout << "\nPress any key to exit...";
            std::cin.get();
        }

        return (successCount == seriesList.size()) ? 0 : 1;

    }
    else {
        // ⭐ 新增：批量模式
        return runBatchMode();
    }
}

    // ⭐ 添加：批量转换函数
    int runBatchMode() {
        std::cout << "=== 批量转换模式 ===" << std::endl;
        std::cout << "提示：可以拖拽根文件夹到窗口，然后按回车" << std::endl;
        std::cout << std::endl;

        fs::path rootFolder;
        fs::path outputRoot;

        // 1. 获取根文件夹
        std::cout << "请输入包含所有DICOM文件夹的根目录: ";
        std::string input;
        std::getline(std::cin, input);

        // 处理引号
        if (input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }

        rootFolder = input;

        if (!fs::exists(rootFolder)) {
            std::cerr << "错误：根目录不存在：" << rootFolder << std::endl;
            return 1;
        }

        // 2. 获取输出根目录
        std::cout << std::endl;
        std::cout << "请输入输出根目录: ";
        std::getline(std::cin, input);

        if (input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }

        outputRoot = input;

        if (!fs::exists(outputRoot)) {
            std::cout << "输出目录不存在，是否创建？(y/n): ";
            std::string confirm;
            std::getline(std::cin, confirm);

            if (confirm.empty() || confirm[0] == 'y' || confirm[0] == 'Y') {
                if (!fs::create_directories(outputRoot)) {
                    std::cerr << "错误：无法创建输出目录" << std::endl;
                    return 1;
                }
            }
            else {
                std::cout << "操作已取消" << std::endl;
                return 0;
            }
        }

        std::cout << std::endl;
        std::cout << "=== 配置 ===" << std::endl;
        std::cout << "根目录: " << rootFolder << std::endl;
        std::cout << "输出目录: " << outputRoot << std::endl;
        std::cout << std::endl;

        // 3. 递归查找所有文件夹
        std::cout << "[1/2] 扫描文件夹..." << std::endl;
        std::vector<fs::path> dicomFolders;

        for (const auto& entry : fs::recursive_directory_iterator(rootFolder)) {
            if (entry.is_directory()) {
                // 检查文件夹中是否有文件
                size_t fileCount = std::distance(
                    fs::directory_iterator(entry.path()),
                    fs::directory_iterator()
                );

                if (fileCount > 0) {
                    dicomFolders.push_back(entry.path());
                }
            }
        }

        std::cout << "找到 " << dicomFolders.size() << " 个包含文件的文件夹" << std::endl;

        if (dicomFolders.empty()) {
            std::cerr << "错误：未找到任何包含文件的文件夹" << std::endl;
            return 1;
        }

        // 4. 确认开始
        std::cout << std::endl;
        std::cout << "按回车键开始转换，或输入 q 退出...";
        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm == "q" || confirm == "Q") {
            std::cout << "已取消" << std::endl;
            return 0;
        }

        // 5. 批量转换
        std::cout << std::endl;
        std::cout << "[2/2] 开始批量转换..." << std::endl;
        std::cout << "========================================" << std::endl;

        Converter converter;
        size_t successCount = 0;
        size_t failCount = 0;
        size_t totalCount = dicomFolders.size();

        auto startTime = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < dicomFolders.size(); ++i) {
            const auto& folder = dicomFolders[i];

            // 显示进度
            std::cout << std::endl;
            std::cout << "[" << (i + 1) << "/" << totalCount << "] ";
            std::cout << folder.filename().string() << std::endl;

            // 计算相对路径，用于保持目录结构
            fs::path relativePath = fs::relative(folder, rootFolder);
            fs::path targetOutputDir = outputRoot / relativePath;

            // 扫描文件夹
            DICOMManager manager;
            if (!manager.scanDirectory(folder)) {
                std::cout << "  跳过：" << manager.getLastError() << std::endl;
                failCount++;
                continue;
            }

            const auto& seriesList = manager.getSeriesList();

            if (seriesList.empty()) {
                std::cout << "  跳过：未找到DICOM系列" << std::endl;
                failCount++;
                continue;
            }

            std::cout << "  找到 " << seriesList.size() << " 个系列" << std::endl;

            // 转换每个系列
            int folderSuccess = 0;
            for (const auto& series : seriesList) {
                bool success = converter.convertSeries(
                    series,
                    targetOutputDir,
                    [](int percent, const std::string& msg) {
                        // 静默模式，不显示进度
                    },
                    [](const std::string& error) {
                        std::cerr << "  错误：" << error << std::endl;
                    }
                );

                if (success) {
                    folderSuccess++;
                }
            }

            if (folderSuccess > 0) {
                std::cout << "  ✓ 成功转换 " << folderSuccess << "/" << seriesList.size() << " 个系列" << std::endl;
                successCount++;
            }
            else {
                std::cout << "  ✗ 转换失败" << std::endl;
                failCount++;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

        // 6. 总结
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "=== 批量转换完成 ===" << std::endl;
        std::cout << "成功：" << successCount << "/" << totalCount << " 个文件夹" << std::endl;
        std::cout << "失败：" << failCount << " 个文件夹" << std::endl;
        std::cout << "输出目录：" << outputRoot << std::endl;
        std::cout << "总耗时：" << duration.count() << " 秒" << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << "\n按任意键退出...";
        std::cin.get();

        return (failCount == 0) ? 0 : 1;
    }