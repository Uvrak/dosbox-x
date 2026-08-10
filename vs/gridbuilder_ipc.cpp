#include "gridbuilder_ipc.h"

#ifdef WIN32

#include <windows.h>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <set>
#include <queue>
#include <mutex>


#include "keyboard.h"
#include "mouse.h"

static std::thread g_ipcThread;
static std::atomic<bool> g_ipcRunning{ false };

static std::set<KBD_KEYS>
    g_gridBuilderPressedKeys;

static std::queue<std::string>
g_gridBuilderCommandQueue;

static std::mutex
g_gridBuilderCommandMutex;

static void GRIDBUILDER_IPC_QueueCommand(
    const char* command
)
{
    std::lock_guard<std::mutex> lock(
        g_gridBuilderCommandMutex
    );

    g_gridBuilderCommandQueue.push(
        command
    );
}

static void GRIDBUILDER_IPC_SetKey(
    KBD_KEYS key,
    bool pressed
)
{
    KEYBOARD_AddKey(
        key,
        pressed
    );

    if(pressed)
    {
        g_gridBuilderPressedKeys.insert(
            key
        );
    }
    else
    {
        g_gridBuilderPressedKeys.erase(
            key
        );
    }
}

void GRIDBUILDER_IPC_ProcessCommands()
{
    std::queue<std::string> commands;

    {
        std::lock_guard<std::mutex> lock(
            g_gridBuilderCommandMutex
        );

        std::swap(
            commands,
            g_gridBuilderCommandQueue
        );
    }

    while(!commands.empty())
    {
        const std::string command =
            commands.front();

        commands.pop();

        printf(
            "GridBuilder main thread: %s\n",
            command.c_str()
        );

        const char* text =
            command.c_str();

        if(std::strcmp(
            text,
            "RELEASE_ALL"
        ) == 0)
        {
            for(const KBD_KEYS key :
            g_gridBuilderPressedKeys)
            {
                KEYBOARD_AddKey(
                    key,
                    false
                );
            }

            g_gridBuilderPressedKeys.clear();

            KEYBOARD_ClrBuffer();

            continue;
        }

        const char* mouseMovePrefix =
            "MOUSEMOVE:";

        if(std::strncmp(
            text,
            mouseMovePrefix,
            std::strlen(mouseMovePrefix)
        ) == 0)
        {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;

            if(std::sscanf(
                text + std::strlen(mouseMovePrefix),
                "%d:%d:%d:%d",
                &x,
                &y,
                &width,
                &height
            ) == 4)
            {
                if(width > 1 &&
                    height > 1)
                {
                    const float normalizedX =
                        static_cast<float>(x) /
                        static_cast<float>(
                            width - 1
                            );

                    const float normalizedY =
                        static_cast<float>(y) /
                        static_cast<float>(
                            height - 1
                            );

                    Mouse_GridBuilderMove(
                        normalizedX,
                        normalizedY
                    );
                }
            }
           
        }

        if(std::strcmp(
            text,
            "MOUSEDOWN:0"
        ) == 0)
        {
            Mouse_ButtonPressed(0);

            continue;
        }

        if(std::strcmp(
            text,
            "MOUSEUP:0"
        ) == 0)
        {
            Mouse_ButtonReleased(0);

            continue;
        }

        const char* keyDownPrefix =
            "KEYDOWN:";

        const char* keyUpPrefix =
            "KEYUP:";

        const bool isKeyDown =
            std::strncmp(
                text,
                keyDownPrefix,
                std::strlen(keyDownPrefix)
            ) == 0;

        const bool isKeyUp =
            std::strncmp(
                text,
                keyUpPrefix,
                std::strlen(keyUpPrefix)
            ) == 0;

        const char* ipcKeyName = nullptr;

        if(isKeyDown)
        {
            ipcKeyName =
                text + std::strlen(
                    keyDownPrefix
                );
        }
        else if(isKeyUp)
        {
            ipcKeyName =
                text + std::strlen(
                    keyUpPrefix
                );
        }

        struct GridBuilderKeyMapping
        {
            const char* command;
            KBD_KEYS key;
        };

        static const GridBuilderKeyMapping ipcLetterMappings[] =
        {
            { "A", KBD_a },
            { "B", KBD_b },
            { "C", KBD_c },
            { "D", KBD_d },
            { "E", KBD_e },
            { "F", KBD_f },
            { "G", KBD_g },
            { "H", KBD_h },
            { "I", KBD_i },
            { "J", KBD_j },
            { "K", KBD_k },
            { "L", KBD_l },
            { "M", KBD_m },
            { "N", KBD_n },
            { "O", KBD_o },
            { "P", KBD_p },
            { "Q", KBD_q },
            { "R", KBD_r },
            { "S", KBD_s },
            { "T", KBD_t },
            { "U", KBD_u },
            { "V", KBD_v },
            { "W", KBD_w },
            { "X", KBD_x },
            { "Y", KBD_y },
            { "Z", KBD_z }
        };

        static const GridBuilderKeyMapping ipcDigitMappings[] =
        {
            { "0", KBD_0 },
            { "1", KBD_1 },
            { "2", KBD_2 },
            { "3", KBD_3 },
            { "4", KBD_4 },
            { "5", KBD_5 },
            { "6", KBD_6 },
            { "7", KBD_7 },
            { "8", KBD_8 },
            { "9", KBD_9 }
        };

        static const GridBuilderKeyMapping ipcFunctionKeyMappings[] =
        {
            { "F1",  KBD_f1 },
            { "F2",  KBD_f2 },
            { "F3",  KBD_f3 },
            { "F4",  KBD_f4 },
            { "F5",  KBD_f5 },
            { "F6",  KBD_f6 },
            { "F7",  KBD_f7 },
            { "F8",  KBD_f8 },
            { "F9",  KBD_f9 },
            { "F10", KBD_f10 },
            { "F11", KBD_f11 },
            { "F12", KBD_f12 }
        };

        static const GridBuilderKeyMapping ipcNavigationKeyMappings[] =
        {
            { "HOME",     KBD_home },
            { "END",      KBD_end },
            { "INSERT",   KBD_insert },
            { "DELETE",   KBD_delete },
            { "PAGEUP",   KBD_pageup },
            { "PAGEDOWN", KBD_pagedown }
        };

        static const GridBuilderKeyMapping ipcSymbolKeyMappings[] =
        {
            { "MINUS",        KBD_minus },
            { "EQUALS",       KBD_equals },
            { "LEFTBRACKET",  KBD_leftbracket },
            { "RIGHTBRACKET", KBD_rightbracket },
            { "BACKSLASH",    KBD_backslash },
            { "SEMICOLON",    KBD_semicolon },
            { "QUOTE",        KBD_quote },
            { "COMMA",        KBD_comma },
            { "PERIOD",       KBD_period },
            { "SLASH",        KBD_slash }
        };
        if(ipcKeyName != nullptr)
        {
            for(const auto& mapping :
                ipcLetterMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcDigitMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcFunctionKeyMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcNavigationKeyMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcSymbolKeyMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }
            if(std::strcmp(ipcKeyName, "ENTER") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_enter,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "SPACE") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_space,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "BACKSPACE") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_backspace,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "TAB") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_tab,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "ESC") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_esc,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "UP") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_up,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "DOWN") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_down,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "LEFT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_left,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "RIGHT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_right,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "SHIFT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_leftshift,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "CTRL") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_leftctrl,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "ALT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_leftalt,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "ALTGR") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_rightalt,
                    isKeyDown
                );
            }
        }
    }
}
static void GRIDBUILDER_IPC_Thread()
{
    while(g_ipcRunning)
    {
        HANDLE pipe =
            CreateNamedPipeA(
                "\\\\.\\pipe\\GridBuilderDOSBox",
                PIPE_ACCESS_INBOUND,
                PIPE_TYPE_MESSAGE |
                PIPE_READMODE_MESSAGE |
                PIPE_WAIT,
                1,
                256,
                256,
                0,
                nullptr
            );

        if(pipe == INVALID_HANDLE_VALUE)
        {
            printf(
                "GridBuilder IPC: CreateNamedPipe failed\n"
            );

            return;
        }

        printf(
            "GridBuilder IPC: waiting for client\n"
        );

        BOOL connected =
            ConnectNamedPipe(
                pipe,
                nullptr
            );

        if(!connected &&
            GetLastError() != ERROR_PIPE_CONNECTED)
        {
            CloseHandle(pipe);

            if(g_ipcRunning)
            {
                printf(
                    "GridBuilder IPC: ConnectNamedPipe failed\n"
                );
            }

            continue;
        }

        if(!g_ipcRunning)
        {
            CloseHandle(pipe);
            break;
        }

        printf(
            "GridBuilder IPC: client connected\n"
        );

        while(g_ipcRunning)
        {
            char buffer[256]{};

            DWORD bytesRead = 0;

            BOOL result =
                ReadFile(
                    pipe,
                    buffer,
                    sizeof(buffer) - 1,
                    &bytesRead,
                    nullptr
                );

            if(!result || bytesRead == 0)
            {
                break;
            }

            buffer[bytesRead] = '\0';

            GRIDBUILDER_IPC_QueueCommand(
                buffer
            );
        }

        DisconnectNamedPipe(
            pipe
        );

        CloseHandle(
            pipe
        );
    }

    printf(
        "GridBuilder IPC: stopped\n"
    );
}

#endif

void GRIDBUILDER_IPC_Init()
{
#ifdef WIN32

    if(g_ipcRunning)
    {
        return;
    }

    g_ipcRunning = true;

    g_ipcThread =
        std::thread(
            GRIDBUILDER_IPC_Thread
        );

#endif
}

void GRIDBUILDER_IPC_Shutdown()
{
#ifdef WIN32

    g_ipcRunning = false;

    HANDLE pipe =
        CreateFileA(
            "\\\\.\\pipe\\GridBuilderDOSBox",
            GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

    if(pipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipe);
    }

    if(g_ipcThread.joinable())
    {
        g_ipcThread.join();
    }

#endif
}
