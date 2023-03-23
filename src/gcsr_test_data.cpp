// ----------------------------------------------------------------------------------
// -- File: gcsr_test_data.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: Test data initialization.
// -- Created: 2020-11-07 19:54:16
// -- Modified: 2020-11-11 23:13:24
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

// ----------------------------------------------------------------------------------

void initObjectLibrary2D(engine_state_t *State)
{
    gl_state_t *GL = &State->GL;

    u32 screenWidth = 0;
    u32 screenHeight = 0;

    AssetBitmapMemory *Bitmaps = (AssetBitmapMemory *) (State->Bitmaps + 1);

    // AssetBitmapMemory *WidthCheckBitmap = &Bitmaps[BITMAP_WIDTH_CHECK];
    // AssetBitmapMemory *MusashiBitmap = &Bitmaps[BITMAP_MUSASHI];
    // AssetBitmapMemory *DudeBitmap = &Bitmaps[BITMAP_DUDE];
    // AssetBitmapMemory *BackgroundBitmap = &Bitmaps[BITMAP_RENEGADE_BACKGROUND];
    // AssetBitmapMemory *CircleBitmap = &Bitmaps[BITMAP_CIRCLE];
    // AssetBitmapMemory *GradientCheckBitmap = &Bitmaps[BITMAP_GRADIENT_CHECK];
    // AssetBitmapMemory *FilteringCheckBitmap = &Bitmaps[BITMAP_FILTERING_CHECK];
    // AssetBitmapMemory *FilteringCheck2Bitmap = &Bitmaps[BITMAP_FILTERING_CHECK_2];
    // AssetBitmapMemory *BoundaryCheckBitmap = &Bitmaps[BITMAP_BOUNDARY_CHECK];
    // AssetBitmapMemory *FillCheckBitmap = &Bitmaps[BITMAP_FILL_CHECK];
    // AssetBitmapMemory *ArmyOfDarknessBitmap = &Bitmaps[BITMAP_ARMY_OF_DARKNESS];

    AssetBitmapMemory *WidthCheckBitmap = 0;
    AssetBitmapMemory *MusashiBitmap = 0;
    AssetBitmapMemory *DudeBitmap = 0;
    AssetBitmapMemory *BackgroundBitmap = 0;
    AssetBitmapMemory *CircleBitmap = 0;
    AssetBitmapMemory *GradientCheckBitmap = 0;
    AssetBitmapMemory *FilteringCheckBitmap = 0;
    AssetBitmapMemory *FilteringCheck2Bitmap = 0;
    AssetBitmapMemory *BoundaryCheckBitmap = 0;
    AssetBitmapMemory *FillCheckBitmap = 0;
    AssetBitmapMemory *ArmyOfDarknessBitmap = 0;

    // ----------------------------------------------------------------------------------

    GlRenderObject2D *triangle = gl_createTriangle2D();
    GlRenderObject2D *Rectangle = gl_createRectangle2D();

    triangle->Transformation.scaling.x = 1;
    triangle->Transformation.scaling.y = 1;
    triangle->Transformation.translation.x = 400;
    triangle->Transformation.translation.y = 400;

    triangle->Transformation.pointRotationSpeed = (r32) PI / 100;
    triangle->Transformation.rotationSpeed = (r32) PI / 80;

    Rectangle->Transformation.scaling.x = 2.5;
    Rectangle->Transformation.scaling.y = 2.5;
    Rectangle->Transformation.translation.x = 450;
    Rectangle->Transformation.translation.y = 450;

    triangle->color = colorToPremultipliedLinear({255, 0, 0, 155});
    Rectangle->color = colorToPremultipliedLinear({100, 0, 255, 200});

    vec2 *tri_p1 = (vec2 *) ta_index(triangle->Points, 0);
    vec2 *tri_p2 = (vec2 *) ta_index(triangle->Points, 1);
    vec2 *tri_p3 = (vec2 *) ta_index(triangle->Points, 2);

    tri_p1->x = 0;
    tri_p1->y = -100;
    tri_p2->x = -100;
    tri_p2->y = 100;
    tri_p3->x = 100;
    tri_p3->y = 100;

    vec2 *rect_p1 = (vec2 *) ta_index(Rectangle->Points, 0);
    vec2 *rect_p2 = (vec2 *) ta_index(Rectangle->Points, 1);
    vec2 *rect_p3 = (vec2 *) ta_index(Rectangle->Points, 2);
    vec2 *rect_p4 = (vec2 *) ta_index(Rectangle->Points, 3);

    rect_p1->x = -50;
    rect_p1->y = -50;
    rect_p2->x = 50;
    rect_p2->y = -50;
    rect_p3->x = 50;
    rect_p3->y = 50;
    rect_p4->x = -50;
    rect_p4->y = 50;

    State->ObjectLibrary2D[OBJ_TRIANGLE] = triangle;
    State->ObjectLibrary2D[OBJ_RECTANGLE] = Rectangle;

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_WIDTH_CHECK] =
            gl_createImage2D(WidthCheckBitmap, {50, 50}, {0.5f, 0.5f});

    GlRenderObject2D *WidthCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_WIDTH_CHECK];
    WidthCheckObject->Transformation.rotation = 0;
    WidthCheckObject->Transformation.scaling = {10, 10};

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_RENEGADE_BACKGROUND] =
            gl_createImage2D(BackgroundBitmap, {500, 500}, {0.5f, 0.5f});

    GlRenderObject2D *BackgroundObject = State->ObjectLibrary2D[OBJ_BITMAP_RENEGADE_BACKGROUND];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_MUSASHI] =
            gl_createImage2D(MusashiBitmap, {300, 400}, {0.5f, 0.5f});

    GlRenderObject2D *MusashiObject = State->ObjectLibrary2D[OBJ_BITMAP_MUSASHI];
    MusashiObject->debugOutline = true;
    MusashiObject->color = {1.0f , 1.0f, 1.0f, 1.0f};
    MusashiObject->Transformation.scaling = {4, 4};

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_FILL_CHECK] =
            gl_createImage2D(FillCheckBitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0, 0});

    GlRenderObject2D *FillCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_FILL_CHECK];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_BOUNDARY_CHECK] =
            gl_createImage2D(BoundaryCheckBitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0, 0});

    GlRenderObject2D *BoundaryCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_BOUNDARY_CHECK];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_FILTERING_CHECK] =
            gl_createImage2D(FilteringCheckBitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0, 0});

    GlRenderObject2D *FilteringCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_FILTERING_CHECK];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_FILTERING_CHECK_2] =
            gl_createImage2D(FilteringCheck2Bitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0, 0});

    GlRenderObject2D *Filtering2CheckObject = State->ObjectLibrary2D[OBJ_BITMAP_FILTERING_CHECK_2];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_GRADIENT_CHECK] =
            gl_createImage2D(GradientCheckBitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0, 0});

    GlRenderObject2D *GradientCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_GRADIENT_CHECK];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_CIRCLE] =
            gl_createImage2D(CircleBitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0.5f, 0.5f});

    GlRenderObject2D *CircleObject = State->ObjectLibrary2D[OBJ_BITMAP_CIRCLE];

    // ----------------------------------------------------------------------------------

    State->ObjectLibrary2D[OBJ_BITMAP_ARMY_OF_DARKNESS] =
            gl_createImage2D(ArmyOfDarknessBitmap,
            {screenWidth * 0.5f, screenHeight * 0.5f}, {0.5f, 0.5f});

    GlRenderObject2D *ArmyOfDarknessObject = State->ObjectLibrary2D[OBJ_BITMAP_ARMY_OF_DARKNESS];
    ArmyOfDarknessObject->Transformation.scaling = {5, 5};
    ArmyOfDarknessObject->Transformation.rotationSpeed = (r32) PI / 50;
    ArmyOfDarknessObject->Transformation.pointRotationSpeed = (r32) PI / 100;

    // ----------------------------------------------------------------------------------
}

void updateObjectLibrary2D(engine_state_t *State)
{
    GlRenderObject2D *TriangleObject = State->ObjectLibrary2D[OBJ_TRIANGLE];

    TriangleObject->Transformation.rotation += TriangleObject->Transformation.rotationSpeed;
    TriangleObject->Transformation.pointRotation += TriangleObject->Transformation.pointRotationSpeed;
    TriangleObject->Transformation.rotationPoint.x = 600;
    TriangleObject->Transformation.rotationPoint.y = 300;

    GlRenderObject2D *RectangleObject = State->ObjectLibrary2D[OBJ_RECTANGLE];

    RectangleObject->Transformation.rotation += RectangleObject->Transformation.rotationSpeed;
    RectangleObject->Transformation.pointRotation += RectangleObject->Transformation.pointRotationSpeed;
    RectangleObject->Transformation.rotationPoint.x = 500;
    RectangleObject->Transformation.rotationPoint.y = 600;

    GlRenderObject2D *WidthCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_WIDTH_CHECK];
    // WidthCheckObject->Transformation.R = angle1;

    GlRenderObject2D *BackgroundObject = State->ObjectLibrary2D[OBJ_BITMAP_RENEGADE_BACKGROUND];
    BackgroundObject->Transformation.rotation += BackgroundObject->Transformation.rotationSpeed;
    BackgroundObject->Transformation.scaling.x = 2.0f;
    BackgroundObject->Transformation.scaling.y = 2.0f;

    GlRenderObject2D *ArmyOfDarknessObject = State->ObjectLibrary2D[OBJ_BITMAP_ARMY_OF_DARKNESS];
    // ArmyOfDarknessObject->Transformation.scaling.x = 2;
    // ArmyOfDarknessObject->Transformation.scaling.y = 2;
    // ArmyOfDarknessObject->Transformation.translation.x = 200;
    ArmyOfDarknessObject->Transformation.rotation -= ArmyOfDarknessObject->Transformation.rotationSpeed;

#if 1
    if (State->direction == 0)
    {
        if (ArmyOfDarknessObject->Transformation.scaling.x < 10)
        {
            ArmyOfDarknessObject->Transformation.scaling.x += 0.05f;
            ArmyOfDarknessObject->Transformation.scaling.y += 0.05f;
        }
        else
            State->direction = 1;
    }
    else
    {
        if (ArmyOfDarknessObject->Transformation.scaling.x > 0.5f)
        {
            ArmyOfDarknessObject->Transformation.scaling.x -= 0.05f;
            ArmyOfDarknessObject->Transformation.scaling.y -= 0.05f;
        }
        else
            State->direction = 0;
    }
#endif

    GlRenderObject2D *MusashiObject = State->ObjectLibrary2D[OBJ_BITMAP_MUSASHI];
    MusashiObject->Transformation.rotation -= MusashiObject->Transformation.rotationSpeed;
}

void initObjectLibrary3D(engine_state_t *State)
{
    gl_program_t *GeneralProgram = &State->Programs[0];
    gl_state_t *GL = &State->GL;

    // ----------------------------------------------------------------------------------
    // NOTE(gabic): la un moment dat va trebui sa aloc dinamic obiectele 3d.
    // ----------------------------------------------------------------------------------

    State->ObjectMap = tht_create(MEMORY_TEMPORARY, MAX_3D_OBJECTS);
    Type_Hashtable *Models = State->ObjectMap;

    GlVertexStructure *Structure = 0;
    GlVertexDataSource *DataSource = 0;
    Type_HashtableElement *Element = 0;
    GlMaterialOld *Material = 0;

    // -- Z buffer test object.
    // ----------------------------------------------------------------------------------

    createObject(State, "z_buffer_test_object", "z_buffer_test_data");
    GlRenderObject3D *ZBufferTestObject = getObject(State, "z_buffer_test_object");
    Material = &ZBufferTestObject->Material;
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_TEX_Z_BUFFER_TEST_OBJECT);

    // -- Witch object.
    // ----------------------------------------------------------------------------------

    createObject(State, "witch_object", "witch_data");
    GlRenderObject3D *WitchObject = getObject(State, "witch_object");
    Material = &WitchObject->Material;
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_WITCH_DIFFUSE_TEXTURE);

    // -- Wall model (x2).
    // ----------------------------------------------------------------------------------

    createObject(State, "wall", "wall_data");
    GlRenderObject3D *WallObject = getObject(State, "wall");

    createObject(State, "wall_red", "wall_data");
    GlRenderObject3D *WallRedObject = getObject(State, "wall_red");

    createObject(State, "wall_blue", "wall_data");
    GlRenderObject3D *WallBlueObject = getObject(State, "wall_blue");

    createObject(State, "testcube", "testcube_data");
    GlRenderObject3D *TestcubeObject = getObject(State, "testcube");

    WallObject->position = {0, 0, 0};
    WallObject->scaling = {1, 1, 1};
    WallObject->rotation = {0, 0, 0};

    // Element = tht_search(State->DataSourceMap, "wall_data");
    // DataSource = (GlVertexDataSource *) tht_data(Element);

    // GlRenderObject3D *WallModel = (GlRenderObject3D *) tht_insert(Models, "wall", gcSize(GlRenderObject3D));
    // GlRenderObject3D *WallModel2 = (GlRenderObject3D *) tht_insert(Models, "wall2", gcSize(GlRenderObject3D));
    // GlRenderObject3D *WallModel3 = (GlRenderObject3D *) tht_insert(Models, "wall3", gcSize(GlRenderObject3D));

    // WallModel2->type = GL_TRIANGLES;
    // WallModel2->DataSource = DataSource;

    WallRedObject->position = {1, 0.5f, -0.4f};
    WallRedObject->scaling = {2, 2, 2};
    WallRedObject->rotation = {0, 0, 0};

    // WallModel3->type = GL_TRIANGLES;
    // WallModel3->DataSource = DataSource;

    WallBlueObject->position = {0.5f, 0.7f, -0.2f};
    WallBlueObject->scaling = {2, 2, 2};
    WallBlueObject->rotation = {0, 0, 0};

    // ----------------------------------------------------------------------------------
    // -- Wall material.
    // ----------------------------------------------------------------------------------

    // WallObject->Material.baseColor = {1, 1, 1, 1};
    // WallObject->Material.emission = {0.0f, 0.0f, 0.0f, 0};
    // // WallObject->Material.ambient = {0.05f, 0.05f, 0.05f, 0};
    // WallObject->Material.ambient = {0.2f, 0.2f, 0.2f, 0};
    // // WallObject->Material.diffuse = {1.0f, 1.0f, 1.0f, 0};
    // WallObject->Material.diffuse = {0.5f, 0.5f, 0.5f, 0};
    // WallObject->Material.specular = {0.4f, 0.4f, 0.4f, 0};
    // WallObject->Material.shine = 70;

    AssetBitmapMemory *WallDiffuseBitmap = gl_getBitmap(State, BITMAP_WALL_TEXTURE);
    AssetBitmapMemory *WallRedDiffuseBitmap = gl_getBitmap(State, BITMAP_WALL_TEXTURE_RED);
    AssetBitmapMemory *WallBlueDiffuseBitmap = gl_getBitmap(State, BITMAP_WALL_TEXTURE_BLUE);
    AssetBitmapMemory *TestcubeDiffuseBitmap = gl_getBitmap(State, BITMAP_TESTCUBE_TEXTURE);

    Material = &WallObject->Material;
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_WALL_TEXTURE);

    Material = &WallRedObject->Material;
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_WALL_TEXTURE_RED);

    Material = &WallBlueObject->Material;
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_WALL_TEXTURE_BLUE);

    Material = &TestcubeObject->Material;
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_TESTCUBE_TEXTURE);

    // ----------------------------------------------------------------------------------
    // -- African head model.
    // ----------------------------------------------------------------------------------

    createObject(State, "african_head", "african_head_data");
    GlRenderObject3D *AfricanHeadModel = getObject(State, "african_head");

    // Element = tht_search(State->DataSourceMap, "african_head_data");
    // DataSource = (GlVertexDataSource *) tht_data(Element);

    // GlRenderObject3D *AfricanHeadModel = (GlRenderObject3D *) tht_insert(Models, "african_head", gcSize(GlRenderObject3D));

    AfricanHeadModel->position = {0, -1, 1};
    AfricanHeadModel->scaling = {2, 2, 2};
    // AfricanHeadModel->rotation = {90, 0, 0};

    // AfricanHeadModel->Material.baseColor = {1, 1, 1, 1};
    // AfricanHeadModel->Material.emission = {0.0f, 0.0f, 0.0f, 0};
    // // AfricanHeadModel->Material.ambient = {0.05f, 0.05f, 0.05f, 0};
    // AfricanHeadModel->Material.ambient = {0.2f, 0.2f, 0.2f, 0};
    // // AfricanHeadModel->Material.diffuse = {1.0f, 1.0f, 1.0f, 0};
    // AfricanHeadModel->Material.diffuse = {0.5f, 0.5f, 0.5f, 0};
    // AfricanHeadModel->Material.specular = {0.4f, 0.4f, 0.4f, 0};
    // AfricanHeadModel->Material.shine = 70;

    Material = &AfricanHeadModel->Material;
    // Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_HEAD_DIFFUSE_TEXTURE_128);
    Material->TEXTURE_BUFFER[0] = gl_getBitmap(State, BITMAP_HEAD_DIFFUSE_TEXTURE);

    // AssetBitmapMemory *HeadDiffuseBitmap = gl_getBitmap(State, BITMAP_HEAD_DIFFUSE_TEXTURE);
    // AssetBitmapMemory *HeadSpecularBitmap = gl_getBitmap(State, BITMAP_HEAD_SPECULAR_TEXTURE);
    // AssetBitmapMemory *ArmyOfDarknessBitmap = gl_getBitmap(State, BITMAP_ARMY_OF_DARKNESS);
    // AfricanHeadModel->Material.diffuseTex = HeadDiffuseBitmap;
    // AfricanHeadModel->Material.specularTex = HeadSpecularBitmap;
    // AfricanHeadModel->Material.specularTex = HeadDiffuseBitmap;
    // AfricanHeadModel->Material.diffuseTex = ArmyOfDarknessBitmap;

    // GeneralUniform *AfricanHeadUniforms = (GeneralUniform *) gc_mem_allocate(MEMORY_PERMANENT, gcSize(GeneralUniform), 0);
    // AfricanHeadUniforms->color = {0.7f, 0, 0, 1};
    // AfricanHeadModel->uniforms = AfricanHeadUniforms;

    // ----------------------------------------------------------------------------------
    // -- African head flat model.
    // ----------------------------------------------------------------------------------

    Structure = (GlVertexStructure *) tht_data(tht_search(State->StructureMap, "african_head_flat_structure"));

    GlRenderObject3D *AfricanHeadFlatModel = (GlRenderObject3D *) tht_insert(Models, "african_head_flat", gcSize(GlRenderObject3D));
    // AfricanHeadFlatModel->Structure = Structure;

    AfricanHeadFlatModel->position = {0, 0, -2};
    AfricanHeadFlatModel->scaling = {1, 1, 1};
    AfricanHeadFlatModel->rotation = {0, 0, 0};
    // AfricanHeadFlatModel->rotation.x = -90;

    // GeneralUniform *AfricanHeadFlatUniforms = (GeneralUniform *) gc_mem_allocate(MEMORY_PERMANENT, gcSize(GeneralUniform));
    // AfricanHeadFlatUniforms->color = {0.7f, 0, 0, 1};
    // AfricanHeadFlatModel->uniforms = AfricanHeadFlatUniforms;

    // ----------------------------------------------------------------------------------
    // -- Chair model.
    // ----------------------------------------------------------------------------------

    AssetObjectMemory *ChairObjectAsset = (AssetObjectMemory *) ta_index(State->Objects, MESH_CHAIR);
    GlRenderObject3D *ChairHeadModel = (GlRenderObject3D *) tht_insert(Models, "chair", gcSize(GlRenderObject3D));
    // ChairHeadModel->Program = &State->Programs[PRG_GENERAL_FLAT];

    ChairHeadModel->position = {0, 1, -1};
    ChairHeadModel->scaling = {0.6f, 0.6f, 0.6f};
    ChairHeadModel->rotation = {-90, 0, 0};

    // ChairHeadModel->Structure->type = GL_TRIANGLES;
    // ChairHeadModel->Structure->vertices = ChairObjectAsset->ObjectInfo.faceCount * 3;

    // gl_addAttribute(ChairHeadModel, 3, ChairObjectAsset->FaceVertices, ChairObjectAsset->Vertices);
    // gl_addAttribute(ChairHeadModel, 2, ChairObjectAsset->FaceUVs, ChairObjectAsset->UVs);
    // gl_addAttribute(ChairHeadModel, 3, ChairObjectAsset->FaceNormals, ChairObjectAsset->Normals);
    // gl_addExtraAttribute(ChairHeadModel, 4);

    // GeneralUniform *ChairUniforms = (GeneralUniform *) gc_mem_allocate(MEMORY_PERMANENT, gcSize(GeneralUniform), 0);
    // ChairUniforms->color = {0, 0.2f, 0.5f, 1};
    // ChairHeadModel->uniforms = ChairUniforms;

    // ----------------------------------------------------------------------------------
    // -- Test triangle model.
    // ----------------------------------------------------------------------------------

    // GlRenderObject3D *TestTriangleModel = (GlRenderObject3D *) tht_insert(Models, "test_triangle", gcSize(GlRenderObject3D));

    // GlGeometry3D *TriangleGeometry = &GL->BasePrimitives[GL_GEOMETRY_TRIANGLE];

    // TestTriangleModel->position = {0, 0, 0};
    // TestTriangleModel->scaling = {1, 1, 1};

    // TestTriangleModel->Structure->type = GL_TRIANGLES;
    // TestTriangleModel->Structure->vertices = TriangleGeometry->faceCount * GL_TRIANGLES;

    // vec4 *TriangleColorBuffer = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec4), 0);
    // u32 *TriangleIndexColorBuffer = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // TriangleIndexColorBuffer[0] = 1;
    // TriangleIndexColorBuffer[1] = 2;
    // TriangleIndexColorBuffer[2] = 3;

    // TriangleColorBuffer[0] = colorToPremultipliedLinear({255, 0, 0, 255});
    // TriangleColorBuffer[1] = colorToPremultipliedLinear({0, 255, 0, 255});
    // TriangleColorBuffer[2] = colorToPremultipliedLinear({0, 0, 255, 255});

    // // [position, uv, normal, color, world_position, world_normal]
    // gl_addAttribute(TestTriangleModel, 3, TriangleGeometry->PositionIndex, (r32 *) TriangleGeometry->PositionData);
    // gl_addAttribute(TestTriangleModel, 2, TriangleGeometry->uvIndex, (r32 *) TriangleGeometry->uvData);
    // gl_addAttribute(TestTriangleModel, 3, TriangleGeometry->NormalIndex, (r32 *) TriangleGeometry->NormalData);
    // gl_addAttribute(TestTriangleModel, 4, TriangleIndexColorBuffer, (r32 *) TriangleColorBuffer);
    // gl_addExtraAttribute(TestTriangleModel, 3);
    // gl_addExtraAttribute(TestTriangleModel, 3);

    // ----------------------------------------------------------------------------------
    // -- Test plane model.
    // ----------------------------------------------------------------------------------

    // GlRenderObject3D *TestPlaneModel = (GlRenderObject3D *) tht_insert(Models, "test_plane", gcSize(GlRenderObject3D));

    // GlGeometry3D *PlaneGeometry = &GL->BasePrimitives[GL_GEOMETRY_PLANE];

    // TestPlaneModel->position = {0, 0, 0};
    // TestPlaneModel->scaling = {1, 1, 1};

//     TestPlaneModel->Structure.type = GL_TRIANGLES;
//     TestPlaneModel->Structure.vertices = PlaneGeometry->faceCount * GL_TRIANGLES;

//     // TestPlaneModel->rotation.x = 40;

//     u32 *PlaneIndexColorBuffer = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, TestPlaneModel->Structure.vertices * gcSize(u32), 0);

// #if 0
//     vec4 *PlaneColorBuffer = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 4 * gcSize(vec4), 0);

//     PlaneColorBuffer[0] = colorToPremultipliedLinear({255, 0, 0, 255});
//     PlaneColorBuffer[1] = colorToPremultipliedLinear({0, 255, 0, 255});
//     PlaneColorBuffer[2] = colorToPremultipliedLinear({0, 0, 255, 255});
//     PlaneColorBuffer[3] = colorToPremultipliedLinear({255, 0, 255, 255});

//     PlaneIndexColorBuffer[0] = 1;
//     PlaneIndexColorBuffer[1] = 2;
//     PlaneIndexColorBuffer[2] = 3;
//     PlaneIndexColorBuffer[3] = 1;
//     PlaneIndexColorBuffer[4] = 3;
//     PlaneIndexColorBuffer[5] = 4;
// #else
//     vec4 *PlaneColorBuffer = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec4), 0);

//     PlaneColorBuffer[0] = colorToPremultipliedLinear({100, 100, 100, 255});

//     PlaneIndexColorBuffer[0] = 1;
//     PlaneIndexColorBuffer[1] = 1;
//     PlaneIndexColorBuffer[2] = 1;
//     PlaneIndexColorBuffer[3] = 1;
//     PlaneIndexColorBuffer[4] = 1;
//     PlaneIndexColorBuffer[5] = 1;
// #endif

//     gl_addAttribute(TestPlaneModel, 3, PlaneGeometry->PositionIndex, (r32 *) PlaneGeometry->PositionData);
//     gl_addAttribute(TestPlaneModel, 2, PlaneGeometry->uvIndex, (r32 *) PlaneGeometry->uvData);
//     gl_addAttribute(TestPlaneModel, 3, PlaneGeometry->NormalIndex, (r32 *) PlaneGeometry->NormalData);
//     gl_addAttribute(TestPlaneModel, 4, PlaneIndexColorBuffer, (r32 *) PlaneColorBuffer);
//     gl_addExtraAttribute(TestPlaneModel, 3);
//     gl_addExtraAttribute(TestPlaneModel, 3);
}

// void initPrimitives(engine_state_t *State)
// {
//     State->PrimitiveMap = tht_create(MEMORY_TEMPORARY, MAX_3D_PRIMITIVES);
//     Type_Hashtable *Primitives = State->PrimitiveMap;

//     // -- General grid.
//     // ----------------------------------------------------------------------------------

//     GlGeneralGrid3D *GeneralGrid = (GlGeneralGrid3D *) tht_insert(Primitives, "general_grid", gcSize(GlGeneralGrid3D));
//     GeneralGrid->Object.Program = &State->Programs[PRG_GENERAL_GRID];
//     gl_GeneralGrid3D_default(GeneralGrid);
//     // GeneralGrid->drawAxis = false;
//     // GeneralGrid->rows = 1;
//     // GeneralGrid->cols = 1;
//     gl_GeneralGrid3D_init(GeneralGrid);
// }

void initDebugTriangles3D(engine_state_t *State)
{
    gl_program_t *GeneralProgram = &State->Programs[0];
    gl_state_t *GL = &State->GL;
    Type_Hashtable *Models = State->ObjectMap;

    GlRenderObject3D *TriangleA = (GlRenderObject3D *) tht_insert(Models, "test_triangle_1", gcSize(GlRenderObject3D));
    GlRenderObject3D *TriangleB = (GlRenderObject3D *) tht_insert(Models, "test_triangle_2", gcSize(GlRenderObject3D));

    // ----------------------------------------------------------------------------------

    // TriangleA->Structure.type = GL_TRIANGLES;
    // TriangleA->Structure.vertices = 1 * GL_TRIANGLES;

    // vec3 *TriangleA_PositionData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec3), 0);
    // u32 *TriangleA_PositionIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec3 *TriangleA_NormalData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec3), 0);
    // u32 *TriangleA_NormalIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec4 *TriangleA_ColorData = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec4), 0);
    // u32 *TriangleA_ColorIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // // winding CW
    // TriangleA_PositionData[0] = {0, 0.3f, 0};
    // TriangleA_PositionData[1] = {-0.1f, 0, 0};
    // TriangleA_PositionData[2] = {-0.3f, 0, 0};

    // TriangleA_PositionIndex[0] = 1;
    // TriangleA_PositionIndex[1] = 2;
    // TriangleA_PositionIndex[2] = 3;

    // TriangleA_NormalData[0] = {0, 0, 1.0f};

    // TriangleA_NormalIndex[0] = 1;
    // TriangleA_NormalIndex[1] = 1;
    // TriangleA_NormalIndex[2] = 1;

    // TriangleA_ColorData[0] = colorToPremultipliedLinear({0, 0, 255, 255});

    // TriangleA_ColorIndex[0] = 1;
    // TriangleA_ColorIndex[1] = 1;
    // TriangleA_ColorIndex[2] = 1;

    // ----------------------------------------------------------------------------------

    // TriangleB->Structure.type = GL_TRIANGLES;
    // TriangleB->Structure.vertices = 1 * GL_TRIANGLES;

    // vec3 *TriangleB_PositionData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec3), 0);
    // u32 *TriangleB_PositionIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec3 *TriangleB_NormalData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec3), 0);
    // u32 *TriangleB_NormalIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec4 *TriangleB_ColorData = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec4), 0);
    // u32 *TriangleB_ColorIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // // winding CW
    // TriangleB_PositionData[0] = {0, 0.3f, 0};
    // TriangleB_PositionData[1] = {0.3f, 0, 0};
    // TriangleB_PositionData[2] = {-0.1f, 0, 0};

    // TriangleB_PositionIndex[0] = 1;
    // TriangleB_PositionIndex[1] = 2;
    // TriangleB_PositionIndex[2] = 3;

    // TriangleB_NormalData[0] = {0, 0, 1.0f};

    // TriangleB_NormalIndex[0] = 1;
    // TriangleB_NormalIndex[1] = 1;
    // TriangleB_NormalIndex[2] = 1;

    // TriangleB_ColorData[0] = colorToPremultipliedLinear({255, 0, 0, 255});

    // TriangleB_ColorIndex[0] = 1;
    // TriangleB_ColorIndex[1] = 1;
    // TriangleB_ColorIndex[2] = 1;

    // ----------------------------------------------------------------------------------

    TriangleA->position = {0, 0, 0};
    TriangleA->scaling = {1, 1, 1};

    // gl_addAttribute(TriangleA, 3, TriangleA_PositionIndex, (r32 *) TriangleA_PositionData);
    // gl_addAttribute(TriangleA, 3, TriangleA_NormalIndex, (r32 *) TriangleA_NormalData);
    // gl_addAttribute(TriangleA, 4, TriangleA_ColorIndex, (r32 *) TriangleA_ColorData);

    TriangleB->position = {0, 0, 0};
    TriangleB->scaling = {1, 1, 1};

    // gl_addAttribute(TriangleB, 3, TriangleB_PositionIndex, (r32 *) TriangleB_PositionData);
    // gl_addAttribute(TriangleB, 3, TriangleB_NormalIndex, (r32 *) TriangleB_NormalData);
    // gl_addAttribute(TriangleB, 4, TriangleB_ColorIndex, (r32 *) TriangleB_ColorData);
}

void initDebugOverlappedTriangles3D(engine_state_t *State)
{
    gl_program_t *GeneralProgram = &State->Programs[0];
    gl_state_t *GL = &State->GL;
    Type_Hashtable *Models = State->ObjectMap;

    GlRenderObject3D *TriangleA = (GlRenderObject3D *) tht_insert(Models, "test_overlapped_triangle_1", gcSize(GlRenderObject3D));
    GlRenderObject3D *TriangleB = (GlRenderObject3D *) tht_insert(Models, "test_overlapped_triangle_2", gcSize(GlRenderObject3D));

    // ----------------------------------------------------------------------------------

    // TriangleA->Structure.type = GL_TRIANGLES;
    // TriangleA->Structure.vertices = 1 * GL_TRIANGLES;

    // vec3 *TriangleA_PositionData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec3), 0);
    // u32 *TriangleA_PositionIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec3 *TriangleA_NormalData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec3), 0);
    // u32 *TriangleA_NormalIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec4 *TriangleA_ColorData = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec4), 0);
    // u32 *TriangleA_ColorIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // // winding CW
    // TriangleA_PositionData[0] = {-0.2f, 0.8f, -1};
    // TriangleA_PositionData[1] = {0.2f, 0.1f, -1};
    // TriangleA_PositionData[2] = {-0.6f, 0.1f, -1};

    // TriangleA_PositionIndex[0] = 1;
    // TriangleA_PositionIndex[1] = 2;
    // TriangleA_PositionIndex[2] = 3;

    // TriangleA_NormalData[0] = {0, 0, 1.0f};

    // TriangleA_NormalIndex[0] = 1;
    // TriangleA_NormalIndex[1] = 1;
    // TriangleA_NormalIndex[2] = 1;

    // TriangleA_ColorData[0] = colorToPremultipliedLinear({0, 0, 255, 255});

    // TriangleA_ColorIndex[0] = 1;
    // TriangleA_ColorIndex[1] = 1;
    // TriangleA_ColorIndex[2] = 1;

    // ----------------------------------------------------------------------------------

    // TriangleB->Structure.type = GL_TRIANGLES;
    // TriangleB->Structure.vertices = 1 * GL_TRIANGLES;

    // vec3 *TriangleB_PositionData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(vec3), 0);
    // u32 *TriangleB_PositionIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec3 *TriangleB_NormalData = (vec3 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec3), 0);
    // u32 *TriangleB_NormalIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // vec4 *TriangleB_ColorData = (vec4 *) gc_mem_allocate(MEMORY_PERMANENT, 1 * gcSize(vec4), 0);
    // u32 *TriangleB_ColorIndex = (u32 *) gc_mem_allocate(MEMORY_PERMANENT, 3 * gcSize(u32), 0);

    // // winding CW
    // TriangleB_PositionData[0] = {0, 0.4f, 0};
    // TriangleB_PositionData[1] = {0.2f, 0, 0};
    // TriangleB_PositionData[2] = {-0.2f, 0, 0};

    // TriangleB_PositionIndex[0] = 1;
    // TriangleB_PositionIndex[1] = 2;
    // TriangleB_PositionIndex[2] = 3;

    // TriangleB_NormalData[0] = {0, 0, 1.0f};

    // TriangleB_NormalIndex[0] = 1;
    // TriangleB_NormalIndex[1] = 1;
    // TriangleB_NormalIndex[2] = 1;

    // TriangleB_ColorData[0] = colorToPremultipliedLinear({255, 0, 0, 255});

    // TriangleB_ColorIndex[0] = 1;
    // TriangleB_ColorIndex[1] = 1;
    // TriangleB_ColorIndex[2] = 1;

    // ----------------------------------------------------------------------------------

    TriangleA->position = {0, 0, 0};
    TriangleA->scaling = {1, 1, 1};

    // gl_addAttribute(TriangleA, 3, TriangleA_PositionIndex, (r32 *) TriangleA_PositionData);
    // gl_addAttribute(TriangleA, 3, TriangleA_NormalIndex, (r32 *) TriangleA_NormalData);
    // gl_addAttribute(TriangleA, 4, TriangleA_ColorIndex, (r32 *) TriangleA_ColorData);

    TriangleB->position = {0, 0, 0};
    TriangleB->scaling = {1, 1, 1};

    // gl_addAttribute(TriangleB, 3, TriangleB_PositionIndex, (r32 *) TriangleB_PositionData);
    // gl_addAttribute(TriangleB, 3, TriangleB_NormalIndex, (r32 *) TriangleB_NormalData);
    // gl_addAttribute(TriangleB, 4, TriangleB_ColorIndex, (r32 *) TriangleB_ColorData);
}

void updateObjectLibrary3D(engine_state_t *State)
{
    Type_Hashtable *Objects3D = State->ObjectMap;

    Type_HashtableElement *Search = tht_search(State->ObjectMap, "test_triangle");
    GlRenderObject3D *TriangleObject = (GlRenderObject3D *) gcGetData(Search);

    Search = tht_search(State->ObjectMap, "test_plane");
    GlRenderObject3D *PlaneObject = (GlRenderObject3D *) gcGetData(Search);

    Search = tht_search(State->ObjectMap, "african_head");
    GlRenderObject3D *AfricanHeadObject = (GlRenderObject3D *) gcGetData(Search);

    Search = tht_search(State->ObjectMap, "chair");
    GlRenderObject3D *ChairObject = (GlRenderObject3D *) gcGetData(Search);

    // PlaneObject->rotation.x = 17;
    // PlaneObject->position.y = 0.07;

    // TriangleObject->position.x = -0.6;
    // TriangleObject->position.y = 0.4;

    // TriangleObject->rotation.x += 1;
    // TriangleObject->rotation.x = 0;

    // ChairObject->rotation.y = 108;
    // ChairObject->rotation.y = -22;
    // ChairObject->rotation.y = -50;
}

// ----------------------------------------------------------------------------------
// -- Various predefined scenes.
// ----------------------------------------------------------------------------------

/*
 * Head and chair.
 */
// void scene_addHeadAndChair(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_GENERAL];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "african_head");
//     GlRenderObject3D *AfricanHeadObject3D = (GlRenderObject3D *) gcGetData(Search);

//     Search = tht_search(State->ObjectMap, "chair");
//     GlRenderObject3D *ChairObject3D = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CCW;

//     gl_pushRenderStack(SceneStack, AfricanHeadObject3D);
//     gl_pushRenderStack(SceneStack, ChairObject3D);

//     // -- Model update.

//     // AfricanHeadObject3D->position = {0, 0, -3.3};
//     // AfricanHeadObject3D->rotation.x += 1;
//     // AfricanHeadObject3D->rotation.x += 1;
//     AfricanHeadObject3D->rotation.y -= 1;
//     // AfricanHeadObject3D->rotation.x = 0;
//     // AfricanHeadObject3D->rotation.y = 0;
// }

// void scene_addHead(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_GENERAL];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "african_head");
//     GlRenderObject3D *AfricanHeadObject3D = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CCW;

//     gl_pushRenderStack(SceneStack, AfricanHeadObject3D);

//     // -- Model update.

//     // AfricanHeadObject3D->position = {0, 0, -3.3};
//     // AfricanHeadObject3D->rotation.x += 1;
//     // AfricanHeadObject3D->rotation.x += 2;
//     AfricanHeadObject3D->rotation.y -= 1;
//     // AfricanHeadObject3D->rotation.y = -34;
//     // AfricanHeadObject3D->rotation.x = 0;
//     // AfricanHeadObject3D->rotation.y = 0;
// }

// void scene_addChair(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_GENERAL];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "chair");
//     GlRenderObject3D *ChairObject3D = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CCW;

//     gl_pushRenderStack(SceneStack, ChairObject3D);

//     // -- Model update.

//     ChairObject3D->position = {0, 0, -2};
//     ChairObject3D->scaling = {0.4f, 0.4f, 0.7f};
//     // ChairObject3D->rotation.y += 1;
//     // ChairObject3D->rotation.x -= 3;
// }

void scene_addGeneralGrid(engine_state_t *State, GlScene *Scene)
{
    GlRenderStack *SceneStack = &Scene->RenderStack;

    // State->GL.Settings.vertexWinding = GC_WINDING_CCW;

    Type_HashtableElement *Search = 0;
    GlGeneralGrid3D *GeneralGrid = (GlGeneralGrid3D *) tht_data(tht_search(State->PrimitiveMap, "general_grid"));

    // Search = tht_search(State->ObjectMap, "african_head");
    // GlRenderObject3D *AfricanHeadObject3D = (GlRenderObject3D *) tht_data(Search);

    // Search = tht_search(State->ObjectMap, "wall");
    // GlRenderObject3D *Wall = (GlRenderObject3D *) tht_data(Search);

    // Search = tht_search(State->ObjectMap, "wall_red");
    // GlRenderObject3D *WallRed = (GlRenderObject3D *) tht_data(Search);

    // Search = tht_search(State->ObjectMap, "wall3");
    // GlRenderObject3D *WallBlue = (GlRenderObject3D *) tht_data(Search);

    GlRenderObject3D *WallRedObject = getObject(State, "wall_red");
    GlRenderObject3D *WallBlueObject = getObject(State, "wall_blue");
    GlRenderObject3D *ZBufferTestObject = getObject(State, "z_buffer_test_object");

    gl_resetRenderStack(SceneStack);
    // gl_pushRenderStack(SceneStack, &GeneralGrid->Object);
    // gl_pushRenderStack(SceneStack, WallObject);
    // gl_pushRenderStack(SceneStack, WallRedObject);
    // gl_pushRenderStack(SceneStack, WallBlueObject);
    // gl_pushRenderStack(SceneStack, ZBufferTestObject);

#if 0
    GlRenderObject3D *TestcubeObject = getObject(State, "testcube");
    gl_pushRenderStack(SceneStack, TestcubeObject);

    u32 flags = global_gl->Config.flags;
    // TestcubeObject->overwrites.flags = flags & (~GC_BACKFACE_CULL);
    TestcubeObject->overwrites.flags = flags;
    // TestcubeObject->overwrites.flags |= GL_TRANSPARENCY_BUFFER | GL_UNIF_MESH_OPACITY;
    TestcubeObject->overwrites.flags |= GL_UNIF_MESH_COLOR;

    TestcubeObject->overwrites.meshColor = {1.0f, 1.0f, 1.0f, 1.0f};
    TestcubeObject->overwrites.meshOpacity = 0.4f;
#endif

#if 0
    GlRenderObject3D *WallObject = getObject(State, "wall");
    gl_pushRenderStack(SceneStack, WallObject);

    u32 flags = global_gl->Config.flags;
    WallObject->overwrites.flags = flags | GL_RMODE_WIREFRAME;
    WallObject->overwrites.flags &= ~(GL_RMODE_NORMAL);
    // WallObject->overwrites.flags = flags & (~GC_BACKFACE_CULL);
    // WallObject->overwrites.flags |= GL_UNIF_MESH_COLOR;
    // WallObject->overwrites.meshColor = {1.0f, 1.0f, 1.0f, 1.0f};

    // WallObject->overwrites.flags |= GL_RMODE_WIREFRAME |
    //                                 GL_RMODE_POINTS |
    //                                 //  GL_UNIF_MESH_COLOR |
    //                                 GL_UNIF_WIREFRAME_COLOR |
    //                                 GL_TRANSPARENCY_BUFFER |
    //                                 GL_SPW;

    // WallObject->flatColor = {0.39f, 0.31f, 0.24f, 0.1f};
    // WallObject->flatColor = {1.0f, 1.0f, 1.0f, 0.1f};
    // WallObject->alpha = 0.5f;
    r32 speed = 0.05f;
    // WallObject->rotation.x += speed;
    // WallObject->rotation.z += speed;
    // WallObject->rotation.y += speed;
    // WallObject->rotation.x = 0;
    // WallObject->rotation.z = 0;
    // WallObject->position.x -= 0.002f;
#endif

#if 1
    GlRenderObject3D *AfricanHeadObject = getObject(State, "african_head");
    gl_pushRenderStack(SceneStack, AfricanHeadObject);

    u32 flags = global_gl->Config.flags;
    AfricanHeadObject->overwrites.flags = flags;
    // AfricanHeadObject->overwrites.flags = flags | GL_RMODE_WIREFRAME;
    // AfricanHeadObject->overwrites.flags = flags | GL_RMODE_POINTS | GL_TRANSPARENCY_BUFFER | GL_UNIF_WIREFRAME_COLOR;
    // AfricanHeadObject->overwrites.flags = flags | GL_RMODE_WIREFRAME | GL_RMODE_POINTS | GL_TRANSPARENCY_BUFFER | GL_UNIF_WIREFRAME_COLOR;
    // AfricanHeadObject->overwrites.flags = flags | GL_RMODE_WIREFRAME | GL_TRANSPARENCY_BUFFER | GL_UNIF_WIREFRAME_COLOR;
    // AfricanHeadObject->overwrites.flags = flags | GL_RMODE_WIREFRAME | GL_RMODE_POINTS | GL_UNIF_WIREFRAME_COLOR;
    // AfricanHeadObject->overwrites.flags = flags | GL_RMODE_POINTS | GL_RMODE_WIREFRAME;
    // AfricanHeadObject->overwrites.flags &= ~(GL_RMODE_NORMAL);

    // AfricanHeadObject->overwrites.flags = GL_UNIF_MESH_OPACITY;
    // AfricanHeadObject->overwrites.meshOpacity = 0.5f;
    // AfricanHeadObject->overwrites.flags = flags | GL_UNIF_MESH_COLOR;
    // AfricanHeadObject->overwrites.flags = flags;
    AfricanHeadObject->overwrites.meshColor = {1.0f, 1.0f, 1.0f, 1.0f};
    AfricanHeadObject->overwrites.wireframeColor = {1.0f, 1.0f, 1.0f, 0.1f};
#endif

#if 0
    GlRenderObject3D *WitchObject = getObject(State, "witch_object");
    gl_pushRenderStack(SceneStack, WitchObject);

    // WitchObject->flatColor = {0.5f, 0.8f, 0.5f, 0.1f};
    // WitchObject->flatColor = {1.0f, 1.0f, 1.0f, 0.1f};
    // WitchObject->overwrites.flags = GL_TRANSPARENCY_BUFFER | GL_SPW | GL_UNIF_WIREFRAME_COLOR;
    u32 flags = global_gl->Config.flags;
    // WitchObject->overwrites.flags = flags & (~(GC_BACKFACE_CULL | GL_RMODE_NORMAL));
    // WitchObject->overwrites.flags = flags;
    // WitchObject->overwrites.flags = flags & (~(GC_BACKFACE_CULL));
    // WitchObject->overwrites.flags = flags;
    // WitchObject->overwrites.flags |= GL_UNIF_MESH_COLOR;
    // WitchObject->overwrites.flags |= GL_TRANSPARENCY_BUFFER;
    //                                  GL_UNIF_POINT_TYPE |
    //                                  GL_RMODE_WIREFRAME |
    //                                  GL_UNIF_WIREFRAME_COLOR;

    // WitchObject->overwrites.flags = flags | GL_RMODE_WIREFRAME;
    // WitchObject->overwrites.flags = flags | GL_RMODE_POINTS;
    // WitchObject->overwrites.flags &= ~(GL_RMODE_NORMAL);
    // WitchObject->overwrites.flags = (flags & (~(GC_BACKFACE_CULL))) | GL_RMODE_WIREFRAME | GL_TRANSPARENCY_BUFFER | GL_UNIF_WIREFRAME_COLOR;
    // WitchObject->overwrites.flags = (flags & (~(GC_BACKFACE_CULL | GL_RMODE_NORMAL))) | GL_RMODE_WIREFRAME | GL_RMODE_POINTS | GL_UNIF_WIREFRAME_COLOR | GL_TRANSPARENCY_BUFFER;
    // WitchObject->overwrites.flags = (flags & (~(GC_BACKFACE_CULL))) | GL_RMODE_WIREFRAME | GL_TRANSPARENCY_BUFFER;
    // WitchObject->overwrites.flags = (flags & (~(GC_BACKFACE_CULL)));
    WitchObject->overwrites.flags = (flags & (~(GC_BACKFACE_CULL))) | GL_TRANSPARENCY_BUFFER;

    // WitchObject->overwrites.flags |= GL_TRANSPARENCY_BUFFER | GL_UNIF_WIREFRAME_COLOR;
    // WitchObject->overwrites.flags |= GL_UNIF_WIREFRAME_COLOR;

    // WitchObject->overwrites.flags = flags | GL_UNIF_MESH_COLOR;

    // WitchObject->overwrites.flags |= GL_RMODE_WIREFRAME |
    //                                  GL_RMODE_POINTS |
    //                                 //  GL_UNIF_MESH_COLOR |
    //                                  GL_UNIF_WIREFRAME_COLOR |
    //                                  GL_TRANSPARENCY_BUFFER |
    //                                  GL_SPW;

    // WitchObject->overwrites.flags |= GL_RMODE_WIREFRAME |
    //                                  GL_RMODE_POINTS |
    //                                  GL_UNIF_WIREFRAME_COLOR |
    //                                  GL_TRANSPARENCY_BUFFER;

    // WitchObject->overwrites.flags |= GL_TRANSPARENCY_BUFFER;

    // WitchObject->overwrites.flags |= GL_TRANSPARENCY_BUFFER;

    WitchObject->overwrites.meshOpacity = 0.3f;
    WitchObject->overwrites.pointType = GL_SPRITE_POINT_TRIANGLE;
    // WitchObject->overwrites.meshColor = {0.3f, 0.3f, 0.3f, 1.0f};
    WitchObject->overwrites.meshColor = {1.0f, 1.0f, 1.0f, 0.2f};
    // WitchObject->overwrites.wireframeColor = {1.0f, 1.0f, 1.0f, 0.2f};
    WitchObject->overwrites.wireframeColor = {1.0f, 1.0f, 1.0f, 0.1f};
    WitchObject->scaling = {0.3f, 0.3f, 0.3f};
    WitchObject->position = {-5.0f, 0, -8.0f};
#endif

    // -- Camera update.

    global_debug_camera = &Scene->Camera;

    global_gl->Light.dir = {1.0f, 0, -1.0f};
    global_gl->Light.color = {1.0f, 1.0f, 1.0f, 1.0f};
    global_gl->Light.minI = 0.2f;

    global_gl->uniforms.Light = &global_gl->Light;

#ifdef DEBUG_DISABLE_CAMERA_DAMPENING
    Scene->Camera.Focus.zRotDampening = 1.06f;
    Scene->Camera.Focus.pitchRotDampening = 1.06f;
#endif

    // Scene->Camera.Focus.zTheta = 234.65788f;
    // Scene->Camera.Focus.pitchTheta = -31.345205f;
}

// void scene_addAnimatedGrid(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;

//     // -- Initialization.

//     // if (!State->AmimatedGrid)
//     // {
//     //     State->AmimatedGrid = gl_createAnimatedGrid3D(40, 40, {0, 0, 0}, 0.05f, MEMORY_TEMPORARY);
//     //     State->AmimatedGrid->Object.Program = &State->Programs[PRG_GRID];
//     // }

//     // if (!State->SimpleGrid)
//     // {
//     //     State->SimpleGrid = gl_createGrid3DSimple(20, 20, {0, 0, 0}, 0.05f, MEMORY_TEMPORARY);
//     //     State->SimpleGrid->Object.Program = &State->Programs[PRG_GRID];
//     //     State->SimpleGrid->Object.rotation = {-70, 0, 0};
//     // }

//     State->GL.Settings.vertexWinding = GC_WINDING_CCW;
//     Scene->Program = &State->Programs[PRG_GENERAL];

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "african_head");
//     GlRenderObject3D *AfricanHeadObject3D = (GlRenderObject3D *) tht_data(Search);

//     gl_resetRenderStack(SceneStack);
//     // gl_pushRenderStack(SceneStack, &State->SimpleGrid->Object);
//     // gl_pushRenderStack(SceneStack, &State->AmimatedGrid->Object);
//     gl_pushRenderStack(SceneStack, AfricanHeadObject3D);

//     // -- Model update.
// #if 0
//     GlAnimatedGrid3D *Grid = State->AmimatedGrid;

//     // Grid->Object.rotation = {-60, 0, 0};
//     Grid->Object.rotation.x = -60;
//     // Grid->Object.rotation.y += 0.5f;

//     Grid->Wave.x_radianInterval = 1 * PI;
//     Grid->Wave.x_animationStep = PI / (2.0f * 20);
//     Grid->Wave.x_stepsPerInterval = 20;
//     Grid->Wave.x_normalizer = 1 / (Grid->Wave.x_stepsPerInterval * Grid->dist);
//     Grid->Wave.x_animationPlayback += Grid->Wave.x_animationStep;
//     Grid->Wave.x_maxHeight = 0.2f;

//     Grid->Wave.y_radianInterval = 2 * PI;
//     Grid->Wave.y_animationStep = PI / (2.0f * 40);
//     Grid->Wave.y_stepsPerInterval = 30;
//     Grid->Wave.y_normalizer = 1 / (Grid->Wave.y_stepsPerInterval * Grid->dist);
//     Grid->Wave.y_animationPlayback += Grid->Wave.y_animationStep;
//     Grid->Wave.y_maxHeight = 0.1f;
// #endif

//     AfricanHeadObject3D->rotation.y -= 1;
// }

// void scene_addBigTriangle(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_GENERAL];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "test_triangle");
//     GlRenderObject3D *TriangleObject3D = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CCW;

//     gl_pushRenderStack(SceneStack, TriangleObject3D);
// }

// void scene_addPlane(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_GENERAL];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "test_plane");
//     GlRenderObject3D *PlaneObject3D = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CCW;

//     gl_pushRenderStack(SceneStack, PlaneObject3D);
// }

/*
 * Test overlapped triangles.
 */
// void scene_addOverlappedTriangles(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_TEST_TRIANGLES];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "test_overlapped_triangle_1");
//     GlRenderObject3D *OverlappedTriangleA = (GlRenderObject3D *) gcGetData(Search);

//     Search = tht_search(State->ObjectMap, "test_overlapped_triangle_2");
//     GlRenderObject3D *OverlappedTriangleB = (GlRenderObject3D *) gcGetData(Search);

//     Search = tht_search(State->ObjectMap, "test_triangle_1");
//     GlRenderObject3D *TriangleA = (GlRenderObject3D *) gcGetData(Search);

//     Search = tht_search(State->ObjectMap, "test_triangle_2");
//     GlRenderObject3D *TriangleB = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CW;

//     // gl_pushRenderStack(SceneStack, TriangleA);
//     // gl_pushRenderStack(SceneStack, TriangleB);
//     gl_pushRenderStack(SceneStack, OverlappedTriangleA);
//     gl_pushRenderStack(SceneStack, OverlappedTriangleB);
// }

/*
 * Test plane.
 */
// void scene_addTestPlane(engine_state_t *State, GlScene *Scene)
// {
//     GlRenderStack *SceneStack = &Scene->RenderStack;
//     Scene->Program = &State->Programs[PRG_TEST_TRIANGLES];

//     gl_resetRenderStack(SceneStack);

//     Type_HashtableElement *Search = tht_search(State->ObjectMap, "test_plane");
//     GlRenderObject3D *PlaneObject = (GlRenderObject3D *) gcGetData(Search);

//     State->GL.Settings.vertexWinding = GC_WINDING_CW;

//     gl_pushRenderStack(SceneStack, PlaneObject);
// }

// ----------------------------------------------------------------------------------

/**
 * Initialization of all the scenes.
 */
void initScenes(engine_state_t *State)
{
    gl_state_t *GL = &State->GL;
    videobuffer_t *buffer = GL->buffer;
    gc_framebuffer_t *framebuffer = GL->framebuffer;

    // ----------------------------------------------------------------------------------
    // -- 3D perspective scene.
    // ----------------------------------------------------------------------------------

    GlScene *PerspectiveScene = &State->Scenes[SCENE_3D_PERSPECTIVE];
    GlRenderStack *PerspectiveStack = &PerspectiveScene->RenderStack;

    PerspectiveScene->WireframeProgram = &State->Programs[SHADER_WIREFRAME];
    PerspectiveScene->MeshProgram = &State->Programs[SHADER_MESH_WITH_TEXTURE];

    // PerspectiveScene->Name = ts_create(MEMORY_PERMANENT, "3D Perspective Scene");
    PerspectiveScene->Name = "3D Perspective Scene";
    PerspectiveScene->type = GC_RENDER_OBJECT_3D;
    gl_initRenderStack(PerspectiveStack, 10);

    PerspectiveScene->Viewport.Size = {(r32) buffer->width, (r32) buffer->height};
    PerspectiveScene->Viewport.Origin = {0, 0};
    PerspectiveScene->Viewport.depth = 1;

    r32 aspect = 1.0f * buffer->width / buffer->height;

    // PerspectiveScene->Projection.nearDir = 1;
    // PerspectiveScene->Projection.nearSpeed = 0.01f;
    // PerspectiveScene->Projection.nearThreshold = 4;

    PerspectiveScene->Projection.type = GC_PROJECTION_PERSPECTIVE;
    PerspectiveScene->Projection.Perspective.aspect = aspect;
    PerspectiveScene->Projection.Perspective.fov = 45;
    PerspectiveScene->Projection.Perspective.near = 0.1f;
    PerspectiveScene->Projection.Perspective.far = 100.0f;

    // -- Scene camera.

    PerspectiveScene->Camera.type = GL_FOCUS_POINT;
    // PerspectiveScene->Camera.translation = {0, -70.0f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -50.0f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -14.5f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -7.5f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -6.5f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -4.5f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -3.5f, 0.0f};
    PerspectiveScene->Camera.translation = {0, -2.5f, 1.0f};
    // PerspectiveScene->Camera.translation = {0, -1.5f, 1.0f};
    // PerspectiveScene->Camera.oneOverScreenWidth = 1.0f / GL_RES_WIDTH;
    // PerspectiveScene->Camera.oneOverScreenHeight = 1.0f / GL_RES_HEIGHT;
    PerspectiveScene->Camera.aspectRatio = aspect;
    PerspectiveScene->Camera.Focus.focusPoint;

    PerspectiveScene->Camera.Focus.zMaxVel = 360;
    PerspectiveScene->Camera.Focus.pitchMaxVel = 360;
    PerspectiveScene->Camera.Focus.zRotDampening = 0.06f;
    PerspectiveScene->Camera.Focus.pitchRotDampening = 0.06f;

    // NOTE(gabic): Este o problema atunci cand pozitionez camera la un anumit unghi.

#ifdef DEBUG_ENABLE_CAMERA_DEBUG
    PerspectiveScene->Camera.Focus.zTheta = DEBUG_CAMERA_YAW;
    PerspectiveScene->Camera.Focus.pitchTheta = DEBUG_CAMERA_PITCH;
#else
    PerspectiveScene->Camera.Focus.zTheta = 0;
    PerspectiveScene->Camera.Focus.pitchTheta = 0;
#endif

    PerspectiveScene->Camera.Focus.zTargetTheta = PerspectiveScene->Camera.Focus.zTheta;
    PerspectiveScene->Camera.Focus.pitchTargetTheta = PerspectiveScene->Camera.Focus.pitchTheta;

    // -- Vedere lateral stanga-jos.

    // PerspectiveScene->Camera.position = {-3.0f, -2.5f, 0.0f};
    // PerspectiveScene->Camera.rotation.x = 110;
    // PerspectiveScene->Camera.rotation.z = -45;

    // TODO(gabic): Bug la miscarea pe verticala.
    // PerspectiveScene->Camera.position = {0, -1.0f, 0.0f};

    // PerspectiveScene->Camera.position = {0, -2.5f, 0.0f};

    // ----------------------------------------------------------------------------------
    // TODO(gabic): bug la optimizarea suprafetei unui bloc.
    // ----------------------------------------------------------------------------------
    // PerspectiveScene->Camera.position = {0.0f, -2.4f, 2.0f};
    // PerspectiveScene->Camera.pos = {0.0f, -2.5f, 1.0f};
    // PerspectiveScene->Camera.position = {0.0f, -5.0f, 1.0f};
    // PerspectiveScene->Camera.pos = {0.0f, -10.0f, 1.0f};
    PerspectiveScene->Camera.pos = {0.0f, -10.0f, 0.0f};

    // ----------------------------------------------------------------------------------
    // TODO(gabic): bug la rutina de rasterizare triunghi.
    // ----------------------------------------------------------------------------------
    // PerspectiveScene->Camera.position = {0, -2.29999876f, 0.0f};

    PerspectiveScene->Camera.fRot.x = 90;

    PerspectiveScene->Camera.Side = {1, 0, 0};
    PerspectiveScene->Camera.Up = {0, 1, 0};
    PerspectiveScene->Camera.Dir = {0, 0, 1};
    PerspectiveScene->Camera.Forward = {0, 1, 0};

    gl_createSceneClippingPlanes(PerspectiveScene);

    // -- Lights.

    GlLightOld *Light0 = gl_addLight(PerspectiveScene);
    strncpy(Light0->Id, "Light0", 50);
    Light0->type = GC_SUN_LIGHT;
    Light0->isEnabled = true;
    Light0->defaultIntensity = 1;
    Light0->scalingFactorAmbDiffSpec = {1, 3, 2};
    Light0->direction = vec3_normalize({0.5f, 0.0f, -1.0f});
    Light0->color = {1.0f, 0.5f, 0.7f, 1};

    // Light0->type = GC_POINT_LIGHT;
    // Light0->position = {1, 1, 2};
    // Light0->point.kC = 0.5f;
    // Light0->point.kL = 0.1f;
    // Light0->point.kQ = 0.1f;
#if 0
    GlLightOld *Light1 = gl_addLight(PerspectiveScene);
    strncpy(Light1->Id, "Light1", 50);
    Light1->type = GC_SPOT_LIGHT;
    Light1->isEnabled = true;
    Light1->color = {1, 1, 0, 1};
    Light1->defaultIntensity = 4;
    Light1->scalingFactorAmbDiffSpec = {1, 1, 5};
    Light1->direction = vec3_normalize({-1.0f, -0.0f, -0.0f});
    Light1->position = {3.0f, 0.0f, 0.0f};
    Light1->Spot.kC = 0.5f;
    Light1->Spot.kL = 0.0f;
    Light1->Spot.kQ = 0.0f;
    Light1->Spot.sE = 100.0f;
    Light1->Spot.angle = 25.0f;
    Light1->Spot.cosTheta = t_cos(DEG2RAD(Light1->Spot.angle));
#endif
    // ----------------------------------------------------------------------------------
    // -- 3D orthographic scene.
    // ----------------------------------------------------------------------------------

    GlScene *OrthographicScene = &State->Scenes[SCENE_3D_ORTHOGRAPHIC];
    GlRenderStack *OrthographicStack = &OrthographicScene->RenderStack;

    // OrthographicScene->Name = ts_create(MEMORY_PERMANENT, "3D Orthographic Scene");
    OrthographicScene->Name = "3D Orthographic Scene";
    OrthographicScene->type = GC_RENDER_OBJECT_3D;
    gl_initRenderStack(OrthographicStack, 10);

    // OrthographicScene->Program = &State->Programs[PRG_GENERAL];
    // OrthographicScene->Program = &State->Programs[PRG_TEST_TRIANGLES];

    OrthographicScene->Viewport.Size = {(r32) buffer->width, (r32) buffer->height};
    OrthographicScene->Viewport.Origin = {0, 0};
    OrthographicScene->Viewport.depth = 1;

    OrthographicScene->Projection.nearDir = 1;
    OrthographicScene->Projection.nearSpeed = 0.01f;
    OrthographicScene->Projection.nearThreshold = 4;

    OrthographicScene->Projection.type = GC_PROJECTION_ORTHOGRAPHIC;
    OrthographicScene->Projection.Orthographic.top = 1.0f;
    OrthographicScene->Projection.Orthographic.bottom = -1.0f;
    OrthographicScene->Projection.Orthographic.right = aspect * OrthographicScene->Projection.Orthographic.top;
    OrthographicScene->Projection.Orthographic.left = aspect * OrthographicScene->Projection.Orthographic.bottom;
    OrthographicScene->Projection.Orthographic.near = 0.1f;
    OrthographicScene->Projection.Orthographic.far = 100.0f;

    // -- Scene camera.

    OrthographicScene->Camera.pos = {0, 0, 1.0f};
    OrthographicScene->Camera.Side = {1, 0, 0};
    OrthographicScene->Camera.Up = {0, 1, 0};
    OrthographicScene->Camera.Dir = {0, 0, 1};
    OrthographicScene->Camera.Forward = {0, 1, 0};

    // ----------------------------------------------------------------------------------
    // -- 2D scene.
    // ----------------------------------------------------------------------------------

    GlScene *Scene2D = &State->Scenes[SCENE_2D];
    GlRenderStack *Scene2DStack = &Scene2D->RenderStack;

    // Scene2D->Name = ts_create(MEMORY_PERMANENT, "2D Scene");
    Scene2D->Name = "2D Scene";
    Scene2D->type = GC_RENDER_OBJECT_2D;

    gl_initRenderStack(Scene2DStack, 10);
}

void updateScenes(engine_state_t *State)
{
    GlScene *SelectedScene = &State->Scenes[DEBUG_SELECTED_SCENE];

#if 0
    // debugging
    if (SelectedScene->Projection.type == GC_PROJECTION_PERSPECTIVE)
    {
        SelectedScene->Projection.Perspective.near += SelectedScene->Projection.nearDir * SelectedScene->Projection.nearSpeed;

        if (SelectedScene->Projection.Perspective.near > SelectedScene->Projection.nearThreshold ||
            SelectedScene->Projection.Perspective.near < 0.1f)
            SelectedScene->Projection.nearDir *= -1;
    }
    else if (SelectedScene->Projection.type == GC_PROJECTION_ORTHOGRAPHIC)
    {
        SelectedScene->Projection.Orthographic.near += SelectedScene->Projection.nearDir * SelectedScene->Projection.nearSpeed;

        if (SelectedScene->Projection.Orthographic.near > SelectedScene->Projection.nearThreshold ||
            SelectedScene->Projection.Orthographic.near < 0.1f)
            SelectedScene->Projection.nearDir *= -1;
    }
#endif

    // scene_addHeadAndChair(State, SelectedScene);
    // scene_addHead(State, SelectedScene);
    // scene_addChair(State, SelectedScene);
    // scene_addBigTriangle(State, SelectedScene);
    // scene_addPlane(State, SelectedScene);
    // scene_addOverlappedTriangles(State, SelectedScene);
    // scene_addTestPlane(State, SelectedScene);
    // scene_addAnimatedGrid(State, SelectedScene);
    scene_addGeneralGrid(State, SelectedScene);

    // ----------------------------------------------------------------------------------

    GlScene *Scene2D = &State->Scenes[SCENE_2D];
    GlRenderStack *Scene2DStack = &Scene2D->RenderStack;

    gl_resetRenderStack(Scene2DStack);

    GlRenderObject2D *TriangleObject = State->ObjectLibrary2D[OBJ_TRIANGLE];
    GlRenderObject2D *RectangleObject = State->ObjectLibrary2D[OBJ_RECTANGLE];
    GlRenderObject2D *WidthCheckObject = State->ObjectLibrary2D[OBJ_BITMAP_WIDTH_CHECK];
    GlRenderObject2D *BackgrounObject = State->ObjectLibrary2D[OBJ_BITMAP_RENEGADE_BACKGROUND];
    GlRenderObject2D *MusashiObject = State->ObjectLibrary2D[OBJ_BITMAP_MUSASHI];
    GlRenderObject2D *ArmyOfDarknessObject = State->ObjectLibrary2D[OBJ_BITMAP_ARMY_OF_DARKNESS];

    gl_pushRenderStack(Scene2DStack, TriangleObject);
    // gl_pushRenderStack(Scene2DStack, RectangleObject);
    // gl_pushRenderStack(Scene2DStack, WidthCheckObject);
    // gl_pushRenderStack(Scene2DStack, MusashiObject);
    // gl_pushRenderStack(Scene2DStack, BackgrounObject);
    gl_pushRenderStack(Scene2DStack, ArmyOfDarknessObject);
}