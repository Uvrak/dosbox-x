#include "dosbox_memory_scanner_ipc.h"

#ifdef WIN32

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <sstream>

#include "mem.h"

#include "dosbox_memory_snapshot_writer.h"
#include "memory_read_tracker.h"

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
                    8192,
                    8192,
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
                    "READTRACK:START"
                ) == 0)
                {
                    MemoryReadTracker::start();

                    response = "OK";
                }
                else if(std::strcmp(
                    buffer,
                    "READTRACK:STOP"
                ) == 0)
                {
                    MemoryReadTracker::stop();

                    response = "OK";
                }

                else if(std::strcmp(
                    buffer,
                    "READTRACK:CLEAR"
                ) == 0)
                {
                    MemoryReadTracker::clear();

                    response = "OK";
                }

                else if(std::strncmp(
                    buffer,
                    "READTRACK:TRANSITIONTARGET:",
                    27
                ) == 0)
                {
                    const size_t address =
                        static_cast<size_t>(
                            std::strtoull(
                                buffer + 27,
                                nullptr,
                                10
                            )
                            );

                    MemoryReadTracker::setTransitionTarget(
                        static_cast<LinearPt>(
                            address
                            )
                    );

                    response = "OK";
                }

                else if(std::strcmp(
                    buffer,
                    "READTRACK:COUNT"
                ) == 0)
                {
                    const auto addresses =
                        MemoryReadTracker::addresses();

                    response =
                        std::to_string(
                            addresses.size()
                        );
                }

                else if(std::strncmp(
                    buffer,
                    "READTRACK:ADDRESS:",
                    18
                ) == 0)
                {
                    const auto addresses =
                        MemoryReadTracker::addresses();

                    const size_t index =
                        static_cast<size_t>(
                            std::strtoull(
                                buffer + 18,
                                nullptr,
                                10
                            )
                            );

                    if(index < addresses.size())
                    {
                        response =
                            std::to_string(
                                addresses[index]
                            );
                    }
                    else
                    {
                        response =
                            "ERROR:INDEX_OUT_OF_RANGE";
                    }
                }

                else if(std::strncmp(
                    buffer,
                    "READTRACK:ADDRESSES:",
                    20
                    ) == 0)
                    {
                        size_t start = 0;
                        size_t count = 0;

                        const char* parameters =
                            buffer +
                            std::strlen(
                                "READTRACK:ADDRESSES:"
                            );

                        if(std::sscanf(
                            parameters,
                            "%zu:%zu",
                            &start,
                            &count
                        ) != 2)
                        {
                            response = "ERROR";
                        }
                        else
                        {
                            const auto addresses =
                                MemoryReadTracker::addresses();

                            std::ostringstream stream;

                            const size_t end =
                                (std::min)(
                                    start + count,
                                    addresses.size()
                                    );

                            for(size_t index = start;
                                index < end;
                                ++index)
                            {
                                if(index != start)
                                {
                                    stream << ',';
                                }

                                stream << addresses[index];
                            }

                            response = stream.str();
                        }
                }

                else if(std::strcmp(
                    buffer,
                    "READTRACK:INSTRUCTIONCOUNT"
                    ) == 0)
                    {
                        const auto instructions =
                            MemoryReadTracker::instructions();

                        response =
                            std::to_string(
                                instructions.size()
                            );
                    }

                else if(std::strcmp(
                    buffer,
                    "READTRACK:TRANSITIONCOUNT"
                    ) == 0)
                    {
                        const auto transitions =
                            MemoryReadTracker::instructionTransitions();

                        response =
                            std::to_string(
                                transitions.size()
                            );
                    }

                else if(std::strcmp(
                    buffer,
                    "READTRACK:TRANSITIONCONTEXTCOUNT"
                    ) == 0)
                    {
                        const auto contexts =
                            MemoryReadTracker::
                            instructionTransitionContexts();

                        response =
                            std::to_string(
                                contexts.size()
                            );
                    }

                else if(std::strncmp(
                    buffer,
                    "READTRACK:TRANSITIONCONTEXTS:",
                    std::strlen(
                    "READTRACK:TRANSITIONCONTEXTS:"
                    )
                    ) == 0)
                    {
                        size_t start = 0;
                        size_t count = 0;

                        const char* parameters =
                            buffer +
                            std::strlen(
                                "READTRACK:TRANSITIONCONTEXTS:"
                            );

                        if(std::sscanf(
                            parameters,
                            "%zu:%zu",
                            &start,
                            &count
                        ) != 2)
                        {
                            response = "ERROR";
                        }
                        else
                        {
                            const auto contexts =
                                MemoryReadTracker::
                                instructionTransitionContexts();

                            std::ostringstream stream;

                            const size_t end =
                                (std::min)(
                                    start + count,
                                    contexts.size()
                                    );

                            for(size_t index = start;
                                index < end;
                                ++index)
                            {
                                if(index != start)
                                {
                                    stream << ',';
                                }

                                stream
                                    << contexts[index].first
                                    << ':'
                                    << contexts[index].second;
                            }

                            response = stream.str();
                        }
                }

                else if(std::strncmp(
                    buffer,
                    "READTRACK:TRANSITIONS:",
                    22
                    ) == 0)
                    {
                        size_t start = 0;
                        size_t count = 0;

                        const char* parameters =
                            buffer +
                            std::strlen(
                                "READTRACK:TRANSITIONS:"
                            );

                        if(std::sscanf(
                            parameters,
                            "%zu:%zu",
                            &start,
                            &count
                        ) != 2)
                        {
                            response = "ERROR";
                        }
                        else
                        {
                            const auto transitions =
                                MemoryReadTracker::instructionTransitions();

                            std::ostringstream stream;

                            const size_t end =
                                (std::min)(
                                    start + count,
                                    transitions.size()
                                    );

                            for(size_t index = start;
                                index < end;
                                ++index)
                            {
                                if(index != start)
                                {
                                    stream << ',';
                                }

                                stream
                                    << transitions[index].first
                                    << ':'
                                    << transitions[index].second;
                            }

                            response = stream.str();
                        }
                }

                else if(std::strncmp(
                    buffer,
                    "READTRACK:INSTRUCTIONS:",
                    23
                    ) == 0)
                    {
                        size_t start = 0;
                        size_t count = 0;

                        const char* parameters =
                            buffer +
                            std::strlen(
                                "READTRACK:INSTRUCTIONS:"
                            );

                        if(std::sscanf(
                            parameters,
                            "%zu:%zu",
                            &start,
                            &count
                        ) != 2)
                        {
                            response = "ERROR";
                        }
                        else
                        {
                            const auto instructions =
                                MemoryReadTracker::instructions();

                            std::ostringstream stream;

                            const size_t end =
                                (std::min)(
                                    start + count,
                                    instructions.size()
                                    );

                            for(size_t index = start;
                                index < end;
                                ++index)
                            {
                                if(index != start)
                                {
                                    stream << ',';
                                }

                                stream
                                    << instructions[index].first
                                    << ':'
                                    << instructions[index].second;
                            }

                            response = stream.str();
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
        ::CloseHandle(
            pipe
        );
    }

    if(g_scannerThread.joinable())
    {
        g_scannerThread.join();
    }

#endif
}
