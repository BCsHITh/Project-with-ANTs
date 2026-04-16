#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// 图像平均配置
struct AverageConfig {
    std::string inputFolder;           // 输入文件夹
    std::string outputFolder;          // 输出文件夹
    std::string outputPrefix;          // 输出前缀
    bool enableDenoise;                // 是否去噪
    bool enableRegistration;           // 是否配准
    std::string regType1;              // 第一轮配准类型
    std::string regType2;              // 第二轮配准类型
    int maxIterations;                 // 最大迭代次数

    AverageConfig()
        : enableDenoise(false)
        , enableRegistration(true)
        , regType1("Rigid")
        , regType2("Similarity")
        , maxIterations(2) {
    }
};

// 图像平均处理器
class ImageAverager {
public:
    ImageAverager();

    // 执行图像平均
    bool average(const AverageConfig& config);

    std::string getLastError() const { return lastError; }

private:
    std::string lastError;

    // 辅助函数
    std::vector<std::string> findNiftiFiles(const fs::path& folder);
    bool registerToReference(const std::string& fixed,
        const std::string& moving,
        const std::string& outputPrefix,
        const std::string& transformType);
    bool averageImages(const std::vector<std::string>& inputImages,
        const std::string& outputFile);
    bool applyTransform(const std::string& reference,
        const std::string& moving,
        const std::string& transformFile,
        const std::string& outputFile);
    bool denoiseImage(const std::string& input,
        const std::string& output);
    std::string executeCommand(const std::string& cmd);
};