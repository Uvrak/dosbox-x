#include "dosbox_memory_scanner_ipc.h"

#ifdef WIN32

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <cstring>

#include "mem.h"

#include "dosbox_memory_snapshot_writer.h"

namespace
{
    constexpr const char* ScannerPipeName =
        R"(\\.\pipe\DosBoxMemoryScanner)";

    std::thread g_scannerThread;

    std::atomic<bool>
        g_scannerRunning{ false };

    std::mutex
        g_scannerMutex;

    std::condition_variable
        g_scannerCondition;

    bool g_publishRequested =
        false;

    bool g_publishCompleted =
        false;

    bool g_publishSucceeded =
        false;

    DosBoxMemorySnapshotWriter
        g_snapshotWriter;

    void scannerThreadMain()
    {
        while(g_scannerRunning)
        {
            HANDLE pipe =
                CreateNamedPipeA(
                    ScannerPipeName,
                    PIPE_ACCESS_DUPLEX,
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE |
                    PIPE_WAIT,
                    1,
                    256,
                    256,
                    0,
                    nullptr
                );

            if(pipe ==
                INVALID_HANDLE_VALUE)
            {
                return;
            }

            const BOOL connected =
                ConnectNamedPipe(
                    pipe,
                    nullptr
                );

            if(!connected &&
                GetLastError() !=
                ERROR_PIPE_CONNECTED)
            {
                CloseHandle(pipe);
                continue;
            }

            while(g_scannerRunning)
            {
                char buffer[256]{};
                DWORD bytesRead = 0;

                const BOOL readSucceeded =
                    ReadFile(
                        pipe,
                        buffer,
                        sizeof(buffer) - 1,
                        &bytesRead,
                        nullptr
                    );

                if(!readSucceeded ||
                    bytesRead == 0)
                {
                    break;
                }

                buffer[bytesRead] = '\0';

                std::string response;

                if(std::strcmp(
                    buffer,
                    "PING"
                ) == 0)
                {
                    response = "PONG";
                }
                else if(std::strncmp(
                    buffer,
                    "WRITE:",
                    6
                ) == 0)
                {
                    size_t address = 0;
                    unsigned int value = 0;

                    if(std::sscanf(
                        buffer + 6,
                        "%zu:%u",
                        &address,
                        &value
                    ) == 2 &&
                        value <= 255)
                    {
                        phys_writeb(
                            static_cast<PhysPt>(
                                address
                                ),
                            static_cast<uint8_t>(
                                value
                                )
                        );

                        response = "OK";
                    }
                    else
                    {
                        response = "ERROR";
                    }
                }
                else if(std::strcmp(
                    buffer,
                    "PUBLISH"
                ) == 0)
                {
                    {
                        std::lock_guard<std::mutex>
                            lock(
                                g_scannerMutex
                            );

                        g_publishRequested =
                            true;

                        g_publishCompleted =
                            false;
                    }

                    std::unique_lock<std::mutex>
                        lock(
                            g_scannerMutex
                        );

                    g_scannerCondition.wait(
                        lock,
                        []()
                        {
                            return
                                g_publishCompleted ||
                                !g_scannerRunning;
                        }
                    );

                    response =
                        g_publishSucceeded
                        ? "PUBLISHED"
                        : "ERROR:PUBLISH_FAILED";
                }
                else
                {
                    response =
                        "ERROR:UNKNOWN_COMMAND";
                }

                DWORD bytesWritten = 0;

                WriteFile(
                    pipe,
                    response.data(),
                    static_cast<DWORD>(
                        response.size()
                        ),
                    &bytesWritten,
                    nullptr
                );
            }

            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
        }
    }
}

#endif

void DOSBOX_MEMORY_SCANNER_IPC_ProcessCommands()
{
#ifdef WIN32

    bool publishRequested = false;

    {
        std::lock_guard<std::mutex> lock(
            g_scannerMutex
        );

        publishRequested =
            g_publishRequested;

        g_publishRequested =
            false;
    }

    if(!publishRequested)
    {
        return;
    }

    const bool published =
        g_snapshotWriter.publish();

    {
        std::lock_guard<std::mutex> lock(
            g_scannerMutex
        );

        g_publishSucceeded =
            published;

        g_publishCompleted =
            true;
    }

    g_scannerCondition.notify_one();

#endif
}

void DOSBOX_MEMORY_SCANNER_IPC_Init()
{
#ifdef WIN32

    if(g_scannerRunning)
    {
        return;
    }

    g_scannerRunning =
        true;

    g_scannerThread =
        std::thread(
            scannerThreadMain
        );

#endif
}

void DOSBOX_MEMORY_SCANNER_IPC_Shutdown()
{
#ifdef WIN32

    if(!g_scannerRunning)
    {
        return;
    }

    g_scannerRunning =
        false;

    g_scannerCondition.notify_all();

    HANDLE pipe =
        CreateFileA(
            ScannerPipeName,
            GENERIC_READ |
            GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

    if(pipe !=
        INVALID_HANDLE_VALUE)
    {
        CloseHandle(
            pipe
        );
    }

    if(g_scannerThread.joinable())
    {
        g_scannerThread.join();
    }

#endif
}
