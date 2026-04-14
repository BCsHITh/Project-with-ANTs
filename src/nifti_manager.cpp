#include "nifti_manager.h"
#include <iostream>
#include <algorithm>

NiftiManager::NiftiManager() {}

bool NiftiManager::scanDirectory(const fs::path& folderPath) {
    clear();

    if (!fs::exists(folderPath)) {
        std::cerr << "文件夹不存在：" << folderPath << std::endl;
        return false;
    }

    // 递归查找所有 .nii 和 .nii.gz 文件
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();

            // 检查是否是 NIfTI 文件
            if (filename.find(".nii.gz") != std::string::npos ||
                filename.find(".nii") != std::string::npos) {

                NiftiInfo info;
                extractFileInfo(entry.path(), info);
                niftiList.push_back(info);
            }
        }
    }

    // 按文件名排序
    std::sort(niftiList.begin(), niftiList.end(),
        [](const NiftiInfo& a, const NiftiInfo& b) {
            return a.fileName < b.fileName;
        });

    std::cout << "扫描完成，找到 " << niftiList.size() << " 个 NIfTI 文件" << std::endl;
    return !niftiList.empty();
}

void NiftiManager::extractFileInfo(const fs::path& filePath, NiftiInfo& info) {
    info.filePath = filePath.string();
    info.fileName = filePath.filename().string();
    info.fileSize = fs::file_size(filePath);
    info.createTime = fs::last_write_time(filePath);

    // 从文件名提取信息（简化版）
    // 格式：MR_seriesNumber_timestamp.nii.gz
    std::string name = filePath.stem().string();
    if (name.find(".nii") != std::string::npos) {
        name = name.substr(0, name.find(".nii"));
    }

    // 分割文件名
    size_t pos1 = name.find('_');
    if (pos1 != std::string::npos) {
        info.modality = name.substr(0, pos1);

        size_t pos2 = name.find('_', pos1 + 1);
        if (pos2 != std::string::npos) {
            info.seriesNumber = name.substr(pos1 + 1, pos2 - pos1 - 1);
        }
    }

    // 从父文件夹名提取患者 ID（如果有的话）
    fs::path parentPath = filePath.parent_path();
    if (parentPath.has_filename()) {
        // 可以尝试从路径中提取更多信息
    }
}

const std::vector<NiftiInfo>& NiftiManager::getNiftiList() const {
    return niftiList;
}

std::map<std::string, std::vector<NiftiInfo>> NiftiManager::groupByPatient() const {
    std::map<std::string, std::vector<NiftiInfo>> groups;

    // 简化：按父文件夹名分组（假设每个患者一个文件夹）
    for (const auto& info : niftiList) {
        fs::path filePath = info.filePath;
        std::string patientID = filePath.parent_path().filename().string();
        groups[patientID].push_back(info);
    }

    return groups;
}

std::map<std::string, std::vector<NiftiInfo>> NiftiManager::groupByDate() const {
    std::map<std::string, std::vector<NiftiInfo>> groups;

    for (const auto& info : niftiList) {
        // 从文件名中提取日期（假设格式包含 timestamp）
        std::string date = "Unknown";
        size_t datePos = info.fileName.find("_20");
        if (datePos != std::string::npos && datePos + 8 <= info.fileName.length()) {
            date = info.fileName.substr(datePos + 1, 8);
        }
        groups[date].push_back(info);
    }

    return groups;
}

void NiftiManager::clear() {
    niftiList.clear();
}

size_t NiftiManager::getTotalSize() const {
    size_t total = 0;
    for (const auto& info : niftiList) {
        total += info.fileSize;
    }
    return total / (1024 * 1024);  // 转换为 MB
}