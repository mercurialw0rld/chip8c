#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define MEMORY_SIZE 4096  
#define INSTRUCCIONES_POR_FRAME 12

typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint16_t pc;
    uint8_t V[16]; 
    uint16_t I;  
    uint8_t display[32][64];  
    uint16_t subroutine_stack[16];
    uint8_t stack_pointer;
    uint8_t teclado[16];
    uint8_t tecla_evento;
    uint8_t delay_timer; 
    uint8_t sound_timer;
} Chip8;

uint8_t mapear_tecla(SDL_Keycode tecla) {
    switch (tecla) {
        case SDLK_1:
        case SDLK_UP:
        case SDLK_w:
            return 0x1;
        case SDLK_2:
            return 0x2;
        case SDLK_3:
            return 0x3;
        case SDLK_4:
        case SDLK_i:
            return 0xC;
        case SDLK_q:
        case SDLK_DOWN:
            return 0x4;
        case SDLK_e:
            return 0x6;
        case SDLK_r:
        case SDLK_k:
            return 0xD;
        case SDLK_a:
        case SDLK_LEFT:
            return 0x7;
        case SDLK_s:
            return 0x8;
        case SDLK_d:
        case SDLK_RIGHT:
            return 0x9;
        case SDLK_f:
            return 0xE;
        case SDLK_z:
            return 0xA;
        case SDLK_x:
            return 0x0;
        case SDLK_c:
            return 0xB;
        case SDLK_v:
            return 0xF;
        default: return 0xFF;
    }
}

void procesar_eventos(Chip8 *chip8, int *ejecutando) {
    SDL_Event evento;

    while (SDL_PollEvent(&evento)) {
        if (evento.type == SDL_QUIT) {
            *ejecutando = 0;
            return;
        }

        if (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE) {
            *ejecutando = 0;
            return;
        }

        if (evento.type == SDL_KEYDOWN || evento.type == SDL_KEYUP) {
            uint8_t tecla = mapear_tecla(evento.key.keysym.sym);

            if (tecla != 0xFF) {
                chip8->teclado[tecla] = (evento.type == SDL_KEYDOWN);

                if (evento.type == SDL_KEYDOWN && !evento.key.repeat) {
                    chip8->tecla_evento = tecla;
                }
            }
        }
    }
}

void esperar_60hz() {
    SDL_Delay(1000 / 60);  // esperar ~16ms
}

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
    assert(chip8->pc <= MEMORY_SIZE - 2);
    chip8->pc += 2;
}

uint16_t fetch(Chip8 *chip8){
    assert(chip8->pc < MEMORY_SIZE - 1);
    return leer_instruccion(chip8, chip8->pc);
}
 
void decode_and_execute(Chip8 *chip8, uint16_t instruccion) {
    uint8_t  tipo = (instruccion & 0xF000) >> 12;  
    uint8_t  X    = (instruccion & 0x0F00) >> 8;   
    uint8_t  Y    = (instruccion & 0x00F0) >> 4;   
    uint8_t  N    = (instruccion & 0x000F);       
    uint8_t  NN   = (instruccion & 0x00FF);         
    uint16_t NNN  = (instruccion & 0x0FFF);         
     switch(tipo)  {
        case 0x6:
            chip8->V[X] = NN;
            break;    
        case 0x1:
            chip8->pc = NNN;
            break;
        case 0x3:
            if (chip8->V[X] == NN) {
                chip8->pc += 2;
            }
            break;
        case 0x4:
            if (chip8->V[X] != NN) {
                chip8->pc += 2;
            }
            break;
        case 0x5:
            if (N == 0x0 && chip8->V[X] == chip8->V[Y]) {
                chip8->pc += 2;
            }
            break;
        case 0x7:
            chip8->V[X] += NN;
            break;
        case 0x8:
            switch (N) {
                case 0x0:
                    chip8->V[X] = chip8->V[Y];
                    break;
                case 0x1:
                    chip8->V[X] |= chip8->V[Y];
                    break;
                case 0x2:
                    chip8->V[X] &= chip8->V[Y];
                    break;
                case 0x3:
                    chip8->V[X] ^= chip8->V[Y];
                    break;
                case 0x4: {
                    uint16_t suma = chip8->V[X] + chip8->V[Y];
                    chip8->V[0xF] = suma > 0xFF;
                    chip8->V[X] = (uint8_t)suma;
                    break;
                }
                case 0x5:
                    chip8->V[0xF] = chip8->V[X] >= chip8->V[Y];
                    chip8->V[X] -= chip8->V[Y];
                    break;
                case 0x6:
                    chip8->V[0xF] = chip8->V[X] & 0x1;
                    chip8->V[X] >>= 1;
                    break;
                case 0x7:
                    chip8->V[0xF] = chip8->V[Y] >= chip8->V[X];
                    chip8->V[X] = chip8->V[Y] - chip8->V[X];
                    break;
                case 0xE:
                    chip8->V[0xF] = (chip8->V[X] & 0x80) != 0;
                    chip8->V[X] <<= 1;
                    break;
            }
            break;
        case 0x9:
            if (N == 0x0 && chip8->V[X] != chip8->V[Y]) {
                chip8->pc += 2;
            }
            break;
        case 0xA:
            chip8->I = NNN;
            break;
        case 0xB:
            chip8->pc = NNN + chip8->V[0];
            break;
        case 0xC:
            chip8->V[X] = (rand() & 0xFF) & NN;
            break;
        case 0x2:
            assert(chip8->stack_pointer < 16);
            chip8->subroutine_stack[chip8->stack_pointer++] = chip8->pc;
            chip8->pc = NNN;
            break;
        case 0x0:
            switch (NN) {
                case 0xE0:
                    for (int y = 0; y < 32; y++) {
                        for (int x = 0; x < 64; x++) {
                            chip8->display[y][x] = 0;
                        }
                    }
                    break;
                case 0xEE:
                    assert(chip8->stack_pointer > 0);
                    chip8->pc = chip8->subroutine_stack[--chip8->stack_pointer];
                    break;
            }
            break;
        case 0xd:
            assert(chip8->V[X] < 64);
            assert(chip8->V[Y] < 32);
            assert(chip8->I + N < MEMORY_SIZE);
            chip8->V[0xF] = 0;
            for (int i = 0; i < N; i++) {
                uint8_t byte = chip8->memory[chip8->I + i];
                for (int j = 0; j < 8; j++) {
                    uint8_t pixel = (byte >> (7 - j)) & 1;
                    uint8_t x = (chip8->V[X] + j) % 64; 
                    uint8_t y = (chip8->V[Y] + i) % 32;
                    chip8->display[y][x] ^= pixel;
                    if (pixel && chip8->display[y][x] == 0) {
                        chip8->V[0xF] = 1;  
                    }
                }

            }
            break;
        case 0xE:
            switch (NN) {
                case 0x9E:
                    if (chip8->teclado[chip8->V[X] & 0x0F]) {
                        chip8->pc += 2;
                    }
                    break;
                case 0xA1:
                    if (!chip8->teclado[chip8->V[X] & 0x0F]) {
                        chip8->pc += 2;
                    }
                    break;
            }
            break;
        case 0xF:
            switch (NN) {
                case 0x07:
                    chip8->V[X] = chip8->delay_timer;
                    break;
                case 0x15:
                    chip8->delay_timer = chip8->V[X];
                    break;
                case 0x18:
                    chip8->sound_timer = chip8->V[X];
                    break;
                case 0x1E:
                    chip8->I += chip8->V[X];
                    break;
                case 0x29:
                    chip8->I = chip8->V[X] * 5;
                    break;
                case 0x33: {
                    uint8_t valor = chip8->V[X];
                    chip8->memory[chip8->I] = valor / 100;
                    chip8->memory[chip8->I + 1] = (valor / 10) % 10;
                    chip8->memory[chip8->I + 2] = valor % 10;
                    break;
                }
                case 0x55:
                    for (uint8_t i = 0; i <= X; i++) {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }
                    break;
                case 0x65:
                    for (uint8_t i = 0; i <= X; i++) {
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    }
                    break;
                case 0x0A:
                    if (chip8->tecla_evento == 0xFF) {
                        chip8->pc -= 2;
                    } else {
                        chip8->V[X] = chip8->tecla_evento;
                        chip8->tecla_evento = 0xFF;
                    }
                    break;
            }
            break;
    }
}

void cargar_rom(Chip8 *chip8, const char *path) {
    FILE *rom = fopen(path, "rb");
    assert(rom != NULL);
    
    fread(&chip8->memory[0x200], 1, MEMORY_SIZE - 0x200, rom);
    
    fclose(rom);
}

Chip8 chip8;

int main(void) {
    chip8 = (Chip8){0};
    chip8.pc = 0x200;
    chip8.tecla_evento = 0xFF;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *ventana = SDL_CreateWindow(
        "CHIP-8",           // título
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,  // posición
        640, 320,           // tamaño (64*10, 32*10)
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(ventana, -1, 0);
    cargar_rom(&chip8, "Pong.ch8");

    int ejecutando = 1;

    while (ejecutando) {
        procesar_eventos(&chip8, &ejecutando);

        if (chip8.pc > MEMORY_SIZE - 2) {
            ejecutando = 0;
            continue;
        }

        // ejecutar varias instrucciones por frame
        for (int i = 0; i < INSTRUCCIONES_POR_FRAME; i++) {
            if (chip8.pc > MEMORY_SIZE - 2) {
                break;
            }

            uint16_t instruccion = fetch(&chip8);
            avanzar_pc(&chip8);
            decode_and_execute(&chip8, instruccion);
        }

        // una vez por frame (60Hz) decrementar timers
        if (chip8.delay_timer > 0) chip8.delay_timer--;
        if (chip8.sound_timer > 0) chip8.sound_timer--;
        // limpiar pantalla
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // negro
        SDL_RenderClear(renderer);

        // dibujar pixels prendidos
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // blanco
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                if (chip8.display[y][x]) {
                    SDL_Rect rect = {x * 10, y * 10, 10, 10};
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }

        // mostrar frame
        SDL_RenderPresent(renderer);

        // esperar hasta el próximo frame
        esperar_60hz();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(ventana);
    SDL_Quit();

    return 0;
}