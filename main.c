#include <assert.h>

#define MEMORY_SIZE 4096  // CHIP-8 tiene 4KB de memoria

typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint16_t pc = 0x200;  // El programa comienza en la dirección 0x200
    uint8_t V[16];  // Registros V0 a VF
    uint16_t I;  // Registro de índice
} Chip8;

uint8_t leer_memoria(Chip8 *chip8, uint16_t direccion) {
    assert(direccion < MEMORY_SIZE);
    uint8_t valor = chip8->memory[direccion];
    return valor;
}

uint16_t leer_instruccion(Chip8 *chip8, uint16_t direccion) {
    assert(direccion < MEMORY_SIZE - 1);
    assert(direccion % 2 == 0);

    uint8_t byte_alto = chip8->memory[direccion];
    uint8_t byte_bajo = chip8->memory[direccion + 1];

    return (byte_alto << 8) | byte_bajo;
}

void avanzar_pc(Chip8 *chip8) {
    assert(chip8->pc + 2 < MEMORY_SIZE);
    chip8->pc += 2;
}

uint16_t fetch(Chip8 *chip8){
    assert(chip8->pc < MEMORY_SIZE - 1);
    return leer_instruccion(chip8, chip8->pc);
}