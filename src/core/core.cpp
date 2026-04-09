#include <string>
#include <iostream>

//调用 dcm2niix 和 ANTs 的逻辑
void PrintToolPaths() {
    std::cout << "dcm2niix: " << DCM2NIIX_EXE << std::endl;
    std::cout << "ANTs: " << ANTS_BIN_PATH << std::endl;
}