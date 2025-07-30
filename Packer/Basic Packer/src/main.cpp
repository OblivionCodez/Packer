#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

enum ConsoleColour {
    DEFAULT = 7,
    RED = 12,
    GREEN = 10,
    YELLOW = 14,
    CYAN = 11
};

void SetConsoleColour(HANDLE hConsole, ConsoleColour colour) {
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(colour));
}

struct FileEntry {
    std::string name;
    std::vector<char> data;
};

int main(int argc, char* argv[]) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (argc < 2) {
        SetConsoleColour(hConsole, YELLOW);
        std::cout << "Usage: drag and drop files or folders onto packer.exe\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }


    std::ifstream stub("stub.exe", std::ios::binary);
    if (!stub) {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] Missing stub.exe. Please place it alongside this executable.\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }
    SetConsoleColour(hConsole, CYAN);
    std::cout << "[INFO] Loaded stub.exe successfully.\n";
    SetConsoleColour(hConsole, DEFAULT);

    std::vector<FileEntry> files;

    int totalArgs = argc - 1;
    int processedArgs = 0;

    for (int i = 1; i < argc; ++i) {
        fs::path pathArg = argv[i];

        if (!fs::exists(pathArg)) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "[ERROR] Path does not exist: " << pathArg.string() << "\n";
            SetConsoleColour(hConsole, DEFAULT);
            continue;
        }

        if (fs::is_directory(pathArg)) {
            SetConsoleColour(hConsole, CYAN);
            std::cout << "[INFO] Processing directory: " << pathArg.string() << "\n";
            SetConsoleColour(hConsole, DEFAULT);

            size_t filesInDir = 0;
            for (const auto& entry : fs::directory_iterator(pathArg)) {
                if (!entry.is_regular_file())
                    continue;

                const auto& filePath = entry.path();
                std::ifstream in(filePath, std::ios::binary);
                if (!in) {
                    SetConsoleColour(hConsole, RED);
                    std::cerr << "Failed to open file: " << filePath.filename().string() << "\n";
                    SetConsoleColour(hConsole, DEFAULT);
                    continue;
                }

                in.seekg(0, std::ios::end);
                size_t size = static_cast<size_t>(in.tellg());
                in.seekg(0, std::ios::beg);

                FileEntry entryFile;
                entryFile.name = filePath.filename().string();
                entryFile.data.resize(size);
                in.read(entryFile.data.data(), size);
                in.close();

                SetConsoleColour(hConsole, GREEN);
                std::cout << "[ADDED] " << entryFile.name << " (" << size << " bytes)\n";
                SetConsoleColour(hConsole, DEFAULT);

                files.push_back(std::move(entryFile));
                ++filesInDir;
            }

            if (filesInDir == 0) {
                SetConsoleColour(hConsole, YELLOW);
                std::cout << "[WARN] No files found in directory: " << pathArg.string() << "\n";
                SetConsoleColour(hConsole, DEFAULT);
            }
        }
        else if (fs::is_regular_file(pathArg)) {
            std::cout << "[INFO] Processing file " << i << "/" << totalArgs << ": " << pathArg.filename().string() << " ... ";

            std::ifstream in(pathArg, std::ios::binary);
            if (!in) {
                SetConsoleColour(hConsole, RED);
                std::cerr << "Failed to open file\n";
                SetConsoleColour(hConsole, DEFAULT);
                continue;
            }

            in.seekg(0, std::ios::end);
            size_t size = static_cast<size_t>(in.tellg());
            in.seekg(0, std::ios::beg);

            FileEntry entryFile;
            entryFile.name = pathArg.filename().string();
            entryFile.data.resize(size);
            in.read(entryFile.data.data(), size);
            in.close();

            SetConsoleColour(hConsole, GREEN);
            std::cout << "added (" << size << " bytes)\n";
            SetConsoleColour(hConsole, DEFAULT);

            files.push_back(std::move(entryFile));
        }
        else {
            SetConsoleColour(hConsole, YELLOW);
            std::cout << "[WARN] Skipping non-regular file path: " << pathArg.string() << "\n";
            SetConsoleColour(hConsole, DEFAULT);
        }

        ++processedArgs;
    }

    if (files.empty()) {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] No valid files were added. Aborting.\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }

    std::ofstream out("Packed.EXE", std::ios::binary);
    if (!out) {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] Failed to create packed.exe for writing.\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }

    SetConsoleColour(hConsole, CYAN);
    std::cout << "[INFO] Writing stub.exe content to packed.exe...\n";
    SetConsoleColour(hConsole, DEFAULT);
    out << stub.rdbuf();

    size_t currentFileIndex = 0;
    for (const auto& file : files) {
        ++currentFileIndex;
        std::cout << "[INFO] Appending file data " << currentFileIndex << "/" << files.size() << ": " << file.name << "\n";
        out.write(file.data.data(), file.data.size());
    }

    std::cout << "[INFO] Writing file metadata...\n";
    for (const auto& file : files) {
        DWORD dataSize = static_cast<DWORD>(file.data.size());
        DWORD nameLen = static_cast<DWORD>(file.name.size());
        out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        out.write(file.name.c_str(), nameLen);
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    }

    DWORD count = static_cast<DWORD>(files.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    SetConsoleColour(hConsole, GREEN);
    std::cout << "[SUCCESS] Created packed.exe with " << count << " file(s) embedded.\n";
    SetConsoleColour(hConsole, DEFAULT);
    Sleep(2500);
    return 0;
}