// ----------------------------------------------------------------------------------
// -- File: gcsr_asset_builder.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-08-15 18:28:57
// -- Modified:
// ----------------------------------------------------------------------------------
#if 0
#include "gcsr_asset_builder.h"
#include "gcsr_xml_parser.cpp"
#include "gcsr_parser.cpp"
// #include "gcsr_text_config_parser.cpp"

static GlPixelFormat GlobalShifts;

char *strToUpper(char *s)
{
    char *res = s;

    if (s)
    {
        while (*s)
        {
            if (*s >= 'a' && *s <= 'z')
                *s = *s - 32;

            if (*s == '-')
                *s = '_';

            s++;
        }
    }

    return res;
}

FileRead readFile(char *filepath)
{
    FileRead Result;

    FILE *f = fopen(filepath, "rb");

    if (f)
    {
        fseek(f, 0, SEEK_END);
        Result.size = ftell(f);
        fseek(f, 0, SEEK_SET);

        Result.data = malloc(Result.size);
        fread(Result.data, Result.size, 1, f);
        fclose(f);
    }
    else
        printf("\nERROR: Cannot open file %s.", filepath);

    return Result;
}

GlPixelFormat getShiftsFromPixelFormat(SDL_PixelFormatEnum Format)
{
    GlPixelFormat Shifts;

    u32 type = SDL_PIXELTYPE(Format);
    u32 order = SDL_PIXELORDER(Format);
    u32 layout = SDL_PIXELLAYOUT(Format);

    if (type || order || layout) {}

    // NOTE(gabic): If more formats are needed they will be added here.

    switch (Format)
    {
        case SDL_PIXELFORMAT_ABGR8888:
        {
            Shifts.red_shift = 0;
            Shifts.green_shift = 8;
            Shifts.blue_shift = 16;
            Shifts.alpha_shift = 24;
        } break;

        case SDL_PIXELFORMAT_RGBA8888:
        {
            Shifts.red_shift = 24;
            Shifts.green_shift = 16;
            Shifts.blue_shift = 8;
            Shifts.alpha_shift = 0;
        } break;

        default:
        {
            Shifts.red_shift = 0;
            Shifts.green_shift = 8;
            Shifts.blue_shift = 16;
            Shifts.alpha_shift = 24;
        } break;
    }

    return Shifts;
}

GlPixelFormat extractBitmapMaskShifts(bitmap_header_t *Header)
{
    GlPixelFormat result;

    u32 alphaMask = ~(Header->redMask | Header->greenMask | Header->blueMask);

    b32 isRedSet = false;
    b32 isGreenSet = false;
    b32 isBlueSet = false;
    b32 isAlphaSet = false;

    for (u32 shift = 0; shift < 32; ++shift)
    {
        u32 value = 1 << shift;

        if (!isRedSet && (value & Header->redMask))
        {
            result.red_shift = shift;
            isRedSet = true;
        }

        if (!isGreenSet && (value & Header->greenMask))
        {
            result.green_shift = shift;
            isGreenSet = true;
        }

        if (!isBlueSet && (value & Header->blueMask))
        {
            result.blue_shift = shift;
            isBlueSet = true;
        }

        if (!isAlphaSet && (value & alphaMask))
        {
            result.alpha_shift = shift;
            isAlphaSet = true;
        }
    }

    return result;
}

b32 loadBitmap(char *filepath, loaded_bitmap_t *Bitmap)
{
    b32 result = false;
    FileRead F = readFile(filepath);

    if (F.data)
    {
        void *FileMemory = F.data;

        bitmap_header_t *Header = (bitmap_header_t *) FileMemory;
        assert(Header->fileType == 0x4d42);

        b32 isTopDown = true;

        if (Header->height > 0)
            isTopDown = false;

        Bitmap->Size.width = Header->width;
        Bitmap->Size.height = isTopDown ? Header->height * -1 : Header->height;
        Bitmap->bytes_per_pixel = Header->bitsPerPixel / 8;
        Bitmap->pitch = Bitmap->Size.width * Bitmap->bytes_per_pixel;
        Bitmap->widthOverHeight = (r32) Bitmap->Size.width / Bitmap->Size.height;
        Bitmap->bytes = Header->sizeOfBitmap;
        Bitmap->memory = malloc(Header->sizeOfBitmap);

        // -- Copy the bitmap pixel data.

        u32 tmpOffset = Header->bitmapOffset;
        s32 tmpPitch = Bitmap->pitch;
        u32 tmpWidth = Bitmap->Size.width;
        u32 tmpHeight = Bitmap->Size.height < 0 ? -1 * Bitmap->Size.height : Bitmap->Size.height;

        if (!isTopDown)
        {
            tmpOffset += (Bitmap->Size.height - 1) * Bitmap->pitch;
            tmpPitch *= -1;
        }

        u32 alphaMask = ~(Header->redMask | Header->greenMask | Header->blueMask);
        Bitmap->Shifts = extractBitmapMaskShifts(Header);

        // NOTE(gabic): Sa pun formatul intr-un loc comun.

        GlPixelFormat OutputShifts = getShiftsFromPixelFormat(GL_PIXEL_FORMAT);

        u8 *source = (u8 *) Header + tmpOffset;
        u8 *dest = (u8 *) Bitmap->memory;
        r32 oneOver255 = 1.0f / 255;

        for (u32 row = 0; row < tmpHeight; ++row)
        {
            u32 *pixel = (u32 *) source;
            u32 *destRow = (u32 *) dest;

            for (u32 col = 0; col < tmpWidth; ++col)
            {
                r32 red = (r32) (((*pixel & Header->redMask) >> Bitmap->Shifts.red_shift) & 0xFF);
                r32 green = (r32) (((*pixel & Header->greenMask) >> Bitmap->Shifts.green_shift) & 0xFF);
                r32 blue = (r32) (((*pixel & Header->blueMask) >> Bitmap->Shifts.blue_shift) & 0xFF);
                r32 alpha = (r32) (((*pixel & alphaMask) >> Bitmap->Shifts.alpha_shift) & 0xFF);

                vec4 Color = {red, green, blue, alpha};
                Color = sRGB_linear1(Color);

                Color.r *= Color.a;
                Color.g *= Color.a;
                Color.b *= Color.a;

                Color = linear1_sRGB255(Color);

                *destRow++ = ((u32) (Color.r + 0.5f) << OutputShifts.red_shift) |
                             ((u32) (Color.g + 0.5f) << OutputShifts.green_shift) |
                             ((u32) (Color.b + 0.5f) << OutputShifts.blue_shift) |
                             ((u32) (Color.a + 0.5f) << OutputShifts.alpha_shift);

                pixel++;
            }

            dest += Bitmap->pitch;
            source += tmpPitch;
        }

        free(FileMemory);
        result = true;
    }

    return result;
}

void generateAssetFile(AssetFileSources *Assets)
{
    FILE *OutHeader = fopen("src/gcsr_asset_enums.h", "w");
    FILE *Out = fopen("data/general.asset", "wb");

    asset_file_header_t *Header = (asset_file_header_t *) malloc(sizeof(asset_file_header_t));
    memset(Header, 0, sizeof(asset_file_header_t));

    Header->MagicValue = 'G' << 0 | 'C' << 8 | 'S' << 16 | 'R' << 24;
    Header->version = VERSION;
    Header->bitmapCount = Assets->bitmapCount;
    Header->textureCount = Assets->textureCount;
    Header->objectCount = Assets->objectCount;
    Header->modelCount = Assets->modelCount;
    Header->fontCount = Assets->fontCount;
    Header->soundCount = Assets->soundCount;
    Header->textCount = Assets->textCount;

    Header->bitmapAssetBytes = Header->bitmapCount * sizeof(AssetBitmap);
    Header->textureAssetBytes = Header->textureCount * sizeof(AssetTexture);
    Header->objectAssetBytes = Header->objectCount * sizeof(AssetObject);
    Header->modelAssetBytes = Header->modelCount * sizeof(AssetModel);
    Header->fontAssetBytes = Header->fontCount * sizeof(AssetFont);
    Header->soundAssetBytes = Header->soundCount * sizeof(AssetSound);
    Header->textAssetBytes = Header->textCount * sizeof(AssetText);

    Header->bitmapAssetOffset = sizeof(asset_file_header_t);
    Header->textureAssetOffset = Header->bitmapAssetOffset + Header->bitmapAssetBytes;
    Header->objectAssetOffset = Header->textureAssetOffset + Header->textureAssetBytes;
    Header->modelAssetOffset = Header->objectAssetOffset + Header->objectAssetBytes;
    Header->fontAssetOffset = Header->modelAssetOffset + Header->modelAssetBytes;
    Header->soundAssetOffset = Header->fontAssetOffset + Header->fontAssetBytes;
    Header->textAssetOffset = Header->soundAssetOffset + Header->soundAssetBytes;

    Header->totalAssetCount = Assets->bitmapCount + Assets->textureCount + Assets->objectCount +
                              Assets->modelCount + Assets->fontCount + Assets->soundCount + Assets->textCount;

    Header->assetsOffset = sizeof(asset_file_header_t);

    u32 assetsSize = Header->bitmapAssetBytes +
                     Header->textureAssetBytes +
                     Header->objectAssetBytes +
                     Header->modelAssetBytes +
                     Header->fontAssetBytes +
                     Header->soundAssetBytes +
                     Header->textAssetBytes;

    fwrite(Header, sizeof(asset_file_header_t), 1, Out);
    fseek(Out, assetsSize, SEEK_CUR);

    fputs("#ifndef GCSR_ASSET_ENUMS_H\n", OutHeader);
    fputs("#define GCSR_ASSET_ENUMS_H\n", OutHeader);

    // ----------------------------------------------------------------------------------
    // -- Bitmap files.
    // ----------------------------------------------------------------------------------

    if (Assets->bitmapCount)
        fputs("\nenum AssetBitmapName\n{", OutHeader);

    for (u32 i = 0; i < Assets->bitmapCount; ++i)
    {
        AssetSource *Source = &Assets->BitmapSources[i];
        AssetBitmap *Bitmap = &Assets->Bitmaps[i];

        fprintf(OutHeader, "\n\tBITMAP_%s,", strToUpper(Source->alias));

        loaded_bitmap_t BitmapFile;
        b32 res = loadBitmap(Source->source, &BitmapFile);

        if (res)
        {
            Bitmap->bytes_per_pixel = BitmapFile.bytes_per_pixel;
            Bitmap->pitch = BitmapFile.pitch;
            Bitmap->width = BitmapFile.Size.width;
            Bitmap->height = BitmapFile.Size.height;

            Bitmap->redMask = BitmapFile.Shifts.red_shift;
            Bitmap->greenMask = BitmapFile.Shifts.green_shift;
            Bitmap->blueMask = BitmapFile.Shifts.blue_shift;
            Bitmap->bytes = BitmapFile.bytes;

            Bitmap->Base.dataOffset = ftell(Out);

            fwrite(BitmapFile.memory, Bitmap->bytes, 1, Out);
            free(BitmapFile.memory);
        }
    }

    if (Assets->bitmapCount)
    {
        fputs("\n\n\tBITMAP_COUNT", OutHeader);
        fputs("\n};\n", OutHeader);
    }

    // ----------------------------------------------------------------------------------
    // -- Obj files.
    // ----------------------------------------------------------------------------------

    if (Assets->objectCount)
        fputs("\nenum AssetObjectName\n{", OutHeader);

    for (u32 i = 0; i < Assets->objectCount; ++i)
    {
        AssetSource *Source = &Assets->ObjectSources[i];
        AssetObject *Object = &Assets->Objects[i];

        fprintf(OutHeader, "\n\tOBJECT_%s,", strToUpper(Source->alias));

        FileRead objf = readFile(Source->source);

        GC_Parser *ObjParser = GC_Parser_create(PARSER_OBJ, Megabytes(4));
        GC_Parser_Obj *ParserData = (GC_Parser_Obj *) ObjParser;
        GC_Parser_setFile(ObjParser, objf);
        b32 res = GC_Parser_parse(ObjParser);

        if (res)
        {
            Object->vertexCount = ParserData->vertexCount;
            Object->faceCount = ParserData->faceCount;
            Object->uvCount = ParserData->uvCount;
            Object->normalCount = ParserData->normalCount;
            Object->faceComponents = ParserData->faceComponents;
            Object->bytes = VERTEX_BYTES * ParserData->vertexCount +
                            UV_BYTES * ParserData->uvCount +
                            NORMAL_BYTES * ParserData->normalCount;

            if (Object->faceComponents & FACE_VERTEX)
                Object->bytes += FACE_VERTICES_BYTES * ParserData->faceCount;

            if (Object->faceComponents & FACE_UV)
                Object->bytes += FACE_UV_BYTES * ParserData->faceCount;

            if (Object->faceComponents & FACE_NORMAL)
                Object->bytes += FACE_NORMAL_BYTES * ParserData->faceCount;

            Object->Base.dataOffset = ftell(Out);

            fwrite(ParserData->vertices, VERTEX_BYTES * ParserData->vertexCount, 1, Out);
            fwrite(ParserData->uvs, UV_BYTES * ParserData->uvCount, 1, Out);
            fwrite(ParserData->normals, NORMAL_BYTES * ParserData->normalCount, 1, Out);

            if (Object->faceComponents & FACE_VERTEX)
                fwrite(ParserData->facesVertices, FACE_VERTICES_BYTES * ParserData->faceCount, 1, Out);

            if (Object->faceComponents & FACE_UV)
                fwrite(ParserData->facesUVs, FACE_UV_BYTES * ParserData->faceCount, 1, Out);

            if (Object->faceComponents & FACE_NORMAL)
                fwrite(ParserData->facesNormals, FACE_NORMAL_BYTES * ParserData->faceCount, 1, Out);
        }

        GC_Parser_destroy(ObjParser);
    }

    if (Assets->objectCount)
    {
        fputs("\n\n\tOBJECT_COUNT", OutHeader);
        fputs("\n};\n", OutHeader);
    }

    // ----------------------------------------------------------------------------------
    // -- Text files.
    // ----------------------------------------------------------------------------------

    if (Assets->textCount)
        fputs("\nenum AssetTextName\n{", OutHeader);

    for (u32 i = 0; i < Assets->textCount; ++i)
    {
        AssetSource *Source = &Assets->TextSources[i];
        AssetText *Text = &Assets->Texts[i];

        fprintf(OutHeader, "\n\tTEXT_%s,", strToUpper(Source->alias));

        FileRead File = readFile(Source->source);

        GC_Parser *TextConfigParser = GC_Parser_create(PARSER_TEXT_CONFIG, 0);
        GC_Parser_TextConfig *ParserData = (GC_Parser_TextConfig *) TextConfigParser;
        GC_Parser_setFile(TextConfigParser, File);
        b32 res = GC_Parser_parse(TextConfigParser);

        if (res)
        {
            Text->count = ParserData->count;
            Text->Base.dataOffset = ftell(Out);
            Text->keyOffset = Text->Base.dataOffset;
            Text->valueOffset = Text->Base.dataOffset + Text->count * sizeof(AssetTextKey);

            // ----------------------------------------------------------------------------------
            // -- Write the keys.
            // ----------------------------------------------------------------------------------

            AssetTextEntry *Start = ParserData->Lines;
            size_t offset = Text->valueOffset;

            for (u32 j = 0; j < Text->count; ++j)
            {
                Start->Key.valueOffset = offset;
                fwrite(&Start->Key, sizeof(AssetTextKey), 1, Out);
                offset += sizeof(size_t) + Start->valueBytes;
                Start++;
            }

            // ----------------------------------------------------------------------------------
            // -- Write the values.
            // ----------------------------------------------------------------------------------

            Start = ParserData->Lines;

            for (u32 j = 0; j < Text->count; ++j)
            {
                fwrite(&Start->valueBytes, sizeof(Start->valueBytes), 1, Out);
                fwrite(Start->value, Start->valueBytes, 1, Out);
                Start++;
            }
        }

        GC_Parser_destroy(TextConfigParser);

        // LoadedText *TextFile = (LoadedText *) malloc(sizeof(LoadedText));
        // memset(TextFile, 0, sizeof(LoadedText));

        // b32 res = loadText(Source->source, TextFile);

        // if (res)
        // {
        //     Text->count = TextFile->count;
        //     Text->dataOffset = ftell(Out);

        //     fwrite(&TextFile->entries, sizeof(AssetTextEntry) * TextFile->count, 1, Out);
        //     free(TextFile);
        // }
    }

    if (Assets->textCount)
    {
        fputs("\n\n\tTEXT_COUNT", OutHeader);
        fputs("\n};\n", OutHeader);
    }

    fseek(Out, sizeof(asset_file_header_t), SEEK_SET);
    fwrite(Assets->Bitmaps, Header->bitmapAssetBytes, 1, Out);
    fwrite(Assets->Objects, Header->objectAssetBytes, 1, Out);
    fwrite(Assets->Texts, Header->textAssetBytes, 1, Out);

    // ----------------------------------------------------------------------------------
    // -- Obj files.
    // ----------------------------------------------------------------------------------

    fclose(Out);

    fputs("#endif", OutHeader);
    fclose(OutHeader);
}

AssetFileSources *createBuilderSources()
{
    AssetFileSources *Assets = (AssetFileSources *) malloc(sizeof(AssetFileSources));
    memset(Assets, 0, sizeof(AssetFileSources));

    XMLDocument _Doc;
    XMLDocument *Doc = &_Doc;

    u32 _id = 1;

    XMLParser_init();

    b32 res = XMLDocument_load(Doc, "asset_builder_config.xml");

    if (res)
    {
        if (strcmp(Doc->Root->tag, "asset-builder") == 0)
        {
            XMLNode *Root = Doc->Root;
            XMLNode *Node = 0;

            // -- Base level.
            if (Root->childrenCount > 0)
            {
                Node = Root->Children;

                while (Node)
                {
                    if (Node->childrenCount > 0)
                    {
                        XMLNode *AssetEntry = Node->Children;

                        while (AssetEntry)
                        {
                            if (AssetEntry->childrenCount != 2)
                            {
                                printf("\nERROR: Invalid asset entry !");
                                exit(EXIT_FAILURE);
                            }

                            // -- Bitmaps.
                            if (strcmp(Node->tag, AssetTags[ASSET_BITMAP]) == 0)
                            {
                                Assets->bitmapCount++;
                                u32 index = Assets->bitmapCount - 1;
                                AssetSource *Source = &Assets->BitmapSources[index];
                                AssetBitmap *Asset = &Assets->Bitmaps[index];

                                Source->type = ASSET_BITMAP;
                                Asset->Base.type = ASSET_BITMAP;
                                Asset->Base.id = _id++;

                                char *alias = AssetEntry->Children->innerText;
                                char *file = AssetEntry->Children->Next->innerText;

                                strncpy(Source->alias, alias, MAX_ASSET_STRING_SIZE);
                                strncpy(Source->source, file, MAX_ASSET_STRING_SIZE);
                                strncpy(Asset->Base.alias, alias, MAX_ASSET_STRING_SIZE);
                            }

                            // -- Textures.
                            else if (strcmp(Node->tag, AssetTags[ASSET_TEXTURE]) == 0)
                            {}

                            // -- Obj.
                            else if (strcmp(Node->tag, AssetTags[ASSET_OBJ]) == 0)
                            {
                                Assets->objectCount++;
                                u32 index = Assets->objectCount - 1;
                                AssetSource *Source = &Assets->ObjectSources[index];
                                AssetObject *Asset = &Assets->Objects[index];

                                Source->type = ASSET_OBJ;
                                Asset->Base.type = ASSET_OBJ;
                                Asset->Base.id = _id++;

                                char *alias = AssetEntry->Children->innerText;
                                char *file = AssetEntry->Children->Next->innerText;

                                strncpy(Source->alias, alias, MAX_ASSET_STRING_SIZE);
                                strncpy(Source->source, file, MAX_ASSET_STRING_SIZE);
                                strncpy(Asset->Base.alias, alias, MAX_ASSET_STRING_SIZE);
                            }

                            // -- Model.
                            else if (strcmp(Node->tag, AssetTags[ASSET_MODEL]) == 0)
                            {}

                            // -- Font.
                            else if (strcmp(Node->tag, AssetTags[ASSET_FONT]) == 0)
                            {}

                            // -- Sound.
                            else if (strcmp(Node->tag, AssetTags[ASSET_SOUND]) == 0)
                            {}

                            // -- Text.
                            else if (strcmp(Node->tag, AssetTags[ASSET_TEXT]) == 0)
                            {
                                Assets->textCount++;
                                u32 index = Assets->textCount - 1;
                                AssetSource *Source = &Assets->TextSources[index];
                                AssetText *Asset = &Assets->Texts[index];

                                Source->type = ASSET_TEXT;
                                Asset->Base.type = ASSET_TEXT;
                                Asset->Base.id = _id++;

                                char *alias = AssetEntry->Children->innerText;
                                char *file = AssetEntry->Children->Next->innerText;

                                strncpy(Source->alias, alias, MAX_ASSET_STRING_SIZE);
                                strncpy(Source->source, file, MAX_ASSET_STRING_SIZE);
                                strncpy(Asset->Base.alias, alias, MAX_ASSET_STRING_SIZE);
                            }

                            AssetEntry = AssetEntry->Next;
                        }
                    }

                    Node = Node->Next;
                }

            }
        }
        else
            printf("\nERROR: Invalid xml root tag: %s !", Doc->Root->tag);
    }

    XMLParser_free();

    return Assets;
}

#define MAP_SIZE 1024

u64 hash_djb2(char *str)
{
    u32 c;
    u64 hash = 5381;
    u32 len = (u32) strlen(str);

    for (u32 i = 0; i < len; ++i)
    {
        c = str[i];
        hash = ((hash << 5) + hash) + c;
    }

    hash = hash % MAP_SIZE;

    return hash;
}

int main(int argc, char *argv[])
{
    AssetFileSources *Assets = createBuilderSources();
    generateAssetFile(Assets);

    free(Assets);

    // ----------------------------------------------------------------------------------

    FileRead tf1 = readFile("data/texts/test_1.txt");
    GC_Parser_TextConfig *TextParser = (GC_Parser_TextConfig *) GC_Parser_create(PARSER_TEXT_CONFIG, 0);

    GC_Parser_setFile(BaseParser(TextParser), tf1);
    b32 res = GC_Parser_parse(BaseParser(TextParser));
    GC_Parser_destroy(BaseParser(TextParser));

    // FileRead objf = readFile("data/objects/african_head.obj");
    FileRead objf = readFile("data/objects/test.obj");
    GC_Parser *ObjParser = GC_Parser_create(PARSER_OBJ, Megabytes(4));
    GC_Parser_setFile(ObjParser, objf);
    GC_Parser_parse(ObjParser);

    GC_Parser_destroy(ObjParser);

    // ObjParser->vertices = (r32 (*)[3]) GC_Parser_allocate(BaseParser(ObjParser), sizeof(r32) * 3);
    // r32 (* tmp)[3] = (r32 (*)[3]) GC_Parser_allocate(BaseParser(ObjParser), sizeof(r32) * 3);
    // tmp = (r32 (*)[3]) GC_Parser_allocate(BaseParser(ObjParser), sizeof(r32) * 3);

    // ----------------------------------------------------------------------------------

    FileRead f = readFile("data/general.asset");

    if (f.size > 0)
    {
        asset_file_header_t *Header = (asset_file_header_t *) f.data;
        void *data = (u8 *) Header + Header->assetsOffset;
        AssetBitmap *Bitmap = (AssetBitmap *) data;
        AssetText *Text = 0;
        AssetTextValue *Test = (AssetTextValue *) Header;

        for (u32 i = 0; i < Header->bitmapCount; ++i)
        {
            Bitmap++;
        }

        Text = (AssetText *) Bitmap;

        for (u32 i = 0; i < Header->textCount; ++i)
        {
            Text++;
        }
    }

    return 0;
}
#endif