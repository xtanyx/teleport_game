/* date = May 24th 2025 6:32 pm */

#ifndef WIN32_TELEPORT_GAME_H
#define WIN32_TELEPORT_GAME_H

#define PATH_LENGTH MAX_PATH

typedef struct win32_rendering_buffer
{
    void *Memory;
    int32 Width;
    int32 Height;
    int32 Pitch;
    int32 BytesPerPixel;
    BITMAPINFO BitmapInfo;
} win32_rendering_buffer;

typedef struct win32_paths
{
    char BuildDirPath[PATH_LENGTH];
    char DataDirPath[PATH_LENGTH];
    char ExePath[PATH_LENGTH];
    char GameRootPath[PATH_LENGTH];
} win32_paths;

#pragma pack(push, 1)
typedef struct bitmap_header
{
    WORD   FileType;
	DWORD  FileSize;
	WORD   Reserved1;
	WORD   Reserved2;
	DWORD  BitmapOffset;
    DWORD Size;
	LONG  Width;
	LONG  Height;
	WORD  Planes;
	WORD  BitsPerPixel;
} bitmap_header;
#pragma pack(pop)

typedef struct bmp_properties
{
    int32 X;
    int32 Y;
    int32 Width;
    int32 Height;
} bmp_properties;

#endif //WIN32_TELEPORT_GAME_H
