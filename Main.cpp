#include <exception>
#include <windows.h>

#include <iostream>

static void Error(const std::string& str) {
    MessageBox(GetActiveWindow(), ("main :" + str).c_str(), "Error",  MB_OK | MB_ICONERROR);
    throw std::runtime_error("");
}

int main() {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    void (*engineCloseFunc)() = nullptr;

    try {
        void (*engineInitFunc)() = nullptr;
        {
            if (const HMODULE hModule = LoadLibrary("Engine.dll")) {
                engineInitFunc = reinterpret_cast<void(*)()>(GetProcAddress(hModule, "InitEngine"));

                if (engineInitFunc == nullptr)
                    Error("Can't load engine init function");

                engineCloseFunc = reinterpret_cast<void(*)()>(GetProcAddress(hModule, "CloseEngine"));

                if (engineCloseFunc == nullptr)
                    Error("Can't load engine close function");

            } 
            else {
                Error("Can't found Engine");
            }
        }

        engineInitFunc();
        return 0;
    }
    catch(const std::exception&){
        engineCloseFunc();
        return 1;
    }
}