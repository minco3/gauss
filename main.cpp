#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <numbers>
#include <ranges>
#include <vector>

int main(int argc, char** argv)
{
    bool quit = false;
    SDL_Event event;

    using vec2_t = glm::vec<2, double>;
    using color_t = glm::vec<4, uint8_t>;

    const constexpr uint32_t PIXEL_SCALE = 4;
    const constexpr uint32_t radius = 120;

    SDL_Init(SDL_INIT_VIDEO);

    const constexpr int32_t xmin = -radius, xmax = radius, ymin = -radius, ymax = radius, deltax = xmax - xmin + 1, deltay = ymax - ymin + 1;
    // constexpr double e0 = 8.8541878128E-12;
    constexpr double k = 4 * std::numbers::pi;

    // orange red blue green
    constexpr color_t color1(0, 255, 0, 255), color2(0, 255, 0, 255);

    struct charge_t
    {
        vec2_t pos;
        float strength;
    };

    const std::array<charge_t, 2> charges = {{{{-60, 0}, -20}, {{60, 0}, 20}}};

    SDL_Window* window = SDL_CreateWindow("Gauss' law", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, PIXEL_SCALE * deltax, PIXEL_SCALE * deltay, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    auto vecs = std::vector<vec2_t>(deltay * deltax);
    vec2_t max(std::numeric_limits<double>::min()), min(std::numeric_limits<double>::max());
    for (size_t y = 0; y < deltay; y++)
    {
        for (size_t x = 0; x < deltax; x++)
        {
            vec2_t p(static_cast<double>(x) + xmin, static_cast<double>(y) + ymin);
            if (std::ranges::find_if(charges, [=](charge_t c) { return c.pos == p; }) != std::cend(charges))
            {
                continue;
            }
            vec2_t sum(0);
            for (charge_t c : charges)
            {
                vec2_t r = p - c.pos;
                vec2_t rhat = glm::normalize(r);
                sum += c.strength / std::pow(glm::length(r) / 2.0, 2) * rhat;
            }
            min = vec2_t(std::min(min.x, sum.x), std::min(min.y, sum.y));
            max = vec2_t(std::max(max.x, sum.x), std::max(max.y, sum.y));
            vecs.at(y * deltax + x) = sum;
        }
    }
    std::cout << max.x << ' ' << max.y;
    auto pixels = std::vector<color_t>(deltax * deltay);
    for (size_t y = 0; y < deltay; y++)
    {
        for (size_t x = 0; x < deltax; x++)
        {
            // double xscale =((vecs.at(y * deltax + x).x / max.x) + 1.0)/2.0,
            //        yscale =((vecs.at(y * deltax + x).y / max.y) + 1.0)/2.0;
            // double xscale = std::lerp(min.x, max.x, ((vecs.at(y * deltax + x).x) + 1.0) / 2.0),
            //    yscale = std::lerp(min.y, max.y, ((vecs.at(y * deltax + x).y) + 1.0) / 2.0);
            double scale = std::lerp(min.x, max.x, (glm::length(vecs.at(y * deltax + x)) + 1.0) / 2.0);
            //    std::cout << vecs.at(y * deltax + x).x / max.x << ' ' << vecs.at(y * deltax + x).y / max.y << '\n';
            // pixels.at(y * deltax + x) = color_t(color1.r * xscale, color1.g * xscale, color1.b * xscale, color1.a);
            // pixels.at(y * deltax + x) += color_t(color2.r * yscale, color2.g * yscale, color2.b * yscale, color2.a);
            pixels.at(y * deltax + x) = color_t(color1.r * scale, color1.g * scale, color1.b * scale, color1.a);
        }
    }
    for (charge_t c : charges)
    {
        pixels.at((c.pos.y - ymin) * deltax + (c.pos.x - xmin)) = c.strength > 0 ? color_t(0, 0, 255, 0) : color_t(255, 0, 0, 0);
    }
    SDL_Surface* image = SDL_CreateRGBSurfaceFrom(reinterpret_cast<void*>(pixels.data()), deltax, deltay, 32, sizeof(color_t) * deltay, 0, 0, 0, 0);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, image);

    while (!quit)
    {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        }

        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}