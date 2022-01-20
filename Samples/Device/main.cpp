#include "CLPlatform.h"
#include "CLContext.h"
#include <iostream>

#pragma comment(lib, "OpenCL.lib")

using namespace std;

int main(int, char*[])
{
    for (auto& platform : CLPlatform::Platforms())
    {
        cout << "Platform: " << platform.Name() << endl;

        for (auto& device : platform.Devices())
        {
            cout << "Device:  " << device.Name() << endl;
            cout << "Driver:  " << device.Driver() << endl;
            cout << "Vendor:  " << device.Vendor() << endl;
            cout << "Version: " << device.Version() << endl;
            cout << "Profile: " << device.Profile() << endl;
            cout << "Extension: " << device.Extensions() << endl;

            cout << "Image Support: " << (device.ImageSupport() ? "Yes" : "No") << endl;
        }
    }

    return 0;
}