#include "pch.h"
#include "MultithreadingContext.h"
#include "Renderer.h"

namespace MultithreadingContext {

    ThreadParameter threadParameters[Config::NUMCONTEXTS];
    HANDLE workerBeginRenderFrame[Config::NUMCONTEXTS];
    HANDLE workerFinishShadowPass[Config::NUMCONTEXTS];
    HANDLE workerFinishedRenderFrame[Config::NUMCONTEXTS];
    HANDLE threadHandles[Config::NUMCONTEXTS];


    void CreateContexts() {

        auto threadLimit = std::thread::hardware_concurrency() - 1; 
        if (Config::NUMCONTEXTS > threadLimit) {
            throw("Thread Number Exceeded!");
        }

        
        struct threadwrapper
        {
            static unsigned int WINAPI prog(LPVOID lpParameter)
            {
                ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
                WorkerThread( parameter->threadIndex);
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
                    threadwrapper::prog,
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


        while (threadIndex >= 0 && threadIndex < Config::NUMCONTEXTS) {

            WaitForSingleObject(workerBeginRenderFrame[threadIndex], INFINITE);

            FrameResource* currentFrameResource = Graphics::gFrameResourceManager.GetCurrentFrameResource();
            auto shadowCommandList = currentFrameResource->shadowComandContexts[threadIndex]->getCommandList();

            Renderer::SetSharedCommandListStates(shadowCommandList);

            Renderer::RenderShadowMap(shadowCommandList);

            shadowCommandList->Close();

            //tell the main thread the shadow command list is ready for submit
            SetEvent(workerFinishShadowPass[threadIndex]);
    
            auto sceneCommandList = currentFrameResource->sceneComandContexts[threadIndex]->getCommandList();

            Renderer::SetSharedCommandListStates(sceneCommandList);
            Renderer::RenderColor(sceneCommandList);

            sceneCommandList->Close();

            SetEvent(workerFinishedRenderFrame[threadIndex]);
        }

    }
    
}