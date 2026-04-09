#pragma once
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct DICOMSeries {
    std::string seriesUID;
    std::string seriesNumber;
    std::string seriesDescription;
    std::string modality;
    std::string patientName;
    std::string patientID;
    std::string studyDate;
    size_t imageCount;
    fs::path sourceFolder;

    DICOMSeries() : imageCount(0), modality("MR") {}

    std::string getDisplayName() const {
        return modality + " - " + seriesDescription + " (" + seriesNumber + ")";
    }
};