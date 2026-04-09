#include "dicom_manager.h"
#include <iostream>
#include <algorithm>

DICOMManager::DICOMManager() {}

bool DICOMManager::scanDirectory(const fs::path& folderPath) {
    clear();

    if (!fs::exists(folderPath)) {
        lastError = "This Folder does not exist " + folderPath.string();
        return false;
    }

    // 收集所有子文件夹
    std::vector<fs::path> subdirs;
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_directory()) {
            subdirs.push_back(entry.path());
        }
    }

    if (subdirs.empty()) {
        // 没有子文件夹，把当前文件夹作为一个系列
        size_t fileCount = std::distance(
            fs::directory_iterator(folderPath),
            fs::directory_iterator()
        );

        if (fileCount == 0) {
            lastError = "No File in folder";
            return false;
        }

        DICOMSeries series;
        series.sourceFolder = folderPath;
        series.imageCount = fileCount;
        extractSeriesInfo(folderPath, series);
        seriesList.push_back(series);
    }
    else {
        // 每个子文件夹作为一个系列
        for (const auto& subdir : subdirs) {
            size_t fileCount = std::distance(
                fs::directory_iterator(subdir),
                fs::directory_iterator()
            );

            if (fileCount > 0) {
                DICOMSeries series;
                series.sourceFolder = subdir;
                series.imageCount = fileCount;
                extractSeriesInfo(subdir, series);
                seriesList.push_back(series);
            }
        }
    }

    std::cout << "Scan completed! found totally " << seriesList.size() << " series" << std::endl;
    return !seriesList.empty();
}

void DICOMManager::extractSeriesInfo(const fs::path& folderPath, DICOMSeries& series) {
    // 从文件夹名提取信息
    series.seriesNumber = folderPath.filename().string();
    series.seriesDescription = folderPath.filename().string();

    // 简化：默认设置为 MR
    series.modality = "MR";
    series.patientName = "Unknown";
    series.patientID = "Unknown";

    // 获取当前日期
    auto now = std::time(nullptr);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d", std::localtime(&now));
    series.studyDate = buffer;
}

const std::vector<DICOMSeries>& DICOMManager::getSeriesList() const {
    return seriesList;
}

void DICOMManager::clear() {
    seriesList.clear();
    lastError.clear();
}

std::string DICOMManager::getLastError() const {
    return lastError;
}