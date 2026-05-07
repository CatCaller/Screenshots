#pragma once

#include <windows.h>
#include <filesystem>
#include <span>
#include <fstream>

namespace Bitmap
{
    inline bool SaveTopDown(
        const std::filesystem::path& outputPath,
        int width,
        int height,
        std::span<const uint8_t> pixelData,
        size_t rowPitch
    )
    {
        std::ofstream stream(outputPath, std::ios::binary);
        if (!stream)
            return false;

        const DWORD pixelDataSize = static_cast<DWORD>(width * height * 4);
        const DWORD headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        BITMAPFILEHEADER fileHeader{};
        fileHeader.bfType = 0x4D42;
        fileHeader.bfSize = headerSize + pixelDataSize;
        fileHeader.bfOffBits = headerSize;

        BITMAPINFOHEADER infoHeader{};
        infoHeader.biSize = sizeof(BITMAPINFOHEADER);
        infoHeader.biWidth = width;
        infoHeader.biHeight = -height;
        infoHeader.biPlanes = 1;
        infoHeader.biBitCount = 32;
        infoHeader.biCompression = BI_RGB;

        stream.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        stream.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

        const uint8_t* src = pixelData.data();

        for (int y = 0; y < height; ++y)
        {
            const uint8_t* row = src + (static_cast<size_t>(y) * rowPitch);
            stream.write(reinterpret_cast<const char*>(row), width * 4);
        }

        return true;
    }
}