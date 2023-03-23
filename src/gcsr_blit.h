// ----------------------------------------------------------------------------------
// -- File: gcsr_blit.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-09-30 19:06:28
// -- Modified:
// ----------------------------------------------------------------------------------

#if USE_SSE_RENDERING == 1

// #define gl_drawLine2D(buffer, p1, p2, color) gl_drawLine2D_SSE(buffer, p1, p2, color)
#define gl_drawLine2D(buffer, p1, p2, color) gl_drawLine2D_noSSE(buffer, p1, p2, color)
#define gl_drawBitmapBasis(buffer, Object) gl_drawBitmapBasis_SSE(buffer, Object)
#define gl_drawLine3D(State) gl_drawLine3D_noSSE(State)
#if USE_EXPERIMENTAL == 1
#define gl_fillTriangle3D(State) gl_fillTriangle3D_exp(State)
#else
#define gl_fillTriangle3D(State) gl_fillTriangle3D_SSE(State)
#endif

#else

#define gl_drawLine2D(buffer, p1, p2, color) gl_drawLine2D_noSSE(buffer, p1, p2, color)
#define gl_drawBitmapBasis(buffer, Object) gl_drawBitmapBasis_noSSE(buffer, Object)
#define gl_drawLine3D(State) gl_drawLine3D_noSSE(State)
#define gl_fillTriangle3D(State) gl_fillTriangle3D_noSSE(State)

#endif
