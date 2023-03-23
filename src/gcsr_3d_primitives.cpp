// ----------------------------------------------------------------------------------
// -- File: gcsr_3d_primitives.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: primitive creation routines.
// -- primitives.
// -- Created: 2020-11-08 22:00:25
// -- Modified: 2021-02-27 19:43:03
// ----------------------------------------------------------------------------------

void gl_createTriangleGeometry(GlGeometry3D *triangle)
{
    if (!triangle)
        return;

    triangle->vertexCount = 3;
    triangle->faceCount = 1;

    u32 vertices = triangle->faceCount * 3 * gcSize(u32);

    triangle->PositionData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec3));
    triangle->PositionIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, vertices);

    triangle->uvData = (vec2 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec2));
    triangle->uvIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, vertices);

    triangle->NormalData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec3));
    triangle->NormalIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, vertices);

    // CCW.
    triangle->PositionData[0] = {0, 0.5f, 0};
    triangle->PositionData[1] = {-0.5f, -0.5f, 0};
    triangle->PositionData[2] = {0.5f, -0.5f, 0};

    triangle->PositionIndex[0] = 1;
    triangle->PositionIndex[1] = 2;
    triangle->PositionIndex[2] = 3;

    triangle->uvData[0] = {0, 1};
    triangle->uvData[1] = {0, 0};
    triangle->uvData[2] = {1, 0};

    triangle->uvIndex[0] = 1;
    triangle->uvIndex[1] = 2;
    triangle->uvIndex[2] = 3;

    triangle->NormalData[0] = {0, 0, 1.0f};

    triangle->NormalIndex[0] = 1;
    triangle->NormalIndex[1] = 1;
    triangle->NormalIndex[2] = 1;
}

void gl_createPlaneGeometry(GlGeometry3D *Plane)
{
    if (!Plane)
        return;

    Plane->faceCount = 2;
    Plane->vertexCount = 4;

    u32 vertices = Plane->faceCount * 3 * gcSize(u32);

    Plane->PositionIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, vertices);
    Plane->PositionData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 4 * gcSize(vec3));

    Plane->uvIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, vertices);
    Plane->uvData = (vec2 *) gc_mem_allocate(MEMORY_PERMANENT, 4 * gcSize(vec2));

    Plane->NormalIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, vertices);
    Plane->NormalData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec3));

    // CCW.
    Plane->PositionData[0] = {-0.5f, 0.5f, 0};
    Plane->PositionData[1] = {-0.5f, -0.5f, 0};
    Plane->PositionData[2] = {0.5f, -0.5f, 0};
    Plane->PositionData[3] = {0.5f, 0.5f, 0};

    Plane->PositionIndex[0] = 1;
    Plane->PositionIndex[1] = 2;
    Plane->PositionIndex[2] = 3;
    Plane->PositionIndex[3] = 1;
    Plane->PositionIndex[4] = 3;
    Plane->PositionIndex[5] = 4;

    Plane->uvData[0] = {0.0f, 1.0f};
    Plane->uvData[1] = {0.0f, 0.0f};
    Plane->uvData[2] = {1.0f, 0.0f};
    Plane->uvData[3] = {1.0f, 1.0f};

    Plane->uvIndex[0] = 1;
    Plane->uvIndex[1] = 2;
    Plane->uvIndex[2] = 3;
    Plane->uvIndex[3] = 1;
    Plane->uvIndex[4] = 3;
    Plane->uvIndex[5] = 4;

    Plane->NormalData[0] = {0, 0, 1.0f};

    Plane->NormalIndex[0] = 1;
    Plane->NormalIndex[1] = 1;
    Plane->NormalIndex[2] = 1;
    Plane->NormalIndex[3] = 1;
    Plane->NormalIndex[4] = 1;
    Plane->NormalIndex[5] = 1;
}

void gl_createCubeGeometry(GlGeometry3D *Cube)
{
    if (!Cube)
        return;

    Cube->faceCount = 6 * 2;
    Cube->vertexCount = 8;

    u32 vertices = Cube->faceCount * GL_TRIANGLES * gcSize(u32);
}

// void gl_createBasePrimitives(gl_state_t *GL)
// {
//     GlGeometry3D *triangle = &GL->BasePrimitives[GL_GEOMETRY_TRIANGLE];
//     gl_createTriangleGeometry(triangle);

//     GlGeometry3D *Plane = &GL->BasePrimitives[GL_GEOMETRY_PLANE];
//     gl_createPlaneGeometry(Plane);
// }

GlAnimatedGrid3D *gl_createAnimatedGrid3D(u32 rows, u32 cols, vec3 pos, r32 dist, memory_type_t location)
{
    SDL_assert(rows > 1 && cols > 1);

    u32 vertexCount = rows * cols;
    u32 totalLines = 2 * vertexCount - rows - cols;
    size_t positionDataBytes = vertexCount * gcSize(vec3);
    size_t positionIndexBytes = 2 * totalLines * gcSize(u32);
    size_t totalBlockBytes = gcSize(GlAnimatedGrid3D) + positionDataBytes + positionIndexBytes;

    GlAnimatedGrid3D *Grid = (GlAnimatedGrid3D *) gc_mem_allocate(location, totalBlockBytes);
    vec3 *Grid_PositionData = (vec3 *) ((u8 *) Grid + gcSize(GlAnimatedGrid3D));
    u32 *Grid_PositionIndex = (u32 *) ((u8 *) Grid_PositionData + positionDataBytes);

    GlRenderObject3D *GridObject = &Grid->Object;

    Grid->rows = rows;
    Grid->cols = cols;
    Grid->dist = dist;
    GridObject->position = pos;
    GridObject->scaling = {1, 1, 1};

    // GridObject->Structure->type = GL_LINES;
    // GridObject->Structure->vertices = 2 * totalLines;

    // -- The vertex position is computed relative to the grid's center.
    // ----------------------------------------------------------------------------------

    r32 rowLineLength = (cols - 1) * dist;
    r32 colLineLength = (rows - 1) * dist;
    vec3 centerOffset = {rowLineLength / 2, colLineLength / 2, 0};
    vec3 topCorner = vec3_add(pos, {-centerOffset.x, centerOffset.y, 0});

    Grid->Wave.topCorner = topCorner;

    Grid->Wave.x_stepsPerInterval = 10;
    Grid->Wave.x_normalizer = 1 / (Grid->Wave.x_stepsPerInterval * dist);
    Grid->Wave.x_radianInterval = (r32) (2 * PI);
    Grid->Wave.x_maxHeight = 0.05f;

    Grid->Wave.y_stepsPerInterval = 10;
    Grid->Wave.y_normalizer = 1 / (Grid->Wave.y_stepsPerInterval * dist);
    Grid->Wave.y_radianInterval = (r32) (2 * PI);
    Grid->Wave.y_maxHeight = 0.05f;

    Grid->Object.uniforms = &Grid->Wave;

    u32 dataOffset = 0;
    u32 indexOffset = 0;

    u32 *IndexVertexA = 0;
    u32 *IndexVertexB = 0;

    // ----------------------------------------------------------------------------------
    // -- Generating the vertices / they will be generated sequentially from left to
    // -- right, up to down.
    // ----------------------------------------------------------------------------------

    r32 tx = topCorner.x;
    r32 ty = topCorner.y;

    for (u32 i = 0; i < vertexCount; ++i)
    {
        // new line.
        if (i > 0 && i % cols == 0)
        {
            tx = topCorner.x;
            ty -= dist;
        }

        vec3 *Vertex = &Grid_PositionData[i];

        Vertex->x = tx;
        Vertex->y = ty;

        tx += dist;
    }

    // -- Indices. I will add the lines in pairs of two, a horizontal line and a
    // -- vertical line both starting from the same vertex, processing 3 vertices
    // -- if possible.
    // ----------------------------------------------------------------------------------

    for (u32 row = 0; row < rows; ++row)
    {
        for (u32 col = 0; col < cols; ++col)
        {
            u32 startIndex = row * cols + col + 1;

            // horizontal line.
            if (col < cols - 1)
            {
                u32 nextIndex = startIndex + 1;

                u32 *CurrentIndexPtr = &Grid_PositionIndex[indexOffset++];
                u32 *NextIndexPtr = &Grid_PositionIndex[indexOffset++];

                *CurrentIndexPtr = startIndex;
                *NextIndexPtr = nextIndex;
            }

            // vertical line.
            if (row < rows - 1)
            {
                u32 nextIndex = startIndex + cols;

                u32 *CurrentIndexPtr = &Grid_PositionIndex[indexOffset++];
                u32 *NextIndexPtr = &Grid_PositionIndex[indexOffset++];

                *CurrentIndexPtr = startIndex;
                *NextIndexPtr = nextIndex;
            }
        }
    }

    // gl_addAttribute(GridObject, 3, Grid_PositionIndex, (r32 *) Grid_PositionData);

    return Grid;
}

/**
 * Initializes the specified grid with default values.
 */
void gl_GeneralGrid3D_default(GlGeneralGrid3D *Grid)
{
    if (Grid)
    {
        Grid->rows = 20;
        Grid->cols = 20;
        Grid->dist = 0.3f;
        Grid->subdivision = 5;
        Grid->drawAxis = true;

        Grid->Object.position = {0, 0, 0};
        Grid->Object.scaling = {1, 1, 1};
        // Grid->Object.rotation = {-70, 0, 0};

        Grid->gridColor = {0.7f, 0.7f, 0.7f, 1.0f};
        Grid->subdivisionColor = {0.3f, 0.3f, 0.3f, 1.0f};
        Grid->xAxisColor = {1.0f, 0.0f, 0.0f, 1.0f};
        Grid->yAxisColor = {0.0f, 1.0f, 0.0f, 1.0f};
        Grid->zAxisColor = {0.0f, 0.0f, 1.0f, 1.0f};
    }
}

#if 0
/**
 * Initializes a given GlGeneralGrid3D structure.
 */
void gl_GeneralGrid3D_init(GlGeneralGrid3D *Grid)
{
    if (Grid)
    {
        SDL_assert(Grid->rows >= 1 && Grid->cols >= 1);

        u32 totalRows = 2 * Grid->rows + 1;
        u32 totalCols = 2 * Grid->cols + 1;
        u32 totalLines = totalRows + totalCols;

        if (Grid->drawAxis)
            totalLines++;   // add the z axis also.

        u32 vertices = GL_LINES * totalLines;

        // -- Setting up the structure.

        // GlVertexStructure *Structure = &Grid->Structure;
        // Grid->Object.Structure = Structure;

        // Structure->type = GL_LINES;
        // Structure->vertices = vertices;

        // -- Reset the attributes.

        for (u32 i = 0; i < Structure->attributeCount; ++i)
        {
            GlAttribute *Attribute = &Structure->Attributes[i];

            Attribute->isExtra = false;
            Attribute->size = 0;
            Attribute->offset = 0;
            Attribute->DataBuffer = 0;
            Attribute->IndexBuffer = 0;
        }

        Structure->attributeCount = 0;
        Structure->vertexPitch = 0;
        Structure->count = 0;

        // -- If memory was allocated then release it.

        if (Structure->memory)
        {
            gc_mem_free(Structure->memory);
            Structure->memory = 0;
        }

        // -- Allocate memory for the grid structure (position/color).

        // TODO(gabic): am uitat sa aloc memorie si pentru axe.

        size_t positionDataBytes = (vertices - 4) * gcSize(vec3);
        size_t positionIndexBytes = vertices * gcSize(u32);
        size_t colorDataBytes = 5 * gcSize(vec4);
        size_t colorIndexBytes = vertices * gcSize(u32);

        size_t totalBlockBytes = positionDataBytes + positionIndexBytes + colorDataBytes + colorIndexBytes;

        Structure->memory = gc_mem_allocate(MEMORY_TEMPORARY, totalBlockBytes);

        r32 *PositionDataBuffer = (r32 *) Structure->memory;
        u32 *PositionIndexBuffer = (u32 *) ((u8 *) PositionDataBuffer + positionDataBytes);

        r32 *ColorDataBuffer = (r32 *) ((u8 *) PositionIndexBuffer + positionIndexBytes);
        u32 *ColorIndexBuffer = (u32 *) ((u8 *) ColorDataBuffer + colorDataBytes);

        vec3 *DestPositionBuffer = (vec3 *) PositionDataBuffer;

        // -- Color data setup.
        // ----------------------------------------------------------------------------------

        vec4 *DestColorBuffer = (vec4 *) ColorDataBuffer;

        *DestColorBuffer++ = Grid->gridColor;
        *DestColorBuffer++ = Grid->subdivisionColor;
        *DestColorBuffer++ = Grid->xAxisColor;
        *DestColorBuffer++ = Grid->yAxisColor;
        *DestColorBuffer++ = Grid->zAxisColor;

        // -- The vertex position is computed relative to the grid's center.
        // ----------------------------------------------------------------------------------

        vec3 pos = Grid->Object.position;

        r32 rowLineLength = (totalCols - 1) * Grid->dist;
        r32 colLineLength = (totalRows - 1) * Grid->dist;
        vec3 centerOffset = {rowLineLength / 2, colLineLength / 2, 0};
        vec3 topCorner = vec3_add(pos, {-centerOffset.x, centerOffset.y, 0});
        vec3 bottomCorner = vec3_add(pos, {centerOffset.x, -centerOffset.y, 0});

        u32 dataOffset = 0;
        u32 indexOffset = 0;
        u32 colorIndexOffset = 0;

        u32 topLeftCornerIndex = 0;
        u32 topRightCornerIndex = 0;
        u32 bottomLeftCornerIndex = 0;
        u32 bottomRightCornerIndex = 0;

        u32 rowsMiddleIndex = Grid->rows;
        u32 colsMiddleIndex = Grid->cols;

        vec3 *VertexA = 0;
        vec3 *VertexB = 0;

        u32 *IndexVertexA = 0;
        u32 *IndexVertexB = 0;
        u32 subdivisionCheck = Grid->subdivision + 1;

        // Rows.
        // ----------------------------------------------------------------------------------

        for (u32 i = 0; i < totalRows; ++i)
        {
            // -- Computed index which is referenced to the middle row line.
            // -- The middle row line is considered to be at index 0 and
            // -- on either side the index grows by 1.
            // -- Used at determining the line color based on the subdivision.

            u32 colorRowIndex = i - rowsMiddleIndex;

            if (i < rowsMiddleIndex)
                colorRowIndex = rowsMiddleIndex - i;

            // Normal color.
            if (colorRowIndex % subdivisionCheck == 0)
            {
                ColorIndexBuffer[colorIndexOffset++] = 1;
                ColorIndexBuffer[colorIndexOffset++] = 1;
            }
            // Subdivision color.
            else
            {
                ColorIndexBuffer[colorIndexOffset++] = 2;
                ColorIndexBuffer[colorIndexOffset++] = 2;
            }

            // x axis color.
            if (Grid->drawAxis && (i == rowsMiddleIndex))
            {
                colorIndexOffset--;
                colorIndexOffset--;

                ColorIndexBuffer[colorIndexOffset++] = 3;
                ColorIndexBuffer[colorIndexOffset++] = 3;
            }

            // -- Setting the line vertex positions in the buffer.

            VertexA = &DestPositionBuffer[dataOffset++];
            VertexB = &DestPositionBuffer[dataOffset++];

            IndexVertexA = &PositionIndexBuffer[indexOffset++];
            IndexVertexB = &PositionIndexBuffer[indexOffset++];

            *VertexA = topCorner;
            VertexA->y -= i * Grid->dist;

            *VertexB = *VertexA;
            VertexB->x += rowLineLength;

            *IndexVertexA = dataOffset - 1;
            *IndexVertexB = dataOffset;

            if (i == 0)
            {
                topLeftCornerIndex = dataOffset - 1;
                topRightCornerIndex = dataOffset;
            }

            if (i == totalRows - 1)
            {
                bottomLeftCornerIndex = dataOffset - 1;
                bottomRightCornerIndex = dataOffset;
            }
        }

        // Cols.
        // ----------------------------------------------------------------------------------

        for (u32 i = 0; i < totalCols; ++i)
        {
            u32 colorColIndex = i - colsMiddleIndex;

            if (i < colsMiddleIndex)
                colorColIndex = colsMiddleIndex - i;

            // Normal color.
            if (colorColIndex % subdivisionCheck == 0)
            {
                ColorIndexBuffer[colorIndexOffset++] = 1;
                ColorIndexBuffer[colorIndexOffset++] = 1;
            }
            // Subdivision color.
            else
            {
                ColorIndexBuffer[colorIndexOffset++] = 2;
                ColorIndexBuffer[colorIndexOffset++] = 2;
            }

            // y axis color.
            if (Grid->drawAxis && (i == colsMiddleIndex))
            {
                colorIndexOffset--;
                colorIndexOffset--;

                ColorIndexBuffer[colorIndexOffset++] = 4;
                ColorIndexBuffer[colorIndexOffset++] = 4;
            }

            // -- The left and right edge columns will use vertices defined
            // -- at the row stage above.

            if (i == 0)
            {
                IndexVertexA = &PositionIndexBuffer[indexOffset++];
                IndexVertexB = &PositionIndexBuffer[indexOffset++];

                *IndexVertexA = topLeftCornerIndex;
                *IndexVertexB = bottomLeftCornerIndex;
            }
            else if (i == totalCols - 1)
            {
                IndexVertexA = &PositionIndexBuffer[indexOffset++];
                IndexVertexB = &PositionIndexBuffer[indexOffset++];

                *IndexVertexA = topRightCornerIndex;
                *IndexVertexB = bottomRightCornerIndex;
            }
            else
            {
                // -- Setting the line vertex positions in the buffer.

                VertexA = &DestPositionBuffer[dataOffset++];
                VertexB = &DestPositionBuffer[dataOffset++];

                IndexVertexA = &PositionIndexBuffer[indexOffset++];
                IndexVertexB = &PositionIndexBuffer[indexOffset++];

                *VertexA = topCorner;
                VertexA->x += i * Grid->dist;

                *VertexB = *VertexA;
                VertexB->y -= colLineLength;

                *IndexVertexA = dataOffset - 1;
                *IndexVertexB = dataOffset;
            }
        }

        // -- The z axis.

        if (Grid->drawAxis)
        {
            VertexA = &DestPositionBuffer[dataOffset++];
            VertexB = &DestPositionBuffer[dataOffset++];

            IndexVertexA = &PositionIndexBuffer[indexOffset++];
            IndexVertexB = &PositionIndexBuffer[indexOffset++];

            *VertexA = vec3_add(pos, {0, 0, rowLineLength / 2});
            *VertexB = vec3_sub(pos, {0, 0, rowLineLength / 2});

            *IndexVertexA = dataOffset - 1;
            *IndexVertexB = dataOffset;

            ColorIndexBuffer[colorIndexOffset++] = 5;
            ColorIndexBuffer[colorIndexOffset++] = 5;
        }

        gl_addAttribute(Structure, 3, PositionIndexBuffer, PositionDataBuffer);
        gl_addAttribute(Structure, 4, ColorIndexBuffer, ColorDataBuffer);
    }
}
#endif