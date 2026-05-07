#include "AmfCapture.hpp"
#include "Bitmap.hpp"
#include <windows.h>
#include <chrono>
#include <format>
#include <thread>

#include "AMF/core/Factory.h"
#include "AMF/core/Context.h"
#include "AMF/core/Surface.h"
#include "AMF/components/Component.h"
#include "AMF/components/DisplayCapture.h"

using namespace amf;

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
            void operator()(AMFContext* context) const {
                if (context != nullptr) {
                    context->Terminate();
                    context->Release();
                }
            }
        };

        struct DeleteComponent {
            void operator()(AMFComponent* component) const {
                if (component != nullptr) {
                    component->Terminate();
                    component->Release();
                }
            }
        };

        struct DeleteData {
            void operator()(AMFData* data) const {
                if (data != nullptr) {
                    data->Release();
                }
            }
        };

    }

    using Module = std::unique_ptr<std::remove_pointer_t<HMODULE>, detail::DeleteModule>;
    using Context = std::unique_ptr<AMFContext, detail::DeleteContext>;
    using Component = std::unique_ptr<AMFComponent, detail::DeleteComponent>;
    using Data = std::unique_ptr<AMFData, detail::DeleteData>;


    std::expected<std::filesystem::path, std::string_view> GrabScreenAMF() {

        Module AMF(LoadLibraryW(AMF_DLL));

        auto fnInit = reinterpret_cast<AMFInit_Fn>(GetProcAddress(AMF.get(), AMF_INIT_FUNCTION_NAME));

        AMFFactory* factory = nullptr;

        fnInit(AMF_FULL_VERSION, &factory);

        AMFContext* rawContext = nullptr;

        factory->CreateContext(&rawContext);

        Context context(rawContext);

        context->InitDX11(nullptr);

        AMFComponent* rawCapture = nullptr;

        factory->CreateComponent(context.get(), AMFDisplayCapture, &rawCapture);

        Component capture(rawCapture);

        capture->SetProperty(AMF_DISPLAYCAPTURE_MONITOR_INDEX, 0LL);
        capture->SetProperty(AMF_DISPLAYCAPTURE_MODE, AMF_DISPLAYCAPTURE_MODE_GET_CURRENT_SURFACE);

        capture->Init(AMF_SURFACE_BGRA, 0, 0);

        AMFData* rawData = nullptr;

        for (int i = 0; i < 40; ++i) {
            if (capture->QueryOutput(&rawData) == AMF_OK) {
                if (rawData != nullptr) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (rawData == nullptr) {
            return std::unexpected("Frame query timeout");
        }

        Data data(rawData);

        AMFSurface* rawSurface = nullptr;

        data->QueryInterface(AMFSurface::IID(), reinterpret_cast<void**>(&rawSurface));

        Data surfaceRelease(rawSurface);

        rawSurface->Convert(AMF_MEMORY_HOST);

        AMFPlane* plane = rawSurface->GetPlane(AMF_PLANE_PACKED);

        std::filesystem::path capturePath = "AMF.bmp";

        std::span<const uint8_t> pixelData(
            static_cast<const uint8_t*>(plane->GetNative()),
            plane->GetHeight() * plane->GetHPitch()
        );

        bool isSaved = Bitmap::SaveTopDown(
            capturePath,
            plane->GetWidth(),
            plane->GetHeight(),
            pixelData,
            plane->GetHPitch()
        );

        return capturePath;
    }

}