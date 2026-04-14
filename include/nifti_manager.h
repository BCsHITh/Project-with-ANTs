#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

// NIfTI 文件信息
struct NiftiInfo {
    std::string filePath;           // 完整路径
    std::string fileName;           // 文件名
    std::string seriesNumber;       // 系列号
    std::string modality;           // 模态
    std::string patientID;          // 患者 ID
    std::string studyDate;          // 检查日期
    fs::file_time_type createTime;  // 创建时间
    size_t fileSize;                // 文件大小 (字节)

    NiftiInfo() : fileSize(0) {}
};

// NIfTI 文件管理器
class NiftiManager {
public:
    NiftiManager();

    // 扫描输出文件夹中的所有 NIfTI 文件
    bool scanDirectory(const fs::path& folderPath);

    // 获取所有文件
    const std::vector<NiftiInfo>& getNiftiList() const;

    // 按患者 ID 分组
    std::map<std::string, std::vector<NiftiInfo>> groupByPatient() const;

    // 按检查日期分组
    std::map<std::string, std::vector<NiftiInfo>> groupByDate() const;

    // 清空数据
    void clear();

    // 获取统计信息
    size_t getTotalCount() const { return niftiList.size(); }
    size_t getTotalSize() const;  // 总大小 (MB)

private:
    std::vector<NiftiInfo> niftiList;

    void extractFileInfo(const fs::path& filePath, NiftiInfo& info);
};