#ifndef IMGUI_SWITCH_H
#define IMGUI_SWITCH_H

#include <switch.h>
#include <SDL2/SDL.h>

void ImGuiSetSwitchTheme(bool dark);
void ImGuiUpdateSwitchInput(PadState* pad, bool enableNav = true);
void ImGuiProcessSDLEvents(void);

#endif
