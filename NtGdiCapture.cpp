#include "NtGdiCapture.hpp"
#include "Bitmap.hpp"
#include <windows.h>
#include <vector>

namespace Capture {

    namespace detail {

        struct DeleteModule {
            void operator()(HMODULE handle) const {
                if (handle != nullptr) {
                    FreeLibrary(handle);
                }
            }
        };

        struct DeleteContext {
            void operator()(HDC handle) const {
                if (handle != nullptr) {
                    DeleteDC(handle);
                }
            }
        };

        struct DeleteBitmap {
            void operator()(HBITMAP handle) const {
                if (handle != nullptr) {
                    DeleteObject(handle);
                }
            }
        };

    }

    using Module = std::unique_ptr<std::remove_pointer_t<HMODULE>, detail::DeleteModule>;
    using DeviceContext = std::unique_ptr<std::remove_pointer_t<HDC>, detail::DeleteContext>;
    using BitmapHandle = std::unique_ptr<std::remove_pointer_t<HBITMAP>, detail::DeleteBitmap>;


    std::expected<std::filesystem::path, std::string_view> GrabScreenNtGdiBitBlt() {

        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        HDC screenDC = GetDC(nullptr);
        DeviceContext screenDCGuard(screenDC);

        HDC compatDC = CreateCompatibleDC(screenDC);
        DeviceContext compatDCGuard(compatDC);

        HBITMAP compatBitmap = CreateCompatibleBitmap(screenDC, width, height);
        BitmapHandle compatBitmapGuard(compatBitmap);

        HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(compatDC, compatBitmap));

        Module win32u(LoadLibraryW(L"win32u.dll"));

        using NtGdiBitBlt_t = BOOL(WINAPI*)(HDC, int, int, int, int, HDC, int, int, DWORD);

        auto fnNtGdiBitBlt = reinterpret_cast<NtGdiBitBlt_t>(
            GetProcAddress(win32u.get(), "NtGdiBitBlt")
        );

        fnNtGdiBitBlt(compatDC, 0, 0, width, height, screenDC, 0, 0, SRCCOPY);

        BITMAPINFOHEADER bmiHeader{};
        bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiHeader.biWidth = width;
        bmiHeader.biHeight = -height;
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = 32;
        bmiHeader.biCompression = BI_RGB;

        std::vector<uint8_t> bitmapData(width * height * 4);

        GetDIBits(
            compatDC,
            compatBitmap,
            0,
            height,
            bitmapData.data(),
            reinterpret_cast<BITMAPINFO*>(&bmiHeader),
            DIB_RGB_COLORS
        );

        SelectObject(compatDC, oldBitmap);

        std::filesystem::path capturePath = "GDI.bmp";

        Bitmap::SaveTopDown(capturePath, width, height, bitmapData, width * 4);

        return capturePath;
    }

}
