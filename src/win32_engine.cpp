// ---------------------------------------------------------------------------------
// -- File: win32_engine.cpp
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

typedef struct
{
    r32 frame_ms;
    r32 fps;
    u32 count;
} fps_debug_t;

engine_core_t gcsr_engine;
engine_code_t LibCode;
fps_debug_t fps_debug;

s64 GlobalPerfCountFrequency;

#define BACKBUFFER_PADDING_BYTES 4

u64 engine_get_wall_clock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    u64 clock = Result.QuadPart;

    return clock;
}

r32 engine_get_seconds_elapsed(u64 start, u64 end)
{
    r32 res = ((r32) (end - start) / GlobalPerfCountFrequency);
    return res;
}

r32 engine_get_ms_elapsed(u64 start, u64 end)
{
    r32 res = 1000 * ((r32) (end - start) / GlobalPerfCountFrequency);
    return res;
}

__INLINE__ LARGE_INTEGER win32_get_wall_clock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

__INLINE__ r32 win32_get_seconds_elapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((r32) (End.QuadPart - Start.QuadPart) /
                     (r32) GlobalPerfCountFrequency);
    return Result;
}

// ----------------------------------------------------------------------------------
// -- Initialize everything.
// ----------------------------------------------------------------------------------

void win32_initialize_memory()
{
    engine_memory_pool_t *memory = &gcsr_engine.memory;

    // size_t pipeline_pool_size = Megabytes(GL_PIPE_THREAD_BUFFER_SIZE * GC_PIPE_NUM_THREADS);

    // -- Considering the fact that the main memory allocation are from Kilobytes to Gigabytes,
    // -- the final size will be MEMORY_ALIGN_SIZE aligned.

    size_t total_bytes = MEM_PERMANENT_SIZE +
                         MEM_TEMPORARY_SIZE +
                         MEM_PIPELINE_SIZE +
                         MEM_ASSETS_SIZE +
                         MEMORY_ALIGN_SIZE;

    memory->total_bytes = total_bytes;

    void *memory_pointer = win32_mem_alloc(total_bytes);
    memory_pointer = ADDRESS_ALIGN(memory_pointer);

    memory->permanent_pool_size = MEM_PERMANENT_SIZE;
    memory->temporary_pool_size = MEM_TEMPORARY_SIZE;
    memory->pipeline_pool_size = MEM_PIPELINE_SIZE;
    memory->asset_pool_size = MEM_ASSETS_SIZE;

    memory->permanent_pool = memory_pointer;
    memory->temporary_pool = (void *) ((u8 *) memory->permanent_pool + memory->permanent_pool_size);
    memory->pipeline_pool = (void *) ((u8 *) memory->temporary_pool + memory->temporary_pool_size);
    memory->asset_pool = (void *) ((u8 *) memory->pipeline_pool + memory->pipeline_pool_size);
}

// void win32_initialize_main_window(engine_resolution_config_t *resolution)
// {
//     engine_window_t *main_window = &gcsr_engine.main_window;
//     videobuffer_t *buffer = &main_window->buffer;

//     u32 mult = 4;
//     resolution->r_width = MULTIPLE_OF(resolution->r_width, mult);
//     resolution->r_height = MULTIPLE_OF(resolution->r_height, mult);

//     buffer->width = resolution->r_width;
//     buffer->height = resolution->r_height;

//     buffer->size = buffer->width * buffer->height;
//     buffer->bytes_per_pixel = 4;
//     buffer->pixel_depth = 32;
//     buffer->pitch = buffer->width * buffer->bytes_per_pixel;
// }

#ifndef GL_USE_SDL_TEXTURE
void win32_create_backbuffer_surface(engine_framebuffer_config_t *framebuffer)
{
    engine_window_t *main_window = &gcsr_engine.main_window;

    if (main_window->backbuffer_surface)
        SDL_FreeSurface(main_window->backbuffer_surface);

    main_window->backbuffer_surface = SDL_CreateRGBSurfaceWithFormatFrom(
                                        framebuffer->video_memory,
                                        framebuffer->width,
                                        framebuffer->height,
                                        framebuffer->pixel_depth,
                                        framebuffer->pitch,
                                        GL_PIXEL_FORMAT);

    SDL_SetSurfaceBlendMode(main_window->backbuffer_surface, SDL_BLENDMODE_NONE);
}
#else
void win32_create_backbuffer_surface(engine_framebuffer_config_t *framebuffer)
{
    engine_window_t *main_window = &gcsr_engine.main_window;

    if (main_window->window_texture)
        SDL_DestroyTexture(main_window->window_texture);

    main_window->framebuffer = framebuffer;
    main_window->window_texture = SDL_CreateTexture(main_window->sdl_renderer,
                                                    GL_PIXEL_FORMAT,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    framebuffer->width,
                                                    framebuffer->height);
}
#endif

b32 win32_create_window()
{
    b32 result = true;
    engine_window_t *main_window = &gcsr_engine.main_window;

    u32 start_width = 0;
    u32 start_height = 0;
    u32 start_fps = 60;

    main_window->target_fps = start_fps;
    main_window->sleep_time = 1000 / start_fps;

    main_window->sdl_window = SDL_CreateWindow("Software renderer",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            start_width, start_height,
            SDL_WINDOW_SHOWN);

    SDL_Window *Window = main_window->sdl_window;
#ifndef GL_USE_SDL_TEXTURE
    main_window->sdl_window_surface = SDL_GetWindowSurface(Window);
#else
    main_window->sdl_renderer = SDL_CreateRenderer(main_window->sdl_window, -1, SDL_RENDERER_ACCELERATED);
#endif

    if (!Window)
        result = false;

    return result;
}

#ifndef GL_USE_SDL_TEXTURE
__INLINE__ void win32_blit_backbuffer()
{
    OPTICK_EVENT("win32_blit_backbuffer");

    engine_core_t *Engine = &gcsr_engine;
    engine_window_t *main_window = &gcsr_engine.main_window;

    if (!main_window || !main_window->backbuffer_surface)
        return;

    s32 result = SDL_BlitSurface(main_window->backbuffer_surface, 0, main_window->sdl_window_surface, 0);

    if (result)
    {
        Engine->is_running = false;
        SDL_Log("SDL_BlitSurface: Surface blitting failed: %s\n", SDL_GetError());
    }

    result = SDL_UpdateWindowSurface(main_window->sdl_window);

    if (result)
    {
        Engine->is_running = false;
        SDL_Log("SDL_UpdateWindowSurface: Window update failed: %s\n", SDL_GetError());
    }
}
#else
__INLINE__ void win32_blit_backbuffer()
{
    OPTICK_EVENT("win32_blit_backbuffer");

    engine_window_t *main_window = &gcsr_engine.main_window;

    s32 result = SDL_UpdateTexture(main_window->window_texture,
                                   0,
                                   main_window->framebuffer->video_memory,
                                   main_window->framebuffer->pitch);

    SDL_RenderCopy(main_window->sdl_renderer,
                   main_window->window_texture,
                   0, 0);
}
#endif

// TODO(gabic):
void Win32_DestroyWindow()
{}

// ----------------------------------------------------------------------------------

void update_engine(engine_config_t *engine_config)
{
    if (!engine_config)
        return;

    engine_window_t *main_window = &gcsr_engine.main_window;

    main_window->target_fps = engine_config->fps;

    if (main_window->target_fps)
        main_window->sleep_time = 1000.0f / engine_config->fps;
    else
        main_window->sleep_time = 0;
}

void switch_resolution(engine_framebuffer_config_t *framebuffer)
{
    if (!framebuffer)
        return;

    engine_window_t *main_window = &gcsr_engine.main_window;
    SDL_SetWindowSize(main_window->sdl_window, framebuffer->width, framebuffer->height);
    SDL_SetWindowPosition(main_window->sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_Window *Window = main_window->sdl_window;
#ifndef GL_USE_SDL_TEXTURE
    main_window->sdl_window_surface = SDL_GetWindowSurface(Window);
#endif
    win32_create_backbuffer_surface(framebuffer);
}

// ----------------------------------------------------------------------------------

void win32_process_events(engine_core_t *Engine)
{
    SDL_Event Event;
    input_bindings_t *bindings = &Engine->bindings;
    binding_t *Input = 0;

    while (SDL_PollEvent(&Event))
    {
        switch (Event.type)
        {
            case SDL_QUIT:
            {
                Engine->is_running = false;
            }
            break;

            // -- Keyboard events.
            // ----------------------------------------------------------------------------------

            case SDL_KEYDOWN:
            {
                if (!Event.key.repeat)
                {
#if 0
                    SDL_Log("----------------------------------------------------------------");
                    SDL_Log("Keyboard event: timestamp: %d state: %d repeat: %d\n",
                                    Event.key.timestamp, Event.key.state, Event.key.repeat);
                    SDL_Log("Scancode: %d\n", Event.key.keysym.scancode);
                    SDL_Log("Keycode: %d\n", Event.key.keysym.sym);
                    SDL_Log("Key name: %s\n", SDL_GetKeyName(Event.key.keysym.sym));
#endif
                    u32 code = Event.key.keysym.sym;

                    for (u32 i = 0; i < ACTION_COUNT; ++i)
                    {
                        Input = &bindings->actions[i];

                        if (Input->primary_keycode == code)
                        {
                            Input->is_active = true;
                            Input->is_up = false;
                            Input->is_down = true;
                        }
                    }
                }
            }
            break;

            case SDL_KEYUP:
            {
                if (!Event.key.repeat)
                {
#if 0
                    SDL_Log("----------------------------------------------------------------");
                    SDL_Log("Keyboard event: timestamp: %d state: %d repeat: %d\n",
                                    Event.key.timestamp, Event.key.state, Event.key.repeat);
                    SDL_Log("Scancode: %d\n", Event.key.keysym.scancode);
                    SDL_Log("Keycode: %d\n", Event.key.keysym.sym);
                    SDL_Log("Key name: %s\n", SDL_GetKeyName(Event.key.keysym.sym));
#endif

                    u32 code = Event.key.keysym.sym;

                    for (u32 i = 0; i < ACTION_COUNT; ++i)
                    {
                        Input = &bindings->actions[i];

                        if (Input->primary_keycode == code && Input->is_active)
                        {
                            Input->is_active = false;
                            Input->is_up = true;
                            Input->is_down = false;
                        }
                    }
                }
            }
            break;

            // -- Mouse events.
            // ----------------------------------------------------------------------------------

            case SDL_MOUSEMOTION:
            {
                // SDL_Log("Mouse move: x[%d] y[%d]\n", Event.motion.xrel, Event.motion.yrel);

                if (Event.motion.xrel < 0 && !bindings->actions[MOUSE_MOVE_LEFT].is_active)
                {
                    // SDL_Log("Mouse left\n");
                    bindings->actions[MOUSE_MOVE_LEFT].is_active = true;
                    bindings->actions[MOUSE_MOVE_LEFT].mouse_x_rel = Event.motion.xrel;
                }

                if (Event.motion.xrel > 0 && !bindings->actions[MOUSE_MOVE_RIGHT].is_active)
                {
                    // SDL_Log("Mouse right\n");
                    bindings->actions[MOUSE_MOVE_RIGHT].is_active = true;
                    bindings->actions[MOUSE_MOVE_RIGHT].mouse_x_rel = Event.motion.xrel;
                }

                if (Event.motion.yrel > 0 && !bindings->actions[MOUSE_MOVE_DOWN].is_active)
                {
                    // SDL_Log("Mouse down\n");
                    bindings->actions[MOUSE_MOVE_DOWN].is_active = true;
                    bindings->actions[MOUSE_MOVE_DOWN].mouse_y_rel = Event.motion.yrel;
                }

                if (Event.motion.yrel < 0 && !bindings->actions[MOUSE_MOVE_UP].is_active)
                {
                    // SDL_Log("Mouse up\n");
                    bindings->actions[MOUSE_MOVE_UP].is_active = true;
                    bindings->actions[MOUSE_MOVE_UP].mouse_y_rel = Event.motion.yrel;
                }

                // if (Event.motion.yrel > 0)
                // else if (Event.motion.yrel < 0)
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                // SDL_Log("Mouse button pressed\n");

                if (Event.button.button == SDL_BUTTON_LEFT)
                    bindings->actions[MOUSE_LEFT_CLICK].is_active = true;

                if (Event.button.button == SDL_BUTTON_RIGHT)
                    bindings->actions[MOUSE_RIGHT_CLICK].is_active = true;
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                // SDL_Log("Mouse button released\n");

                if (Event.button.button == SDL_BUTTON_LEFT)
                    bindings->actions[MOUSE_LEFT_CLICK].is_active = false;

                if (Event.button.button == SDL_BUTTON_RIGHT)
                    bindings->actions[MOUSE_RIGHT_CLICK].is_active = false;
            }
            break;

            case SDL_MOUSEWHEEL:
            {
                bindings->actions[MOUSE_WHEEL_SCROLL].is_active = true;
                bindings->actions[MOUSE_WHEEL_SCROLL].mouse_y_rel = Event.wheel.y;
            }
            break;

            case SDL_WINDOWEVENT:
            {
                if (Event.window.event == SDL_WINDOWEVENT_RESIZED)
                {}
            }
            break;
        }
    }
}

b32 EqualsLargeNumbers(large_number_t *A, large_number_t *B)
{
    b32 result = false;

    if (A->highPart == B->highPart && A->lowPart == B->lowPart)
        result = true;

    return result;
}

// ----------------------------------------------------------------------------------
// -- Main routine.
// ----------------------------------------------------------------------------------

void win32_run()
{
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
    SDL_Log("Starting the engine ...\n");

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    int initResult = SDL_Init(SDL_INIT_VIDEO);

    if (initResult != 0)
    {
        SDL_Log("SDL_Init failure: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if (!win32_create_window()) {
        SDL_Log("Could not create the window: %s\n", SDL_GetError());
    }
    else
    {
        gcsr_engine.is_initialized = true;

        SDL_version sdlVersion;
        SDL_VERSION(&sdlVersion);
        SDL_Window *Window = gcsr_engine.main_window.sdl_window;

        if (Window)
        {
            gcsr_engine.is_running = true;

            // platform_api_t *API = &gcsr_engine.API;
            engine_file_system_t *file_system = &LibCode.file_system;

            win32_initialize_file_system(file_system);
            win32_load_engine_code(&LibCode);

            SDL_assert(LibCode.is_valid);

            engine_api_t *EngineAPI = LibCode.API;

            // -- Main loop.

            // DEBUG_MedianInit(DebugApi, 0, 60, "Median[%u]: [Ms per frame] %f\n");
            // DEBUG_MedianInit(DebugApi, 1, 60, "Median[%u]: [FPS] %f\n");

            uint64 LastCycleCount = __rdtsc();
            LARGE_INTEGER LastCounter = win32_get_wall_clock();
            LARGE_INTEGER ExecutionStartCounter = win32_get_wall_clock();

            // ----------------------------------------------------------------------------------
            // -- Main loop.
            // ----------------------------------------------------------------------------------

            while (gcsr_engine.is_running)
            {
                OPTICK_FRAME("MainThread");

                // -- Hotloading.

                #if ENGINE_HOTLOAD

                u64 WriteTime = get_last_write_time(file_system->game_code_library_path);

                if (!file_exists(file_system->lock_filepath) && WriteTime != LibCode.last_write_time)
                    // !EqualsLargeNumbers(&LibCode.last_write_time, &WriteTime))
                // if (!EqualsLargeNumbers(&LibCode.last_write_time, &WriteTime))
                {
                    SDL_Log("Reloading the game code");

                    win32_unload_engine_code(&LibCode);
                    win32_load_engine_code(&LibCode);
                    EngineAPI = LibCode.API;
                }

                #endif

                // Reset the mouse actions.

                for (u32 i = MOUSE_MOVE_UP; i <= MOUSE_MOVE_RIGHT; ++i)
                {
                    binding_t *Input = &gcsr_engine.bindings.actions[i];

                    Input->is_active = false;
                    Input->mouse_x = 0;
                    Input->mouse_y = 0;
                    Input->mouse_x_rel = 0;
                    Input->mouse_y_rel = 0;
                }

                win32_process_events(&gcsr_engine);

                LARGE_INTEGER ExecutionCurrentCounter = win32_get_wall_clock();
                r32 step = 1000.0f * win32_get_seconds_elapsed(ExecutionStartCounter, ExecutionCurrentCounter);

                EngineAPI->render_and_update(step);
                win32_blit_backbuffer();

                LARGE_INTEGER EndCounter = win32_get_wall_clock();
                r32 ms_per_frame = 1000.0f * win32_get_seconds_elapsed(LastCounter, EndCounter);
                r32 msdiff = gcsr_engine.main_window.sleep_time - ms_per_frame;

                if (msdiff > 0)
                {
                    ms_per_frame += msdiff;
                    SDL_Delay((u32) msdiff);
                }

                r32 fps = 1000.0f / ms_per_frame;

                fps_debug.frame_ms += ms_per_frame;
                fps_debug.fps += fps;
                fps_debug.count++;

                u64 EndCycleCount = __rdtsc();
                // u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                LastCycleCount = EndCycleCount;

                // Report the fps as a 60 frame median.
                if (fps_debug.count == 60)
                {
                    SDL_Log("ms per frame: %f / fps: %f\n", fps_debug.frame_ms / 60, fps_debug.fps / 60);

                    fps_debug.frame_ms = 0;
                    fps_debug.fps = 0;
                    fps_debug.count = 0;
                }

                // SDL_Log("Ms per frame: %f / fps: %f\n", ms_per_frame, fps);
                // DEBUG_MedianStore(DebugApi, 0, ms_per_frame);
                // DEBUG_MedianStore(DebugApi, 1, fps);
                // DEBUG_RM_Result(DebugApi);

                LastCounter = win32_get_wall_clock();
            }

            win32_unload_engine_code(&LibCode);
        }
        else
        {
            SDL_Log("SDL_Init failure: %s\n", SDL_GetError());
        }

        SDL_DestroyWindow(Window);
        SDL_Quit();
    }
}