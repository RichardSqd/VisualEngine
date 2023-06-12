# Visual Engine (preview)
Visual Engine is a multithreaded real-time DirectX 12 renderer that has been developed from scratch, providing the ability to experiment with various rendering techniques and shading options.

* The current preview version supports loading glTF 2.0 scenes and is integrated with an imGui user interface for making runtime modifications, which is handy for comparing rendering results and statistics using different rendering methods and combinations at runtime

* One of the project's goals is to create a hybrid renderer capable of supporting both the rasterization pipeline and ray tracing pipeline, offering users sufficient control over essential rendering parameters to explore different rendering outcomes. While the current version only supports the rasterization portion, implementing the ray tracing component is a significant milestone that will be accomplished in the near future.

## Gallerys
PBR with IBL
![](Images/PBR_example1.png)
![](Images/PBR_example2.png)
Blinn Phong
![](Images/phong_example2.png)
Sponza
![sponza](https://github.com/RichardSqd/VisualEngine/assets/100163115/26bbfbaf-e9a6-4dd0-9cfe-bfffcc06aacf)
Sample Chess Animatgion (modeled by Puppetswar Miniatures)





https://github.com/RichardSqd/VisualEngine/assets/100163115/3a697b5a-ca5e-47f6-b560-dbb522032461




## Features
- glTF 2.0 loading
- Multi-threaded: command list recording
- Multiple Lighting paths
- Phong lighting model
- Physically based rendering（PBR）
    - bidirectional reflectance distribution function (BRDF)
    - Image based lighting 
    - HDRI environments to irradiance/specular cubemap convolution
    - HDR to LDR tonemapping 
- Skybox Rendering
- ImGui graphical user interface
- Shader Model 5.0
- MSAA X4 Anti Aliasing 
- Display: VSync 


## In progress
- DirectX Raytracing (DXR) hybrid rendering 
- Deferred rendering path

## Build
- Visual Studio 2019+
- Visual C++
- Windows 10 SDK [10.0.18362.0]

## Third party Used
- D3DX12
- DirectXTex
- ImGui
- tinygltf
- KhronosGroup/glTF-Sample
- HDRI Haven
