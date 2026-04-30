#include <assert.h>

#define MEMORY_SIZE 4096  // CHIP-8 tiene 4KB de memoria

typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint16_t pc = 0x200;  // El programa comienza en la dirección 0x200
    uint8_t V[16];  // Registros V0 a VF
    uint16_t I;  // Registro de índice
    uint8_t display[32][64];  // Pantalla de 64x32 píxeles
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
 
void decode_and_execute(Chip8 *chip8, uint16_t instruccion) {
    uint8_t  tipo = (instruccion & 0xF000) >> 12;  // primer nibble
    uint8_t  X    = (instruccion & 0x0F00) >> 8;   // segundo nibble
    uint8_t  Y    = (instruccion & 0x00F0) >> 4;   // tercer nibble
    uint8_t  N    = (instruccion & 0x000F);         // cuarto nibble
    uint8_t  NN   = (instruccion & 0x00FF);         // byte bajo
    uint16_t NNN  = (instruccion & 0x0FFF);         // dirección completa
        switch(tipo){
        case 0x6:
            chip8->V[X] = NN;
            break;    
        case 0x1:
            chip8->pc = NNN;
            break;
        case 0x7:
            chip8->V[X] += NN;
            break;
        case 0xa:
            chip8->I = NNN;
            break;
        case 0xd:
            assert(chip8->V[X] < 64);
            assert(chip8->V[Y] < 32);
            assert(chip8->I + N < MEMORY_SIZE);
            for (int i = 0; i < N; i++) {
                uint8_t byte = chip8->memory[chip8->I + i];
                for (int j = 0; j < 8; j++) {
                    uint8_t pixel = (byte >> (7 - j)) & 1;
                    chip8->display[chip8->V[Y] + i][chip8->V[X] + j] ^= pixel;
                    if (pixel && chip8->display[chip8->V[Y] + i][chip8->V[X] + j]) {
                        chip8->V[0xF] = 1;  
                    }
                }

            }
    }
}

Chip8 chip8;

while (true) {
    uint16_t instruccion = fetch(&chip8);
    avanzar_pc(&chip8);
    decode_and_execute(&chip8, instruccion);
}