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
int32_t numLines = 2;
float scale = 100;
const constexpr float lineDist = 2;

const constexpr float FLOAT_EPSILON = 0.05;

const constexpr int32_t radius = 120;
const constexpr int64_t xmin = -radius, xmax = radius, ymin = -radius, ymax = radius, deltax = 2 * radius + 1, deltay = 2 * radius + 1;

// const constexpr int32_t width = 1920;
// const constexpr int32_t height = 1080;
// const constexpr int64_t xmin = -(width / 2), xmax = width / 2, ymin = -(height / 2), ymax = height / 2, deltax = width, deltay = height;

constexpr float e0 = 8.8541878128E-12;
constexpr float k = 4 * std::numbers::pi * e0;

float first_ring = 10;
int32_t tmax = 1000;
float epsilon = 0.1;
int32_t ring_count = 1;
glm::vec<2, int32_t> cursor_pos;

namespace colors
{
    constexpr color_t red(0, 0, 0, 255), green(0, 255, 0, 255), blue(255, 0, 0, 255), white(255, 255, 255, 255), black(0, 0, 0, 255);
}

struct charge_t
{
    vec2_t pos;
    float strength;
};

std::array<charge_t, 2> charges = {{{{-60, 0}, -20}, {{60, 0}, -20}}};
// std::array<charge_t, 3> charges = {{{{-60, 0}, -20}, {{60, 0}, -20}, {{0, 60}, 20}}};

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

void render(std::span<color_t>& pixels, bool equipotential, bool fieldlines)
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
    for (size_t y = 0; y < deltay; y++)
    {
        for (size_t x = 0; x < deltax; x++)
        {
            color_t color = color_t(
                0, static_cast<uint8_t>((vecs.at(y * deltax + x).y - min.y) / delta.y * 255 * scale),
                static_cast<uint8_t>((vecs.at(y * deltax + x).x - min.x) / delta.x * 255 * scale), 0);
            pixels[y * deltax + x] = color;
        }
    }
    if (fieldlines)
    {
        for (charge_t c : charges)
        {
            pixels[c.pos.y - ymin * deltax + (c.pos.x - xmin)] = c.strength > 0 ? colors::red : colors::blue;
            for (int i = 1; i <= numLines; i++)
            {
                float theta = i * (2 * std::numbers::pi) / numLines;
                vec2_t p = c.pos + lineDist * vec2_t(std::cos(theta), std::sin(theta));
                if (std::ranges::find_if(
                        charges,
                        [=](charge_t c2)
                        {
                            if ((c.pos != c2.pos) && ((c.strength < 0 && c2.strength < 0) || (c.strength > 0 && c2.strength > 0)))
                            {
                                vec2_t v1 = glm::normalize(p-c.pos), v2 = glm::normalize(c2.pos - c.pos);
                                return std::abs(v1.x - v2.x) < FLOAT_EPSILON && std::abs(v1.y - v2.y) < FLOAT_EPSILON;
                            }
                            return false;
                        }) != std::cend(charges))
                {
                    continue;
                }
                vec2_t force = forceAt(p);
                size_t t = 0;
                while (force != vec2_t(0.0f) && !(p.x < xmin || p.x > xmax || p.y < ymin || p.y > ymax) && t < tmax)
                {
                    size_t pos = static_cast<size_t>(p.y - ymin) * deltax + static_cast<size_t>(p.x - xmin);
                    pixels[pos] = colors::white;
                    force = forceAt(p);
                    p += (c.strength > 0 ? 1.0f : -1.0f) * glm::normalize(force);
                    t++;
                }
            }
        }
    }
    if (equipotential)
    {
        for (size_t y = 0; y < deltay; y++)
        {
            for (size_t x = 0; x < deltax; x++)
            {
                for (int i = 0; i < ring_count; i++)
                {
                    if (std::abs(glm::length(vecs.at(y * deltax + x)) - first_ring * (i + 1)) < epsilon)
                    {
                        pixels[y * deltax + x] = colors::red;
                    }
                }
            }
        }
    }
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

    bool rerender = true;
    bool live = false;
    bool equipotential = true;
    bool fieldlines = true;
    while (!quit)
    {
        void* ptr;
        int pitch = sizeof(color_t) * deltax;

        if (rerender || live)
        {
            SDL_LockTexture(texture, NULL, &ptr, &pitch);
            auto pixels = std::span<color_t>(static_cast<color_t*>(ptr), deltax * deltay);
            render(pixels, equipotential, fieldlines);
            SDL_UnlockTexture(texture);
            rerender = false;
        }

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                quit = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                quit = true;
            if (event.type == SDL_MOUSEMOTION)
            {
                cursor_pos.x = event.motion.x / PIXEL_SCALE;
                cursor_pos.y = event.motion.y / PIXEL_SCALE;
            }
        }
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("controls");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Checkbox("Field Lines", &fieldlines);
        if (fieldlines)
        {
            ImGui::SliderInt("NumLines", &numLines, 0, 32);
            ImGui::SliderInt("tmax", &tmax, 0, 1000);
        }
        ImGui::Checkbox("Equipotential Lines", &equipotential);
        if (equipotential)
        {
            ImGui::SliderFloat("first ring", &first_ring, 1, 1000);
            ImGui::SliderInt("ring count", &ring_count, 0, 10);
            ImGui::SliderFloat("epsilon", &epsilon, 0, 100);
        }
        ImGui::SliderFloat("scale", &scale, 1, 10000);
        vec2_t force = forceAt(cursor_pos);
        ImGui::Text(
            "Force under cursor, x:%d, y:%d,\n %.3fi+%.3fj, magnitude:%.3f", cursor_pos.x + xmin, cursor_pos.y + ymin, force.x, force.y, glm::length(force));
        for (auto& c : charges)
        {
            ImGui::PushID(&c);
            ImGui::SliderFloat("charge", &c.strength, -100.0f, 100.0f);
            ImGui::InputFloat2("position", static_cast<float*>(glm::value_ptr(c.pos)));
            ImGui::PopID();
        }
        rerender = ImGui::Button("Render");
        ImGui::Checkbox("Live Update", &live);
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