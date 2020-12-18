/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "sdl_input.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>

#include "util.h"

char dbpath[PATH_MAX] = {'\0'};

int initialized = 0;
SDL_GameController *con = NULL;
int joy_inst = -1;

void try_init(void)
{
    if (initialized) return;
    dlog("Initializing");

    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_GAMECONTROLLER))
    {
        /* deal with the unnessessary initial controller connected
           events so they don't clog up the log file */
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        int mapcount = SDL_GameControllerAddMappingsFromFile(dbpath);
        if (mapcount == -1)
            dlog("    Unable to load mappings from %s", dbpath);
        else
            dlog("    Successfully loaded %d mappings from %s", mapcount, dbpath);

        initialized = 1;
        dlog("    ...done");
    }
    else
        dlog("    SDL has failed to initialize");
}

void deinit(void)
{
    if (!initialized) return;
    dlog("Deinitializing");

    close_controller();
    SDL_Quit();
    initialized = 0;
}

void open_controller(void)
{
    if (!initialized) try_init();
    if (!initialized || con != NULL) return;

    dlog("Attempting to open a controller");
    dlog("    # of joysticks: %d", SDL_NumJoysticks());

    // open the first available controller
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i) && (con = SDL_GameControllerOpen(i)) != NULL)
        {
            dlog("    Found a viable controller: %s (joystick %d)", SDL_GameControllerName(con), i);

            SDL_Joystick *joy = SDL_GameControllerGetJoystick(con);
    
            joy_inst = SDL_JoystickInstanceID(joy);
            dlog("        Joystick instance ID: %d", joy_inst);

            SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
            char guidstr[33];
            SDL_JoystickGetGUIDString(guid, guidstr, sizeof(guidstr));
            dlog("        Joystick GUID: %s", guidstr);

            char *mapping = SDL_GameControllerMapping(con);
            if (mapping != NULL)
                dlog("        Controller mapping: %s", mapping);
            else
            {
                dlog("        This controller has no mapping! Closing it");
                // skip this controller
                close_controller();
                continue;
            }

            break;
        }
        else
            dlog("    Couldn't use joystick %d", i);
    }

    if (con == NULL)
        dlog("    Couldn't find a viable controller :(");
}

void close_controller(void)
{
    if (!initialized && con != NULL) con = NULL;
    if (!initialized || con == NULL) return;

    dlog("Closing current controller");
    SDL_GameControllerClose(con);
    con = NULL;
    joy_inst = -1;
}

void get_inputs(inputs_t *i)
{
    if (!initialized)
    {
        try_init();
        if (!initialized) return;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e))
        switch (e.type)
        {
        case SDL_CONTROLLERDEVICEADDED:
            dlog("A device has been added");
            if (con == NULL)
            {
                dlog("    ...and there is no active controller");
                open_controller();
            }
            else
                dlog("    ...but there is already an active controller");
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            dlog("A device has been removed");
            if (e.cdevice.which == joy_inst)
            {
                dlog("    ...it was the active controller");
                close_controller();
                open_controller();
            }
            else
                dlog("    ...it was not the active controller");
            break;
        }

    if (con != NULL)
        write_inputs(i);
}

static inline uint8_t get_but(SDL_GameControllerButton b)
{
    return SDL_GameControllerGetButton(con, b);
}

static inline int16_t get_axis(SDL_GameControllerAxis a)
{
    return sclamp(SDL_GameControllerGetAxis(con, a), -32767, 32767);
}

void write_inputs(inputs_t *i)
{
    i->a      = get_but(SDL_CONTROLLER_BUTTON_A);
    i->b      = get_but(SDL_CONTROLLER_BUTTON_B);
    i->x      = get_but(SDL_CONTROLLER_BUTTON_X);
    i->y      = get_but(SDL_CONTROLLER_BUTTON_Y);
    i->back   = get_but(SDL_CONTROLLER_BUTTON_BACK);
    i->guide  = get_but(SDL_CONTROLLER_BUTTON_GUIDE);
    i->start  = get_but(SDL_CONTROLLER_BUTTON_START);
    i->lstick = get_but(SDL_CONTROLLER_BUTTON_LEFTSTICK);
    i->rstick = get_but(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    i->lshoul = get_but(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    i->rshoul = get_but(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    i->dup    = get_but(SDL_CONTROLLER_BUTTON_DPAD_UP);
    i->ddown  = get_but(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    i->dleft  = get_but(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    i->dright = get_but(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    i->alx    = get_axis(SDL_CONTROLLER_AXIS_LEFTX);
    i->aly    = get_axis(SDL_CONTROLLER_AXIS_LEFTY);
    i->arx    = get_axis(SDL_CONTROLLER_AXIS_RIGHTX);
    i->ary    = get_axis(SDL_CONTROLLER_AXIS_RIGHTY);
    i->altrig = get_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    i->artrig = get_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
}
