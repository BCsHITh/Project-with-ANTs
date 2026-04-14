#ifndef _CORE_H_
#define _CORE_H_
#include <filesystem>
#include "dicom_manager.h"
#include "nifti_manager.h"
#include "dicom_converter.h"

namespace fs = std::filesystem;

std::string getUserInput(const std::string& prompt);

bool getFolderPath(const std::string& prompt, fs::path& outPath, bool mustExist = true);

int runSingleConversion(int argc, char* argv[]);

int runBatchMode();

int runNiftiManager();

int runRegistration();

int runBatchRegistration();

#endif // !_CORE_H_
