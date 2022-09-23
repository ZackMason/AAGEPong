
#define WIN32_MEAN_AND_LEAN
#define NOMINMAX

#include "core.hpp"

#include <glad/glad.h>
#include "glfw/glfw3.h"

#include "imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


#include "Engine/window.hpp"
#include "Engine/asset_loader.hpp"

#include "Networking/server.hpp"
#include "Networking/client.hpp"
#include "Networking/packet.hpp"

#include "Graphics/batch2d.hpp"

#include "Math/transform.hpp"

#include "nlohmann/json.hpp"

using nlohmann::json;

#include <chrono>
#include <thread>

struct pong_game_t;

struct game_server_t : public server_t {
    // using pointers to access game data because pong_game_t is not defined here 
    // and I want to keep this example just in one file
    f32* player_positions{nullptr};
    bool connected = false;
    
    void on_connect(const ENetEvent& event) override {
        logger_t::info(fmt::format("Player connected - {}", static_cast<void*>(event.peer)));

        connected = true;

        json j;
        //j["type"] = "connection"_sid;
        j["data"] = "hello";

        packet_t packet{j.dump()};
        packet.send_to_peer(event.peer);

        server_t::on_connect(event);
    }

    void on_disconnect(const ENetEvent& event) override {
        logger_t::info("Player disconnected");

        server_t::on_disconnect(event);
    }

    void on_packet(const ENetEvent& event) override {
        std::string data = reinterpret_cast<const char*>(event.packet->data);

        json j = json::parse(data);
        if (j.contains("type") == false) return;

        switch(static_cast<sid_t>(j["type"].get<size_t>())) {
            case "move"_sid: {
                auto dir = j["data"].get<int>();
                player_positions[1] += dir / 10.0f;
                player_positions[1] = player_positions[1] < 0.0f ? 0.0f : player_positions[1] > 1.0f ? 1.0f : player_positions[1];
            } break;
            default:
                logger_t::warn("Server - Unknown packet type");
        }

        server_t::on_packet(event);
    }
};

struct game_client_t : public client_t {
    f32* player_positions;
    f32* ball_position;
    f32* ball_velocity;
    std::function<void(void)> reset_game;

    void on_packet(const ENetEvent& event) override {
        if (event.packet->data != nullptr) {
            std::string data = reinterpret_cast<const char*>(event.packet->data);

            json j = json::parse(data);
            if (j.contains("type") == false) return;

            switch(static_cast<sid_t>(j["type"].get<size_t>())) {
                case "sync"_sid: {
                    auto [px,py] = j["data"]["paddle"].get<std::array<f32, 2>>();
                    auto [bx,by] = j["data"]["ball"]["position"].get<std::array<f32, 2>>();
                    auto [bvx,bvy] = j["data"]["ball"]["velocity"].get<std::array<f32, 2>>();

                    player_positions[0] = px; // update server paddle position
                    player_positions[1] = py;
                    ball_position[0] = bx;
                    ball_position[1] = by;
                    ball_velocity[0] = bvx;
                    ball_velocity[1] = bvy;
                }
                    break;
                case "reset"_sid:
                    reset_game();
                    break;
                case "disconnect"_sid:
                    break;
                default:
                    logger_t::warn("Unrecognized packet type");
            }
        }
        client_t::on_packet(event);
    }
};

struct pong_game_t {
    // server = 0, client = 1
    v2f player_positions{0.5f};
    v2f ball_position{};
    v2f ball_velocity{1};
    v2i player_score{0};

    window_t& window;
    batch2d_t gfx;
    asset_loader_t asset_loader;

    game_server_t* server{nullptr};
    game_client_t* client{nullptr};

    resource_handle_t<texture2d_t> paddle_texture;
    resource_handle_t<texture2d_t> ball_texture;

    explicit pong_game_t(window_t& _window, bool is_server = true) : window(_window) {
        window.width = 640;
        window.height = 480;

        window.set_event_callback([&](auto& event){
            event_handler_t handler;

            handler.dispatch<const key_event_t>(event, [&](const key_event_t& key){
                if (client) {
                    json j;
                    j["type"] = "move"_sid;
                    j["data"] = 
                        key.key == GLFW_KEY_W ? -1 : 
                        key.key == GLFW_KEY_S ?  1 : 0;

                    packet_t p{j.dump()};
                    p.send_to_peer(client->peer);
                }

                switch(key.key) {
                    case GLFW_KEY_R:
                        reset();
                        break;  
                    case GLFW_KEY_W:
                        player_positions[!!client] -= 0.1f;
                        break;
                    case GLFW_KEY_S:
                        player_positions[!!client] += 0.1f;
                        break;
                    case GLFW_KEY_I:
                        player_positions[!client] -= 0.1f;
                        break;
                    case GLFW_KEY_K:
                        player_positions[!client] += 0.1f;
                        break;
                }
                player_positions = glm::clamp(player_positions, v2f{0}, v2f{1});

                return true;
            });
        });

        shader_t::add_attribute_definition(
            {
                "vec2 aPos",
                "vec3 aUV",
            },
            "batch",
            GAME_ASSETS_PATH
        );

        asset_loader.asset_dir = GAME_ASSETS_PATH;
        paddle_texture = asset_loader.get_texture2d("textures/paddle.png");
        ball_texture = asset_loader.get_texture2d("textures/ball.png");

        enet_initialize();
        atexit(enet_deinitialize);

        if (is_server) {
            server = new game_server_t;
            server->player_positions = &player_positions.x;

            int fail_count = 5;
            wait_for_connection:
            server->tick_rate = 300;
            server->poll_events(10);
            server->tick_rate = 0;

            server->pump_event();
            if (!server->connected) {
                if (fail_count-- > 0) {
                    goto wait_for_connection;
                } else {
                    logger_t::info("No connections");
                    std::terminate();
                }
            }
        } else {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);

            logger_t::info("Connecting to server");

            client = new game_client_t;

            client->player_positions = &player_positions.x;
            client->ball_position = &ball_position.x;
            client->ball_velocity = &ball_velocity.x;

            client->reset_game = [&](){
                reset();
            };

            client->host_connect("127.0.0.1", 1234);
            client->tick_rate = 100;
            client->poll_events(1);
            client->tick_rate = 0;            
        }

        reset();
    }

    void reset() {
        player_positions = v2f{0.5f};
        ball_position = v2f{window.width/2, window.height/2};
        ball_velocity = v2f{1};

        if (server) {
            json j;
            j["type"] = "reset"_sid;
            j["data"] = "idk yet";

            server->broadcast_packet(packet_t{j.dump()});
        }
    }

    bool hit_paddle() const {
        const f32 paddle_height = static_cast<f32>(paddle_texture.get().height);
        const f32 paddle_width = static_cast<f32>(paddle_texture.get().width);
        const f32 move_height = static_cast<f32>(window.height) - paddle_height;
        const f32 half_height = paddle_height * 0.5f;

        auto p0 = v2f{10.0f, move_height * player_positions[0]};
        auto p1 = v2f{window.width - 10.0f - paddle_width, move_height * player_positions[1]};

        aabb_t<v2f> b0;
        aabb_t<v2f> b1;
        b0.expand(p0);
        b0.expand(p0 + v2f{paddle_width, paddle_height});

        b1.expand(p1);
        b1.expand(p1 + v2f{paddle_width, paddle_height});

        return b0.contains(ball_position) || b1.contains(ball_position);
    }

    void update(const f32 dt) {
        if (server) {
            json j;
            j["type"] = "sync"_sid;
            j["data"]["paddle"] = std::array<f32, 2>{player_positions.x, player_positions.y};
            j["data"]["ball"]["position"] = std::array<f32, 2>{ball_position.x, ball_position.y};
            j["data"]["ball"]["velocity"] = std::array<f32, 2>{ball_velocity.x, ball_velocity.y};

            packet_t p{j.dump()};
            server->broadcast_packet(p);

            server->pump_event();
        } else if (client) {
            client->poll_events(1);
        }

        if (ball_position.y < 0 || ball_position.y > window.height) {
            ball_velocity.y *= -1;
        }
        if (hit_paddle()) {
            ball_velocity.x *= -1;
        }

        if (ball_position.x < 0) {
            player_score[1] += 1;
            reset();
        } else if (ball_position.x > window.width) {
            player_score[0] += 1;
            reset();
        }

        ball_position += ball_velocity * dt * 100.0f;
    }

    void draw() {
        const f32 paddle_height = static_cast<f32>(paddle_texture.get().height);
        const f32 paddle_width = static_cast<f32>(paddle_texture.get().width);
        const f32 move_height = static_cast<f32>(window.height) - paddle_height;
        const f32 half_height = paddle_height * 0.5f;

        gfx.clear();
        gfx.screen_size = v2f{window.width, window.height};

        gfx.draw(paddle_texture, v2f{10, move_height * player_positions[0]});
        gfx.draw(paddle_texture, v2f{window.width - 10 - paddle_width, move_height * player_positions[1]});
    
        gfx.draw(ball_texture, ball_position - v2f{4.0f});

        auto batch_shader = asset_loader.get_shader_nameless("shaders/batch.vs", "shaders/batch.fs");

        if (batch_shader.valid() == false) {
            logger_t::warn("Failed to load batch shader");
        }

        gfx.present(
            batch_shader
        );
    }

    // to remove static analyzer complaints
    pong_game_t& operator=(const pong_game_t&) = delete;
    pong_game_t& operator=(pong_game_t&&) = delete;
    pong_game_t(const pong_game_t&) = delete;
    pong_game_t(pong_game_t&&) = delete;
};

int main(int argc, char** argv) {
    logger_t::open_file(argc>1?"server.log":"client.log", "./");

    window_t window;
    window.open_window();

    window.set_title((std::string("AAGE Pong - ") + (argc>1 ? "server" : "client")).c_str());
    pong_game_t game{window, argc > 1};

    v3f color{0.0f, 0.0f, 0.0f};
    f32 last_time{};

    while(!window.should_close()) {
        glClearColor(color.r, color.g, color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        /////////////////////////////

        game.update(last_time - window.get_ticks());
        last_time = window.get_ticks();
        
        game.draw();

        /////////////////////////////
        window.poll_events();
        window.swap_buffers();
    }

    return 0;
}