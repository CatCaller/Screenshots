#include "AmfCapture.hpp"
#include "NtGdiCapture.hpp"
#include <iostream>

int main()
{
    while (true)
    {
        std::cout << "SS method (1 = AMF, 2 = GDI): ";

        int choice = 1;
        std::cin >> choice;
        std::cin.ignore();

        auto result = (choice == 1)
            ? Capture::GrabScreenAMF()
            : Capture::GrabScreenNtGdiBitBlt();

        if (result) {
            std::cout << "Captured BMP: " << result->string() << '\n';
        }
        else {
            std::cerr << "Capture failed: " << result.error() << '\n';
        }
    }

    return 0;
}