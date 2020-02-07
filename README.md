![GFLogo](./logo/logo.svg)

GlacierFormats is a C++17 library for processing of Glacier 2 Engine resource files. This library is part of a research and reverse-engineering project that aims to provide documentation and tools to facilitate interoperability with the Glacier 2 Engine. 

# Feature overview
GlacierFormats supports a growing selection of Glacier 2 Engine resource formats. The scope of support varies from format to format depending on the importance of the underlying resources in the Glacier eco system and the complexity of the format. The general goal is to provide at least basic deserialization, serialization functionality for all supported resources. Modification and de novo creation of resources is also supported whenever possible, practical and useful.

Currently supported formats:
- **PRIM** (Primitive render mesh)
    - Full support for de/serialization, modification and creation. This includes static and rigged meshes. Meshes of subtype `LINKED` and `SPEEDTREE` are currently not supported. Furthermore, not all features are supported for the creation of new files, cloth physics and collision data are notable examples.
- **BORG** (Bone Rig)
    - Full support for de/serialization. Modification and creation are theoretically possible but currently not useful since animation formats are not supported to any useful degree.
- **TEXD** (Detail Textures)
    - Full support for de/serialization as well as import and export from and to a variety of texture exchange formats like TGA, PNG and DDS.
- **TEXT** (Placeholder Textures)
    - The format is almost identical to TEXD but it's only trivially  supported because of it's limited usefulness.
- **MATE** (Material Entity)
    - Only trivially supported. 
- **MATI** (Material Instance)
    - Full support for de/serialization. 
- **RPKG** (Retail Package)
    - Full support for de/serialization, modification and creation of both the `BASE` and `PATCH` subtypes. 

In addition to the very openly designed, research oriented resource file classes, the library provides a number of higher level interfaces that sit on top of resource classes and provide more restricted, generic interfaces for the underlying abstract types. `IMesh`, a generic face-vertex mesh interface that's implemented by `PRIM`, is an example of such an interface. 

Even higher level interfaces are provided to operate on logical collections of resources. `IRenderAsset` is one such interface, it provides transparent access to meshes, skeletons and materials. GLTF and FBX import/export is handled at this level of abstraction. 

## Resource I/O
GlacierFormats provides two major interfaces for resource loading and storing. The first interface operates on files and buffers, while the other interface operates on `ResourceRepository`.

### Resource loading from Files and Buffers:
All resource classes inherit from `GlacierResource` and share a set of de/serialization functions. Namely `readFromBuffer`, `readFromFile`, `serializeToBuffer` and `serializeToFile`. Additionally, each class provides `serialize(BinaryReader&)`, which can support arbitrary serialization sinks via implementations of `IBinaryReaderSink`. Some resource classes may also provide additional serialization methods specific to the resource in question. The texture resource format `TEXD` for example provides `saveToDDSBuffer`, `saveToDDSFile`, `saveToTGAFile` and `saveToPNGFile`.
```cpp
//File system IO
RuntimeId prim_runtime_id = 0x00277DE292B8D13F;
auto path = std::filesystem::path("E:/Glacier/277DE292B8D13F.PRIM")
std::unique_ptr<PRIM> resource  = GlacierResource<PRIM>::readFromFile(path, prim_runtime_id);
//...
resource->serializeToFile(path);
``` 

### Resource loading from `ResourceRepository`
`ResourceRepository` is a singleton that gets initilized during startup of the library. It provides transparent and unified access to resources inside the heirarchy of archives that ship with Glacier 2 Engine titles. Loading directly from the repository, as opposed to unpacking the repository and loading from individual resource files, has the advantage that all the complexities of version control and archive patches are handled automatically. 
Another major advantage of using the repository for resource loading is that it can provide additional meta data like dependency relationships between resources. This meta data isn't availble when using unpacked archives. 

```cpp
//Loading a PRIM resource from a resource repository
auto repo = ResourceRepository::instance();
RuntimeId prim_runtime_id = 0x00277DE292B8D13F;
std::unique_ptr<PRIM> resource  = repo->getResource<PRIM>(prim_runtime_id);
``` 

## Finding Runtime IDs
All Glacier resources are identified and referenced by a unique 56 bit runtime id. These ids are generated at built time by hashing the full resource path with a platform specific extension. The hashing process removes identifying information which can make finding specific resources difficult. Fortunately, there is a partial solution to this issue. Many resources contain strings that can give hints about their use or the use of their child and parent resources. The `GlacierFormatsTools` folder contains a tool that can generate a mapping between material instance names and runtime ids of meshes that use those materials. Similar maps can be generated between other resources and they can simplify the search for specific ids greatly. 

## Documentation
GlacierFormats is currently mainly documented via a collection of small, well documented sample applications that cover most basic use cases like de/serialisation and modification of resources as well as import/export from/to exchange formats. All the samples can be found in `GlacierFormatsSamples`.

## Building GlacierFormats 
Windows (with cmake-gui / VS 16 2019):
- Install [FBX SDK 2020.0.1](https://www.autodesk.com/content/dam/autodesk/www/adn/fbx/2020-0-1/fbx202001_fbxsdk_vs2017_win.exe).
- Install and run the latest version of [CMake](https://cmake.org/download/).
- Select source and build folders.
- Configure.
- Set the option `GLACIERFORAMTS_FBX_SDK_PATH` to your FBX SDK installation path.
- Configure.
- Generate.
- Open Project.
- Build All. 

 Linux/MacOS/Other:
- GlacierFormats depends on platform specific components like DirectXTex and can therefore currently not be build on other platforms. 

## Dependencies
GlacierFormats depends on [LZ4](https://github.com/lz4/lz4), [DirectXTex](https://github.com/microsoft/DirectXTex), [glTF-SDK](https://github.com/microsoft/glTF-SDK) and [FBX SDK 2020.0.1](https://www.autodesk.com/content/dam/autodesk/www/adn/fbx/2020-0-1/fbx202001_fbxsdk_vs2017_win.exe). With the exception of the FBX SDK, all the dependencies needed to build and run the library ship with the source code. 

## Thanks 
 - Special thanks to Notex for useful discussions and motivation.