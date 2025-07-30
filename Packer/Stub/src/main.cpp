#include <windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <iomanip>

namespace fs = std::filesystem;

enum ConsoleColour {
    RED = 12,
    GREEN = 10,
    YELLOW = 14,
    DEFAULT = 7
};

void SetConsoleColour(HANDLE hConsole, ConsoleColour colour) {
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(colour));
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    std::ifstream in(exePath, std::ios::binary | std::ios::ate);
    if (!in) {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] Failed to open own executable\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }

    size_t fileSize = static_cast<size_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    in.read(buffer.data(), fileSize);
    in.close();

    SetConsoleColour(hConsole, GREEN);
    std::cout << "[INFO] Executable size: " << fileSize << " bytes\n";
    SetConsoleColour(hConsole, DEFAULT);

    DWORD fileCount = 0;
    size_t cursor = fileSize;
    if (cursor < sizeof(DWORD)) {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] File too small for metadata\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }

    cursor -= sizeof(DWORD);
    std::memcpy(&fileCount, &buffer[cursor], sizeof(DWORD));

    if (fileCount == 0)
    {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] No embedded files found to unpack.\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }

    SetConsoleColour(hConsole, GREEN);
    std::cout << "[INFO] File count found: " << fileCount << "\n";
    SetConsoleColour(hConsole, DEFAULT);

    struct FileMeta {
        std::string name;
        DWORD size;
    };

    std::vector<FileMeta> files(fileCount);

    for (int i = static_cast<int>(fileCount) - 1; i >= 0; --i) {
        DWORD nameLen = 0;

        if (cursor < sizeof(DWORD)) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "[ERROR] Metadata corrupt or incomplete (nameLen) for file #" << i << "\n";
            SetConsoleColour(hConsole, DEFAULT);
            continue;
        }
        cursor -= sizeof(DWORD);
        std::memcpy(&nameLen, &buffer[cursor], sizeof(DWORD));

        if (cursor < nameLen) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "[ERROR] Metadata corrupt or incomplete (name) for file #" << i << "\n";
            SetConsoleColour(hConsole, DEFAULT);
            continue;
        }
        cursor -= nameLen;
        files[i].name.assign(&buffer[cursor], nameLen);

        if (cursor < sizeof(DWORD)) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "[ERROR] Metadata corrupt or incomplete (size) for file #" << i << "\n";
            SetConsoleColour(hConsole, DEFAULT);
            continue;
        }
        cursor -= sizeof(DWORD);
        std::memcpy(&files[i].size, &buffer[cursor], sizeof(DWORD));
    }

    size_t dataOffset = 0;
    for (const auto& meta : files) {
        dataOffset += meta.size;
    }

    if (dataOffset > cursor) {
        SetConsoleColour(hConsole, RED);
        std::cerr << "[ERROR] Data size mismatch\n";
        SetConsoleColour(hConsole, DEFAULT);
        Sleep(2500);
        return 1;
    }

    size_t dataStart = cursor - dataOffset;

    SetConsoleColour(hConsole, GREEN);
    std::cout << "[INFO] Data section starts at offset: " << dataStart << "\n";
    SetConsoleColour(hConsole, DEFAULT);

    if (!fs::exists("unpacked")) {
        if (!fs::create_directory("unpacked")) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "[ERROR] Failed to create unpacked directory\n";
            SetConsoleColour(hConsole, DEFAULT);
            Sleep(2500);
            return 1;
        }
    }

    size_t offset = dataStart;
    size_t successCount = 0;

    for (size_t i = 0; i < files.size(); ++i) {
        const auto& meta = files[i];

        std::cout << "[*] Unpacking " << (i + 1) << "/" << files.size() << ": " << meta.name << " (" << meta.size << " bytes) ... ";

        std::string outPath = "Unpacked/" + fs::path(meta.name).filename().string();

        if (offset + meta.size > buffer.size()) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "FAILED: File data overflow\n";
            SetConsoleColour(hConsole, DEFAULT);
            continue;
        }

        std::ofstream out(outPath, std::ios::binary);
        if (!out) {
            SetConsoleColour(hConsole, RED);
            std::cerr << "FAILED: Cannot write to " << outPath << "\n";
            SetConsoleColour(hConsole, DEFAULT);
            continue;
        }

        out.write(&buffer[offset], meta.size);
        out.close();

        SetConsoleColour(hConsole, GREEN);
        std::cout << "OK\n";
        SetConsoleColour(hConsole, DEFAULT);

        offset += meta.size;
        ++successCount;
    }

    SetConsoleColour(hConsole, YELLOW);
    std::cout << "[DONE] Unpacked " << successCount << "/" << files.size() << " files successfully to ./Unpacked/\n";
    SetConsoleColour(hConsole, DEFAULT);
    Sleep(5000);
    return 0;
}