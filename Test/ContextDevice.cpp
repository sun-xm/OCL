#include "CLContext.h"
#include <iostream>

using namespace std;

int main()
{
    auto context = CLContext::CreateDefault();
    if (!context)
    {
        return -1;
    }

    auto device = context.Device();
    if (!device)
    {
        return -1;
    }

    cout << "Device:  " << device.Name() << endl;
    cout << "Driver:  " << device.Driver() << endl;
    cout << "Vendor:  " << device.Vendor() << endl;
    cout << "Version: " << device.Version() << endl;
    cout << "Profile: " << device.Profile() << endl;
    cout << "Extends: " << device.Extensions() << endl;

    return 0;
}