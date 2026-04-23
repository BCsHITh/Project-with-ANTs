#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// 空间标准化配置
struct NormalizeConfig {
    std::string t1wImage;              // 平均T1w图像
    std::string templateImage;         // 标准模板
    std::string boldFolder;            // BOLD序列文件夹
    std::string outputFolder;          // 输出文件夹
    std::string outputPrefix;          // 输出前缀
    bool saveTransforms;               // 保存变换文件
    std::string regType;               // 配准类型（默认 SyN）
    bool verbose;                      // 详细输出

    NormalizeConfig()
        : saveTransforms(true)
        , regType("SyN")
        , verbose(false) {
    }
};

// 空间标准化处理器
class TemplateNormalizer {
public:
    TemplateNormalizer();

    // 执行空间标准化
    bool normalize(const NormalizeConfig& config);

    std::string getLastError() const { return lastError; }

private:
    std::string lastError;
    bool verbose;

    // 辅助函数
    bool registerT1ToTemplate(const std::string& t1w,
        const std::string& templateImg,
        const std::string& outputPrefix,
        std::string& affineTransform,
        std::string& warpTransform);

    bool applyTransformToBold(const std::string& boldImage,
        const std::string& t1wImage,
        const std::string& templateImage,
        const std::string& affineTransform,
        const std::string& warpTransform,
        const std::string& outputFile);

    std::vector<std::string> findBoldFiles(const fs::path& folder);
    std::string executeCommand(const std::string& cmd);
};