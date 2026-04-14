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




// 验证并获取文件夹路径



int main(int argc, char* argv[])
{
    setupConsole();

    std::cout << "=== DICOM to NIfTI Converter (CLI) ===" << std::endl;
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
    std::cout << std::endl;

    int mode = 1;
    if (argc < 3) {
        std::cout << "请输入模式: ";
        std::string modeInput;
        std::getline(std::cin, modeInput);
        if (!modeInput.empty()) {
            mode = std::stoi(modeInput);
        }
    }

    switch (mode)
    {
    case 1:
        return runSingleConversion(argc, argv);
    case 2:
        return runBatchMode();
    case 3:
        return runNiftiManager();
    case 4:
        return runRegistration();
    case 5:
		return runBatchRegistration();
    default:
        std::cout << "无效选项" << std::endl;
        return 1;
    }
}



