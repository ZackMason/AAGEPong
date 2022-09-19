#include <fmt/core.h>
#include <fmt/color.h>

#include <cassert>
#include <exception>

static size_t tests_run = 0;
static size_t tests_passed = 0;

struct test_failed : std::exception {
    std::string message;
    test_failed(std::string&& text){ 
        message = std::move(text);
    }

    const char* what() const noexcept override {
        return message.c_str();
    }
};

constexpr auto throw_assert(const bool b, std::string&& text) -> auto {
    if (!b) throw test_failed(std::move(text));
}

constexpr auto throw_assert(const bool b) -> auto {
    throw_assert(b, "Assert Failed");
}

#define TEST_ASSERT( x ) throw_assert(x, "TEST FAILED: " #x);

template <typename Fn>
auto run_test(const char* name, const Fn& test) -> auto {
    ++tests_run;
    try {
        test();
        ++tests_passed;
        fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold,
            "{} - Passed\n", name);
    } catch (std::exception & e) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
            "{} - Failed\n\n", name);
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold,
            "{}\n\n", e.what());
    }
}

int main(int argc, char** argv) {
    run_test("equal", [](){
        TEST_ASSERT(1 == 1);
    });

    const auto fmt_color = tests_passed == 0 ? fg(fmt::color::crimson) : 
                            tests_passed == tests_run ? fg(fmt::color::green) : 
                                                        fg(fmt::color::yellow);
    fmt::print(fmt_color | fmt::emphasis::bold,
        "===================================================\n");
    fmt::print(fmt_color | fmt::emphasis::bold,
        "{} / {} Tests Passed\n", tests_passed, tests_run);
    return 0;
}
