#include <iostream>
#include <string>
#include <filesystem>
#include "dicom_manager.h"
#include "dicom_converter.h"
#include "core.h"

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


void showMenu() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "=== DICOM to NIfTI Converter (CLI)   ===\n";
    std::cout << "=== Build Version: 0.0.7             ===\n";
    std::cout << "========================================\n";
    std::cout << "\n请选择功能：\n";
    std::cout << "  1. DICOM 转 NIfTI（单个文件夹）\n";
    std::cout << "  2. DICOM 转 NIfTI（批量转换）\n";
    std::cout << "  3. 查看已转换的 NIfTI 文件\n";
    std::cout << "  4. 图像配准（单张，自定义输出）\n";
    std::cout << "  5. 图像配准（批量，多张配准到同一基准）\n";
    std::cout << "  6. 图像平均化（迭代配准 + 平均）\n";
    std::cout << "  7. 空间标准化到模板（T1w + BOLD）" << std::endl;  // ⭐ 新增
    std::cout << "  0. 退出程序\n";
    std::cout << "\n";
}


// 验证并获取文件夹路径



int main(int argc, char* argv[])
{
    setupConsole();

    /*std::cout << "=== DICOM to NIfTI Converter (CLI) ===" << std::endl;
    std::cout << "Build Version: 0.0.5" << std::endl;
    std::cout << "dcm2niix: " << DCM2NIIX_EXE << std::endl;
    std::cout << "ANTs: " << ANTS_BIN_PATH << std::endl;
    std::cout << std::endl;

    std::cout << "请选择模式：" << std::endl;
    std::cout << "  1. DICOM 转 NIfTI（单个文件夹）" << std::endl;
    std::cout << "  2. DICOM 转 NIfTI（批量转换）" << std::endl;
    std::cout << "  3. 查看已转换的 NIfTI 文件" << std::endl;
    std::cout << "  4. 图像配准（刚性/仿射）" << std::endl;
    std::cout << "  5. 图像配准（批量，多张配准到同一基准）" << std::endl;
    std::cout << "  6. 图像平均化（迭代配准+平均）" << std::endl;
    std::cout << std::endl;*/

    /*int mode = 1;
    if (argc < 3) {
        std::cout << "请输入模式: ";
        std::string modeInput;
        std::getline(std::cin, modeInput);
        if (!modeInput.empty()) {
            mode = std::stoi(modeInput);
        }
    }*/
    while (true) {
        showMenu();

        int mode = 0;

        // 如果有命令行参数，直接使用（仅第一次有效）
        if (argc >= 2) {
            try {
                mode = std::stoi(argv[1]);
                // 使用过一次后清除参数，避免死循环
                argc = 1;
            }
            catch (...) {
                mode = 0;
            }
        }
        else {
            // 交互模式：等待用户输入
            std::cout << "请输入功能编号 (0-7，默认 1): ";
            std::string modeInput;
            std::getline(std::cin, modeInput);

            if (!modeInput.empty()) {
                try {
                    mode = std::stoi(modeInput);
                }
                catch (...) {
                    mode = 0;
                }
            }
        }
        int result = 0;
        switch (mode)
        {
            case 1:
                result = runSingleConversion(argc, argv); break;
            case 2:
                result = runBatchMode(); break;
            case 3:
                result = runNiftiManager(); break;
            case 4:
                result = runRegistration(); break;
            case 5:
                result = runBatchRegistration(); break;
            case 6:  // ⭐ 新增
                result = runImageAverage(); break;
            case 7:  // ⭐ 新增
                result = runNormalizeToTemplate(); break;
            case 0:
                return 0;
            default:
                std::cout << "无效选项" << std::endl;
                continue;
        }
        std::cout << "\n----------------------------------------\n";
        std::cout << "按回车键返回主菜单...";
        std::cin.get(); 

    }
    return 0;
}



