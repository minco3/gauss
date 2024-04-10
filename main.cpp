#define SDL_MAIN_HANDLED
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <SDL_image.h>
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

int32_t num_lines = 16;
const constexpr float line_dist = 2;

const constexpr float FLOAT_EPSILON = 0.05;

const constexpr int32_t PIXEL_SCALE = 4;
const constexpr int32_t radius = 150;
const constexpr int32_t xmin = -150, xmax = 150, ymin = -75, ymax = 75, deltax = 2 * 150 + 1, deltay = 2 * 75 + 1;

// const constexpr int32_t PIXEL_SCALE = 1;
// const constexpr int32_t width = 1920;
// const constexpr int32_t height = 1080;
// const constexpr int64_t xmin = -(width / 2), xmax = width / 2, ymin =
// -(height / 2), ymax = height / 2, deltax = width, deltay = height;

constexpr float e0 = 8.8541878128E-12;
constexpr float k = 4 * std::numbers::pi * e0;

int32_t arrow_distance = 50;
int32_t head_length = 5;
int32_t head_thickness = 3;
int32_t tmax = 200;
float equi_scale = 0.5f;
int32_t equipotential_t = 200;
int32_t equipotential_dist = 20;
int32_t ring_count = 2;
glm::vec<2, int32_t> cursor_pos;

bool equipotential = true, fieldlines = true, fieldcolor = false, arrows = true, symmetry = true;

namespace colors
{
    constexpr color_t red(255, 0, 0, 0), green(0, 255, 0, 0), blue(0, 0, 255, 0), white(255, 255, 255, 0), black(0, 0, 0, 0), yellow(255, 255, 0, 0);
}

color_t line_color = colors::black;

struct charge_t
{
    vec2_t pos;
    float strength;
};

std::ostream& operator<<(std::ostream& outs, vec2_t v)
{
    outs << v.x << " " << v.y;
    return outs;
}

// std::array<charge_t, 1> charges = {{{{0, 0}, 60}}};
std::array<charge_t, 2> charges = {{{{-60, 0}, -60}, {{60, 0}, -60}}};
// std::array<charge_t, 3> charges = {{{{-60, 0}, -20}, {{60, 0}, -20}, {{0,
// 60}, 20}}};

vec2_t forceAt(vec2_t p)
{
    vec2_t sum(0);
    for (charge_t c : charges)
    {
        if (p == c.pos)
        {
            return vec2_t(0.0f);
        }
        vec2_t r = p - c.pos;
        vec2_t rhat = glm::normalize(r);
        sum += c.strength / std::pow(glm::length(r), 2.0f) * rhat;
    }
    return sum;
}

void render(std::span<color_t>& pixels)
{
    auto vecs = std::vector<vec2_t>(deltay * deltax);
    vec2_t max(std::numeric_limits<float>::min()), min(std::numeric_limits<float>::max());
    vec2_t delta = max - min;
    for (size_t pos = 0; pos < deltay * deltax; pos++)
    {
        pixels[pos] = colors::white;
    }
    if (equipotential)
    {

        for (charge_t c : charges)
        {
            for (int k = 1; k <= ring_count; k++)
            {
                vec2_t p = c.pos + (static_cast<float>(k) * equipotential_dist) * glm::normalize(c.pos);
                for (int j = 0; j < equipotential_t; j++)
                {
                    vec2_t force = forceAt(p);
                    vec2_t p2 = p + glm::normalize(force);
                    vec2_t tangent = glm::normalize(p2 - p);
                    vec2_t normal(-tangent.y, tangent.x);
                    size_t pos = static_cast<size_t>(p.y - ymin) * deltax + static_cast<size_t>(p.x - xmin);
                    if (p.y >= ymin && p.y < ymax && p.x >= xmin && p.x < xmax)
                    {
                        pixels[pos] = colors::green;
                    }
                    size_t pos2 = static_cast<size_t>(-p.y - ymin) * deltax + static_cast<size_t>(p.x - xmin);
                    if (-p.y >= ymin && -p.y < ymax && p.x >= xmin && p.x < xmax)
                    {
                        pixels[pos2] = colors::green;
                    }
                    p += equi_scale * normal;
                }
            }
        }
    }
    if (fieldlines)
    {
        for (charge_t c : charges)
        {
            for (int i = 1; i <= num_lines; i++)
            {
                float theta = i * (2 * std::numbers::pi) / num_lines;
                vec2_t p = c.pos + line_dist * vec2_t(std::cos(theta), std::sin(theta));
                if (std::ranges::find_if(
                        charges,
                        [=](charge_t c2)
                        {
                            if ((c.pos != c2.pos) && ((c.strength < 0 && c2.strength < 0) || (c.strength > 0 && c2.strength > 0)))
                            {
                                vec2_t v1 = glm::normalize(p - c.pos), v2 = glm::normalize(c2.pos - c.pos);
                                return std::abs(v1.x - v2.x) < FLOAT_EPSILON && std::abs(v1.y - v2.y) < FLOAT_EPSILON;
                            }
                            return false;
                        }) != std::cend(charges))
                {
                    continue;
                }
                size_t t = 0;
                for (vec2_t force = forceAt(p); force != vec2_t(0.0f) && !(p.x < xmin || p.x > xmax || p.y < ymin || p.y > ymax) && t < tmax;
                     p += (c.strength > 0 ? 1.0f : -1.0f) * glm::normalize(force), t++)
                {
                    if (symmetry && std::ranges::find_if(
                                        charges,
                                        [=](charge_t c2)
                                        {
                                            if (c.pos != c2.pos)
                                            {
                                                return glm::distance(c.pos, p) > glm::distance(c2.pos, p);
                                            }
                                            return false;
                                        }) != std::cend(charges))
                    {
                        continue;
                    }
                    size_t pos = static_cast<size_t>(p.y - ymin) * deltax + static_cast<size_t>(p.x - xmin);
                    force = forceAt(p);
                    if (arrows && t == arrow_distance)
                    {
                        vec2_t p2 = p + glm::normalize(force);
                        vec2_t tangent = glm::normalize(p2 - p);
                        float phi = static_cast<float>(std::numbers::pi) / 4.0f;
                        float s = std::sin(phi);
                        float c = std::cos(phi);
                        glm::mat<2, 2, float> m{c, -s, s, c};  // rotation matrix
                        glm::mat<2, 2, float> m2{c, s, -s, c}; // rotation matrix
                        for (int t = 0; t < head_thickness; t++)
                        {
                            for (int l = 0; l < head_length; l++)
                            {
                                vec2_t p3 = p + static_cast<float>(l) * m * tangent + static_cast<float>(t) * tangent;
                                vec2_t p4 = p + static_cast<float>(l) * m2 * tangent + static_cast<float>(t) * tangent;
                                pixels[static_cast<size_t>(p3.y - ymin) * deltax + static_cast<size_t>(p3.x - xmin)] = colors::black;
                                pixels[static_cast<size_t>(p4.y - ymin) * deltax + static_cast<size_t>(p4.x - xmin)] = colors::black;
                            }
                        }
                        continue;
                    }
                    pixels[pos] = line_color;
                }
            }
        }
    }
    for (charge_t c : charges)
    {
        std::array<glm::ivec2, 9> kernel = {{{0, 0}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}};
        for (glm::ivec2 p : kernel)
        {
            pixels[(c.pos.y + p.y - ymin) * deltax + c.pos.x + p.x - xmin] = c.strength > 0 ? colors::red : colors::blue;
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

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    SDL_Surface* image = SDL_CreateRGBSurface(0, deltax, deltay, 32, 0, 0, 0, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, deltax, deltay);

    bool rerender = true;
    bool live = false;
    while (!quit)
    {
        void* ptr;
        int pitch = sizeof(color_t) * deltax;

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("controls");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        if (rerender || live)
        {
            SDL_LockTexture(texture, NULL, &ptr, &pitch);
            auto pixels = std::span<color_t>(static_cast<color_t*>(ptr), deltax * deltay);
            render(pixels);
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
        ImGui::Checkbox("Symmetry", &symmetry);
        ImGui::Checkbox("Field Lines", &fieldlines);
        if (fieldlines)
        {
            ImGui::SliderInt("NumLines", &num_lines, 0, 32);
            ImGui::SliderInt("tmax", &tmax, 0, 1000);
            ImGui::Checkbox("Arrows", &arrows);
            if (arrows)
            {
                ImGui::SliderInt("distance", &arrow_distance, 0, 100);
                ImGui::SliderInt("head length", &head_length, 0, 10);
                ImGui::SliderInt("head thickness", &head_thickness, 0, 5);
            }
        }
        ImGui::Checkbox("Equipotential Lines", &equipotential);
        if (equipotential)
        {
            ImGui::SliderInt("ring count", &ring_count, 0, 10);
            ImGui::SliderInt("equi t", &equipotential_t, 0, 10000);
            ImGui::SliderFloat("equi scale", &equi_scale, 0.0f, 1.0f);
            ImGui::SliderInt("equi dist", &equipotential_dist, 1, 100);
        }
        vec2_t force = forceAt(cursor_pos + glm::vec<2, int32_t>(xmin, ymin));
        ImGui::Text(
            "Force under cursor, x:%d, y:%d,\n %.3fi+%.3fj, magnitude:%.3f", cursor_pos.x + xmin, cursor_pos.y + ymin, force.x, force.y, glm::length(force));
        for (auto& c : charges)
        {
            ImGui::PushID(&c);
            ImGui::InputFloat("charge", &c.strength, -100.0f, 100.0f);
            ImGui::InputFloat2("position", static_cast<float*>(glm::value_ptr(c.pos)));
            ImGui::PopID();
        }
        rerender = ImGui::Button("Render");
        ImGui::Checkbox("Live Update", &live);
        if (ImGui::Button("Save to png"))
        {
            int width = deltax, height = deltay;
            SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);
            auto pixels = std::span<color_t>(static_cast<color_t*>(surface->pixels), deltax * deltay);
            render(pixels);
            IMG_SavePNG(surface, "out.png");
            SDL_FreeSurface(surface);
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