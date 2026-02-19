#include "common_main.h"

#include <conio.h>
#include <iostream>

int main() {
    initCommon();
    
    std::cout << "Server running. Press 'q' or Ctrl+C to exit.\n";
    
    while (true) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 'q' || key == 'Q') {
                std::cout << "Shutting down server...\n";
                break;
            }
        }
        
        stepCommon();
    }
    
    return 0;
}
