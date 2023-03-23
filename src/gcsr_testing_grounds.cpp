// ----------------------------------------------------------------------------------
// -- File: gcsr_testing_grounds.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description: Testing routines.
// -- Created:
// -- Modified:
// ----------------------------------------------------------------------------------

vec2 tmp_uv;

struct Factorial
{
    SDL_mutex *Mutex;
    SDL_cond *Condition;

    b32 hasValue;
    u32 start;

    u32 current;
    u64 computed;
};

Factorial FactorialData;

SDL_Thread *T1;
SDL_Thread *T2;
SDL_Thread *T3;
SDL_Thread *T4;
SDL_Thread *T5;

void DEBUG_doWork(void *data)
{
    Factorial *D = (Factorial *) data;

    if (SDL_TryLockMutex(D->Mutex) == 0)
    {
        while (!D->hasValue)
        {
            printf("Waiting for condition...\n");
            SDL_CondWait(D->Condition, D->Mutex);
        }

        if (D->current > D->start)
        {
            D->hasValue = false;
            printf("Factorial of %d = %llu\n", D->start, D->computed);
        }
        else
        {
            D->computed *= D->current++;

            // printf("Current: %d\n", D->current - 1);
            // printf("Computed: %d ...\n", D->computed);
        }

        SDL_UnlockMutex(D->Mutex);
    }
}

int DEBUG_thread_proc(void *data)
{
    int res = 1;

    while (true)
    {
        DEBUG_doWork(data);
        // SDL_Delay(10);
    }

    return res;
}

void DEBUG_threads()
{
    // -- Thread creation.

    if (!T1 && !T2)
    {
        FactorialData.Mutex = SDL_CreateMutex();
        FactorialData.Condition = SDL_CreateCond();

        T1 = SDL_CreateThread(DEBUG_thread_proc, "Test thread 1", &FactorialData);
        T2 = SDL_CreateThread(DEBUG_thread_proc, "Test thread 2", &FactorialData);

        printf("Created thread: 0x%016llx\n", (u64) T1);
        printf("Created thread: 0x%016llx\n", (u64) T2);
    }

    if (SDL_TryLockMutex(FactorialData.Mutex) == 0)
    {
        printf("Starting the computation...\n");

        FactorialData.start = 20;
        FactorialData.computed = 1;
        FactorialData.current = 1;
        FactorialData.hasValue = true;

        SDL_CondSignal(FactorialData.Condition);
        SDL_UnlockMutex(FactorialData.Mutex);
    }

    b32 ended = false;

    while (!ended)
    {
        // DEBUG_doWork(&FactorialData);

        if (SDL_TryLockMutex(FactorialData.Mutex) == 0)
        {
            ended = !FactorialData.hasValue;
            SDL_UnlockMutex(FactorialData.Mutex);
        }
    }

    // if (SDL_TryLockMutex(TD.Mutex) == 0)
    // {
    //     TD.count++;
    //     TD.hasData = true;

    //     SDL_UnlockMutex(TD.Mutex);
    // }
}

#define PIXEL_BLOCK_SIZE 256
#define PIXEL_BLOCK_SIZE_2 PIXEL_BLOCK_SIZE * PIXEL_BLOCK_SIZE
#define PIXEL_BLOCK_ROWS 768 / PIXEL_BLOCK_SIZE
#define PIXEL_BLOCK_COLS 1280 / PIXEL_BLOCK_SIZE
#define PIXEL_BLOCK_COUNT PIXEL_BLOCK_ROWS * PIXEL_BLOCK_COLS

struct PixelAreaWork
{
    b32 isProcessed;
    vec2i topLeft;
    vec2i bottomRight;
};

struct TextureFillWork
{
    SDL_mutex *Lock;
    SDL_cond *Condition;

    videobuffer_t *buffer;
    AssetBitmapMemory *Texture;

    b32 isDone;
    u32 current;
    u32 total;

    PixelAreaWork blocks[PIXEL_BLOCK_COUNT];
};

TextureFillWork FillWork;

void DEBUG_fillBlock(void *data, u32 current)
{
    GlPixelFormat *Shifts = getPixelFormat();
    GlBlockSampler Sampler;
    gl_BlockSamplerInit(&Sampler, TEX_TILE_SIZE);

    TextureFillWork *Work = (TextureFillWork *) data;
    u32 *TextureMemory = (u32 *) Work->Texture->memory;

    u32 texW = Work->Texture->BitmapInfo.width;
    u32 texH = Work->Texture->BitmapInfo.height;

    r32 oneOverWidth = 1.0f / (Work->buffer->width - 1);
    r32 oneOverHeight = 1.0f / (Work->buffer->height - 1);

    // DEBUG_start_counter();
    PixelAreaWork CurrentBlock = Work->blocks[current];

    u32 output[PIXEL_BLOCK_SIZE_2];
    u32 outputIndex = 0;

    vec2 uv;

    for (s32 row = CurrentBlock.topLeft.y; row <= CurrentBlock.bottomRight.y; ++row)
    {
        uv.v = row * oneOverHeight;

        for (s32 col = CurrentBlock.topLeft.x; col <= CurrentBlock.bottomRight.x; ++col)
        {
            uv.u = col * oneOverWidth;

#if USE_CACHE_TEXTURE_OPTIMIZATION == 1
            u32 packedSample = gl_blockSample(&Sampler, TextureMemory, uv, texW, texH);
#else
            u32 packedSample = gl_sample(Work->Texture, uv, GL_FILTER_NEAREST);
#endif
            output[outputIndex++] = packedSample;
        }
    }

    u32 *PixelStart = (u32 *) Work->buffer->memory +
                              CurrentBlock.topLeft.y * Work->buffer->width +
                              CurrentBlock.topLeft.x;

    u32 *Pixel = PixelStart;
    // printf("%u %u %u\n", current, CurrentBlock.topLeft.x, CurrentBlock.topLeft.y);
    // printf("%llx\n", (u64) PixelStart);

    if (SDL_LockMutex(Work->Lock) == 0)
    {
        for (u32 i = 0; i < PIXEL_BLOCK_SIZE_2; ++i)
        {
            if (i != 0 && i % PIXEL_BLOCK_SIZE == 0)
            {
                PixelStart += Work->buffer->width;
                Pixel = PixelStart;
            }

            *Pixel++ = output[i];
        }

        SDL_UnlockMutex(Work->Lock);
    }
}

void DEBUG_fillBlock_4x(void *data, u32 current)
{
    GlPixelFormat *Shifts = getPixelFormat();
    GlBlockSampler Sampler;
    gl_BlockSamplerInit(&Sampler, TEX_TILE_SIZE);

    TextureFillWork *Work = (TextureFillWork *) data;
    u32 *TextureMemory = (u32 *) Work->Texture->memory;

    u32 texW = Work->Texture->BitmapInfo.width;
    u32 texH = Work->Texture->BitmapInfo.height;

    r32 oneOverWidth = 1.0f / (Work->buffer->width - 1);
    r32 oneOverHeight = 1.0f / (Work->buffer->height - 1);

    // DEBUG_start_counter();
    PixelAreaWork CurrentBlock = Work->blocks[current];

    u32 output[PIXEL_BLOCK_SIZE_2];
    u32 outputIndex = 0;

    __m128 u_4x;
    __m128 v_4x;

    for (s32 row = CurrentBlock.topLeft.y; row <= CurrentBlock.bottomRight.y; ++row)
    {
        v_4x = _mm_set1_ps(row * oneOverHeight);

        for (s32 col = CurrentBlock.topLeft.x; col <= CurrentBlock.bottomRight.x; col += 4)
        {
            r32 tmp_u1 = col * oneOverWidth;
            r32 tmp_u2 = tmp_u1 + oneOverWidth;
            r32 tmp_u3 = tmp_u2 + oneOverWidth;
            r32 tmp_u4 = tmp_u3 + oneOverWidth;

            u_4x = _mm_setr_ps(tmp_u1, tmp_u2, tmp_u3, tmp_u4);

#if USE_CACHE_TEXTURE_OPTIMIZATION == 1
            __m128i samples_4x = gl_blockSample_4x(&Sampler, TextureMemory, u_4x, v_4x, texW, texH);
#else
            __m128i samples_4x = gl_sample_4x(Work->Texture, u_4x, v_4x, GL_FILTER_NEAREST);
#endif

            _mm_storeu_si128((__m128i *) (output + outputIndex), samples_4x);
            outputIndex += 4;
        }
    }

    u32 *PixelStart = (u32 *) Work->buffer->memory +
                              CurrentBlock.topLeft.y * Work->buffer->width +
                              CurrentBlock.topLeft.x;

    u32 *Pixel = PixelStart;

    if (SDL_LockMutex(Work->Lock) == 0)
    {
        for (u32 i = 0; i < PIXEL_BLOCK_SIZE_2; i += 4)
        {
            if (i != 0 && i % PIXEL_BLOCK_SIZE == 0)
            {
                PixelStart += Work->buffer->width;
                Pixel = PixelStart;
            }

            __m128i _tmp_ = _mm_loadu_si128((__m128i *) &output[i]);
            _mm_storeu_si128((__m128i *) Pixel, _tmp_);
            Pixel += 4;

            // *Pixel++ = output[i];
        }

        SDL_UnlockMutex(Work->Lock);
    }
}

int DEBUG_textureFillrate_thread(void *data)
{
    int res = 1;

    TextureFillWork *Work = (TextureFillWork *) data;

    // r32 oneOverBlockSize = 1.0f / PIXEL_BLOCK_SIZE;

    u32 current = 0;
    b32 isProcessed = true;
    u32 *TextureMemory = (u32 *) Work->Texture->memory;
    SDL_threadID thread_id = SDL_ThreadID();

    while (true)
    {
        if (SDL_LockMutex(Work->Lock) == 0)
        {
            while(Work->isDone)
            {
                // printf("Thread waiting: %u...\n", thread_id);
                SDL_CondWait(Work->Condition, Work->Lock);
            }

            // printf("Thread lock: %u\n", thread_id);

            if (Work->current == Work->total)
            {
                // printf("Work done...\n");
                Work->isDone = true;
            }
            else
            {
                current = Work->current++;
                SDL_assert(Work->current <= Work->total);
                isProcessed = Work->blocks[current].isProcessed;
            }

            SDL_UnlockMutex(Work->Lock);
        }

        // printf("Thread:%u -> isProcessed: %u %u\n", thread_id, current, isProcessed);

        if (isProcessed)
            DEBUG_fillBlock_4x(data, current);
            // DEBUG_fillBlock(data, current);
    }

    return res;
}

void DEBUG_pixelfill_thread_start(videobuffer_t *buffer, AssetBitmapMemory *Texture)
{
    // -- Thread creation.

    if (!T1 && !T2)
    {
        FillWork.Lock = SDL_CreateMutex();
        FillWork.Condition = SDL_CreateCond();

        FillWork.buffer = buffer;
        FillWork.Texture = Texture;

        FillWork.isDone = false;
        FillWork.current = 0;
        FillWork.total = PIXEL_BLOCK_COUNT;

        for (u32 row = 0; row < PIXEL_BLOCK_ROWS; ++row)
        {
            for (u32 col = 0; col < PIXEL_BLOCK_COLS; ++col)
            {
                u32 index = row * PIXEL_BLOCK_COLS + col;
                PixelAreaWork *Area = &FillWork.blocks[index];

#if 1
                Area->isProcessed = true;
#else
                Area->isProcessed = false;

                if (row == 0 && col == 3)
                    Area->isProcessed = true;
#endif

                Area->topLeft.x = col * PIXEL_BLOCK_SIZE;
                Area->topLeft.y = row * PIXEL_BLOCK_SIZE;

                // printf("Area %u %u: \n", Area->topLeft.x, Area->topLeft.y);

                Area->bottomRight.x = (col + 1) * PIXEL_BLOCK_SIZE - 1;
                Area->bottomRight.y = (row + 1) * PIXEL_BLOCK_SIZE - 1;
            }
        }

#if 1
        T1 = SDL_CreateThread(DEBUG_textureFillrate_thread, "Test thread 1", &FillWork);
        T2 = SDL_CreateThread(DEBUG_textureFillrate_thread, "Test thread 2", &FillWork);
        T3 = SDL_CreateThread(DEBUG_textureFillrate_thread, "Test thread 3", &FillWork);
        // T4 = SDL_CreateThread(DEBUG_textureFillrate_thread, "Test thread 4", &FillWork);
        // T5 = SDL_CreateThread(DEBUG_textureFillrate_thread, "Test thread 5", &FillWork);

        // printf("Created thread: 0x%016llx\n", (u64) T1);
        // printf("Created thread: 0x%016llx\n", (u64) T2);
        // printf("Created thread: 0x%016llx\n", (u64) T3);
#endif
    }
    else
    {
        // -- Restart the work queue.
#if 1
        if (SDL_LockMutex(FillWork.Lock) == 0)
        {
            FillWork.isDone = false;
            FillWork.current = 0;

            // SDL_CondSignal(FillWork.Condition);
            SDL_CondBroadcast(FillWork.Condition);
            SDL_UnlockMutex(FillWork.Lock);
        }
#endif
    }

    b32 isDone = false;

    // -- Wait until all work is done.

    while (!isDone)
    {
        u32 current = 0;
        b32 isProcessed = false;

        // if (SDL_TryLockMutex(FillWork.Lock) == 0)
        if (SDL_LockMutex(FillWork.Lock) == 0)
        {
            if (FillWork.current == FillWork.total)
            {
                // printf("Work done...\n");
                FillWork.isDone = true;
                isDone = FillWork.isDone;
            }
            else
            {
                current = FillWork.current++;
                isProcessed = FillWork.blocks[current].isProcessed;
            }

            SDL_UnlockMutex(FillWork.Lock);
        }

        // printf("Main->isProcessed: %u %u\n", current, isProcessed);

        if (isProcessed)
            DEBUG_fillBlock_4x(&FillWork, current);
            // DEBUG_fillBlock(&FillWork, current);
    }
}
