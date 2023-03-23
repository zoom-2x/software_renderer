// ---------------------------------------------------------------------------------
// -- File: gcsr_rasterizer.cpp
// ---------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description: Various bliting routines.
// -- Created: 2021-03-27 11:45:12
// -- Modified: 2021-03-27 11:45:13
// ---------------------------------------------------------------------------------

__INLINE__ u32 packColor(vec4 unpacked)
{
    u32 result = 0;

    result = ((u32) (unpacked.r * 255) << GL_PIXEL_FORMAT_RED_SHIFT) |
             ((u32) (unpacked.g * 255) << GL_PIXEL_FORMAT_GREEN_SHIFT) |
             ((u32) (unpacked.b * 255) << GL_PIXEL_FORMAT_BLUE_SHIFT) |
             ((u32) (unpacked.a * 255) << GL_PIXEL_FORMAT_ALPHA_SHIFT);

    return result;
}

// __INLINE__ vec4 unpackColor(u32 packed, GlPixelFormat *shifts)
__INLINE__ vec4 unpackColor(u32 packed)
{
    vec4 result;

    result.r = (r32) ((packed >> GL_PIXEL_FORMAT_RED_SHIFT) & 0xFF);
    result.g = (r32) ((packed >> GL_PIXEL_FORMAT_GREEN_SHIFT) & 0xFF);
    result.b = (r32) ((packed >> GL_PIXEL_FORMAT_BLUE_SHIFT) & 0xFF);
    result.a = (r32) ((packed >> GL_PIXEL_FORMAT_ALPHA_SHIFT) & 0xFF);

    return result;
}

__INLINE__ vec4 sampleBitmap(AssetBitmapMemory *Bitmap, vec2 UVs)
{
    vec4 result = {0, 0, 0, 1};

    u32 col = roundR32ToU32(UVs.u * (Bitmap->BitmapInfo.width - 1));
    u32 row = roundR32ToU32(UVs.v * (Bitmap->BitmapInfo.height - 1));
    u32 *pixel = (u32 *) Bitmap->memory + row * Bitmap->BitmapInfo.width + col;

    result = unpackColor(*pixel);

    return result;
}

#if 0
bilinear_sample_t getBilinearSample(AssetBitmapMemory *Bitmap, u32 row, u32 col)
{
    bilinear_sample_t result;

    u32 *pixel = (u32 *) Bitmap->memory + row * Bitmap->BitmapInfo.width + col;
    u32 colPlusOne = col + 1;
    u32 rowPlusOne = row + 1;

    result.A = *pixel;

    if (rowPlusOne < Bitmap->BitmapInfo.height)
    {
        result.C = *(pixel + Bitmap->BitmapInfo.width);
        result.D = result.C;
    }
    else
    {
        result.C = *pixel;
        result.D = result.C;
    }

    if (colPlusOne < Bitmap->BitmapInfo.width)
        result.B = *(pixel + 1);
    else
        result.B = *pixel;

    if (colPlusOne < Bitmap->BitmapInfo.width && rowPlusOne < Bitmap->BitmapInfo.height)
        result.D = *(pixel + Bitmap->BitmapInfo.width + 1);

    return result;
}
#else
bilinear_sample_t getBilinearSample(AssetBitmapMemory *Bitmap, r32 row, r32 col)
{
    bilinear_sample_t Result;

    u32 rowMin = floorR32ToU32(row);
    u32 rowMax = roundR32ToU32(row + 0.5f);

    u32 colMin = floorR32ToU32(col);
    u32 colMax = roundR32ToU32(col + 0.5f);

    Result.A = *((u32 *) Bitmap->memory + rowMin * Bitmap->BitmapInfo.width + colMin);
    Result.B = *((u32 *) Bitmap->memory + rowMin * Bitmap->BitmapInfo.width + colMax);
    Result.C = *((u32 *) Bitmap->memory + rowMax * Bitmap->BitmapInfo.width + colMin);
    Result.D = *((u32 *) Bitmap->memory + rowMax * Bitmap->BitmapInfo.width + colMax);

    return Result;
}
#endif

vec4 bilinearBlend(bilinear_sample_t Samples, r32 tRow, r32 tCol)
{
    vec4 result;
    vec4 SampleA = unpackColor(Samples.A);
    vec4 SampleB = unpackColor(Samples.B);
    vec4 SampleC = unpackColor(Samples.C);
    vec4 SampleD = unpackColor(Samples.D);

    SampleA = sRGB_linear1(SampleA);
    SampleB = sRGB_linear1(SampleB);
    SampleC = sRGB_linear1(SampleC);
    SampleD = sRGB_linear1(SampleD);

    r32 tmpR1 = SampleA.r + (SampleB.r - SampleA.r) * tCol;
    r32 tmpG1 = SampleA.g + (SampleB.g - SampleA.g) * tCol;
    r32 tmpB1 = SampleA.b + (SampleB.b - SampleA.b) * tCol;
    r32 tmpA1 = SampleA.a + (SampleB.a - SampleA.a) * tCol;

    r32 tmpR2 = SampleC.r + (SampleD.r - SampleC.r) * tCol;
    r32 tmpG2 = SampleC.g + (SampleD.g - SampleC.g) * tCol;
    r32 tmpB2 = SampleC.b + (SampleD.b - SampleC.b) * tCol;
    r32 tmpA2 = SampleC.a + (SampleD.a - SampleC.a) * tCol;

    result.r = tmpR1 + (tmpR2 - tmpR1) * tRow;
    result.g = tmpG1 + (tmpG2 - tmpG1) * tRow;
    result.b = tmpB1 + (tmpB2 - tmpB1) * tRow;
    result.a = tmpA1 + (tmpA2 - tmpA1) * tRow;

    return result;
}

#if 0
__INLINE__ Color gl_sample_old(AssetBitmapMemory *Texture, vec2 uv, gc_constant_t filtering)
{
    Color Sample;

    uv.u = clamp(0, 1, uv.u);
    uv.v = clamp(0, 1, uv.v);

    r32 sampleColR32 = uv.u * (Texture->BitmapInfo.width - 1);
    r32 sampleRowR32 = (1 - uv.v) * (Texture->BitmapInfo.height - 1);

    if (filtering == GL_FILTER_BILINEAR)
    {
        r32 tCol = sampleColR32 - (r32) ((u32) sampleColR32);
        r32 tRow = sampleRowR32 - (r32) ((u32) sampleRowR32);

        bilinear_sample_t Samples = getBilinearSample(Texture, sampleRowR32, sampleColR32);
        Sample = bilinearBlend(Samples, tRow, tCol, shifts);
    }
    else
    {
        u32 sampleCol = (u32) (sampleColR32 + 0.5f);
        u32 sampleRow = (u32) (sampleRowR32 + 0.5f);

        u32 *pixel = (u32 *) Texture->memory + sampleRow * Texture->BitmapInfo.width + sampleCol;
        u32 packedSample = *pixel;

        Sample.r = (r32) ((packedSample >> GL_PIXEL_FORMAT_RED_SHIFT) & 0xFF);
        Sample.g = (r32) ((packedSample >> GL_PIXEL_FORMAT_GREEN_SHIFT) & 0xFF);
        Sample.b = (r32) ((packedSample >> GL_PIXEL_FORMAT_BLUE_SHIFT) & 0xFF);
        Sample.a = (r32) ((packedSample >> GL_PIXEL_FORMAT_ALPHA_SHIFT) & 0xFF);

        Sample = sRGB_linear1(Sample);
    }

    return Sample;
}
#endif

// ----------------------------------------------------------------------------------
// -- Drawing routines.
// ----------------------------------------------------------------------------------

void gl_drawLine2D_SSE(videobuffer_t *buffer, vec2 p1, vec2 p2, vec4 LineColor)
{}

// ----------------------------------------------------------------------------------
// -- Bresenham's line algorithm implementation.
// -- Color should be in premultiplied linear form. See colorToPremultipliedLinear gcsr_math.h.
// ----------------------------------------------------------------------------------

void gl_drawLine2D_noSSE(videobuffer_t *buffer, vec2 p1, vec2 p2, vec4 Color)
{
    b32 flipped = false;
    b32 reversed = false;
    s32 width = buffer->width;
    s32 height = buffer->height;
    r32 ratio = 0;
    u32 t = 0;

    r32 oneMinusAlpha = 1 - Color.a;

    s32 x0 = roundR32ToS32(p1.x);
    s32 y0 = roundR32ToS32(p1.y);
    s32 x1 = roundR32ToS32(p2.x);
    s32 y1 = roundR32ToS32(p2.y);

    if ((x0 < 0 && x1 < 0) || (x0 > width && x1 > width) ||
        (y0 < 0 && y1 < 0) || (y0 > height && y1 > height))
        return;

    // -- Swap x with y, basically draw the line using the y coordinate as the increment.
    if (absDiff(x1, x0) < absDiff(y1, y0))
    {
        t = y0;
        y0 = x0;
        x0 = t;

        t = y1;
        y1 = x1;
        x1 = t;

        flipped = true;
    }

    // -- Left to right drawing.
    if (x0 > x1)
    {
        t = x0;
        x0 = x1;
        x1 = t;

        t = y0;
        y0 = y1;
        y1 = t;

        reversed = true;
    }

    s32 dx = x1 - x0;
    s32 dy = y1 - y0;
    s32 derror = abs(dy) * 2;
    s32 error = 0;
    s32 y = y0;
    r32 oneOverDx = 1.0f / dx;

    u8 *Dest = (u8 *) buffer->memory + y0 * buffer->pitch + x0 * buffer->bytes_per_pixel;
    // u32 *DestPixel = (u32 *) Dest;

    for (s32 cx = x0; cx <= x1; ++cx)
    {
        s32 bx = cx;
        s32 by = y;

        if (flipped)
        {
            bx = y;
            by = cx;
        }

        if (bx >= 0 && by >= 0 && bx < width && by < height)
        {
            u32 index = (u32) (width * by + bx);
            u32 *DestPixel = (u32 *) ((u8 *) buffer->memory + by * buffer->pitch + bx * buffer->bytes_per_pixel);

            vec4 DestColor = unpackColor(*DestPixel);
            DestColor = sRGB_linear1(DestColor);

            DestColor.r = Color.r + DestColor.r * oneMinusAlpha;
            DestColor.g = Color.g + DestColor.g * oneMinusAlpha;
            DestColor.b = Color.b + DestColor.b * oneMinusAlpha;
            DestColor.a = 1.0f;

            DestColor = linear1_sRGB255(DestColor);

            *DestPixel = ((u32) (DestColor.r) << GL_PIXEL_FORMAT_RED_SHIFT) |
                         ((u32) (DestColor.g) << GL_PIXEL_FORMAT_GREEN_SHIFT) |
                         ((u32) (DestColor.b) << GL_PIXEL_FORMAT_BLUE_SHIFT) |
                         ((u32) (DestColor.a) << GL_PIXEL_FORMAT_ALPHA_SHIFT);
        }

        error += derror;

        if (error > dx)
        {
            y += y1 > y0 ? 1 : -1;
            error -= dx * 2;
        }
    }
}

/**
 * It's assumed that Top and Bottom are actually in order.
 */
void drawRectangle2D(videobuffer_t *buffer, vec2 Top, vec2 Bottom, vec4 Color)
{
    r32 width = absR32(Top.x - Bottom.x);
    r32 height = absR32(Top.y - Bottom.y);

    vec2 P1 = Top;
    vec2 P2 = vec2_add(P1, {width, 0});
    vec2 P3 = Bottom;
    vec2 P4 = vec2_add(P1, {0, height});

    gl_drawLine2D(buffer, P1, P2, Color);
    gl_drawLine2D(buffer, P2, P3, Color);
    gl_drawLine2D(buffer, P3, P4, Color);
    gl_drawLine2D(buffer, P4, P1, Color);
}

// void drawPolygon2D(videobuffer_t *buffer, Type_Array *Points, vec4 Color)
void drawPolygon2D(videobuffer_t *buffer, GlRenderObject2D *Object)
{
    Type_Array *Points = Object->WorldPoints;
    vec4 Color = Object->color;

    if (Points->length > 1)
    {
        vec2 *Prev = 0;
        vec2 *Current = 0;

        for (u32 i = 1; i < Points->length; ++i)
        {
            Prev = (vec2 *) ta_index(Points, i - 1);
            Current = (vec2 *) ta_index(Points, i);

            gl_drawLine2D(buffer, *Prev, *Current, Color);

            if (i == Points->length - 1)
            {
                Prev = Current;
                Current = (vec2 *) ta_index(Points, 0);

                gl_drawLine2D(buffer, *Prev, *Current, Color);
            }
        }
    }
}

/**
 * SSE bitmap drawing routine.
 */
void gl_drawBitmapBasis_SSE(videobuffer_t *buffer, GlRenderObject2D *Object)
{
    if (Object->WorldPoints->length != 4)
        return;

    AssetBitmapMemory *Bitmap = Object->Bitmap;

    vec2 P1 = *((vec2 *) ta_index(Object->WorldPoints, 0));
    vec2 P2 = *((vec2 *) ta_index(Object->WorldPoints, 1));
    vec2 P3 = *((vec2 *) ta_index(Object->WorldPoints, 2));
    vec2 P4 = *((vec2 *) ta_index(Object->WorldPoints, 3));

    vec2 AxisX = vec2_sub(P2, P1);
    vec2 AxisY = vec2_sub(P4, P1);
    r32 AxisWidth = vec2_len(AxisX);
    r32 AxisHeight = vec2_len(AxisY);

    vec2 Position = Object->Transformation.translation;

    r32 axisMat[3][3];
    r32 invAxisMat[3][3];
    mat3_create_transform(axisMat, AxisX, AxisY, P1);
    mat3_inv(axisMat, invAxisMat);

    r32 oneOverAxisWidth = 1.0f / AxisWidth;
    r32 oneOverAxisHeight = 1.0f / AxisHeight;

    // This is to correct the imprecisions of the floating points representation
    // in the UV computation.
    r32 uDelta = oneOverAxisWidth / 10;
    r32 vDelta = oneOverAxisHeight / 10;

    r32 realMinX = (r32) buffer->width;
    r32 realMinY = (r32) buffer->height;
    r32 realMaxX = 0;
    r32 realMaxY = 0;

    vec2 Points[4] =  { P1, P2, P3, P4 };

    for (u32 i = 0; i < 4; ++i)
    {
        r32 cX = Points[i].x;
        r32 cY = Points[i].y;

        if (cX < realMinX)
            realMinX = cX;

        if (cX > realMaxX)
            realMaxX = cX + 1;

        if (cY < realMinY)
            realMinY = cY;

        if (cY > realMaxY)
            realMaxY = cY + 1;
    }

    s32 minX = floorR32ToS32(realMinX);
    s32 minY = floorR32ToS32(realMinY);
    s32 maxX = floorR32ToS32(realMaxX);
    s32 maxY = floorR32ToS32(realMaxY);

    if (minX < 0)
        minX = 0;

    if (minY < 0)
        minY = 0;

    if (maxX > (s32) buffer->width)
        maxX = (s32) buffer->width;

    if (maxY > (s32) buffer->height)
        maxY = (s32) buffer->height;

    // if (mustLock)
    //     SDL_LockSurface(backbuffer_surface);

    __m128 PositionX_4x = _mm_set1_ps(Position.x);
    __m128 PositionY_4x = _mm_set1_ps(Position.y);
    __m128 Four_4x = _mm_set1_ps(4.0f);
    __m128 MinX_4x = _mm_set1_ps((r32) minX);
    __m128 MinY_4x = _mm_set1_ps((r32) minY);

    __m128i BitmapHeight_u32_4x = _mm_set1_epi32(Bitmap->BitmapInfo.height);
    __m128i BitmapHeightMinusOne_u32_4x = _mm_set1_epi32(Bitmap->BitmapInfo.height - 1);
    __m128i BitmapWidth_u32_4x = _mm_set1_epi32(Bitmap->BitmapInfo.width);
    __m128 BitmapWidth_r32_4x = _mm_set1_ps((r32) (Bitmap->BitmapInfo.width - 1));
    __m128 BitmapHeight_r32_4x = _mm_set1_ps((r32) (Bitmap->BitmapInfo.height - 1));

    __m128i BitmapPitch_4x = _mm_set1_epi32(Bitmap->BitmapInfo.width);
    __m128 Half_4x = _mm_set1_ps((r32) 0.5f);
    __m128 One255_4x = _mm_set1_ps(255.0f);
    __m128 OneOver255_4x = _mm_set1_ps(ONE_OVER_255);

    __m128i Mask0xFF_4x = _mm_set1_epi32(0xFF);
    __m128i ZeroInt_4x = _mm_set1_epi32(0);

    __m128i RedShift_4x = _mm_set1_epi32(GL_PIXEL_FORMAT_RED_SHIFT);
    __m128i GreenShift_4x = _mm_set1_epi32(GL_PIXEL_FORMAT_GREEN_SHIFT);
    __m128i BlueShift_4x = _mm_set1_epi32(GL_PIXEL_FORMAT_BLUE_SHIFT);
    __m128i AlphaShift_4x = _mm_set1_epi32(GL_PIXEL_FORMAT_ALPHA_SHIFT);

    __m128i DebugRedBkg_4x = _mm_set1_epi32(0xFF);
    __m128i DebugGreenBkg_4x = _mm_set1_epi32(0x00);
    __m128i DebugBlueBkg_4x = _mm_set1_epi32(0xFF);

    __m128 UDelta_4x = _mm_set1_ps(uDelta);
    __m128 VDelta_4x = _mm_set1_ps(vDelta);

    __m128 Zero_4x = _mm_set1_ps(0.0f);
    __m128 One_4x = _mm_set1_ps(1.0f);

    // -- Position the pixel cursor to start at a 4 multiple.
    minX = minX & ~3;
    u8 *dest = (u8 *) buffer->memory + minY * buffer->pitch + minX * buffer->bytes_per_pixel;

    // -- Compute the index of the final 4 pixel row block and how many pixel there are until the end.
    u32 rowLastPixelBlockIndex = (maxX - 1) & ~3;
    u32 rowLastPixelBlockCount = ((maxX - 1) & 3) + 1;

    for (s32 y = minY; y < maxY; ++y)
    {
        u32 *destPixel = (u32 *) dest;

        for (s32 x = minX; x < maxX; x += 4)
        {
            #if 1
            __m128 InvAxisMat1_4x = _mm_setr_ps(invAxisMat[0][0], invAxisMat[1][0], invAxisMat[2][0], 0);
            __m128 InvAxisMat2_4x = _mm_setr_ps(invAxisMat[0][1], invAxisMat[1][1], invAxisMat[2][1], 0);
            __m128 InvAxisMat3_4x = _mm_setr_ps(invAxisMat[0][2], invAxisMat[1][2], invAxisMat[2][2], 0);

            r32 tx = (r32) x;
            r32 ty = (r32) y;

            __m128 CurrentPoint1X_4x = _mm_set1_ps(tx);
            __m128 CurrentPoint1Y_4x = _mm_set1_ps(ty);
            __m128 CurrentPoint1Z_4x = One_4x;

            __m128 CurrentPoint2X_4x = _mm_set1_ps(tx + 1);
            __m128 CurrentPoint2Y_4x = _mm_set1_ps(ty);
            __m128 CurrentPoint2Z_4x = One_4x;

            __m128 CurrentPoint3X_4x = _mm_set1_ps(tx + 2);
            __m128 CurrentPoint3Y_4x = _mm_set1_ps(ty);
            __m128 CurrentPoint3Z_4x = One_4x;

            __m128 CurrentPoint4X_4x = _mm_set1_ps(tx + 3);
            __m128 CurrentPoint4Y_4x = _mm_set1_ps(ty);
            __m128 CurrentPoint4Z_4x = One_4x;

            __m128 UV1_4x = _mm_add_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(InvAxisMat1_4x, CurrentPoint1X_4x),
                                    _mm_mul_ps(InvAxisMat2_4x, CurrentPoint1Y_4x)),
                                _mm_mul_ps(InvAxisMat3_4x, CurrentPoint1Z_4x));

            __m128 UV2_4x = _mm_add_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(InvAxisMat1_4x, CurrentPoint2X_4x),
                                    _mm_mul_ps(InvAxisMat2_4x, CurrentPoint2Y_4x)),
                                _mm_mul_ps(InvAxisMat3_4x, CurrentPoint2Z_4x));

            __m128 UV3_4x = _mm_add_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(InvAxisMat1_4x, CurrentPoint3X_4x),
                                    _mm_mul_ps(InvAxisMat2_4x, CurrentPoint3Y_4x)),
                                _mm_mul_ps(InvAxisMat3_4x, CurrentPoint3Z_4x));

            __m128 UV4_4x = _mm_add_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(InvAxisMat1_4x, CurrentPoint4X_4x),
                                    _mm_mul_ps(InvAxisMat2_4x, CurrentPoint4Y_4x)),
                                _mm_mul_ps(InvAxisMat3_4x, CurrentPoint4Z_4x));

            __m128 U_4x = _mm_add_ps(_mm_setr_ps(M(UV1_4x, 0),
                                                 M(UV2_4x, 0),
                                                 M(UV3_4x, 0),
                                                 M(UV4_4x, 0)), UDelta_4x);

            __m128 V_4x = _mm_add_ps(_mm_setr_ps(M(UV1_4x, 1),
                                                 M(UV2_4x, 1),
                                                 M(UV3_4x, 1),
                                                 M(UV4_4x, 1)), VDelta_4x);

            #else

            vec3 CurrentPoint1 = {(r32) x, (r32) y, 1};
            vec3 UV1 = mat3_mulvec(invAxisMat, CurrentPoint1);

            vec3 CurrentPoint2 = {(r32) (x + 1), (r32) y, 1};
            vec3 UV2 = mat3_mulvec(invAxisMat, CurrentPoint2);

            vec3 CurrentPoint3 = {(r32) (x + 2), (r32) y, 1};
            vec3 UV3 = mat3_mulvec(invAxisMat, CurrentPoint3);

            vec3 CurrentPoint4 = {(r32) (x + 3), (r32) y, 1};
            vec3 UV4 = mat3_mulvec(invAxisMat, CurrentPoint4);

            // TODO(gabic): de adaugat si un clamp(0, 1) aici.
            __m128 U_4x = _mm_add_ps(_mm_setr_ps(UV1.u, UV2.u, UV3.u, UV4.u), UDelta_4x);
            __m128 V_4x = _mm_add_ps(_mm_setr_ps(UV1.v, UV2.v, UV3.v, UV4.v), VDelta_4x);

            #endif

            __m128i WriteMask_4x = _mm_castps_si128(
                                _mm_and_ps(
                                    _mm_and_ps(
                                        _mm_cmpge_ps(U_4x, Zero_4x),
                                        _mm_cmplt_ps(U_4x, One_4x)),
                                    _mm_and_ps(
                                        _mm_cmpge_ps(V_4x, Zero_4x),
                                        _mm_cmplt_ps(V_4x, One_4x))));

            if (Mi(WriteMask_4x, 0) == 0 && Mi(WriteMask_4x, 1) == 0 && Mi(WriteMask_4x, 2) == 0 && Mi(WriteMask_4x, 3) == 0)
            {
                destPixel += 4;
                continue;
            }

            __m128 tCol_4x = _mm_mul_ps(U_4x, BitmapWidth_r32_4x);
            __m128 tRow_4x = _mm_mul_ps(V_4x, BitmapHeight_r32_4x);

            __m128i TexelCol_4x = _mm_cvttps_epi32(tCol_4x);
            __m128i TexelRow_4x = _mm_cvttps_epi32(tRow_4x);

            __m128i offset_4x = _mm_add_epi32(
                                    _mm_or_si128(
                                        _mm_slli_epi32(_mm_mulhi_epi16(TexelRow_4x, BitmapPitch_4x), 16),
                                        _mm_mullo_epi16(TexelRow_4x, BitmapPitch_4x)),
                                    TexelCol_4x);

            offset_4x = _mm_and_si128(offset_4x, WriteMask_4x);

#if USE_BILINEAR_FILTERING

            __m128i tRowMin_4x = _mm_cvttps_epi32(tRow_4x);
            __m128i tRowMax_4x = _mm_cvtps_epi32(_mm_add_ps(tRow_4x, Half_4x));

            __m128i tColMin_4x = _mm_cvttps_epi32(tCol_4x);
            __m128i tColMax_4x = _mm_cvtps_epi32(_mm_add_ps(tCol_4x, Half_4x));

            __m128i tRowMinMul_4x = _mm_or_si128(
                                        _mm_slli_epi32(_mm_mulhi_epi16(tRowMin_4x, BitmapPitch_4x), 16),
                                        _mm_mullo_epi16(tRowMin_4x, BitmapPitch_4x));

            __m128i tRowMaxMul_4x = _mm_or_si128(
                                        _mm_slli_epi32(_mm_mulhi_epi16(tRowMax_4x, BitmapPitch_4x), 16),
                                        _mm_mullo_epi16(tRowMax_4x, BitmapPitch_4x));

            __m128i TexelA_offset_4x = _mm_and_si128(_mm_add_epi32(tRowMinMul_4x, tColMin_4x), WriteMask_4x);
            __m128i TexelB_offset_4x = _mm_and_si128(_mm_add_epi32(tRowMinMul_4x, tColMax_4x), WriteMask_4x);
            __m128i TexelC_offset_4x = _mm_and_si128(_mm_add_epi32(tRowMaxMul_4x, tColMin_4x), WriteMask_4x);
            __m128i TexelD_offset_4x = _mm_and_si128(_mm_add_epi32(tRowMaxMul_4x, tColMax_4x), WriteMask_4x);

            __m128 deltaRow_4x = _mm_sub_ps(tRow_4x, _mm_cvtepi32_ps(TexelRow_4x));
            __m128 deltaCol_4x = _mm_sub_ps(tCol_4x, _mm_cvtepi32_ps(TexelCol_4x));

            __m128i Sample1_4x = _mm_setr_epi32(
                *((u32 *) Bitmap->memory + Mi(TexelA_offset_4x, 0)),
                *((u32 *) Bitmap->memory + Mi(TexelA_offset_4x, 1)),
                *((u32 *) Bitmap->memory + Mi(TexelA_offset_4x, 2)),
                *((u32 *) Bitmap->memory + Mi(TexelA_offset_4x, 3))
            );

            __m128i Sample2_4x = _mm_setr_epi32(
                *((u32 *) Bitmap->memory + Mi(TexelB_offset_4x, 0)),
                *((u32 *) Bitmap->memory + Mi(TexelB_offset_4x, 1)),
                *((u32 *) Bitmap->memory + Mi(TexelB_offset_4x, 2)),
                *((u32 *) Bitmap->memory + Mi(TexelB_offset_4x, 3))
            );

            __m128i Sample3_4x = _mm_setr_epi32(
                *((u32 *) Bitmap->memory + Mi(TexelC_offset_4x, 0)),
                *((u32 *) Bitmap->memory + Mi(TexelC_offset_4x, 1)),
                *((u32 *) Bitmap->memory + Mi(TexelC_offset_4x, 2)),
                *((u32 *) Bitmap->memory + Mi(TexelC_offset_4x, 3))
            );

            __m128i Sample4_4x = _mm_setr_epi32(
                *((u32 *) Bitmap->memory + Mi(TexelD_offset_4x, 0)),
                *((u32 *) Bitmap->memory + Mi(TexelD_offset_4x, 1)),
                *((u32 *) Bitmap->memory + Mi(TexelD_offset_4x, 2)),
                *((u32 *) Bitmap->memory + Mi(TexelD_offset_4x, 3))
            );
    #if 1
            // ----------------------------------------------------------------------------------
            // -- Extract the components for each sample.
            // ----------------------------------------------------------------------------------

            __m128i Sample1r_4x = _mm_and_si128(_mm_srli_epi32(Sample1_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);
            __m128i Sample2r_4x = _mm_and_si128(_mm_srli_epi32(Sample2_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);
            __m128i Sample3r_4x = _mm_and_si128(_mm_srli_epi32(Sample3_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);
            __m128i Sample4r_4x = _mm_and_si128(_mm_srli_epi32(Sample4_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);

            __m128i Sample1g_4x = _mm_and_si128(_mm_srli_epi32(Sample1_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);
            __m128i Sample2g_4x = _mm_and_si128(_mm_srli_epi32(Sample2_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);
            __m128i Sample3g_4x = _mm_and_si128(_mm_srli_epi32(Sample3_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);
            __m128i Sample4g_4x = _mm_and_si128(_mm_srli_epi32(Sample4_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);

            __m128i Sample1b_4x = _mm_and_si128(_mm_srli_epi32(Sample1_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);
            __m128i Sample2b_4x = _mm_and_si128(_mm_srli_epi32(Sample2_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);
            __m128i Sample3b_4x = _mm_and_si128(_mm_srli_epi32(Sample3_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);
            __m128i Sample4b_4x = _mm_and_si128(_mm_srli_epi32(Sample4_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);

            __m128i Sample1a_4x = _mm_and_si128(_mm_srli_epi32(Sample1_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);
            __m128i Sample2a_4x = _mm_and_si128(_mm_srli_epi32(Sample2_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);
            __m128i Sample3a_4x = _mm_and_si128(_mm_srli_epi32(Sample3_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);
            __m128i Sample4a_4x = _mm_and_si128(_mm_srli_epi32(Sample4_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);

            // -- sRGB255 to linear.

            __m128 Sample1r_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample1r_4x), OneOver255_4x);
            __m128 Sample2r_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample2r_4x), OneOver255_4x);
            __m128 Sample3r_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample3r_4x), OneOver255_4x);
            __m128 Sample4r_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample4r_4x), OneOver255_4x);

            Sample1r_r32_4x = mmSquare(Sample1r_r32_4x);
            Sample2r_r32_4x = mmSquare(Sample2r_r32_4x);
            Sample3r_r32_4x = mmSquare(Sample3r_r32_4x);
            Sample4r_r32_4x = mmSquare(Sample4r_r32_4x);

            __m128 Sample1g_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample1g_4x), OneOver255_4x);
            __m128 Sample2g_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample2g_4x), OneOver255_4x);
            __m128 Sample3g_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample3g_4x), OneOver255_4x);
            __m128 Sample4g_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample4g_4x), OneOver255_4x);

            Sample1g_r32_4x = mmSquare(Sample1g_r32_4x);
            Sample2g_r32_4x = mmSquare(Sample2g_r32_4x);
            Sample3g_r32_4x = mmSquare(Sample3g_r32_4x);
            Sample4g_r32_4x = mmSquare(Sample4g_r32_4x);

            __m128 Sample1b_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample1b_4x), OneOver255_4x);
            __m128 Sample2b_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample2b_4x), OneOver255_4x);
            __m128 Sample3b_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample3b_4x), OneOver255_4x);
            __m128 Sample4b_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample4b_4x), OneOver255_4x);

            Sample1b_r32_4x = mmSquare(Sample1b_r32_4x);
            Sample2b_r32_4x = mmSquare(Sample2b_r32_4x);
            Sample3b_r32_4x = mmSquare(Sample3b_r32_4x);
            Sample4b_r32_4x = mmSquare(Sample4b_r32_4x);

            __m128 Sample1a_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample1a_4x), OneOver255_4x);
            __m128 Sample2a_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample2a_4x), OneOver255_4x);
            __m128 Sample3a_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample3a_4x), OneOver255_4x);
            __m128 Sample4a_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(Sample4a_4x), OneOver255_4x);

            // -- Blending.

            __m128 tmpR_up_4x = _mm_add_ps(Sample1r_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample2r_r32_4x, Sample1r_r32_4x),
                           deltaCol_4x));

            __m128 tmpG_up_4x = _mm_add_ps(Sample1g_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample2g_r32_4x, Sample1g_r32_4x),
                           deltaCol_4x));

            __m128 tmpB_up_4x = _mm_add_ps(Sample1b_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample2b_r32_4x, Sample1b_r32_4x),
                           deltaCol_4x));

            __m128 tmpA_up_4x = _mm_add_ps(Sample1a_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample2a_r32_4x, Sample1a_r32_4x),
                           deltaCol_4x));

            __m128 tmpR_down_4x = _mm_add_ps(Sample3r_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample4r_r32_4x, Sample3r_r32_4x),
                           deltaCol_4x));

            __m128 tmpG_down_4x = _mm_add_ps(Sample3g_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample4g_r32_4x, Sample3g_r32_4x),
                           deltaCol_4x));

            __m128 tmpB_down_4x = _mm_add_ps(Sample3b_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample4b_r32_4x, Sample3b_r32_4x),
                           deltaCol_4x));

            __m128 tmpA_down_4x = _mm_add_ps(Sample3a_r32_4x,
                _mm_mul_ps(_mm_sub_ps(Sample4a_r32_4x, Sample3a_r32_4x),
                           deltaCol_4x));

            __m128 SourceSamplesLinearR_4x = _mm_add_ps(tmpR_up_4x,
                                                _mm_mul_ps(_mm_sub_ps(tmpR_down_4x, tmpR_up_4x),
                                                        deltaRow_4x));

            __m128 SourceSamplesLinearG_4x = _mm_add_ps(tmpG_up_4x,
                                                _mm_mul_ps(_mm_sub_ps(tmpG_down_4x, tmpG_up_4x),
                                                        deltaRow_4x));

            __m128 SourceSamplesLinearB_4x = _mm_add_ps(tmpB_up_4x,
                                                _mm_mul_ps(_mm_sub_ps(tmpB_down_4x, tmpB_up_4x),
                                                        deltaRow_4x));

            __m128 SourceSamplesLinearA_4x = _mm_add_ps(tmpA_up_4x,
                                                _mm_mul_ps(_mm_sub_ps(tmpA_down_4x, tmpA_up_4x),
                                                        deltaRow_4x));
    #else
            // ----------------------------------------------------------------------------------

            __m128 BilinearSampleFinal_r32_4x[4];

            __m128i TexelRow_inc_4x = _mm_setr_epi32(0, 0, 1, 1);
            __m128i TexelCol_inc_4x = _mm_setr_epi32(0, 1, 0, 1);
            __m128i __offset_inc_4x = _mm_setr_epi32(0, 1, Bitmap->BitmapInfo.width, Bitmap->BitmapInfo.width + 1);

            __m128i __offset_4x = _mm_setr_epi32(Mi(offset_4x, 0), Mi(offset_4x, 1), Mi(offset_4x, 2), Mi(offset_4x, 3));

            for (u32 ti = 0; ti < 4; ++ti)
            {
                __m128 dCol_4x = _mm_set1_ps(M(deltaCol_4x, ti));
                __m128 dRow_4x = _mm_set1_ps(M(deltaRow_4x, ti));

                u32 _offset = *((u32 *) &offset_4x + ti);
                __m128i __offset_4x = _mm_set1_epi32(_offset);

                u32 _texelRow = *((u32 *) &TexelRow_4x + ti);
                u32 _texelCol = *((u32 *) &TexelCol_4x + ti);

                __m128i TexelRowA_4x = _mm_set1_epi32(_texelRow);
                __m128i TexelColA_4x = _mm_set1_epi32(_texelCol);
                TexelRowA_4x = _mm_add_epi32(TexelRowA_4x, TexelRow_inc_4x);
                TexelColA_4x = _mm_add_epi32(TexelColA_4x, TexelCol_inc_4x);

                __m128i tmpmask = _mm_and_si128(
                                    _mm_cmplt_epi32(TexelRowA_4x, BitmapHeight_u32_4x),
                                    _mm_cmplt_epi32(TexelColA_4x, BitmapWidth_u32_4x));

                __offset_4x = _mm_and_si128(_mm_add_epi32(__offset_4x, __offset_inc_4x), tmpmask);

                __m128i BilinearSamplesPacked_4x = _mm_setr_epi32(
                            *((u32 *) Bitmap->memory + Mi(__offset_4x, 0)),
                            *((u32 *) Bitmap->memory + Mi(__offset_4x, 1)),
                            *((u32 *) Bitmap->memory + Mi(__offset_4x, 2)),
                            *((u32 *) Bitmap->memory + Mi(__offset_4x, 3)));
                BilinearSamplesPacked_4x = _mm_and_si128(BilinearSamplesPacked_4x, tmpmask);

                __m128i BilinearSamplesR_4x = _mm_and_si128(_mm_srli_epi32(BilinearSamplesPacked_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);
                __m128i BilinearSamplesG_4x = _mm_and_si128(_mm_srli_epi32(BilinearSamplesPacked_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);
                __m128i BilinearSamplesB_4x = _mm_and_si128(_mm_srli_epi32(BilinearSamplesPacked_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);
                __m128i BilinearSamplesA_4x = _mm_and_si128(_mm_srli_epi32(BilinearSamplesPacked_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);

                __m128i BilinearSample1_u32_4x = _mm_setr_epi32(
                                                    Mi(BilinearSamplesR_4x, 0),
                                                    Mi(BilinearSamplesG_4x, 0),
                                                    Mi(BilinearSamplesB_4x, 0),
                                                    Mi(BilinearSamplesA_4x, 0));

                __m128i BilinearSample2_u32_4x = _mm_setr_epi32(
                                                    Mi(BilinearSamplesR_4x, 1),
                                                    Mi(BilinearSamplesG_4x, 1),
                                                    Mi(BilinearSamplesB_4x, 1),
                                                    Mi(BilinearSamplesA_4x, 1));

                __m128i BilinearSample3_u32_4x = _mm_setr_epi32(
                                                    Mi(BilinearSamplesR_4x, 2),
                                                    Mi(BilinearSamplesG_4x, 2),
                                                    Mi(BilinearSamplesB_4x, 2),
                                                    Mi(BilinearSamplesA_4x, 2));

                __m128i BilinearSample4_u32_4x = _mm_setr_epi32(
                                                    Mi(BilinearSamplesR_4x, 3),
                                                    Mi(BilinearSamplesG_4x, 3),
                                                    Mi(BilinearSamplesB_4x, 3),
                                                    Mi(BilinearSamplesA_4x, 3));

                // -- sRGB255 to linear.

                __m128 BilinearSample1_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(BilinearSample1_u32_4x), OneOver255_4x);
                __m128 BilinearSample2_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(BilinearSample2_u32_4x), OneOver255_4x);
                __m128 BilinearSample3_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(BilinearSample3_u32_4x), OneOver255_4x);
                __m128 BilinearSample4_r32_4x = _mm_mul_ps(_mm_cvtepi32_ps(BilinearSample4_u32_4x), OneOver255_4x);

                r32 tmpA1 = M(BilinearSample1_r32_4x, 3);
                r32 tmpA2 = M(BilinearSample2_r32_4x, 3);
                r32 tmpA3 = M(BilinearSample3_r32_4x, 3);
                r32 tmpA4 = M(BilinearSample4_r32_4x, 3);

                BilinearSample1_r32_4x = mmSquare(BilinearSample1_r32_4x);
                BilinearSample2_r32_4x = mmSquare(BilinearSample2_r32_4x);
                BilinearSample3_r32_4x = mmSquare(BilinearSample3_r32_4x);
                BilinearSample4_r32_4x = mmSquare(BilinearSample4_r32_4x);

                *((r32 *) &BilinearSample1_r32_4x + 3) = tmpA1;
                *((r32 *) &BilinearSample2_r32_4x + 3) = tmpA2;
                *((r32 *) &BilinearSample3_r32_4x + 3) = tmpA3;
                *((r32 *) &BilinearSample4_r32_4x + 3) = tmpA4;

                __m128 BilinearSampleTmp1_r32_4x = _mm_add_ps(BilinearSample1_r32_4x,
                                                        _mm_mul_ps(_mm_sub_ps(BilinearSample2_r32_4x, BilinearSample1_r32_4x),
                                                            dCol_4x));

                __m128 BilinearSampleTmp2_r32_4x = _mm_add_ps(BilinearSample3_r32_4x,
                                                        _mm_mul_ps(_mm_sub_ps(BilinearSample4_r32_4x, BilinearSample3_r32_4x),
                                                            dCol_4x));

                BilinearSampleFinal_r32_4x[ti] = _mm_add_ps(BilinearSampleTmp1_r32_4x,
                                                        _mm_mul_ps(_mm_sub_ps(BilinearSampleTmp2_r32_4x, BilinearSampleTmp1_r32_4x),
                                                            dRow_4x));
            }

            __m128 SourceSamplesLinearR_4x = _mm_setr_ps(
                                                M(BilinearSampleFinal_r32_4x[0], 0),
                                                M(BilinearSampleFinal_r32_4x[1], 0),
                                                M(BilinearSampleFinal_r32_4x[2], 0),
                                                M(BilinearSampleFinal_r32_4x[3], 0));

            __m128 SourceSamplesLinearG_4x = _mm_setr_ps(
                                                M(BilinearSampleFinal_r32_4x[0], 1),
                                                M(BilinearSampleFinal_r32_4x[1], 1),
                                                M(BilinearSampleFinal_r32_4x[2], 1),
                                                M(BilinearSampleFinal_r32_4x[3], 1));

            __m128 SourceSamplesLinearB_4x = _mm_setr_ps(
                                                M(BilinearSampleFinal_r32_4x[0], 2),
                                                M(BilinearSampleFinal_r32_4x[1], 2),
                                                M(BilinearSampleFinal_r32_4x[2], 2),
                                                M(BilinearSampleFinal_r32_4x[3], 2));

            __m128 SourceSamplesLinearA_4x = _mm_setr_ps(
                                                M(BilinearSampleFinal_r32_4x[0], 3),
                                                M(BilinearSampleFinal_r32_4x[1], 3),
                                                M(BilinearSampleFinal_r32_4x[2], 3),
                                                M(BilinearSampleFinal_r32_4x[3], 3));
    #endif

#else
            // ----------------------------------------------------------------------------------
            // -- Source bitmap samples.
            // ----------------------------------------------------------------------------------

    #if 1
            u32 *SourceSample1 = (u32 *) Bitmap->memory + Mi(offset_4x, 0);
            u32 *SourceSample2 = (u32 *) Bitmap->memory + Mi(offset_4x, 1);
            u32 *SourceSample3 = (u32 *) Bitmap->memory + Mi(offset_4x, 2);
            u32 *SourceSample4 = (u32 *) Bitmap->memory + Mi(offset_4x, 3);
    #else
            u32 *SourceSample1 = (u32 *) Bitmap->memory + Mi(TexelRow_4x, 0) * (Bitmap->BitmapInfo.width) + Mi(TexelCol_4x, 0);
            u32 *SourceSample2 = (u32 *) Bitmap->memory + Mi(TexelRow_4x, 1) * (Bitmap->BitmapInfo.width) + Mi(TexelCol_4x, 1);
            u32 *SourceSample3 = (u32 *) Bitmap->memory + Mi(TexelRow_4x, 2) * (Bitmap->BitmapInfo.width) + Mi(TexelCol_4x, 2);
            u32 *SourceSample4 = (u32 *) Bitmap->memory + Mi(TexelRow_4x, 3) * (Bitmap->BitmapInfo.width) + Mi(TexelCol_4x, 3);
    #endif

            // ---------------------------------------------------------------------------------
            // -- Unpacking the four source bitmap color samples.
            // ---------------------------------------------------------------------------------

            __m128i SourceSamplesPacked_4x = _mm_setr_epi32(*SourceSample1, *SourceSample2, *SourceSample3, *SourceSample4);

            __m128i SourceSamplesR_4x = _mm_and_si128(_mm_srli_epi32(SourceSamplesPacked_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);
            __m128i SourceSamplesG_4x = _mm_and_si128(_mm_srli_epi32(SourceSamplesPacked_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);
            __m128i SourceSamplesB_4x = _mm_and_si128(_mm_srli_epi32(SourceSamplesPacked_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);
            __m128i SourceSamplesA_4x = _mm_and_si128(_mm_srli_epi32(SourceSamplesPacked_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);

            #if DEBUG_BITMAP_BACKGROUND
            // NOTE(gabic): aici cred ca trebuie sa fac blend intre debug background is culoare.
            __m128i AlphaMask_4x = _mm_cmpgt_epi32(SourceSamplesA_4x, ZeroInt_4x);

            SourceSamplesA_4x = _mm_set1_epi32(0xFF);

            SourceSamplesR_4x = _mm_or_si128(_mm_and_si128(AlphaMask_4x, SourceSamplesR_4x),
                                                _mm_andnot_si128(AlphaMask_4x, DebugRedBkg_4x));
            SourceSamplesG_4x = _mm_or_si128(_mm_and_si128(AlphaMask_4x, SourceSamplesG_4x),
                                                _mm_andnot_si128(AlphaMask_4x, DebugGreenBkg_4x));
            SourceSamplesB_4x = _mm_or_si128(_mm_and_si128(AlphaMask_4x, SourceSamplesB_4x),
                                                _mm_andnot_si128(AlphaMask_4x, DebugBlueBkg_4x));
            #endif

            // -- sRGB255 to linear.

            __m128 SourceSamplesLinearR_4x = _mm_mul_ps(_mm_cvtepi32_ps(SourceSamplesR_4x), OneOver255_4x);
            __m128 SourceSamplesLinearG_4x = _mm_mul_ps(_mm_cvtepi32_ps(SourceSamplesG_4x), OneOver255_4x);
            __m128 SourceSamplesLinearB_4x = _mm_mul_ps(_mm_cvtepi32_ps(SourceSamplesB_4x), OneOver255_4x);
            __m128 SourceSamplesLinearA_4x = _mm_mul_ps(_mm_cvtepi32_ps(SourceSamplesA_4x), OneOver255_4x);

            SourceSamplesLinearR_4x = mmSquare(SourceSamplesLinearR_4x);
            SourceSamplesLinearG_4x = mmSquare(SourceSamplesLinearG_4x);
            SourceSamplesLinearB_4x = mmSquare(SourceSamplesLinearB_4x);
#endif

            // ----------------------------------------------------------------------------------
            // -- Destination samples.
            // ----------------------------------------------------------------------------------

            u32 *DestinationSample1 = (u32 *) dest;
            u32 *DestinationSample2 = (u32 *) dest;
            u32 *DestinationSample3 = (u32 *) dest;
            u32 *DestinationSample4 = (u32 *) dest;

            // -- Last row specific processing - reading only the necessary pixels within the
            // -- buffer boundary.

            if (y == (maxY - 1) && (u32) x == rowLastPixelBlockIndex && rowLastPixelBlockCount < 4)
            {
                u32 count = rowLastPixelBlockCount;

                if (count-- > 0)
                    DestinationSample1 = destPixel + 0;
                if (count-- > 0)
                    DestinationSample2 = destPixel + 1;
                if (count-- > 0)
                    DestinationSample3 = destPixel + 2;
                if (count-- > 0)
                    DestinationSample4 = destPixel + 3;
            }
            else
            {
                DestinationSample1 = destPixel + 0;
                DestinationSample2 = destPixel + 1;
                DestinationSample3 = destPixel + 2;
                DestinationSample4 = destPixel + 3;
            }

            // ---------------------------------------------------------------------------------
            // -- Unpacking the four destination buffer color samples.
            // ---------------------------------------------------------------------------------

            __m128i DestinationSamplesPacked_4x = _mm_setr_epi32(*DestinationSample1, *DestinationSample2, *DestinationSample3, *DestinationSample4);

            __m128i DestinationSamplesA_4x = _mm_and_si128(_mm_srli_epi32(DestinationSamplesPacked_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT), Mask0xFF_4x);
            __m128i DestinationSamplesR_4x = _mm_and_si128(_mm_srli_epi32(DestinationSamplesPacked_4x, GL_PIXEL_FORMAT_RED_SHIFT), Mask0xFF_4x);
            __m128i DestinationSamplesG_4x = _mm_and_si128(_mm_srli_epi32(DestinationSamplesPacked_4x, GL_PIXEL_FORMAT_GREEN_SHIFT), Mask0xFF_4x);
            __m128i DestinationSamplesB_4x = _mm_and_si128(_mm_srli_epi32(DestinationSamplesPacked_4x, GL_PIXEL_FORMAT_BLUE_SHIFT), Mask0xFF_4x);

            // -- sRGB255 to linear.

            __m128 DestinationSamplesLinearR_4x = _mm_mul_ps(_mm_cvtepi32_ps(DestinationSamplesR_4x), OneOver255_4x);
            __m128 DestinationSamplesLinearG_4x = _mm_mul_ps(_mm_cvtepi32_ps(DestinationSamplesG_4x), OneOver255_4x);
            __m128 DestinationSamplesLinearB_4x = _mm_mul_ps(_mm_cvtepi32_ps(DestinationSamplesB_4x), OneOver255_4x);
            __m128 DestinationSamplesLinearA_4x = _mm_mul_ps(_mm_cvtepi32_ps(DestinationSamplesA_4x), OneOver255_4x);

            DestinationSamplesLinearR_4x = mmSquare(DestinationSamplesLinearR_4x);
            DestinationSamplesLinearG_4x = mmSquare(DestinationSamplesLinearG_4x);
            DestinationSamplesLinearB_4x = mmSquare(DestinationSamplesLinearB_4x);

            // -- lerp.

            __m128 oneMinusSourceAlpha = _mm_sub_ps(One_4x, SourceSamplesLinearA_4x);

            __m128 BlendedR_4x = _mm_add_ps(
                                        SourceSamplesLinearR_4x,
                                        _mm_mul_ps(DestinationSamplesLinearR_4x, oneMinusSourceAlpha));

            __m128 BlendedG_4x = _mm_add_ps(
                                        SourceSamplesLinearG_4x,
                                        _mm_mul_ps(DestinationSamplesLinearG_4x, oneMinusSourceAlpha));

            __m128 BlendedB_4x = _mm_add_ps(
                                        SourceSamplesLinearB_4x,
                                        _mm_mul_ps(DestinationSamplesLinearB_4x, oneMinusSourceAlpha));

            // -- linear to sRGB255

            BlendedR_4x = _mm_mul_ps(_mm_sqrt_ps(BlendedR_4x), One255_4x);
            BlendedG_4x = _mm_mul_ps(_mm_sqrt_ps(BlendedG_4x), One255_4x);
            BlendedB_4x = _mm_mul_ps(_mm_sqrt_ps(BlendedB_4x), One255_4x);

            __m128i BlendedUnpackedR_4x = _mm_slli_epi32(
                                                _mm_and_si128(_mm_cvtps_epi32(BlendedR_4x), Mask0xFF_4x),
                                                GL_PIXEL_FORMAT_RED_SHIFT);

            __m128i BlendedUnpackedG_4x = _mm_slli_epi32(
                                                _mm_and_si128(_mm_cvtps_epi32(BlendedG_4x), Mask0xFF_4x),
                                                GL_PIXEL_FORMAT_GREEN_SHIFT);

            __m128i BlendedUnpackedB_4x = _mm_slli_epi32(
                                                _mm_and_si128(_mm_cvtps_epi32(BlendedB_4x), Mask0xFF_4x),
                                                GL_PIXEL_FORMAT_BLUE_SHIFT);

            __m128i BlendedUnpackedA_4x = _mm_slli_epi32(Mask0xFF_4x, GL_PIXEL_FORMAT_ALPHA_SHIFT);
            __m128i BlendedPacked_4x = _mm_or_si128(
                                            _mm_or_si128(BlendedUnpackedR_4x, BlendedUnpackedG_4x),
                                            _mm_or_si128(BlendedUnpackedB_4x, BlendedUnpackedA_4x));

            BlendedPacked_4x = _mm_or_si128(
                                    _mm_andnot_si128(WriteMask_4x, DestinationSamplesPacked_4x),
                                    _mm_and_si128(WriteMask_4x, BlendedPacked_4x));

            // -- At the last row the pixels will be set manually to be within the buffer boundary.

            if (y == (maxY - 1) && (u32) x == rowLastPixelBlockIndex && rowLastPixelBlockCount < 4)
            {
                for (u32 i = 0; i < rowLastPixelBlockCount; ++i) {
                    *destPixel++ = Mi(BlendedPacked_4x, i);
                }
            }
            else
            {
                _mm_store_si128((__m128i *) destPixel, BlendedPacked_4x);
                destPixel += 4;
            }

            // CheckPixelX_4x = _mm_add_ps(CheckPixelX_4x, Four_4x);
            // TODO(gabic): aici trebuie sa iau in calcul ca se proceseaza de fapt 4 pixeli.
        }

        dest += buffer->pitch;
    }

    // if (mustLock)
    //     SDL_UnlockSurface(backbuffer_surface);
}

/**
 * No SSE bitmap drawing routine.
 */
void gl_drawBitmapBasis_noSSE(videobuffer_t *buffer, GlRenderObject2D *Object)
{
    if (Object->WorldPoints->length != 4)
        return;

    AssetBitmapMemory *Bitmap = Object->Bitmap;

    vec2 P1 = *((vec2 *) ta_index(Object->WorldPoints, 0));
    vec2 P2 = *((vec2 *) ta_index(Object->WorldPoints, 1));
    vec2 P3 = *((vec2 *) ta_index(Object->WorldPoints, 2));
    vec2 P4 = *((vec2 *) ta_index(Object->WorldPoints, 3));

    vec2 AxisX = vec2_sub(P2, P1);
    vec2 AxisY = vec2_sub(P4, P1);
    r32 AxisWidth = vec2_len(AxisX);
    r32 AxisHeight = vec2_len(AxisY);

    vec2 Position = Object->Transformation.translation;

    r32 axisMat[3][3];
    r32 invAxisMat[3][3];

    mat3_create_transform(axisMat, AxisX, AxisY, P1);
    mat3_inv(axisMat, invAxisMat);

    r32 oneOverAxisWidth = 1.0f / AxisWidth;
    r32 oneOverAxisHeight = 1.0f / AxisHeight;

    // This is to correct the imprecisions of the floating points representation
    // in the UV computation.
    r32 uDelta = oneOverAxisWidth / 10;
    r32 vDelta = oneOverAxisHeight / 10;

    r32 width = vec2_len(AxisX);
    r32 height = vec2_len(AxisY);

    s32 minX = buffer->width;
    s32 minY = buffer->height;
    s32 maxX = 0;
    s32 maxY = 0;

    vec2 Points[4] = {P1, P2, P3, P4};

    for (u32 i = 0; i < 4; ++i)
    {
        vec2 Current = Points[i];
        s32 cX = (s32) Current.x;
        s32 cY = (s32) Current.y;

        if (cX < minX)
            minX = cX;

        if (cX > maxX)
            maxX = cX + 1;

        if (cY < minY)
            minY = cY;

        if (cY > maxY)
            maxY = cY + 1;
    }

    if (minX < 0)
        minX = 0;

    if (minY < 0)
        minY = 0;

    if (maxX > (s32) buffer->width)
        maxX = (s32) buffer->width;

    if (maxY > (s32) buffer->height)
        maxY = (s32) buffer->height;

    // if (mustLock)
    //     SDL_LockSurface(backbuffer_surface);

    // u8 *source = (u8 *) Bitmap->memory;
    u8 *dest = (u8 *) buffer->memory + minY * buffer->pitch + minX * buffer->bytes_per_pixel;

    // r32 oneOverWidth = 1 / width;
    // r32 oneOverHeight = 1 / height;

    for (s32 row = minY; row < maxY; ++row)
    {
        // u32 *pixel = (u32 *) source;
        u32 *destRow = (u32 *) dest;

        for (s32 col = minX; col < maxX; ++col)
        {
            vec3 CurrentPoint = {(r32) col, (r32) row, 1};
            vec3 UV = mat3_mulvec(invAxisMat, CurrentPoint);

            UV.u += uDelta;
            UV.v += vDelta;

            if (UV.u >= 0 && UV.v >= 0 && UV.u <= 1 && UV.v <= 1)
            {
#if 0
                r32 sampleColR32 = UV.u * (Bitmap->BitmapInfo.width - 1);
                r32 sampleRowR32 = UV.v * (Bitmap->BitmapInfo.height - 1);

                r32 tCol = sampleColR32 - (r32) ((u32) sampleColR32);
                r32 tRow = sampleRowR32 - (r32) ((u32) sampleRowR32);

                vec4 TexelSample;

#if USE_BILINEAR_FILTERING
                // bilinear_sample_t Samples = getBilinearSample(Bitmap, sampleRow, sampleCol);
                bilinear_sample_t Samples = getBilinearSample(Bitmap, sampleRowR32, sampleColR32);

                TexelSample = bilinearBlend(Samples, tRow, tCol, shifts);
#else
                u32 sampleCol = (u32) sampleColR32;
                u32 sampleRow = (u32) sampleRowR32;

                u32 *pixel = (u32 *) Bitmap->memory + sampleRow * Bitmap->BitmapInfo.width + sampleCol;
                u32 packedSample = *pixel;

                TexelSample.r = (r32) ((packedSample >> GL_PIXEL_FORMAT_RED_SHIFT) & 0xFF);
                TexelSample.g = (r32) ((packedSample >> GL_PIXEL_FORMAT_GREEN_SHIFT) & 0xFF);
                TexelSample.b = (r32) ((packedSample >> GL_PIXEL_FORMAT_BLUE_SHIFT) & 0xFF);
                TexelSample.a = (r32) ((packedSample >> GL_PIXEL_FORMAT_ALPHA_SHIFT) & 0xFF);

                TexelSample = sRGB_linear1(TexelSample);
#endif

#endif

// #if USE_BILINEAR_FILTERING
//                 vec4 TexelSample = gl_sample(Bitmap, {UV.u, UV.v}, shifts, GL_FILTER_BILINEAR);
// #else
// #endif

                // u32 packedSample = gl_sample(Bitmap, {UV.u, UV.v}, GL_FILTER_NEAREST);
                u32 packedSample = 0;
                vec4 TexelSample = gl_srgb_to_linear1(packedSample);

                r32 oneMinusAlpha = 1 - TexelSample.a;
                u32 destSample = *destRow;

                #if DEBUG_BITMAP_BACKGROUND
                vec4 DestSample = {1.0f, 0.0f, 1.0f, 1.0f};
                #else
                vec4 DestSample = unpackColor(destSample);
                DestSample = sRGB_linear1(DestSample);
                DestSample.a = 1.0f;
                #endif

                DestSample.r = TexelSample.r + DestSample.r * oneMinusAlpha;
                DestSample.g = TexelSample.g + DestSample.g * oneMinusAlpha;
                DestSample.b = TexelSample.b + DestSample.b * oneMinusAlpha;

                DestSample = linear1_sRGB255(DestSample);

                *destRow++ = ((u32) (DestSample.r) << GL_PIXEL_FORMAT_RED_SHIFT) |
                            ((u32) (DestSample.g) << GL_PIXEL_FORMAT_GREEN_SHIFT) |
                            ((u32) (DestSample.b) << GL_PIXEL_FORMAT_BLUE_SHIFT) |
                            ((u32) (DestSample.a) << GL_PIXEL_FORMAT_ALPHA_SHIFT);
            }
            else
                destRow++;
        }

        dest += buffer->pitch;
    }

    // if (mustLock)
    //     SDL_UnlockSurface(backbuffer_surface);
}

// ----------------------------------------------------------------------------------
// -- 3D routines.
// ----------------------------------------------------------------------------------

struct DebugData
{
    vec4 position;
    vec4 color;
};
#if 0
void gl_drawLine3D_noSSE(gl_state_t *GL)
{
    // gl_vertex_buffer_t *VertexBuffer = &GL->VertexBuffer;
    videobuffer_t *buffer = GL->buffer;
    r32 *depthbuffer = GL->depthbuffer;
    // r32 *FragmentVertexBuffer = GL->FragmentVertexBuffer;
    gl_program_t *Program = GL->CurrentProgram;
    GlPixelFormat *Shifts = getPixelFormat();

    s32 width = buffer->width;
    s32 height = buffer->height;
    s32 widthMinusOne = width - 1;
    s32 heightMinusOne = height - 1;

    b32 flipped = false;
    b32 reversed = false;
    r32 ratio = 0;
    u32 tmp = 0;

    u8 *Vertex1 = (u8 *) GetVertexBuffer(VertexBuffer, 0);
    u8 *Vertex2 = (u8 *) GetVertexBuffer(VertexBuffer, 1);

    DebugData *Data1 = (DebugData *) Vertex1;
    DebugData *Data2 = (DebugData *) Vertex2;

    vec3 *V1Pos = (vec3 *) Vertex1;
    vec3 *V2Pos = (vec3 *) Vertex2;

    s32 x0 = roundR32ToS32(V1Pos->x);
    s32 y0 = roundR32ToS32(V1Pos->y);
    s32 x1 = roundR32ToS32(V2Pos->x);
    s32 y1 = roundR32ToS32(V2Pos->y);

    // -- Swap x with y, basically draw the line using the y coordinate as the increment.
    if (absDiff(x1, x0) < absDiff(y1, y0))
    {
        tmp = y0;
        y0 = x0;
        x0 = tmp;

        tmp = y1;
        y1 = x1;
        x1 = tmp;

        flipped = true;
    }

    // -- Left to right drawing.
    if (x0 > x1)
    {
        tmp = x0;
        x0 = x1;
        x1 = tmp;

        tmp = y0;
        y0 = y1;
        y1 = tmp;

        reversed = true;
    }

    s32 dx = x1 - x0;
    s32 dy = y1 - y0;
    // s32 dx_x2 = dx * 2;
    // s32 derror = abs(dy) * 2;
    s32 dx_x2 = dx;
    s32 derror = abs(dy);
    s32 error = 0;
    s32 y = y0;
    r32 oneOver = 1.0f / (x1 - x0);

    for (s32 cx = x0; cx <= x1; ++cx)
    {
        s32 bx = cx;
        s32 by = y;

        r32 t = (bx - x0) * oneOver;

        if (reversed)
            t = 1 - t;

        r32 oneMinusT = 1 - t;

        if (flipped)
        {
            bx = y;
            by = cx;
        }

        if (bx >= 0 && by >= 0 && bx < width && by < height)
        {
            r32 pixelZ = V1Pos->z + (V2Pos->z - V1Pos->z) * t;
            u32 index = (u32) (width * by + bx);

            if (bx == 639 && by == 299)
            {
                u32 a = 0;
                a++;
            }

            if (index < GL->depthBufferSize)
            {
                r32 bufferZ = depthbuffer[index];

                if (pixelZ <= bufferZ)
                {
                    // --- Execute the fragment shader.
                    // ----------------------------------------------------------------------------------

                    // GL->Params.Barycentric.x = t;
                    // GL->Params.Barycentric.y = 0;
                    // GL->Params.Barycentric.z = 0;

                    // -- Interpolate the attributes using the computed barycentric coordinates.
                    // ----------------------------------------------------------------------------------

                    r32 *VertexAttr1 = (r32 *) Vertex1 + 4;
                    r32 *VertexAttr2 = (r32 *) Vertex2 + 4;

                    r32 v1W = *((r32 *) Vertex1 + 3);
                    r32 v2W = *((r32 *) Vertex2 + 3);

                    r32 oneOverW = 1 / (v1W * oneMinusT + v2W * t);

                    r32 *FragDest = FragmentVertexBuffer;

                    *FragDest++ = (r32) bx;
                    *FragDest++ = (r32) by;
                    *FragDest++ = (r32) pixelZ;
                    FragDest++;

                    for (u32 k = 4; k < GL->VertexBuffer.count; ++k) {
                        *FragDest++ = (*VertexAttr1++ * oneMinusT + *VertexAttr2++ * t) * oneOverW;
                    }

                    // vec4 Color = Program->fs(FragmentVertexBuffer, GL);
                    vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};

                    // -- Postprocessing.
#if 0
                    r32 cA = 0.7f;
                    r32 gamma = 0.8f;

                    r32 tr = (cA * power(Color.r, gamma));
                    r32 tg = (cA * power(Color.g, gamma));
                    r32 tb = (cA * power(Color.b, gamma));

                    Color.r = tr > 1 ? 1 : tr;
                    Color.g = tg > 1 ? 1 : tg;
                    Color.b = tb > 1 ? 1 : tb;
#else
                    // Color clamp.
                    Color.r = Color.r > 1 ? 1 : Color.r;
                    Color.g = Color.g > 1 ? 1 : Color.g;
                    Color.b = Color.b > 1 ? 1 : Color.b;
#endif

                    Color = linear1_sRGB255(Color);

                    u32 *Dest = (u32 *) ((u8 *) buffer->memory + by * buffer->pitch + bx * buffer->bytes_per_pixel);
                    depthbuffer[index] = pixelZ;

                    *Dest = ((u32) Color.r << GL_PIXEL_FORMAT_RED_SHIFT) |
                            ((u32) Color.g << GL_PIXEL_FORMAT_GREEN_SHIFT) |
                            ((u32) Color.b << GL_PIXEL_FORMAT_BLUE_SHIFT) |
                            ((u32) Color.a << GL_PIXEL_FORMAT_ALPHA_SHIFT);
                }
            }
        }

        error += derror;

        if (error > dx)
        {
            y += y1 > y0 ? 1 : -1;
            error -= dx_x2;
        }
    }
}
#endif

void gl_drawLine3D_SSE(gl_state_t *GL)
{}

// ----------------------------------------------------------------------------------
// -- Debug routines.
// ----------------------------------------------------------------------------------

void renderGradient(videobuffer_t *buffer, u32 x_offset, u32 y_offset)
{
    u32 width = buffer->width;
    u32 height = buffer->height;

    // SDL_bool mustLock = (SDL_bool) SDL_MUSTLOCK(backbuffer_surface);

    // if (mustLock)
    //     SDL_LockSurface(backbuffer_surface);

    u32 *pixelRow = (u32 *) buffer->memory;

    for (u32 row = 0; row < height; ++row)
    {
        u32 *pixel = (u32 *) pixelRow;

        for (u32 col = 0; col < width; ++col)
        {
            u8 red = 0x0;
            u8 green = (row + y_offset) % 256 ;
            u8 blue = (col + x_offset) % 256;
            u8 alpha = 0xFF;

#if 0
            red = 0;
            green = 246;
            blue = 211;
#endif

            *pixel++ = (red << GL_PIXEL_FORMAT_RED_SHIFT) |
                       (green << GL_PIXEL_FORMAT_GREEN_SHIFT) |
                       (blue << GL_PIXEL_FORMAT_BLUE_SHIFT) |
                       (alpha << GL_PIXEL_FORMAT_ALPHA_SHIFT);
        }

        pixelRow += width;
    }

    // if (mustLock)
    //     SDL_UnlockSurface(backbuffer_surface);
}
