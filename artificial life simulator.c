#define SDL_MAIN_HANDLED
#include <stdio.h>    // For input/output operations (printf)
#include <stdlib.h>   // For dynamic memory allocation (malloc, free) and random numbers (rand, srand)
#include <time.h>     // For seeding the random number generator (time)
#include <math.h>     // For mathematical functions (sqrt, atan2, cos, sin, round)

// Include SDL2 headers
#include <SDL2/SDL.h>

// --- Simulation Parameters ---
#define INITIAL_LIFE_FORMS 10
#define INITIAL_FOOD_SOURCES 50
#define MAX_LIFE_FORMS 200 // Maximum number of life forms to prevent excessive growth
#define MAX_FOOD_SOURCES 100 // Maximum number of food sources

// SDL window dimensions
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Scale simulation units to pixel units
#define SCALE_FACTOR 1.0 // 1 unit = 1 pixel for now, can be adjusted later

#define LIFE_FORM_RADIUS_PX 8  // Radius in pixels for rendering
#define FOOD_RADIUS_PX 3       // Radius in pixels for rendering

#define LIFE_FORM_RADIUS 8.0 // Conceptual radius for collision detection (same as px for simplicity)
#define FOOD_RADIUS 3.0      // Conceptual radius for collision detection (same as px for simplicity)

#define MAX_ENERGY 100.0
#define REPRODUCTION_THRESHOLD 80.0
#define ENERGY_LOSS_PER_STEP 0.05 // Slower for smoother animation
#define ENERGY_GAIN_FROM_FOOD 20.0
#define MAX_SPEED 1.5 // Max speed in simulation units

// --- Struct Definitions ---

// Represents a single artificial life form
typedef struct {
    double x, y;        // Position in simulation units
    double vx, vy;      // Velocity in simulation units per step
    double energy;      // Current energy level
    double speed_factor; // Genetic trait: affects movement speed
    int id;             // Unique identifier for the life form
    // Add color components for SDL rendering
    Uint8 r, g, b;
} LifeForm;

// Represents a food source in the environment
typedef struct {
    double x, y;        // Position in simulation units
    int is_present;     // Flag to check if food exists
} Food;

// --- Global Arrays for Simulation Entities ---
LifeForm* life_forms;
Food* food_sources;
int life_form_count = 0;
int food_count = 0;

// SDL related global variables
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

// --- Function Prototypes ---
// SDL Initialization and Cleanup
int init_sdl();
void close_sdl();

// Simulation core functions
void initialize_simulation();
void spawn_life_form(double x, double y, double energy, double speed_factor, Uint8 r, Uint8 g, Uint8 b);
void spawn_food(double x, double y);
void update_life_form(LifeForm* lf, const Food* foods, int num_foods);
void handle_interactions();
void simulate_step();
double distance_sq(double x1, double y1, double x2, double y2);
void cleanup_simulation_data(); // Cleans up dynamic arrays

// Drawing functions
void draw_circle(SDL_Renderer* renderer, int x, int y, int radius);
void draw_simulation_state();

// --- Main Function ---
int main(int argc, char* args[]) {
    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Initialize SDL
    if (!init_sdl()) {
        printf("Failed to initialize SDL!\n");
        return 1;
    }

    // Allocate memory for entities
    life_forms = (LifeForm*)malloc(MAX_LIFE_FORMS * sizeof(LifeForm));
    food_sources = (Food*)malloc(MAX_FOOD_SOURCES * sizeof(Food));

    if (life_forms == NULL || food_sources == NULL) {
        fprintf(stderr, "Memory allocation failed for simulation entities!\n");
        close_sdl();
        return 1;
    }

    // Initialize the simulation data
    initialize_simulation();

    // Main simulation loop flag
    int quit = 0;
    SDL_Event e;

    printf("Artificial Life Simulator (C Language with SDL2)\n");
    printf("----------------------------------------------\n");
    printf("Press ESC or close the window to quit.\n");
    printf("Life forms: %d, Food: %d\n", life_form_count, food_count);

    // Game loop
    while (!quit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
            // User presses a key
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1; // Quit on ESC key
                }
            }
        }

        // --- Simulation Logic Update ---
        simulate_step();

        // --- Render ---
        draw_simulation_state();

        // Optional: Add a small delay to control simulation speed
        SDL_Delay(10); // Adjust for desired speed

        // Update console counts (optional, for debugging)
        // printf("\rLife Forms: %d, Food: %d", life_form_count, food_count); // Use \r to overwrite line
        // fflush(stdout); // Flush stdout to show update immediately
    }

    printf("\nSimulation ended.\n");

    // Clean up allocated memory for simulation data
    cleanup_simulation_data();
    // Close SDL subsystems
    close_sdl();

    return 0;
}

// --- Function Implementations ---

// Initializes SDL and creates window/renderer
int init_sdl() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Create window
    gWindow = SDL_CreateWindow("Artificial Life Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Create renderer for window
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Set render color to light blue background
    SDL_SetRenderDrawColor(gRenderer, 173, 216, 230, 255); // Light sky blue

    return 1;
}

// Closes SDL subsystems and destroys window/renderer
void close_sdl() {
    // Destroy renderer
    if (gRenderer != NULL) {
        SDL_DestroyRenderer(gRenderer);
        gRenderer = NULL;
    }

    // Destroy window
    if (gWindow != NULL) {
        SDL_DestroyWindow(gWindow);
        gWindow = NULL;
    }

    // Quit SDL subsystems
    SDL_Quit();
}

// Calculates the squared Euclidean distance between two points
double distance_sq(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return dx * dx + dy * dy;
}

// Initializes the life forms and food sources
void initialize_simulation() {
    life_form_count = 0;
    food_count = 0;

    for (int i = 0; i < INITIAL_LIFE_FORMS; ++i) {
        // Generate random color for each initial life form
        Uint8 r = rand() % 256;
        Uint8 g = rand() % 256;
        Uint8 b = rand() % 256;
        spawn_life_form(
            (double)rand() / RAND_MAX * WINDOW_WIDTH,   // Random X within window
            (double)rand() / RAND_MAX * WINDOW_HEIGHT,  // Random Y within window
            MAX_ENERGY / 2.0,                           // Half energy
            1.0,                                        // Default speed factor
            r, g, b
        );
    }

    for (int i = 0; i < INITIAL_FOOD_SOURCES; ++i) {
        spawn_food(
            (double)rand() / RAND_MAX * WINDOW_WIDTH,
            (double)rand() / RAND_MAX * WINDOW_HEIGHT
        );
    }
}

// Spawns a new life form at a given position with initial properties
void spawn_life_form(double x, double y, double energy, double speed_factor, Uint8 r, Uint8 g, Uint8 b) {
    if (life_form_count < MAX_LIFE_FORMS) {
        life_forms[life_form_count].x = x;
        life_forms[life_form_count].y = y;
        life_forms[life_form_count].energy = energy;
        life_forms[life_form_count].speed_factor = speed_factor;
        life_forms[life_form_count].id = life_form_count; // Simple ID
        life_forms[life_form_count].r = r;
        life_forms[life_form_count].g = g;
        life_forms[life_form_count].b = b;
        // Initial random velocity
        life_forms[life_form_count].vx = ((double)rand() / RAND_MAX - 0.5) * MAX_SPEED * speed_factor;
        life_forms[life_form_count].vy = ((double)rand() / RAND_MAX - 0.5) * MAX_SPEED * speed_factor;
        life_form_count++;
    } else {
        // printf("Max life forms reached! Cannot spawn new life form.\n");
    }
}

// Spawns a new food source at a given position
void spawn_food(double x, double y) {
    if (food_count < MAX_FOOD_SOURCES) {
        food_sources[food_count].x = x;
        food_sources[food_count].y = y;
        food_sources[food_count].is_present = 1; // Mark as present
        food_count++;
    } else {
        // printf("Max food sources reached! Cannot spawn new food.\n");
    }
}

// Updates the state of a single life form
void update_life_form(LifeForm* lf, const Food* foods, int num_foods) {
    // 1. Energy loss
    lf->energy -= ENERGY_LOSS_PER_STEP;

    // 2. Movement
    lf->x += lf->vx;
    lf->y += lf->vy;

    // 3. Bounce off walls (window boundaries)
    if (lf->x - LIFE_FORM_RADIUS < 0) {
        lf->x = LIFE_FORM_RADIUS;
        lf->vx *= -1;
    } else if (lf->x + LIFE_FORM_RADIUS > WINDOW_WIDTH) {
        lf->x = WINDOW_WIDTH - LIFE_FORM_RADIUS;
        lf->vx *= -1;
    }

    if (lf->y - LIFE_FORM_RADIUS < 0) {
        lf->y = LIFE_FORM_RADIUS;
        lf->vy *= -1;
    } else if (lf->y + LIFE_FORM_RADIUS > WINDOW_HEIGHT) {
        lf->y = WINDOW_HEIGHT - LIFE_FORM_RADIUS;
        lf->vy *= -1;
    }

    // 4. Simple seeking behavior (towards nearest food)
    double nearest_food_dist_sq = -1.0;
    int nearest_food_idx = -1;

    for (int i = 0; i < num_foods; ++i) {
        if (foods[i].is_present) {
            double dist_sq = distance_sq(lf->x, lf->y, foods[i].x, foods[i].y);
            if (nearest_food_idx == -1 || dist_sq < nearest_food_dist_sq) {
                nearest_food_dist_sq = dist_sq;
                nearest_food_idx = i;
            }
        }
    }

    if (nearest_food_idx != -1) {
        // Adjust velocity towards nearest food
        double angle = atan2(foods[nearest_food_idx].y - lf->y, foods[nearest_food_idx].x - lf->x);
        double current_speed = sqrt(lf->vx * lf->vx + lf->vy * lf->vy);
        if (current_speed == 0) current_speed = MAX_SPEED * lf->speed_factor; // Avoid division by zero, give it initial speed
        
        lf->vx = cos(angle) * MAX_SPEED * lf->speed_factor;
        lf->vy = sin(angle) * MAX_SPEED * lf->speed_factor;
    } else {
        // If no food, randomly change direction occasionally
        if ((double)rand() / RAND_MAX < 0.01) { // 1% chance to change direction
            lf->vx = ((double)rand() / RAND_MAX - 0.5) * MAX_SPEED * lf->speed_factor;
            lf->vy = ((double)rand() / RAND_MAX - 0.5) * MAX_SPEED * lf->speed_factor;
        }
    }

    // Clamp energy
    if (lf->energy > MAX_ENERGY) lf->energy = MAX_ENERGY;
    if (lf->energy < 0) lf->energy = 0;
}

// Handles interactions between life forms and food
void handle_interactions() {
    // Check for feeding
    for (int i = 0; i < life_form_count; ++i) {
        for (int j = 0; j < food_count; ++j) {
            if (food_sources[j].is_present) {
                double combined_radius_sq = (LIFE_FORM_RADIUS + FOOD_RADIUS) * (LIFE_FORM_RADIUS + FOOD_RADIUS);
                if (distance_sq(life_forms[i].x, life_forms[i].y, food_sources[j].x, food_sources[j].y) < combined_radius_sq) {
                    life_forms[i].energy += ENERGY_GAIN_FROM_FOOD;
                    food_sources[j].is_present = 0; // Food consumed
                    // Try to respawn new food
                    if ((double)rand() / RAND_MAX < 0.8) { // 80% chance to respawn food
                         spawn_food(
                            (double)rand() / RAND_MAX * WINDOW_WIDTH,
                            (double)rand() / RAND_MAX * WINDOW_HEIGHT
                        );
                    }
                }
            }
        }
    }

    // Clean up consumed food and compact the array (simple removal)
    int current_food_idx = 0;
    for (int i = 0; i < food_count; ++i) {
        if (food_sources[i].is_present) {
            food_sources[current_food_idx++] = food_sources[i];
        }
    }
    food_count = current_food_idx;
}

// Performs one step of the simulation
void simulate_step() {
    // 1. Update all life forms
    for (int i = 0; i < life_form_count; ++i) {
        update_life_form(&life_forms[i], food_sources, food_count);
    }

    // 2. Handle interactions (feeding)
    handle_interactions();

    // 3. Handle reproduction and death
    // Create a temporary array for the next generation of life forms
    LifeForm* temp_life_forms = (LifeForm*)malloc(MAX_LIFE_FORMS * sizeof(LifeForm));
    if (temp_life_forms == NULL) {
        fprintf(stderr, "Memory allocation failed during reproduction temp array!\n");
        // Handle error: perhaps exit or log and continue with existing life forms
        return;
    }
    int temp_life_form_count = 0;

    for (int i = 0; i < life_form_count; ++i) {
        LifeForm* lf = &life_forms[i];

        // If life form is alive, potentially reproduce and add to temp array
        if (lf->energy > 0) {
            // Check if it's ready to reproduce and if there's space for offspring
            if (lf->energy >= REPRODUCTION_THRESHOLD && temp_life_form_count + 1 < MAX_LIFE_FORMS) {
                lf->energy /= 2; // Share energy with offspring
                double new_speed_factor = lf->speed_factor + ((double)rand() / RAND_MAX - 0.5) * 0.4; // Mutation
                // Clamp speed factor to reasonable range
                if (new_speed_factor < 0.5) new_speed_factor = 0.5;
                if (new_speed_factor > 2.0) new_speed_factor = 2.0;

                // Add parent to temp array
                if (temp_life_form_count < MAX_LIFE_FORMS) {
                    temp_life_forms[temp_life_form_count++] = *lf;
                }

                // Spawn offspring
                // Offspring inherits parent's color for simplicity
                spawn_life_form(
                    lf->x + ((double)rand() / RAND_MAX - 0.5) * 10.0, // Slightly offset position
                    lf->y + ((double)rand() / RAND_MAX - 0.5) * 10.0,
                    lf->energy, // Offspring gets half parent's energy
                    new_speed_factor,
                    lf->r, lf->g, lf->b
                );
            } else {
                // If not reproducing, just copy the life form to the new array
                if (temp_life_form_count < MAX_LIFE_FORMS) {
                    temp_life_forms[temp_life_form_count++] = *lf;
                }
            }
        }
    }

    // Replace old life_forms array with the new one
    // We can't directly assign as life_forms is a pointer to the start of memory.
    // Instead, we copy elements from temp_life_forms to life_forms
    if (life_form_count > temp_life_form_count) { // If there were deaths, reduce count
        life_form_count = temp_life_form_count;
    }
    for (int i = 0; i < temp_life_form_count; ++i) {
        life_forms[i] = temp_life_forms[i];
    }
    life_form_count = temp_life_form_count; // Update the global count

    free(temp_life_forms); // Free temporary array
    temp_life_forms = NULL; // Prevent dangling pointer
}


// Draws a filled circle using SDL_RenderDrawPoint
// This is a basic implementation and can be optimized or replaced with SDL_gfx
void draw_circle(SDL_Renderer* renderer, int x, int y, int radius) {
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if (distance_sq(x, y, i, j) <= radius * radius) {
                SDL_RenderDrawPoint(renderer, i, j);
            }
        }
    }
}

// Renders the current state of the simulation using SDL2
void draw_simulation_state() {
    // Clear screen
    SDL_SetRenderDrawColor(gRenderer, 173, 216, 230, 255); // Light sky blue background
    SDL_RenderClear(gRenderer);

    // Draw food sources
    SDL_SetRenderDrawColor(gRenderer, 76, 175, 80, 255); // Green for food
    for (int i = 0; i < food_count; ++i) {
        if (food_sources[i].is_present) {
            // Convert simulation coordinates to pixel coordinates
            int px = (int)round(food_sources[i].x * SCALE_FACTOR);
            int py = (int)round(food_sources[i].y * SCALE_FACTOR);
            draw_circle(gRenderer, px, py, FOOD_RADIUS_PX);
        }
    }

    // Draw life forms
    for (int i = 0; i < life_form_count; ++i) {
        LifeForm* lf = &life_forms[i];
        if (lf->energy > 0) {
            // Set life form's color
            SDL_SetRenderDrawColor(gRenderer, lf->r, lf->g, lf->b, 255);

            // Convert simulation coordinates to pixel coordinates
            int px = (int)round(lf->x * SCALE_FACTOR);
            int py = (int)round(lf->y * SCALE_FACTOR);
            draw_circle(gRenderer, px, py, LIFE_FORM_RADIUS_PX);

            // Draw energy bar (optional, simpler for graphical output)
            // Energy bar color from green to red
            Uint8 energy_r = (Uint8)(255 * (1 - (lf->energy / MAX_ENERGY)));
            Uint8 energy_g = (Uint8)(255 * (lf->energy / MAX_ENERGY));
            SDL_SetRenderDrawColor(gRenderer, energy_r, energy_g, 0, 255);
            SDL_Rect energy_bar = { px - LIFE_FORM_RADIUS_PX, py - LIFE_FORM_RADIUS_PX - 5,
                                    (int)(LIFE_FORM_RADIUS_PX * 2 * (lf->energy / MAX_ENERGY)), 3 };
            SDL_RenderFillRect(gRenderer, &energy_bar);
        }
    }

    // Update screen
    SDL_RenderPresent(gRenderer);
}

// Frees dynamically allocated memory for simulation data
void cleanup_simulation_data() {
    free(life_forms);
    free(food_sources);
    life_forms = NULL;
    food_sources = NULL;
}
