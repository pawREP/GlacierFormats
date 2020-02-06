!preliminary!

![GFLogo](./res/drawing.svg)

----------------------------------
#### De/Serialization
All resource classes share a set of de/serialization functions. Namely `readFromBuffer`, `readFromFile`, `serializeToBuffer` and `serializeToFile` which are inherited from `GlacierResource`, as well as `serialize(BinaryReader&)`, which can be used to support arbitrary write sinks. Some resource classes may also provide additional serialization methods specific to the resource in question. The texture resource format `TEXD` for example provides `saveToDDSBuffer`, `saveToDDSFile`, `saveToTGAFile` and ` saveToPNGFile`.

#### Resource loading
Resources can either be loaded directly from a resource file or buffer via 
```cpp
template<typename Resource> 
std::unique_ptr<Resource> GlacierResource<Resource>::loadFromFile(const std::filesystem::path&, RuntimeId);
template<typename Resource> 
std::unique_ptr<Resource> GlacierResource<Resource>::loadFromBuffer(const std::filesystem::path&, RuntimeId);
```
or they can be accessed through `ResourceRepository` which provides transparent access to Glacier's hierarchical resource archive system. Using the repository class has two major advantages, the resource archives don't have to be unpacked first and resource versioning is automatically handled by the repository. 
```cpp
std::unique_ptr<Resource> ResourceRepository::instance()->getResource<Resource>(RuntimeId id);
``` 

## High level API
- Load mode from model via call to GlacierRenderAsset constructor
- Export to GLTF using GLFTExporter and TGAExporter
- Modify mesh offline
- Import GLTF to GLTFAsset. 
- Convert GLTFAsset to PRIM

## Supported Formats
#### PRIM
Render mesh format
Full read, write and creation with the exception of a few data buffer, most notably collision and cloth physics data.
#### BORG (Bone Rig)
Full read, write.
#### TEXD (Texture Detail?)
Full read, write and creation.
#### TEXT (Texture Template/Temporary)
Only trivially supported. 
#### MATE (Material Entity?)
Only trivially supported. 
#### MATI (Material Instance?)
Full read, write.
#### RPKG (Retail Package)

## Library design
The library has two basic types off interfaces.  
The low level interface is design to facilitate and aid with reverse engineering of file foramts. As a consequence, it's very open, much more than necessary. 