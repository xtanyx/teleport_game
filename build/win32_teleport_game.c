#include <windows.h>
#include "teleport_game.h"
#include <stdio.h>
#include "win32_teleport_game.h"

global_variable bool32 GlobalRunning;
global_variable LARGE_INTEGER GlobalFrequency;
global_variable win32_rendering_buffer GlobalPixelBuffer;

internal void Win32InitRendering(win32_rendering_buffer *Buffer, int32 Width, int32 Height)
{
    BITMAPINFOHEADER *BitmapInfoHeader = &Buffer->BitmapInfo.bmiHeader;
    
    BitmapInfoHeader->biSize = sizeof(*BitmapInfoHeader);
    BitmapInfoHeader->biWidth = Width;
    BitmapInfoHeader->biHeight = -Height;
    BitmapInfoHeader->biPlanes = 1;
    BitmapInfoHeader->biBitCount = 32;
    BitmapInfoHeader->biCompression = BI_RGB;
    BitmapInfoHeader->biSizeImage = 0;
    BitmapInfoHeader->biXPelsPerMeter = 0;
    BitmapInfoHeader->biYPelsPerMeter = 0;
    BitmapInfoHeader->biClrUsed = 0;
    BitmapInfoHeader->biClrImportant = 0;
    
    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    Buffer->Pitch = Width * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, (Buffer->Width * Buffer->Height * Buffer->BytesPerPixel), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, win32_rendering_buffer *Buffer)
{
    StretchDIBits(
                  DeviceContext,
                  0, 0, Buffer->Width, Buffer->Height,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->BitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY
                  );
}

LRESULT Win32WindowProcedure(
                             HWND Handle,
                             UINT Message,
                             WPARAM WParam,
                             LPARAM LParam
                             )
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            Result = 0;
        }break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Handle, &Paint);
            Win32UpdateWindow(DeviceContext, &GlobalPixelBuffer);
            EndPaint(Handle, &Paint);
        }break;
        default:
        {
            Result = DefWindowProcA(Handle, Message, WParam, LParam);
        }break;
    }
    
    return Result;
}

internal void Win32ProcessKeyboardMessage(WPARAM WParam, LPARAM LParam, game_input *Input)
{
    WPARAM VKCode = WParam;
    bool32 IsDown = ((LParam & (1 << 31)) == 0);
    
    
    switch(VKCode)
    {
        case 'W':
        {
            Input->Up = IsDown;
        }break;
        case 'A':
        {
            Input->Left = IsDown;
        }break;
        case 'S':
        {
            Input->Down = IsDown;
        }break;
        case 'D':
        {
            Input->Right = IsDown;
        }break;
        case 'X':
        {
            Input->Action = IsDown;
        }break;
        default:
        {
            
        }break;
    }
}

internal LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    
    return Result;
}

internal real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalFrequency.QuadPart);
    
    return Result;
}

internal void Win32DrawRectangle(win32_rendering_buffer *Buffer, int32 Left, int32 Top, int32 Width, int32 Height, int32 RectColor)
{
    if(Left < 0)
    {
        Left = 0;
    }
    if((Left + Width) > Buffer->Width)
    {
        Left = Buffer->Width - Width;
    }
    if(Top < 0)
    {
        Top = 0;
    }
    if((Top + Height) > Buffer->Height)
    {
        Top = Buffer->Height - Height;
    }
    
    uint8 *Row = (uint8 *)Buffer->Memory + (Top * Buffer->Pitch) + (Left * Buffer->BytesPerPixel);
    for(int32 Y = Top; Y < (Top + Height); ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int32 X = Left; X < (Left + Width); ++X)
        {
            *Pixel++ = RectColor;
        }
        Row += Buffer->Pitch;
    }
}

internal void RectCopyToRenderBuffer(win32_rendering_buffer *RenderBuffer, void *RectBuffer, int32 Left, int32 Top, int32 Width, int32 Height, int32 RectColor, uint32 BackgroundColor)
{
    uint8 *Row = (uint8 *)RenderBuffer->Memory + (Top * RenderBuffer->Pitch) + (Left * RenderBuffer->BytesPerPixel);
    uint32 *RectPixel = (uint32 *)RectBuffer;
    for(int32 Y = Top; Y < (Top + Height); ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int32 X = Left; X < (Left + Width); ++X)
        {
            if(*RectPixel == 0)
            {
                *Pixel++ = BackgroundColor;
                RectPixel++;
            }
            else
            {
                *Pixel++ = *RectPixel++;
            }
        }
        Row += RenderBuffer->Pitch;
    }
}

internal void RenderBitmap(win32_rendering_buffer *RenderBuffer, void *BitmapBuffer, int32 Left, int32 Top, int32 Width, int32 Height, int32 BackgroundColor)
{
    uint8 *Row = (uint8 *)RenderBuffer->Memory + (Top * RenderBuffer->Pitch) + (Left * RenderBuffer->BytesPerPixel);
    uint32 *BitmapPixel = (uint32 *)BitmapBuffer + (Height * Width) - Width;
    for(int32 Y = Top; Y < (Top + Height); ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int32 X = Left; X < (Left + Width); ++X)
        {
            if(*BitmapPixel == 0)
            {
                *Pixel++ = BackgroundColor;
                ++BitmapPixel;
            }
            else
            {
                *Pixel++ = *BitmapPixel++;
            }
        }
        Row += RenderBuffer->Pitch;
        BitmapPixel -= (2 * Width);
    }
}

internal bool32 Win32CloseFileHandle(HANDLE FileHandle)
{
    return CloseHandle(FileHandle);
}

internal void *Win32ReadEntireFile(char *FileName)
{
    void *Buffer = 0;
    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            DWORD BytesToRead = (DWORD)FileSize.QuadPart;
            if(BytesToRead == FileSize.QuadPart)
            {
                Buffer = VirtualAlloc(0, BytesToRead, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                
                DWORD BytesRead;
                if(ReadFile(FileHandle, Buffer, BytesToRead, &BytesRead, 0) && (BytesToRead == BytesRead))
                {
                    Win32CloseFileHandle(FileHandle);
                }
            }
        }
    }
    
    return Buffer;
}

internal uint32 StringLength(char *String)
{
    uint32 Length = 0;
    for(char *Char = String; *Char != '\0'; ++Char)
    {
        ++Length;
    }
    
    return Length;
}

internal void StringConcat(char *String1, char *String2, char *OutputString)
{
    int OutputStringIndex = 0;
    for(char *Char = String1; *Char != '\0'; ++Char)
    {
        OutputString[OutputStringIndex++] = *Char;
    }
    
    for(char *Char = String2; *Char != '\0'; ++Char)
    {
        OutputString[OutputStringIndex++] = *Char;
    }
    OutputString[OutputStringIndex] = '\0';
}

internal void GetParentFolderPath(char *Path, char *OutputPath)
{
    int32 CurrentSlashIndex = 0;
    int32 PrevSlashIndex = 0;
    int32 PathIndex = 0;
    for(char *Char = Path; *Char != '\0'; ++Char)
    {
        if(*Char == '\\')
        {
            if(PrevSlashIndex != CurrentSlashIndex)
            {
                PrevSlashIndex = CurrentSlashIndex;
            }
            CurrentSlashIndex = PathIndex;
        }
        ++PathIndex;
    }
    
    for(int32 Index = 0; Index <= PrevSlashIndex; ++Index)
    {
        OutputPath[Index] = Path[Index];
    }
    OutputPath[PrevSlashIndex + 1] = '\0';
}

internal void GetDirPath(char *FilePath, char *DirPath)
{
    int32 LastSlashIndex = 0;
    int32 PathIndex = 0;
    for(char *Char = FilePath; *Char != '\0'; ++Char)
    {
        if(*Char == '\\')
        {
            LastSlashIndex = PathIndex++;
        }
    }
    
    for(int32 Index = 0; Index <= LastSlashIndex; ++Index)
    {
        DirPath[Index] = FilePath[Index];
    }
    DirPath[LastSlashIndex + 1] = '\0';
}

internal void GetFilePath(char *FilePath, char *FileName, char *OutputPath)
{
    char DirPath[PATH_LENGTH];
    GetDirPath(FilePath, DirPath);
    StringConcat(DirPath, FileName, OutputPath);
}

internal bool32 IsTileEmpty(uint32 *TileMap, int32 TestX, int32 TestY)
{
    bool32 Empty = false;
    int32 TileX = (TestX / 100);
    int32 TileY = (TestY / 100);
    
    if(TileX >= 0 && TileX < 17 && TileY >= 0 && TileY < 11)
    {
        Empty = ((TileMap[(TileY * 17) + TileX]) == 0);
    }
    
    return Empty;
}

internal bool32 AreBitmapsColliding(bmp_properties *Bitmap1, bmp_properties *Bitmap2)
{
    bool32 AreColliding = false;
    if((Bitmap1->X >= Bitmap2->X && Bitmap1->X <= (Bitmap2->X + Bitmap2->Width)) ||
       ((Bitmap1->X + Bitmap1->Width) >= Bitmap2->X && (Bitmap1->X + Bitmap1->Width) <= (Bitmap2->X + Bitmap2->Width)))
    {
        if((Bitmap1->Y >= Bitmap2->Y && Bitmap1->Y <= (Bitmap2->Y + Bitmap2->Height)) ||
           ((Bitmap1->Y + Bitmap1->Height) >= Bitmap2->Y && (Bitmap1->Y + Bitmap1->Height) <= (Bitmap2->Y + Bitmap2->Height)))
        {
            AreColliding = true;
        }
    }
    
    return AreColliding;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, int nCmdShow)
{
    //Setting schedular time period
    bool32 SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);
    
    //Setting timer frequency
    QueryPerformanceFrequency(&GlobalFrequency);
    
    //Init exe-file and dir paths
    win32_paths Paths = {};
    GetModuleFileNameA(hInstance, Paths.ExePath, PATH_LENGTH);
    GetParentFolderPath(Paths.ExePath, Paths.GameRootPath);
    StringConcat(Paths.GameRootPath, "data\\", Paths.DataDirPath);
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WindowProcedure;
    WindowClass.hInstance = hInstance;
    WindowClass.lpszClassName = "teleport_game_class";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowExA(
                                            0,
                                            WindowClass.lpszClassName,
                                            "Teleport_Game",
                                            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            0,
                                            0,
                                            hInstance,
                                            0);
        
        if(WindowHandle)
        {
            //Init rendering
            HDC ResolutionDeviceContext = GetDC(WindowHandle);
            uint32 ScreenWidth = GetDeviceCaps(ResolutionDeviceContext, HORZRES);
            uint32 ScreenHeight = GetDeviceCaps(ResolutionDeviceContext, VERTRES);
            Win32InitRendering(&GlobalPixelBuffer, ScreenWidth, ScreenHeight);
            ReleaseDC(WindowHandle, ResolutionDeviceContext);
            
            
            int32 GameUpdateHz = 60;
            real32 TargetSecondsPerFrame = (1.0f / (real32)GameUpdateHz);
            
            game_input Input = {};
            game_input *NewInput = &Input;
            
            //Read bitmaps
            char CoinBmpPath[PATH_LENGTH];
            StringConcat(Paths.DataDirPath, "coin_00.bmp", CoinBmpPath);
            void *CoinFileContents = Win32ReadEntireFile(CoinBmpPath);
            bitmap_header *CoinBitmapHeader = (bitmap_header *)CoinFileContents;
            
            char PlayerBmpPath[PATH_LENGTH];
            StringConcat(Paths.DataDirPath, "pc.bmp", PlayerBmpPath);
            void *PlayerFileContents = Win32ReadEntireFile(PlayerBmpPath);
            bitmap_header *PlayerBitmapHeader = (bitmap_header *)PlayerFileContents;
            
            char PortalBmpPath[PATH_LENGTH];
            StringConcat(Paths.DataDirPath, "portal.bmp", PortalBmpPath);
            void *PortalFileContents = Win32ReadEntireFile(PortalBmpPath);
            bitmap_header *PortalBitmapHeader = (bitmap_header *)PortalFileContents;
            
            GlobalRunning = true;
            
            //Initialize bitmap postions
            bmp_properties Player;
            Player.X = 179;
            Player.Y = 105;
            Player.Width = PlayerBitmapHeader->Width;
            Player.Height = PlayerBitmapHeader->Height;
            
            int32 BackgroundColor = 0x00333333;
            
            LARGE_INTEGER LastCounter = Win32GetWallClock();
            
            bmp_properties Coin;
            srand(Win32GetWallClock().LowPart);
            Coin.X = 300;
            Coin.Y = 300;
            Coin.Width = CoinBitmapHeader->Width;
            Coin.Height = CoinBitmapHeader->Height;
            
            bmp_properties Portals[5];
            Portals[0].X = 105;
            Portals[0].Y = 105;
            Portals[0].Width = PortalBitmapHeader->Width;
            Portals[0].Height = PortalBitmapHeader->Height;
            
            Portals[1] = Portals[0];
            Portals[1].X = 825;
            Portals[1].Y = 765;
            
            Portals[2] = Portals[0];
            Portals[2].X = 1525;
            Portals[2].Y = 250;
            
            Portals[3] = Portals[0];
            Portals[3].X = 405;
            Portals[3].Y = 505;
            
            Portals[4] = Portals[0];
            Portals[4].X = 1105;
            Portals[4].Y = 765;
            
            int32 Score = 0;
            
            while(GlobalRunning)
            {
                MSG Message;
                while(PeekMessageA(&Message,0,0,0,PM_REMOVE))
                {
                    switch(Message.message)
                    {
                        case WM_QUIT:
                        {
                            GlobalRunning = false;
                            //MessageBoxA(0, "This is a teleport game!", "Teleport Game", MB_OK);
                        }break;
                        case WM_SYSKEYUP:
                        case WM_SYSKEYDOWN:
                        case WM_KEYUP:
                        case WM_KEYDOWN:
                        {
                            Win32ProcessKeyboardMessage(Message.wParam, Message.lParam, NewInput);
                        }break;
                        default:
                        {
                            TranslateMessage(&Message);
                            DispatchMessage(&Message);
                        }break;
                    }
                }
                
                uint32 TileMap[10][17] = 
                {
                    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
                    {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1},
                    {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1},
                    {1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1},
                    {1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1},
                    {1,0,0,0,0,1,1,0,0,0,1,0,0,0,1,1,1},
                    {1,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,1},
                    {1,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,1},
                    {1,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,1},
                    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
                };
                
                int32 TileWidth = 100;
                int32 TileHeight = 100;
                for(int32 Y = 0; Y < 10; ++Y)
                {
                    for(int32 X = 0; X < 17; ++X)
                    {
                        int32 TileColor = 0x00333333;
                        if(TileMap[Y][X] == 1)
                        {
                            TileColor = 0x00000000;
                        }
                        int32 TileLeft = X * TileWidth;
                        int32 TileTop = Y * TileHeight;
                        Win32DrawRectangle(&GlobalPixelBuffer, TileLeft, TileTop, TileWidth, TileHeight, TileColor);
                        
                    }
                }
                
                int32 dRectX = 0;
                int32 dRectY = 0;
                if(NewInput->Up)
                {
                    dRectY = -1;
                }
                if(NewInput->Down)
                {
                    dRectY = 1;
                }
                if(NewInput->Left)
                {
                    dRectX = -1;
                }
                if(NewInput->Right)
                {
                    dRectX = 1;
                }
                
                int32 TestX = Player.X + (int32)(dRectX * TargetSecondsPerFrame * 175.0f);
                int32 TestY = Player.Y + (int32)(dRectY * TargetSecondsPerFrame * 175.0f);
                
                
                if(IsTileEmpty((uint32 *)TileMap, TestX, TestY) && IsTileEmpty((uint32 *)TileMap, TestX + Player.Width, TestY) && IsTileEmpty((uint32 *)TileMap, TestX, TestY + Player.Height) && IsTileEmpty((uint32 *)TileMap, TestX + Player.Width, TestY + Player.Height))
                {
                    Player.X = TestX;
                    Player.Y = TestY;
                }
                
                //Portal Collision Checking
                for(int32 PortalIndex = 0; PortalIndex < ArrayCount(Portals); ++PortalIndex)
                {
                    if(AreBitmapsColliding(&Portals[PortalIndex], &Player) && Input.Action)
                    {
                        int32 NewPortalIndex = PortalIndex + 1;
                        if(NewPortalIndex == ArrayCount(Portals))
                        {
                            NewPortalIndex = 0;
                        }
                        
                        int32 Counter = 0;
                        int32 Padding = 5;
                        do
                        {
                            ++Counter;
                            switch(Counter)
                            {
                                case 1:
                                {
                                    Player.X = Portals[NewPortalIndex].X + Portals[NewPortalIndex].Width + Padding;
                                    Player.Y = Portals[NewPortalIndex].Y;
                                }break;
                                case 2:
                                {
                                    Player.X = Portals[NewPortalIndex].X;
                                    Player.Y = Player.Y = Portals[NewPortalIndex].Y + Portals[NewPortalIndex].Height + Padding;
                                }break;
                                case 3:
                                {
                                    Player.Y = Portals[NewPortalIndex].Y;
                                    Player.X = Portals[NewPortalIndex].X - Player.Width - Padding;
                                }break;
                                case 4:
                                {
                                    Player.X = Portals[NewPortalIndex].X;
                                    Player.Y = Portals[NewPortalIndex].Y - Player.Height - Padding;
                                }break;
                            }
                        } while(!(IsTileEmpty((uint32 *)TileMap, Player.X, Player.Y) && IsTileEmpty((uint32 *)TileMap, Player.X + Player.Width, Player.Y) && IsTileEmpty((uint32 *)TileMap, Player.X, Player.Y + Player.Height) && IsTileEmpty((uint32 *)TileMap, Player.X + Player.Width, Player.Y + Player.Height)));
                        
                    }
                }
                
                //Coin Collision checking
                if(AreBitmapsColliding(&Coin, &Player))
                {
                    ++Score;
                    
                    do
                    {
                        real32 RandX = (real32)rand() / (real32)RAND_MAX;
                        real32 RandY = (real32)rand() / (real32)RAND_MAX;
                        
                        Coin.X = (int32)(RandX * (GlobalPixelBuffer.Width - CoinBitmapHeader->Width));
                        Coin.Y = (int32)(RandY * (GlobalPixelBuffer.Height - CoinBitmapHeader->Height));
                    } while(!(IsTileEmpty((uint32 *)TileMap, Coin.X, Coin.Y) && IsTileEmpty((uint32 *)TileMap, (Coin.X + Coin.Width), Coin.Y) && IsTileEmpty((uint32 *)TileMap, Coin.X, Coin.Y + Coin.Height) && IsTileEmpty((uint32 *)TileMap, Coin.X + Coin.Width, Coin.Y + Coin.Height)));
                }
                
                
                //Render bitmaps
                RenderBitmap(&GlobalPixelBuffer, (char *)PortalFileContents + PortalBitmapHeader->BitmapOffset, Portals[0].X, Portals[0].Y, Portals[0].Width, Portals[0].Height, BackgroundColor);
                RenderBitmap(&GlobalPixelBuffer, (char *)PortalFileContents + PortalBitmapHeader->BitmapOffset, Portals[1].X, Portals[1].Y, Portals[1].Width, Portals[1].Height, BackgroundColor);
                RenderBitmap(&GlobalPixelBuffer, (char *)PortalFileContents + PortalBitmapHeader->BitmapOffset, Portals[2].X, Portals[2].Y, Portals[2].Width, Portals[2].Height, BackgroundColor);
                RenderBitmap(&GlobalPixelBuffer, (char *)PortalFileContents + PortalBitmapHeader->BitmapOffset, Portals[3].X, Portals[3].Y, Portals[3].Width, Portals[3].Height, BackgroundColor);
                RenderBitmap(&GlobalPixelBuffer, (char *)PortalFileContents + PortalBitmapHeader->BitmapOffset, Portals[4].X, Portals[4].Y, Portals[4].Width, Portals[4].Height, BackgroundColor);
                RenderBitmap(&GlobalPixelBuffer, (char *)PlayerFileContents + PlayerBitmapHeader->BitmapOffset, Player.X, Player.Y, PlayerBitmapHeader->Width, PlayerBitmapHeader->Height, BackgroundColor);
                RenderBitmap(&GlobalPixelBuffer, (char *)CoinFileContents + CoinBitmapHeader->BitmapOffset, Coin.X, Coin.Y, CoinBitmapHeader->Width, CoinBitmapHeader->Height, BackgroundColor);
                
                //Display score
                RECT TextRect;
                TextRect.left = 800;
                TextRect.top = 0;
                TextRect.right = 900;
                TextRect.bottom = 100;
                
                char TextBuffer[256];
                sprintf_s(TextBuffer, ArrayCount(TextBuffer), "Score: %d", Score);
                
                HDC TextDeviceContext = GetDC(WindowHandle);
                TextOutA(TextDeviceContext, 800, 50, TextBuffer, StringLength(TextBuffer));
                //RectCopyToRenderBuffer(&GlobalPixelBuffer, TextRect.left, TextRect.top, TextRect.right - TextRect.left, TextRect.bottom - TextRect.top, );
                
                
                //Sleep
                LARGE_INTEGER WorkCounter = Win32GetWallClock();
                real32 WorkSeconds = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                
                real32 SecondsForWork = WorkSeconds;
                if(SecondsForWork < TargetSecondsPerFrame)
                {
                    if(SleepIsGranular)
                    {
                        DWORD SleepMS = (DWORD)((1000.0f * (TargetSecondsPerFrame - SecondsForWork)) - 1);
                        Sleep(SleepMS);
                    }
                    
                    SecondsForWork = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                    
                    while(SecondsForWork < TargetSecondsPerFrame)
                    {
                        SecondsForWork = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                    }
                }
                
                //Flip
                HDC DeviceContext = GetDC(WindowHandle);
                Win32UpdateWindow(DeviceContext, &GlobalPixelBuffer);
                ReleaseDC(WindowHandle, DeviceContext);
                
                LastCounter = Win32GetWallClock();
                
                char OutputBuffer[256];
                sprintf_s(OutputBuffer, ArrayCount(OutputBuffer), "%f\n", (1000.0f * SecondsForWork));
                OutputDebugStringA(OutputBuffer);
                
                if(Score == 15)
                {
                    GlobalRunning = false;
                }
            }
        }
    }
    
    return 0;
}