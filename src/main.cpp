#include <iostream>
#include <string>
#include <filesystem>
#include "dicom_manager.h"
#include "dicom_converter.h"

namespace fs = std::filesystem;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <input DICOM folder> <output NIfTI folder>" << std::endl;
    std::cout << "Example: " << progName << " D:\\DICOM\\Study1 D:\\NIfTI\\Output" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== DICOM to NIfTI Converter (CLI) ===" << std::endl;
	std::cout << "Author: BCShi" << std::endl;
	std::cout << "version: build 0.0.1" << std::endl;
    std::cout << "dcm2niix: " << DCM2NIIX_EXE << std::endl;
    std::cout << "ANTs: " << ANTS_BIN_PATH << std::endl;
    std::cout << std::endl;

    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    fs::path inputFolder = argv[1];
    fs::path outputFolder = argv[2];

    if (!fs::exists(inputFolder)) {
        std::cerr << "Warning: specified folder does not exist! " << inputFolder << std::endl;
        return 1;
    }

    std::cout << "[1/2] Scaning DICOM folder..." << std::endl;
    DICOMManager manager;
    if (!manager.scanDirectory(inputFolder)) {
        std::cerr << "Warning: " << manager.getLastError() << std::endl;
        return 1;
    }

    const auto& seriesList = manager.getSeriesList();
    std::cout << "\nfound " << seriesList.size() << " DICOM series:" << std::endl;

    for (size_t i = 0; i < seriesList.size(); ++i) {
        const auto& s = seriesList[i];
        std::cout << "  [" << (i + 1) << "] "
            << s.getDisplayName()
            << " - " << s.imageCount << " dicom" << std::endl; //"张图像"
    }

    std::cout << "\n[2/2] Starting convertion..." << std::endl;
    Converter converter;

    size_t successCount = 0;
    for (size_t i = 0; i < seriesList.size(); ++i) {
        const auto& series = seriesList[i];

        std::cout << "\n[" << (i + 1) << "/" << seriesList.size() << "] "
            << "Conversion: " << series.getDisplayName() << std::endl;//"转换"

        bool success = converter.convertSeries(
            series,
            outputFolder,
            [](int percent, const std::string& msg) {
                std::cout << "  Progress: " << msg << std::endl;
            },
            [](const std::string& error) {
                std::cerr << "  Error：" << error << std::endl;
            }
        );

        if (success) {
            std::cout << "  Success" << std::endl;
            successCount++;
        }
        else {
            std::cerr << "  Failed: " << converter.getLastError() << std::endl;
        }
    }

    std::cout << "\n=== Conversion Complete ===" << std::endl;
    std::cout << "Success: " << successCount << "/" << seriesList.size() << std::endl;
    std::cout << "Output directory: " << outputFolder << std::endl;

    if (successCount > 0) {
        std::cout << "\nGenerated files: " << std::endl;
        for (const auto& entry : fs::directory_iterator(outputFolder)) {
            if (entry.path().extension() == ".gz" ||
                entry.path().extension().string() == ".nii") {
                std::cout << "  - " << entry.path().filename().string() << std::endl;
            }
        }
    }

    return (successCount == seriesList.size()) ? 0 : 1;
}