#include <iostream>
#include <string>
#include <iomanip>
#include <cstring>  // Para memcpy
#include "auxiliary.h"
#include "addition.h"
#include "division.h"

using namespace std;

void displayMenu() {
    cout << "\n========================================" << endl;
    cout << "   SIMULADOR ALU IEEE-754 (32-bits)" << endl;
    cout << "========================================" << endl;
    cout << "1. Sumar (A + B)" << endl;
    cout << "2. Restar (A - B)" << endl;
    cout << "3. Multiplicar (A * B)" << endl;
    cout << "4. Dividir (A / B)" << endl;
    cout << "5. Salir" << endl;
    cout << "Elija una opcion: ";
}

void printResult(string label, uint32_t result) {
    float f;
    memcpy(&f, &result, sizeof(float));

    cout << "\n--- RESULTADO ---" << endl;
    cout << label << " (Hex): 0x" << hex << uppercase << setw(8) << setfill('0') << result << dec << endl;
    cout << label << " (Bin): 0b" << bitset<32>(result) << endl;
    cout << label << " (Float): " << f << endl;
}

int main() {
    int option;
    string valA, valB;

    while (true) {
        displayMenu();
        if (!(cin >> option)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }

        if (option == 5) break;

        if (option >= 1 && option <= 4) {
            cout << "Ingrese Valor A (float o 0b...): ";
            cin >> valA;
            cout << "Ingrese Valor B (float o 0b...): ";
            cin >> valB;

            uint32_t a = textToFloatingPointBinary(valA);
            uint32_t b = textToFloatingPointBinary(valB);

            if (a == INF || b == INF) {
                cout << "\033[1;31m[Error]\033[0m Entrada invalida." << endl;
                continue;
            }

            uint32_t result = 0;

            switch (option) {
                case 1:
                    result = add(a, b);
                    printResult("Suma", result);
                    break;
                case 2: {
                    uint32_t bInv = b ^ (1U << 31);
                    result = add(a, bInv);
                    printResult("Resta", result);
                    break;
                }
                case 3:
                    result = multiplyIEEE754(a, b);
                    cout << "\n[!] Multiplicacion aun no implementada." << endl;
                    break;
                case 4:
                    result = divide(a, b);
                    printResult("Division", result);
                    break;
                default:
                    break;
            }
        } else {
            cout << "Opcion no valida." << endl;
        }
    }

    cout << "Saliendo del simulador..." << endl;
    return 0;
}