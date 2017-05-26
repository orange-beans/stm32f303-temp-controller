#include "mbed.h"

Serial pc(SERIAL_TX, SERIAL_RX);

AnalogIn ain1(A0);
// AnalogIn ain2(A1);
// AnalogIn ain3(A2);
// AnalogIn ain4(A3);
// AnalogIn ain5(A4);
// AnalogIn ain6(A5);
// AnalogIn ain7(A6);

int main() {
    int i;
    while(1) {
        for(i = 0; i < 50; i++) {
            wait(0.5f); // wait a small period of time
            pc.printf("Analog 1:%f \n", ain1.read()); // print the value of variable i
            // pc.printf("Analog 2:%f \n", ain2.read()); // print the value of variable i
            // pc.printf("Analog 3:%f \n", ain3.read()); // print the value of variable i
            // pc.printf("Analog 4:%f \n", ain4.read()); // print the value of variable i
            // pc.printf("Analog 5:%f \n", ain5.read()); // print the value of variable i
            // pc.printf("Analog 6:%f \n", ain6.read()); // print the value of variable i
            // pc.printf("Analog 7:%f \n", ain7.read()); // print the value of variable i
        }
    }
}
