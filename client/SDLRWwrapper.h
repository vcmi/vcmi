#pragma once

struct SDL_RWops;
class CInputStream;

SDL_RWops* MakeSDLRWops(std::unique_ptr<CInputStream> in);
