#include "CLPlatform.h"
#include <iostream>

#pragma comment(lib, "OpenCL.lib")

using namespace std;

int main(int argc, char* argv[])
{
    for (auto& platform : CLPlatform::Platforms())
    {
        cout << "Platform: " << platform.Name() << endl;
        cout << "Version:  " << platform.Version() << endl;
        cout << "Vendor:   " << platform.Vendor() << endl;
        cout << "Profile:  " << platform.Profile() << endl;
        cout << "Extensions: " << platform.Extensions() << endl;
        cout << endl;
    }
    return 0;
}