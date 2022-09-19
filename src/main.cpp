
#define WIN32_MEAN_AND_LEAN
#define NOMINMAX

#include "core.hpp"

#include <glad/glad.h>

#include "imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Engine/window.hpp"

#include "Math/transform.hpp"

int main(int argc, char** argv) {
    window_t window;
    window.width = 640;
    window.height = 480;

    window.set_event_callback([](auto& event){});

    window.open_window();
    window.set_title("AAGE Template");
    
    v3f color{1.0f, 0.0f, 0.0f};
    transform_t transform;

    while(!window.should_close()) {
        color = glm::min(transform.xform(color), 1.0f);
        transform.rotate({0.00004f, 0.00002f, 0.00001f});

        glClearColor(color.r, color.g, color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /////////////////////////////
        window.poll_events();
        window.swap_buffers();
    }

    

    return 0;
}