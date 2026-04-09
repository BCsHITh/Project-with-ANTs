#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include "dicom_series.h"

namespace fs = std::filesystem;

class DICOMManager {
public:
    DICOMManager();

    bool scanDirectory(const fs::path& folderPath);
    const std::vector<DICOMSeries>& getSeriesList() const;
    void clear();
    std::string getLastError() const;

private:
    std::vector<DICOMSeries> seriesList;
    std::string lastError;

    void extractSeriesInfo(const fs::path& folderPath, DICOMSeries& series);
};