#include <stdio.h>

/* partly AI generated content

PT1000 table for comparision: https://s8b8d6a7.delivery.rocketcdn.me/wp-content/uploads/2022/08/Pt1000-Tabelle-Screen.pdf

Issue reference: https://github.com/uhi22/ccs32clara/issues/64

AI: https://claude.ai/
AI prompt: Write a C code, which translates a resistance value (in ohm) to the temperature of a PT1000 sensor in Celsius. Precision of 1K is sufficient.

How to compile and run?
gcc playground_PT1000_calculation.c
a.exe

*/

/**
 * Converts PT1000 resistance (in ohms) to temperature (in Celsius)
 * Uses linear approximation: R = R0 * (1 + alpha * T)
 * where R0 = 1000Ω at 0°C, alpha ≈ 0.00385 Ω/Ω/°C
 * 
 * Valid range: approximately -200°C to +850°C
 * Precision: ±1K
 * 
 * @param resistance Resistance value in ohms
 * @return Temperature in degrees Celsius
 */
float pt1000_to_celsius(float resistance) {
    const float R0 = 1000.0;      // Resistance at 0°C (ohms)
    const float ALPHA = 0.00385;  // Temperature coefficient (Ω/Ω/°C)
    
    // Linear approximation: T = (R - R0) / (R0 * alpha)
    float temperature = (resistance - R0) / (R0 * ALPHA);
    
    return temperature;
}

int main() {
    // Test cases
    float test_resistances[] = {
        800.0,   // -50°C Cold temperature
        1000.0,  // 0°C reference
        1078.0,  // 20°C Room temperature range
        1194.0,  // 50°C
        1385.0   // 100°C
    };
    
    printf("PT1000 Resistance to Temperature Converter\n");
    printf("===========================================\n\n");
    
    for (int i = 0; i < 5; i++) {
        float resistance = test_resistances[i];
        float temperature = pt1000_to_celsius(resistance);
        printf("Resistance: %7.1f Ω  →  Temperature: %6.1f °C\n", 
               resistance, temperature);
    }
    
    return 0;
}