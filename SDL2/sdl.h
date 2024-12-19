SDL_Init(SDL_INIT_VIDEO);
SDL_Window* window = SDL_CreateWindow("Media Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
