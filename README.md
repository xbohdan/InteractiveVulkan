# Interactive Vulkan

Interactive Vulkan is a repository that aims to provide comprehensive examples of using C++ bindings for the Vulkan API
to facilitate the creation of interactive graphics applications.

The project is in its early stages and may not be ready for production use.
The goal is to visualize how Vulkan's complexities can be abstracted without relying on third-party libraries.

The first example is a simple cube renderer that demonstrates some concepts of Vulkan 1.3.
Most importantly, its render loop utilizes dynamic rendering, which could be more suitable for game engines because
it is a more flexible alternative to hard-coded render passes and framebuffers.

The project is structured as follows:

```
└── src
	├── apps
    └── intvlk
        ├── glm_utils
        ├── glslang_utils
        └── vma_utils
```

The `src` directory immediately contains the application source code for creating and manipulating visual content.
Each subdirectory has a corresponding namespace and focuses on a different aspect of the application.

The `apps` folder currently hosts a single example, which renders a static image of a colored cube. It is meant to
serve as a base for any serious Vulkan application and address essentials many tutorials overlook: why to structure and
design the program in a certain way, accounting for the future increase in complexity, what is dynamic rendering, how
to utilize validation layers to the maximum extent during development, and in which way to handle window resizing.

The `intvlk` namespace contains the main Vulkan wrapper classes and utility methods.
It also includes SDL functionality, as it is the chosen framework for window management and input handling.

The `glm_utils` namespace handles mathematical operations and data structures in Vulkan and GLSL.

The `glslang_utils` namespace provides tools for compiling GLSL shaders to SPIR-V.
In this project, shaders are compiled at runtime to demonstrate working with Glslang from C++.

The `vma_utils` namespace contains functionality that relies on the Vulkan Memory Allocator library.
It manages memory allocation and deallocation in a safe and performant way.

Contributions are welcome, and the project is open to suggestions and improvements.

## License

Copyright(c) 2024, Bohdan Soproniuk

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
