#include "pch.h"
#include "MultithreadingContext.h"

namespace MultithreadingContext {

    ThreadParameter threadParameters[Config::NUMCONTEXTS];
    HANDLE workerBeginRenderFrame[Config::NUMCONTEXTS];
    HANDLE workerFinishShadowPass[Config::NUMCONTEXTS];
    HANDLE workerFinishedRenderFrame[Config::NUMCONTEXTS];
    HANDLE threadHandles[Config::NUMCONTEXTS];


    void CreateContexts() {
        struct threadwrapper
        {
            static unsigned int WINAPI thunk(LPVOID lpParameter)
            {
                ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
                WorkerThread(parameter->threadIndex);
                return 0;
            }
        };

        for (int i = 0; i < Config::NUMCONTEXTS; i++) {
            workerBeginRenderFrame[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            workerFinishShadowPass[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            workerFinishedRenderFrame[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            threadParameters[i].threadIndex = i;
            threadHandles[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr,
                    0,
                    threadwrapper::thunk,
                    reinterpret_cast<LPVOID>(&threadParameters[i]),
                    0,
                    nullptr)
                );
            assert(workerBeginRenderFrame[i] != NULL);
            assert(workerFinishShadowPass[i] != NULL);
            assert(workerFinishedRenderFrame[i] != NULL);
            assert(threadHandles[i] != NULL);

        }


    }

    void WorkerThread(int threadIndex) {
        assert(threadIndex >= 0);
        assert(threadIndex < Config::NUMCONTEXTS);

#if !SINGLETHREADED

        while (threadIndex >= 0 && threadIndex < Config::NUMCONTEXTS) {
            WaitForSingleObject(workerBeginRenderFrame[threadIndex], INFINITE);

#endif



#if !SINGLETHREADED
            SetEvent(workerFinishShadowPass[threadIndex]);
#endif





#if !SINGLETHREADED
            SetEvent(workerFinishedRenderFrame[threadIndex]);
        }
#endif
    }
    
}