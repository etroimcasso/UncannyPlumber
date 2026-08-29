// Hello-engine probe: the port links the Retro++ engine, reports both versions, and exits 0.
// The boot host that opens a window replaces this once there is something to draw.

#include <spdlog/spdlog.h>

#include <retropp/version.h>

#include "version.h"

int main() {
    spdlog::info("UncannyPlumber {} — Retro++ {}", uncannyplumber::version(), retropp::version());
    return 0;
}
