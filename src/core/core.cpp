#include <string>
#include <iostream>
#include <chrono>
#include <algorithm> 
#include "core.h"

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

bool getFolderPath(const std::string& prompt, fs::path& outPath, bool mustExist) {
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

//调用 dcm2niix 和 ANTs 的逻辑
void PrintToolPaths() {
    std::cout << "dcm2niix: " << DCM2NIIX_EXE << std::endl;
    std::cout << "ANTs: " << ANTS_BIN_PATH << std::endl;
}

int runSingleConversion(int argc, char* argv[]){
    fs::path inputFolder;
    fs::path outputFolder;
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

// ⭐ 新增：NIfTI 文件管理
int runNiftiManager() {
    std::cout << "=== NIfTI 文件管理 ===" << std::endl;

    fs::path niftiFolder;
    std::cout << "请输入 NIfTI 文件所在文件夹: ";
    std::string input;
    std::getline(std::cin, input);

    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    niftiFolder = input;

    NiftiManager manager;
    if (!manager.scanDirectory(niftiFolder)) {
        std::cerr << "未找到 NIfTI 文件" << std::endl;
        return 1;
    }

    const auto& list = manager.getNiftiList();

    std::cout << "\n找到的 NIfTI 文件:" << std::endl;
    std::cout << "========================================" << std::endl;

    for (size_t i = 0; i < list.size(); ++i) {
        const auto& info = list[i];
        std::cout << "[" << (i + 1) << "] "
            << info.fileName << std::endl;
        std::cout << "    路径：" << info.filePath << std::endl;
        std::cout << "    模态：" << info.modality << std::endl;
        std::cout << "    系列：" << info.seriesNumber << std::endl;
        std::cout << "    大小：" << (info.fileSize / 1024 / 1024) << " MB" << std::endl;
        std::cout << std::endl;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "总计：" << list.size() << " 个文件，"
        << manager.getTotalSize() << " MB" << std::endl;

    // 按患者分组显示
    auto byPatient = manager.groupByPatient();
    if (byPatient.size() > 1) {
        std::cout << "\n按患者分组:" << std::endl;
        for (const auto& [patient, files] : byPatient) {
            std::cout << "  " << patient << ": " << files.size() << " 个文件" << std::endl;
        }
    }

    std::cout << "\n按任意键退出...";
    std::cin.get();

    return 0;
}

// ⭐ 新增：图像配准
// ⭐ 修改后的：图像配准（支持自定义输出路径）
int runRegistration() {
    std::cout << "=== 图像配准 ===" << std::endl;
    std::cout << "使用 ANTs 进行刚性/仿射配准" << std::endl;
    std::cout << std::endl;

    std::string fixedImage, movingImage, outputPrefix;
    fs::path outputDir;

    // 1. 输入固定图像
    std::cout << "请输入固定图像路径 (参考图像): ";
    std::getline(std::cin, fixedImage);
    if (fixedImage.front() == '"' && fixedImage.back() == '"') {
        fixedImage = fixedImage.substr(1, fixedImage.size() - 2);
    }

    if (!fs::exists(fixedImage)) {
        std::cerr << "错误：固定图像不存在：" << fixedImage << std::endl;
        return 1;
    }

    // 2. 输入移动图像
    std::cout << "请输入移动图像路径 (需要配准的图像): ";
    std::getline(std::cin, movingImage);
    if (movingImage.front() == '"' && movingImage.back() == '"') {
        movingImage = movingImage.substr(1, movingImage.size() - 2);
    }

    if (!fs::exists(movingImage)) {
        std::cerr << "错误：移动图像不存在：" << movingImage << std::endl;
        return 1;
    }

    // 3. ⭐ 选择输出目录
    std::cout << std::endl;
    std::cout << "请选择输出目录 (留空则使用移动图像所在目录): ";
    std::string outputInput;
    std::getline(std::cin, outputInput);

    if (outputInput.empty()) {
        // 使用移动图像所在目录
        outputDir = fs::path(movingImage).parent_path();
    }
    else {
        if (outputInput.front() == '"' && outputInput.back() == '"') {
            outputInput = outputInput.substr(1, outputInput.size() - 2);
        }
        outputDir = outputInput;

        // 创建目录（如果不存在）
        if (!fs::exists(outputDir)) {
            std::cout << "输出目录不存在，是否创建？(y/n): ";
            std::string confirm;
            std::getline(std::cin, confirm);

            if (confirm.empty() || confirm[0] == 'y' || confirm[0] == 'Y') {
                if (!fs::create_directories(outputDir)) {
                    std::cerr << "错误：无法创建输出目录" << std::endl;
                    return 1;
                }
            }
            else {
                std::cout << "操作已取消" << std::endl;
                return 0;
            }
        }
    }

    // 4. 输入输出前缀
    std::cout << "请输入输出文件前缀 (留空则使用移动图像文件名): ";
    std::getline(std::cin, outputPrefix);

    if (outputPrefix.empty()) {
        outputPrefix = fs::path(movingImage).stem().string();
    }

    // 5. 选择配准类型
    std::cout << std::endl;
    std::cout << "选择配准类型:" << std::endl;
    std::cout << "  1. 刚性配准 (Rigid) - 快速，适合头部" << std::endl;
    std::cout << "  2. 仿射配准 (Affine) - 中等速度" << std::endl;
    std::cout << "  3. 非线性配准 (SyN) - 慢，精度高" << std::endl;
    std::cout << "  4. 快速刚性 (QuickRigid) - 最快" << std::endl;
    std::cout << std::endl;

    int regType = 1;
    std::cout << "请输入选项 (1-4，默认 1): ";
    std::string typeInput;
    std::getline(std::cin, typeInput);
    if (!typeInput.empty()) {
        try {
            regType = std::stoi(typeInput);
        }
        catch (...) {
            regType = 1;
        }
    }

    RegistrationType type;
    switch (regType) {
    case 1: type = RegistrationType::Rigid; break;
    case 2: type = RegistrationType::Affine; break;
    case 3: type = RegistrationType::SyN; break;
    case 4: type = RegistrationType::QuickRigid; break;
    default: type = RegistrationType::Rigid;
    }

    // 构建完整输出路径
    std::string fullPath = (outputDir / outputPrefix).string();

    // 6. 确认
    std::cout << std::endl;
    std::cout << "=== 配置 ===" << std::endl;
    std::cout << "固定图像：" << fixedImage << std::endl;
    std::cout << "移动图像：" << movingImage << std::endl;
    std::cout << "输出目录：" << outputDir << std::endl;
    std::cout << "输出前缀：" << outputPrefix << std::endl;
    std::cout << "完整输出：" << fullPath << "_Warped.nii.gz" << std::endl;
    std::cout << "配准类型：" << regType << std::endl;
    std::cout << std::endl;
    std::cout << "按回车开始配准，或输入 q 退出...";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm == "q" || confirm == "Q") {
        return 0;
    }

    // 7. 执行配准
    std::cout << std::endl;
    Converter converter;

    bool success = converter.registerImages(
        fixedImage,
        movingImage,
        fullPath,  // ⭐ 使用完整路径
        type,
        [](int percent, const std::string& msg) {
            std::cout << "进度：" << msg << " (" << percent << "%)" << std::endl;
        },
        [](const std::string& error) {
            std::cerr << "错误：" << error << std::endl;
        }
    );

    std::cout << std::endl;
    if (success) {
        std::cout << "✅ 配准成功！" << std::endl;
        std::cout << "输出文件：" << fullPath << "_Warped.nii.gz" << std::endl;
        std::cout << "变换文件：" << fullPath << "0GenericAffine.mat" << std::endl;
    }
    else {
        std::cerr << "❌ 配准失败：" << converter.getLastError() << std::endl;
    }

    std::cout << "\n按任意键退出...";
    std::cin.get();

    return success ? 0 : 1;
}

// ⭐ 新增：批量配准功能
int runBatchRegistration() {
    std::cout << "=== 批量图像配准 ===" << std::endl;
    std::cout << "将多张移动图像与同一张固定图像配准" << std::endl;
    std::cout << std::endl;

    std::string fixedImage;
    fs::path outputDir;
    std::vector<std::string> movingImages;

    // 1. 输入固定图像
    std::cout << "请输入固定图像路径 (参考图像，所有图像将配准到此): ";
    std::string input;
    std::getline(std::cin, input);
    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    fixedImage = input;

    if (!fs::exists(fixedImage)) {
        std::cerr << "错误：固定图像不存在：" << fixedImage << std::endl;
        return 1;
    }

    // 2. 选择输出目录
    std::cout << std::endl;
    std::cout << "请输入输出目录 (留空则在每张移动图像所在目录生成): ";
    std::getline(std::cin, input);

    if (input.empty()) {
        outputDir = "";  // 使用默认
    }
    else {
        if (input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }
        outputDir = input;

        if (!fs::exists(outputDir)) {
            std::cout << "输出目录不存在，是否创建？(y/n): ";
            std::string confirm;
            std::getline(std::cin, confirm);

            if (confirm.empty() || confirm[0] == 'y' || confirm[0] == 'Y') {
                if (!fs::create_directories(outputDir)) {
                    std::cerr << "错误：无法创建输出目录" << std::endl;
                    return 1;
                }
            }
            else {
                std::cout << "操作已取消" << std::endl;
                return 0;
            }
        }
    }

    // 3. 输入多张移动图像
    std::cout << std::endl;
    std::cout << "请输入移动图像路径 (可输入多个，每行一个，输入空行结束):" << std::endl;

    while (true) {
        std::cout << "  [" << (movingImages.size() + 1) << "] ";
        std::string movingPath;
        std::getline(std::cin, movingPath);

        if (movingPath.empty()) {
            break;  // 空行结束输入
        }

        if (movingPath.front() == '"' && movingPath.back() == '"') {
            movingPath = movingPath.substr(1, movingPath.size() - 2);
        }

        if (!fs::exists(movingPath)) {
            std::cout << "    ⚠ 文件不存在，跳过：" << movingPath << std::endl;
            continue;
        }

        movingImages.push_back(movingPath);
    }

    if (movingImages.empty()) {
        std::cerr << "错误：未输入任何移动图像" << std::endl;
        return 1;
    }

    // 4. 选择配准类型
    std::cout << std::endl;
    std::cout << "选择配准类型:" << std::endl;
    std::cout << "  1. 刚性配准 (Rigid)" << std::endl;
    std::cout << "  2. 仿射配准 (Affine)" << std::endl;
    std::cout << "  3. 非线性配准 (SyN)" << std::endl;
    std::cout << "  4. 快速刚性 (QuickRigid)" << std::endl;
    std::cout << std::endl;

    int regType = 1;
    std::cout << "请输入选项 (1-4，默认 1): ";
    std::string typeInput;
    std::getline(std::cin, typeInput);
    if (!typeInput.empty()) {
        try {
            regType = std::stoi(typeInput);
        }
        catch (...) {
            regType = 1;
        }
    }

    RegistrationType type;
    switch (regType) {
    case 1: type = RegistrationType::Rigid; break;
    case 2: type = RegistrationType::Affine; break;
    case 3: type = RegistrationType::SyN; break;
    case 4: type = RegistrationType::QuickRigid; break;
    default: type = RegistrationType::Rigid;
    }

    // 5. 确认
    std::cout << std::endl;
    std::cout << "=== 配置 ===" << std::endl;
    std::cout << "固定图像：" << fixedImage << std::endl;
    std::cout << "移动图像数量：" << movingImages.size() << std::endl;
    std::cout << "输出目录：" << (outputDir.empty() ? "[与移动图像同目录]" : outputDir.string()) << std::endl;
    std::cout << "配准类型：" << regType << std::endl;
    std::cout << std::endl;
    std::cout << "按回车开始批量配准，或输入 q 退出...";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm == "q" || confirm == "Q") {
        return 0;
    }

    // 6. 批量配准
    std::cout << std::endl;
    std::cout << "开始批量配准..." << std::endl;
    std::cout << "========================================" << std::endl;

    Converter converter;
    size_t successCount = 0;
    size_t failCount = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < movingImages.size(); ++i) {
        const std::string& movingImage = movingImages[i];

        std::cout << std::endl;
        std::cout << "[" << (i + 1) << "/" << movingImages.size() << "] ";
        std::cout << fs::path(movingImage).filename().string() << std::endl;

        // 确定输出路径
        std::string outputPrefix;
        if (outputDir.empty()) {
            // 使用移动图像所在目录
            outputPrefix = fs::path(movingImage).stem().string();
        }
        else {
            // 使用指定目录
            outputPrefix = (outputDir / fs::path(movingImage).stem()).string();
        }

        std::cout << "  输出：" << outputPrefix << "_Warped.nii.gz" << std::endl;
        std::cout << "  配准中... ";

        bool success = converter.registerImages(
            fixedImage,
            movingImage,
            outputPrefix,
            type,
            [](int percent, const std::string& msg) {
                // 静默模式
            },
            [](const std::string& error) {
                std::cerr << "    错误：" << error << std::endl;
            }
        );

        if (success) {
            std::cout << "✓" << std::endl;
            successCount++;
        }
        else {
            std::cout << "✗" << std::endl;
            std::cerr << "  失败：" << converter.getLastError() << std::endl;
            failCount++;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    // 7. 总结
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "=== 批量配准完成 ===" << std::endl;
    std::cout << "成功：" << successCount << "/" << movingImages.size() << std::endl;
    std::cout << "失败：" << failCount << std::endl;
    std::cout << "总耗时：" << duration.count() << " 秒" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n按任意键退出...";
    std::cin.get();

    return (failCount == 0) ? 0 : 1;
}

// ⭐ 新增：图像平均化功能
// ⭐ 修改后的：图像平均化功能
int runImageAverage() {
    std::cout << "=== 图像平均化处理 ===" << std::endl;
    std::cout << "通过迭代配准和平均生成高质量模板" << std::endl;
    std::cout << std::endl;

    AverageConfig config;

    // 1. 输入目录
    std::cout << "请输入包含 NIfTI 文件的文件夹: ";
    std::string input;
    std::getline(std::cin, input);

    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    config.inputFolder = input;

    if (!fs::exists(config.inputFolder)) {
        std::cerr << "错误：输入目录不存在：" << config.inputFolder << std::endl;
        return 1;
    }

    // 2. 输出目录
    std::cout << "请输入输出目录 (留空则在输入目录下创建 average 子目录): ";
    std::getline(std::cin, input);

    if (input.empty()) {
        config.outputFolder = config.inputFolder + "/average";
    }
    else {
        if (input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }
        config.outputFolder = input;
    }

    // 3. 输出前缀
    std::cout << "请输入输出文件前缀 (默认 average): ";
    std::getline(std::cin, config.outputPrefix);

    if (config.outputPrefix.empty()) {
        config.outputPrefix = "average";
    }

    // ⭐ 4. 是否进行配准
    std::cout << std::endl;
    std::cout << "图像是否已经配准？" << std::endl;
    std::cout << "  y - 是，已配准（直接平均，跳过配准步骤）" << std::endl;
    std::cout << "  n - 否，未配准（先配准再平均）" << std::endl;
    std::cout << "请选择 (y/n，默认 n): ";
    std::string regInput;
    std::getline(std::cin, regInput);

    // 如果选择"是，已配准"，则禁用配准
    config.enableRegistration = (regInput.empty() ||
        regInput[0] == 'n' || regInput[0] == 'N') ? true : false;

    std::cout << std::endl;
    std::cout << "请选择配准变换应用的插值方式：" << std::endl;
    std::cout << "  1. Linear（线性插值，推荐用于连续图像）" << std::endl;
    std::cout << "  2. NearestNeighbor（最近邻，适用于标签/分割图像）" << std::endl;
    std::cout << "  3. BSpline（B样条，平滑插值）" << std::endl;
    std::cout << "  4. Gaussian（高斯插值）" << std::endl;
    std::cout << "  5. MultiLabel（多标签插值）" << std::endl;
    std::cout << "  6. LanczosWindowedSinc（Lanczos，高质量但较慢）" << std::endl;
    std::cout << std::endl;
    std::cout << "请输入选项 (1-6，默认 1): ";
    std::string interpInput;
    std::getline(std::cin, interpInput);

    if (!interpInput.empty()) {
        try {
            int interpChoice = std::stoi(interpInput);
            switch (interpChoice) {
            case 1:
                config.interpolation = InterpolationType::Linear;
                break;
            case 2:
                config.interpolation = InterpolationType::NearestNeighbor;
                break;
            case 3:
                config.interpolation = InterpolationType::BSpline;
                break;
            case 4:
                config.interpolation = InterpolationType::Gaussian;
                break;
            case 5:
                config.interpolation = InterpolationType::MultiLabel;
                break;
            case 6:
                config.interpolation = InterpolationType::LanczosWindowedSinc;
                break;
            default:
                config.interpolation = InterpolationType::Linear;
                std::cout << "  无效选项，使用默认：Linear" << std::endl;
            }
        }
        catch (...) {
            config.interpolation = InterpolationType::Linear;
        }
    }
    else {
        config.interpolation = InterpolationType::Linear;
    }

    // 6. 是否去噪
    std::cout << std::endl;
    std::cout << "是否对最终平均图像去噪？(y/n，默认 n): ";
    std::string dnInput;
    std::getline(std::cin, dnInput);
    config.enableDenoise = (!dnInput.empty() &&
        (dnInput[0] == 'y' || dnInput[0] == 'Y'));

    // 6. 迭代次数（仅当启用配准时）
    if (config.enableRegistration) {
        std::cout << "请输入迭代次数 (1-3，默认 2): ";
        std::string iterInput;
        std::getline(std::cin, iterInput);

        if (!iterInput.empty()) {
            try {
                int iters = std::stoi(iterInput);
                config.maxIterations = (std::max)(1, (std::min)(3, iters));
            }
            catch (...) {
                config.maxIterations = 2;
            }
        }
    }
    else {
        config.maxIterations = 1;  // 不配准时只需一轮平均
    }

    // 8. 确认配置
    std::cout << std::endl;
    std::cout << "=== 配置 ===" << std::endl;
    std::cout << "输入目录：" << config.inputFolder << std::endl;
    std::cout << "输出目录：" << config.outputFolder << std::endl;
    std::cout << "输出前缀：" << config.outputPrefix << std::endl;
    std::cout << "图像已配准：" << (config.enableRegistration ? "否（将先配准）" : "是（直接平均）") << std::endl;
    std::cout << "插值方式：" << interpolationToString(config.interpolation) << std::endl;  // ⭐ 显示插值方式
    if (config.enableRegistration) {
        std::cout << "迭代次数：" << config.maxIterations << std::endl;
    }
    std::cout << "去噪：" << (config.enableDenoise ? "是" : "否") << std::endl;
    std::cout << std::endl;
    std::cout << "按回车开始处理，或输入 q 退出...";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm == "q" || confirm == "Q") {
        std::cout << "已取消" << std::endl;
        return 0;
    }

    // 9. 执行平均化
    std::cout << std::endl;
    ImageAverager averager;

    bool success = averager.average(config);

    std::cout << std::endl;
    if (success) {
        std::cout << "✅ 图像平均化成功！" << std::endl;
    }
    else {
        std::cerr << "❌ 图像平均化失败：" << averager.getLastError() << std::endl;
    }

    std::cout << "\n按任意键退出...";
    std::cin.get();

    return success ? 0 : 1;
}

// ⭐ 新增：空间标准化到模板
int runNormalizeToTemplate() {
    std::cout << "=== 空间标准化到模板空间 ===" << std::endl;
    std::cout << "将 T1w 和 BOLD 配准到标准模板（如 MNI152）" << std::endl;
    std::cout << std::endl;

    NormalizeConfig config;

    // 1. 输入平均 T1w 图像
    std::cout << "请输入平均 T1w 图像路径: ";
    std::string input;
    std::getline(std::cin, input);

    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    config.t1wImage = input;

    if (!fs::exists(config.t1wImage)) {
        std::cerr << "错误：T1w 图像不存在：" << config.t1wImage << std::endl;
        return 1;
    }

    // 2. 输入标准模板
    std::cout << "请输入标准模板图像路径 (如 MNI152): ";
    std::getline(std::cin, input);

    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    config.templateImage = input;

    if (!fs::exists(config.templateImage)) {
        std::cerr << "错误：模板图像不存在：" << config.templateImage << std::endl;
        return 1;
    }

    // 3. 输入 BOLD 文件夹
    std::cout << "请输入 BOLD 序列文件夹路径: ";
    std::getline(std::cin, input);

    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    config.boldFolder = input;

    if (!fs::exists(config.boldFolder)) {
        std::cerr << "错误：BOLD 文件夹不存在：" << config.boldFolder << std::endl;
        return 1;
    }

    // 4. 输出目录
    std::cout << "请输入输出目录: ";
    std::getline(std::cin, input);

    if (input.front() == '"' && input.back() == '"') {
        input = input.substr(1, input.size() - 2);
    }
    config.outputFolder = input;

    // 5. 输出前缀
    std::cout << "请输入输出文件前缀 (默认 normalized): ";
    std::getline(std::cin, config.outputPrefix);

    if (config.outputPrefix.empty()) {
        config.outputPrefix = "normalized";
    }

    // 6. 确认配置
    std::cout << std::endl;
    std::cout << "=== 配置 ===" << std::endl;
    std::cout << "T1w 图像：" << config.t1wImage << std::endl;
    std::cout << "标准模板：" << config.templateImage << std::endl;
    std::cout << "BOLD 文件夹：" << config.boldFolder << std::endl;
    std::cout << "输出目录：" << config.outputFolder << std::endl;
    std::cout << "输出前缀：" << config.outputPrefix << std::endl;
    std::cout << "配准方法：SyN 非线性配准" << std::endl;
    std::cout << std::endl;
    std::cout << "按回车开始处理，或输入 q 退出...";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm == "q" || confirm == "Q") {
        std::cout << "已取消" << std::endl;
        return 0;
    }

    // 7. 执行标准化
    std::cout << std::endl;
    TemplateNormalizer normalizer;

    bool success = normalizer.normalize(config);

    std::cout << std::endl;
    if (success) {
        std::cout << "✅ 空间标准化成功！" << std::endl;
        std::cout << "\n输出文件说明：" << std::endl;
        std::cout << "  - " << config.outputPrefix << "*_MNI.nii.gz : 标准化后的 BOLD" << std::endl;
        std::cout << "  - " << config.outputPrefix << "0GenericAffine.mat : 仿射矩阵" << std::endl;
        std::cout << "  - " << config.outputPrefix << "1Warp.nii.gz : 形变场" << std::endl;
    }
    else {
        std::cerr << "❌ 空间标准化失败：" << normalizer.getLastError() << std::endl;
    }

    std::cout << "\n按任意键退出...";
    std::cin.get();

    return success ? 0 : 1;
}