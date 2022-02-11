#include "CLContext.h"
#include "CLPlatform.h"
#include <iostream>

using namespace std;

int main()
{
    for (auto& p : CLPlatform::Platforms())
    {
        cout << "Platform:   " << p.Name()       << endl;
        cout << "Version:    " << p.Version()    << endl;
        cout << "Vendor:     " << p.Vendor()     << endl;
        cout << "Profile:    " << p.Profile()    << endl;
        cout << "Extensions: " << p.Extensions() << endl;
    }

    auto context = CLContext::CreateDefault();
    if (!context)
    {
        return -1;
    }

    return 0;
}