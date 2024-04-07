#define SDL_MAIN_HANDLED
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <limits>
#include <numbers>
#include <ranges>
#include <span>
#include <vector>

using vec2_t = glm::vec<2, float>;
using color_t = glm::vec<4, uint8_t>;

const constexpr int32_t PIXEL_SCALE = 4;
int32_t numLines = 16;
float scale = 100;
const constexpr float lineDist = 1;

const constexpr int32_t radius = 120;

const constexpr int64_t xmin = -radius, xmax = radius, ymin = -radius, ymax = radius, deltax = xmax - xmin + 1, deltay = ymax - ymin + 1;
constexpr float e0 = 8.8541878128E-12;
constexpr float k = 4 * std::numbers::pi * e0;

namespace colors
{
    constexpr color_t red(0, 0, 255, 255), green(0, 255, 0, 255), blue(255, 0, 0, 255), white(255, 255, 255, 255), black(0, 0, 0, 255);
}

struct charge_t
{
    vec2_t pos;
    float strength;
};

std::array<charge_t, 2> charges = {{{{-60, 0}, 20}, {{60, 0}, -20}}};

vec2_t forceAt(vec2_t p)
{
    vec2_t sum(0);
    for (charge_t c : charges)
    {
        if (p == c.pos)
        {
            return vec2_t(0);
        }
        vec2_t r = p - c.pos;
        vec2_t rhat = glm::normalize(r);
        sum += c.strength / std::pow(glm::length(r) / 100.0f, 2.0f) * rhat;
    }
    return sum;
}

int main(int argc, char** argv)
{
    bool quit = false;
    SDL_Event event;

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Gauss' law", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, PIXEL_SCALE * deltax, PIXEL_SCALE * deltay, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    SDL_Surface* image = SDL_CreateRGBSurface(0, deltax, deltay, 32, 0, 0, 0, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, deltax, deltay);

    while (!quit)
    {
        auto vecs = std::vector<vec2_t>(deltay * deltax);
        vec2_t max(std::numeric_limits<float>::min()), min(std::numeric_limits<float>::max());
        for (size_t y = 0; y < deltay; y++)
        {
            for (size_t x = 0; x < deltax; x++)
            {
                vec2_t p(static_cast<float>(x) + xmin, static_cast<float>(y) + ymin);
                if (std::ranges::find_if(charges, [=](charge_t c) { return c.pos == p; }) != std::cend(charges))
                {
                    continue;
                }
                vec2_t force = forceAt(p);
                min = vec2_t(std::min(min.x, force.x), std::min(min.y, force.y));
                max = vec2_t(std::max(max.x, force.x), std::max(max.y, force.y));
                vecs.at(y * deltax + x) = force;
            }
        }
        vec2_t delta = max - min;
        void* ptr;
        int pitch = sizeof(color_t) * deltax;
        SDL_LockTexture(texture, NULL, &ptr, &pitch);
        auto pixels = std::span<color_t>(static_cast<color_t*>(ptr), deltax * deltay);
        for (size_t y = 0; y < deltay; y++)
        {
            for (size_t x = 0; x < deltax; x++)
            {
                color_t color = color_t(
                    0,
                    static_cast<uint8_t>((vecs.at(y * deltax + x).y - min.y) / delta.y * 255*scale), static_cast<uint8_t>((vecs.at(y * deltax + x).x - min.x) / delta.x * 255*scale), 0);
                pixels[y * deltax + x] = color;
            }
        }
        for (charge_t c : charges)
        {
            pixels[c.pos.y - ymin * deltax + (c.pos.x - xmin)] = c.strength > 0 ? color_t(0, 0, 255, 0) : color_t(255, 0, 0, 0);
            for (float theta = 0; theta < 2 * std::numbers::pi; theta += 2 * std::numbers::pi / numLines)
            {
                vec2_t p = c.pos + lineDist * vec2_t(std::cos(theta), std::sin(theta));
                vec2_t force = forceAt(p);
                size_t t = 0;
                while (force != vec2_t(0.0f) && !(p.x < xmin || p.x > xmax || p.y < ymin || p.y > ymax) && t < 10000)
                {
                    size_t pos = static_cast<size_t>(p.y - ymin) * deltax + static_cast<size_t>(p.x - xmin);
                    pixels[pos] = colors::white;
                    force = forceAt(p);
                    p += (c.strength > 0 ? 1.0f : -1.0f) * glm::normalize(force);
                    t++;
                }
            }
        }
        SDL_UnlockTexture(texture);

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                quit = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                quit = true;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("controls");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::SliderInt("NumLines", &numLines, 0, 32);
        ImGui::SliderFloat("scale", &scale, 1, 10000);
        for (auto& c : charges)
        {
            ImGui::PushID(&c);
            ImGui::SliderFloat("charge", &c.strength, -100.0f, 100.0f);
            ImGui::InputFloat2("position", static_cast<float*>(glm::value_ptr(c.pos)));
            ImGui::PopID();
        }
        ImGui::End();

        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

        SDL_RenderPresent(renderer);
    }
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}