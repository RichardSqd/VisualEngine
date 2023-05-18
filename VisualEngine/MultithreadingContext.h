#pragma once
#include "pch.h"

namespace MultithreadingContext {

    struct ThreadParameter
    {
        int threadIndex;
    };

    void CreateContexts();
    void WorkerThread(int threadIndex);


    extern ThreadParameter threadParameters[Config::NUMCONTEXTS];
    extern HANDLE workerBeginRenderFrame[Config::NUMCONTEXTS];
    extern HANDLE workerFinishShadowPass[Config::NUMCONTEXTS];
    extern HANDLE workerFinishedRenderFrame[Config::NUMCONTEXTS];
    extern HANDLE threadHandles[Config::NUMCONTEXTS];
}