/* date = May 24th 2025 4:36 pm */

#ifndef TELEPORT_GAME_H
#define TELEPORT_GAME_H

#define ArrayCount(name) (sizeof(name)/sizeof(name[0]))
#define Assert(Value) if(!Value) {*(int *)0 = 0;}

#define FALSE 0
#define TRUE 1
#define false FALSE
#define true TRUE

#define internal static
#define global_variable static
#define local_persist static

typedef UINT8 uint8;
typedef UINT16 uint16;
typedef UINT32 uint32;
typedef UINT64 uint64;

typedef INT8 int8;
typedef INT16 int16;
typedef INT32 int32;
typedef INT64 int64;

typedef int32 bool32;
typedef float real32;
typedef double real64; 

typedef struct game_input
{
    bool32 Up;
    bool32 Down;
    bool32 Left;
    bool32 Right;
    bool32 Action;
} game_input;

#endif //TELEPORT_GAME_H
