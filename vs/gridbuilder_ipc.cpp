#include "gridbuilder_ipc.h"

#ifdef WIN32

#include <windows.h>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "keyboard.h"

static std::thread g_ipcThread;
static std::atomic<bool> g_ipcRunning{ false };

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

        if(result && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';

            printf(
                "GridBuilder IPC received: %s\n",
                buffer
            );

            if(std::strcmp(buffer, "A") == 0)
            {
                KEYBOARD_AddKey(KBD_a, true);
                KEYBOARD_AddKey(KBD_a, false);
            }
            else if(std::strcmp(buffer, "ENTER") == 0)
            {
                KEYBOARD_AddKey(KBD_enter, true);
                KEYBOARD_AddKey(KBD_enter, false);
            }
            else if(std::strcmp(buffer, "UP") == 0)
            {
                KEYBOARD_AddKey(KBD_up, true);
                KEYBOARD_AddKey(KBD_up, false);
            }
            else if(std::strcmp(buffer, "DOWN") == 0)
            {
                KEYBOARD_AddKey(KBD_down, true);
                KEYBOARD_AddKey(KBD_down, false);
            }
            else if(std::strcmp(buffer, "LEFT") == 0)
            {
                KEYBOARD_AddKey(KBD_left, true);
                KEYBOARD_AddKey(KBD_left, false);
            }
            else if(std::strcmp(buffer, "RIGHT") == 0)
            {
                KEYBOARD_AddKey(KBD_right, true);
                KEYBOARD_AddKey(KBD_right, false);
            }
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
