#include "CLContext.h"
#include "CLPlatform.h"
#include <iostream>

using namespace std;

int main()
{
    for (auto& p : CLPlatform::Platforms())
    {
        cout << "Platform: " << p.Name()       << endl;
        cout << "Version:  " << p.Version()    << endl;
        cout << "Vendor:   " << p.Vendor()     << endl;
        cout << "Profile:  " << p.Profile()    << endl;
        cout << "Extends:  " << p.Extensions() << endl;

        for (auto& d : p.Devices())
        {
            cout << "Device:  " << d.Name() << endl;
            cout << "Driver:  " << d.Driver() << endl;
            cout << "Vendor:  " << d.Vendor() << endl;
            cout << "Version: " << d.Version() << endl;
            cout << "Profile: " << d.Profile() << endl;
            cout << "Extends: " << d.Extensions() << endl;
        }
    }

    auto context = CLContext::CreateDefault();
    if (!context)
    {
        return -1;
    }

    return 0;
}