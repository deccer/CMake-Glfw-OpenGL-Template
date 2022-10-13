# Refactoring the Project

### Introduction
You have cloned the project, now what?

Having everything in `ProjectApplication.cpp` can and will get
annoying as your Engine grows.

You will begin by splitting the application in different files, so 
far the application needs these 3 things to draw the frog:
- Shader
- Mesh
- Model

Since you will create more shaders/models, it will be very useful
to abstract them away in their own classes, such that it becomes
much easier to create a new **Model** or a new **Shader**.

### The Shader class
Let's begin with the shader, here's how a basic shader class 
should look like.
```c++
class Shader
{
public:
    Shader(std::string_view vertex, std::string_view fragment);
    ~Shader();

    void Bind() const;
    void Set(uint32_t location, const glm::mat4& matrix) const;
    void Set(uint32_t location, uint32_t value) const;

private:
    uint32_t _program;
};
```
The `Shader` holds onto a single `uint32_t`, which is the OpenGL handle
type.

The next thing is the constructor: it takes two paths as arguments, one
for the vertex shader and one for the fragment shader, it creates the shader
and stores the result in our `_program` member variable.

The destructor calls the OpenGL function to destroy the shader program.

The next thing is the functionality, this is very basic, and you may
expand it to suit your needs.
- `Bind()`: tells OpenGL to use this shader program in the next draw calls, until a new shader is bound.
- `Set(uint32_t, mat4)`: tells OpenGL to set a `uniform` matrix 4x4 in the shader, at a specific location, if you are not familiar with this, think of this as assigning a value to a "global variable" in the vertex or fragment shader.
- `Set(uint32_t, int32_t)`: tells OpenGL to set a single `uniform` (unsigned) integer in the shader, at a specific location, if you are not familiar with this, think of this as assigning a value to a "global variable" in the vertex or fragment shader.

Finally, the implementation:
```c++
// Helper function to read the whole file
static std::string Slurp(std::string_view path)
{
    std::ifstream file(path.data(), std::ios::ate);
    std::string result(file.tellg(), '\0');
    file.seekg(0);
    file.read((char*)result.data(), result.size());
    return result;
}

Shader::Shader(std::string_view vertex, std::string_view fragment)
{
    // Used to check the compilation status of the shader.
    int success = false;
    // Holds the compiler error messages, in case of an error.
    char log[1024] = {};
    
    // Reads the vertex shader
    const auto vertexShaderSource = Slurp(vertex);
    const char* vertexShaderSourcePtr = vertexShaderSource.c_str();
    // Calls OpenGL to make a new vertex shader handle
    const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // Associate the shader source to the vertex shader handle
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    // Compile the shader
    glCompileShader(vertexShader);
    // Get compilation status
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    // If there was an error, print it
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 1024, NULL, log);
        std::printf("%s\n", log);
    }

    // Reads the fragment shader 
    const auto fragmentShaderSource = Slurp(fragment);
    const char* fragmentShaderSourcePtr = fragmentShaderSource.c_str();
    // Calls OpenGL to make a new fragment shader handle
    const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // Associate the shader source to the vertex shader handle
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    // Compile the shader
    glCompileShader(fragmentShader);
    // Get compilation status
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 1024, NULL, log);
        std::printf("%s\b", log);
    }

    // Create the shader program, also called "pipeline" in other APIs
    _program = glCreateProgram();
    // Attach both the vertex and the fragment shader to the program
    glAttachShader(_program, vertexShader);
    glAttachShader(_program, fragmentShader);
    glLinkProgram(_program);
    // Get link status
    glGetProgramiv(_program, GL_LINK_STATUS, &success);
    // If linking failed, display the error message
    if (!success)
    {
        glGetProgramInfoLog(_program, 1024, NULL, log);
        std::printf("%s\b", log);
    }

    // Delete the shader handles, since we have our program they are unnecessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() = default;

void Shader::Bind() const
{
    glUseProgram(_program);
}

void Shader::Set(uint32_t location, const glm::mat4& matrix) const
{
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(matrix));
}

void Shader::Set(uint32_t location, int32_t value) const
{
    glUniform1i(location, value);
}
```

And that's it, a very basic `Shader` class.

### The Mesh class
A mesh is a collection of vertices and indices that make up the triangles
(or quads, points, etc.), of the mesh. This collection of vertices is usually
organized in a "Vertex Format", that is, what kind of data is each vertex of
a triangle composed of.

This is our vertex format:
```c++
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
};
```

Each vertex will then have a:
- `position` (vec3),
- `normal` (vec3),
- `uv` (vec2),
- `tangent` (vec4)

For a total or 12 components.

Now let's look at the `Mesh` class
```c++
struct MeshCreateInfo
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t transformIndex;
    uint32_t baseColorTexture;
    uint32_t normalTexture;
    size_t vertexOffset;
    size_t indexOffset;
};

struct MeshIndirectInfo
{
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t baseVertex;
    uint32_t baseInstance;
};

class Mesh
{
public:
    Mesh(const MeshCreateInfo& info);
    ~Mesh();

    MeshIndirectInfo Info() const;
    uint32_t TransformIndex() const;
    uint32_t BaseColorTexture() const;
    uint32_t NormalTexture() const;

private:
    uint32_t _indexCount = 0;
    int32_t _vertexOffset = 0;
    uint32_t _indexOffset = 0;
    // NOT OpenGL handles, just indices
    uint32_t _transformIndex = 0;
    uint32_t _baseColorTexture = 0;
    uint32_t _normalTexture = 0;
};
```

It's quite a bit bigger than the `Shader` class, but do not fear, there's not much going on here.

Let's begin with the constructor: it takes a `MeshCreateInfo`, which is a `struct` containing
all the information needed for the mesh to exist, and it basically copies over the information
to the class' members.

Then, there are 3 getters:
- `TransformIndex()`
- `BaseColorTexture()`
- `NormalTexture()`

Since we are putting everything in one big buffer for both transforms and textures, having
these getters is useful, because it helps us tell OpenGL which mesh uses which texture/transform.

And finally the `Info()` member function, which returns a struct `MeshIndirectInfo` and it contains
all the information OpenGL needs to draw this mesh, and they are:
- `count`: How many indices is this mesh made of
- `instanceCount`: How many instances OpenGL should draw
- `firstIndex`: The starting index offset in the Element Buffer (not a byte offset)
- `baseVertex`: The starting vertex offset in the Vertex Buffer (not a byte offset)
- `baseInstance`: Basically unused, it's a free `uint32_t` that you can use however you want, it maps to `gl_BaseInstance` in GLSL

And there's the Mesh class implementation:
```c++
#include <Project/Mesh.hpp>

#include <glad/glad.h>

Mesh::Mesh(const MeshCreateInfo& info)
{
    _indexCount = info.indices.size();
    _vertexOffset = info.vertexOffset / sizeof(Vertex);
    _indexOffset = info.indexOffset / sizeof(uint32_t);
    _transformIndex = info.transformIndex;
    _baseColorTexture = info.baseColorTexture;
    _normalTexture = info.normalTexture;
}

Mesh::~Mesh() = default;

MeshIndirectInfo Mesh::Info() const
{
    return
    {
        _indexCount,
        1,
        _indexOffset,
        _vertexOffset,
        1
    };
}

uint32_t Mesh::TransformIndex() const
{
    return _transformIndex;
}

uint32_t Mesh::BaseColorTexture() const
{
    return _baseColorTexture;
}

uint32_t Mesh::NormalTexture() const
{
    return _normalTexture;
}
```

Pretty straightforward, isn't it? Now onto the real beast, the `Model` class

### The Model class
This is a bit complicated, so far both `Shader` and `Mesh` were fairly small
but this is quite long, don't worry if you can't wrap your head around it immediately
this is not trivial at all.

Let's begin with the class declaration:
```c++
class Model
{
public:
    Model(std::string_view path);
    ~Model();

    void Draw(const Shader& shader) const;

private:
    // Holds all the meshes that compose the model
    std::vector<Mesh> _meshes;
    // Holds OpenGL texture handles
    std::vector<uint32_t> _textures;
    // Holds all the local transforms for each mesh
    std::vector<glm::mat4> _transforms;
    // OpenGL buffers
    uint32_t _vao;
    uint32_t _vbo;
    uint32_t _ibo;
    // These are vectors because we'll be batching our draws
    std::vector<uint32_t> _cmds;
    std::vector<uint32_t> _objectData;
    uint32_t _transformData;
};
```

As usual, let's look at the constructor: it takes the model path (this only supports glTF for the moment,
but you may use any model loader you want, e.g. Assimp) and it does all sort of stuff, in order:
- Reads the glTF file with [cgltf](https://github.com/jkuhlmann/cgltf)
- Loads the glTF buffers
- Reads all the materials and their textures
- Upload all the loaded textures to the GPU
- Reads all the mesh data and uploads it to the GPU

Phew, that's a lot of work we have to do, but we'll look at the implementation later.

Now about `Draw()`, this function does a little more than calling `glMultiDrawElementsIndirect()`
it actually set-ups and writes all the uniform buffers needed for the draw, mainly the object data buffer,
the texture buffer, and the transform buffer.

Finally, the implementation:
```c++
namespace fs = std::filesystem;

// Helper function to find the actual texture path given a CGLTF image.
static std::string FindTexturePath(const fs::path& basePath, const cgltf_image* image)
{
    std::string texturePath;
    if (!image->uri)
    {
        auto newPath = basePath / image->name;
        if (!newPath.has_extension())
        {
            if (std::strcmp(image->mime_type, "image/png") == 0)
            {
                newPath.replace_extension("png");
            }
            else if (std::strcmp(image->mime_type, "image/jpg") == 0)
            {
                newPath.replace_extension("jpg");
            }
        }
        texturePath = newPath.generic_string();
    }
    else
    {
        texturePath = (basePath / image->uri).generic_string();
    }
    return texturePath;
}

Model::Model(std::string_view file)
{
    cgltf_options options = {};
    cgltf_data* model = nullptr;
    // Read GLTF, no additional options are required
    cgltf_parse_file(&options, file.data(), &model);
    // Load all GLTF buffers
    cgltf_load_buffers(&options, model, file.data());

    // Get the base path (useful when loading textures)
    fs::path path(file.data());
    const auto basePath = path.parent_path();
    // This is our texture cache, to make sure we don't load the same texture twice
    std::unordered_map<std::string, size_t> textureIds;
    // Reserves space for our texture vector
    _textures.reserve(model->materials_count);
    // Since we'll be batching our draws based on the textures it has, we need to calculate
    // how many batches this model needs, this is done by dividing by 16, which is the "batch size"
    // and rounding up, because we always need at least one batch.
    const uint32_t maxBatches = model->materials_count / 16 + 1;
    for (uint32_t i = 0; i < model->materials_count; ++i) // For each material
    {
        const auto& material = model->materials[i];
        // Get the material's base color texture
        const auto* image = material.pbr_metallic_roughness.base_color_texture.texture->image;
        // Find its texture path
        const auto texturePath = FindTexturePath(basePath, image);
        if (textureIds.contains(texturePath))
        {
            // If we already loaded the texture, go onto the next material
            continue;
        }
        // Ask OpenGL to give us a new texture handle
        uint32_t texture;
        glCreateTextures(GL_TEXTURE_2D, 1, &texture);

        // Sets the texture's sampler's parameters
        // if you are not familiar with these, LearnOpenGL.com has a great tutorial
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Loads the texture data with STB_Image
        int32_t width = 0;
        int32_t height = 0;
        int32_t channels = STBI_rgb_alpha;
        const auto* textureData = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        // Calculate how many mip levels we need to generate for the texture.
        const auto levels = (uint32_t)std::floor(std::log2(std::max(width, height)));
        // Actually allocate the texture
        glTextureStorage2D(texture, levels, GL_RGBA8, width, height);
        // Copy our texture data to the GPU
        glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
        // Generate mipmaps
        glGenerateTextureMipmap(texture);
        // Free texture memory on our end
        stbi_image_free((void*)textureData);
        // Add the new texture handle to the texture vector
        _textures.emplace_back(texture);
        // Register this texture index in our cache
        textureIds[texturePath] = _textures.size() - 1;
    }
    
    uint32_t transformIndex = 0;
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    std::vector<MeshCreateInfo> meshCreateInfos;
    meshCreateInfos.reserve(1024);
    // For each node in the scene
    for (uint32_t i = 0; i < model->scene->nodes_count; ++i)
    {
        std::queue<cgltf_node*> nodes;
        // Add the node to the queue (Exercise: can you use recursion instead of a queue?)
        nodes.push(model->scene->nodes[i]);
        while (!nodes.empty())
        {
            // Get a node from the queue
            const auto* node = nodes.front();
            nodes.pop();
            // If there are no meshes
            if (!node->mesh)
            {
                // Then for each node (if any)
                for (uint32_t j = 0; j < node->children_count; ++j)
                {
                    // Add it to the queue again
                    nodes.push(node->children[j]);
                }
                continue;
            }
            // For each primitive in the node
            for (uint32_t j = 0; j < node->mesh->primitives_count; ++j)
            {
                const auto& primitive = node->mesh->primitives[j];
                const glm::vec3* positionPtr = nullptr;
                const glm::vec3* normalPtr = nullptr;
                const glm::vec2* uvPtr = nullptr;
                const glm::vec4* tangentPtr = nullptr;
                uint64_t vertexCount = 0;
                // Get its vertex data
                for (uint32_t k = 0; k < primitive.attributes_count; ++k)
                {
                    // Get the attribute information (position, normal, ...)
                    const auto& attribute = primitive.attributes[k];
                    const auto* accessor = attribute.data;
                    // Get the buffer view associated with this attribute
                    const auto* view = accessor->buffer_view;
                    const auto* dataPtr = (const char*)view->buffer->data;
                    // If this is confusing you can refer to the glTF main scheme by Khronos, it should clear up some things
                    switch (attribute.type)
                    {
                        case cgltf_attribute_type_position:
                            vertexCount = accessor->count;
                            // Set the `positionPtr`
                            positionPtr = (const glm::vec3*)(dataPtr + view->offset + accessor->offset);
                            break;

                        case cgltf_attribute_type_normal:
                            // Set the `normalPtr`
                            normalPtr = (const glm::vec3*)(dataPtr + view->offset + accessor->offset);
                            break;

                        case cgltf_attribute_type_texcoord:
                            // Set the `uvPtr`
                            uvPtr = (const glm::vec2*)(dataPtr + view->offset + accessor->offset);
                            break;

                        case cgltf_attribute_type_tangent:
                            // Set the `tangentPtr`
                            tangentPtr = (const glm::vec4*)(dataPtr + view->offset + accessor->offset);
                            break;

                        default: break;
                    }
                }
                // Reserve space for the vertices in our own vertex format
                std::vector<Vertex> vertices;
                vertices.resize(vertexCount);
                {
                    // Get the pointer to the base of the vector
                    auto* ptr = vertices.data();
                    // For each vertex
                    for (uint32_t v = 0; v < vertexCount; ++v, ++ptr)
                    {
                        // Copy the attribute (if available) to the current pointer (will increment every iteration)
                        if (positionPtr)
                        {
                            std::memcpy(&ptr->position, positionPtr + v, sizeof(glm::vec3));
                        }
                        if (normalPtr)
                        {
                            std::memcpy(&ptr->normal, normalPtr + v, sizeof(glm::vec3));
                        }
                        if (uvPtr)
                        {
                            std::memcpy(&ptr->uv, uvPtr + v, sizeof(glm::vec2));
                        }
                        if (tangentPtr)
                        {
                            std::memcpy(&ptr->tangent, tangentPtr + v, sizeof(glm::vec4));
                        }
                    }
                }

                std::vector<uint32_t> indices;
                {
                    // Get the indices information for the primitive
                    const auto* accessor = primitive.indices;
                    const auto* view = accessor->buffer_view;
                    const char* dataPtr = (const char*)view->buffer->data;
                    // Reserve space for our indices buffer
                    indices.reserve(accessor->count);
                    // Check the index type (uint8, uint16 or uin32)
                    switch (accessor->component_type)
                    {
                        // Copy the whole index buffer to our vector
                        case cgltf_component_type_r_8:
                        case cgltf_component_type_r_8u:
                        {
                            const auto* ptr = (const uint8_t*)(dataPtr + view->offset + accessor->offset);
                            std::copy(ptr, ptr + accessor->count, std::back_inserter(indices));
                        }
                        break;

                        case cgltf_component_type_r_16:
                        case cgltf_component_type_r_16u:
                        {
                            const auto* ptr = (const uint16_t*)(dataPtr + view->offset + accessor->offset);
                            std::copy(ptr, ptr + accessor->count, std::back_inserter(indices));
                        }
                        break;

                        case cgltf_component_type_r_32f:
                        case cgltf_component_type_r_32u:
                        {
                            const auto* ptr = (const uint32_t*)(dataPtr + view->offset + accessor->offset);
                            std::copy(ptr, ptr + accessor->count, std::back_inserter(indices));
                        }
                        break;

                        default: break;
                    }
                }
                // Get the primitive's material base color texture path
                const auto baseColorURI = FindTexturePath(basePath, primitive.material->pbr_metallic_roughness.base_color_texture.texture->image);
                const auto indexCount = indices.size();
                // Emplace a `MeshCreateInfo` (we will use this later)
                meshCreateInfos.emplace_back(MeshCreateInfo
                {
                    std::move(vertices),
                    std::move(indices),
                    transformIndex++,
                    // Exercise: this doesn't handle missing textures, it's possible that a mesh may not have any color
                    // texture, can you change this behavior and display a default texture of your choice when this happens?
                    (uint32_t)textureIds[baseColorURI],
                    // Exercise: We don't load normal textures, can you load the normal textures (when available)
                    // and apply some basic normal mapping?
                    0,
                    vertexOffset,
                    indexOffset,
                });
                // Apply the node transformation and emplace it to the vector
                cgltf_node_transform_world(node, glm::value_ptr(_transforms.emplace_back()));
                // Increment the vertex and index byte offset
                vertexOffset += vertexCount * sizeof(Vertex);
                indexOffset += indexCount * sizeof(uint32_t);
            }
            // Push children nodes
            for (uint32_t j = 0; j < node->children_count; ++j)
            {
                nodes.push(node->children[j]);
            }
        }
    }
    // Resize the indirect commands vector and the object data vector.
    _cmds.resize(maxBatches);
    _objectData.resize(maxBatches);
    // Allocate GL buffers
    // This is the Vertex Array Object, it specifies how the vertex data should be read by the GPU
    glCreateVertexArrays(1, &_vao);
    // This is the Vertex Buffer Object, it holds on all the mesh vertex data in this model
    glCreateBuffers(1, &_vbo);
    // This is the Element Buffer Object (I call it the Index Buffer Object, same thing)
    // it holds all the indices for all the meshes
    glCreateBuffers(1, &_ibo);
    // This is the transform data buffer, it holds the local transform for each mesh
    glCreateBuffers(1, &_transformData);
    // Create the object data buffers, they are useful when drawing because they associate all the 
    // necessary indices (transform index, base color index, ...) to a mesh, allowing the shader
    // to fetch the information from the other buffers, keep in mind this is per batch.
    glCreateBuffers(_objectData.size(), _objectData.data());
    // Finally this is the indirect data buffer, it holds all the information required for OpenGL to draw the mesh,
    // this is also per batch.
    glGenBuffers(_cmds.size(), _cmds.data());
    
    // First, we need to figure out how big our vertex and index buffer should be
    size_t vertexSize = 0;
    size_t indexSize = 0;
    for (const auto& info : meshCreateInfos)
    {
        vertexSize += info.vertices.size() * sizeof(Vertex);
        indexSize += info.indices.size() * sizeof(uint32_t);
    }
    
    // Allocate the storage
    glNamedBufferStorage(_vbo, vertexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(_ibo, indexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Associate the vertex array object, with our vertex and index buffer
    glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(_vao, _ibo);

    // Tell OpenGL which vertex location index we want to use
    // it maps to the "layout (location = N) in vecN position/normal/..." in our vertex shader
    glEnableVertexArrayAttrib(_vao, 0);
    glEnableVertexArrayAttrib(_vao, 1);
    glEnableVertexArrayAttrib(_vao, 2);
    glEnableVertexArrayAttrib(_vao, 3);

    // Tell OpenGL how to interpret each vertex attribute
    //                              location          components  type      transpose  offset
    glVertexArrayAttribFormat(_vao, /*location = */0, /*vec*/3,   GL_FLOAT, GL_FALSE,  offsetof(Vertex, position));
    glVertexArrayAttribFormat(_vao, /*location = */1, /*vec*/3,   GL_FLOAT, GL_FALSE,  offsetof(Vertex, normal));
    glVertexArrayAttribFormat(_vao, /*location = */2, /*vec*/2,   GL_FLOAT, GL_FALSE,  offsetof(Vertex, uv));
    glVertexArrayAttribFormat(_vao, /*location = */3, /*vec*/4,   GL_FLOAT, GL_FALSE,  offsetof(Vertex, tangent));

    // Finally, bind each vertex attribute to a vertex buffer,
    // the 0th buffer, which is the only one we bound
    glVertexArrayAttribBinding(_vao, 0, 0);
    glVertexArrayAttribBinding(_vao, 1, 0);
    glVertexArrayAttribBinding(_vao, 2, 0);
    glVertexArrayAttribBinding(_vao, 3, 0);

    // For each mesh
    for (auto& info : meshCreateInfos)
    {
        // Upload at a given vertex and index offset the data
        glNamedBufferSubData(
            _vbo,
            info.vertexOffset,
            info.vertices.size() * sizeof(Vertex),
            info.vertices.data());
        glNamedBufferSubData(
            _ibo,
            info.indexOffset,
            info.indices.size() * sizeof(uint32_t),
            info.indices.data());
        _meshes.emplace_back(info);
    }
}

Model::~Model() = default;

void Model::Draw(const Shader& shader) const
{
    // Define an object data structure (this should match with the one in the shader)
    struct ObjectData
    {
        uint32_t transformIndex;
        uint32_t baseColorIndex;
        uint32_t normalIndex;
    };
    struct BatchData {
        std::vector<ObjectData> objects;
        std::vector<MeshIndirectInfo> commands;
    };
    std::vector<BatchData> objectBatches(_cmds.size());
    std::vector<std::set<uint32_t>> textureHandles(_cmds.size());
    // For each mesh
    for (const auto& mesh : _meshes)
    {
        // Calculate the batch index this mesh belongs to (just divide by the "batch size")
        const auto index = mesh.baseColorTexture / 16;
        // Get the mesh indirect info structure
        objectBatches[index].commands.emplace_back(mesh.Info());
        // Get the mesh general information
        objectBatches[index].objects.emplace_back(ObjectData
        {
            // Restrict the texture range to [0, 15], because by batching texture
            // indices must not be "global", but local to the batch group
            mesh.TransformIndex(),
            mesh.BaseColorTexture() % 16,
            // Exercise: Can you do the same for normal textures?
            mesh.NormalTexture()
        });
        // Insert the texture index for this batch in a set, this is useful
        // when binding the textures because we will need unique handles
        textureHandles[index].insert(_textures[mesh.BaseColorTexture()]);
    }
    
    // Copy the transform data we just created to the GPU
    glNamedBufferData(
        _transformData,
        _transforms.size() * sizeof(glm::mat4),
        _transforms.data(),
        GL_DYNAMIC_DRAW);
    // Bind the buffer to the storage buffer, location = 1
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _transformData);

    // Bind the shader because we will be setting uniforms now
    shader.Bind();
    // For each batch
    for (uint32_t index = 0; const auto& batch : objectBatches)
    {
        // Write the object data to the current object uniform buffer 
        glNamedBufferData(
            _objectData[index],
            batch.objects.size() * sizeof(ObjectData),
            batch.objects.data(),
            GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _objectData[index]);

        // Write the indirect commands to the current indirect buffer
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _cmds[index]);
        glNamedBufferData(
            _cmds[index],
            batch.commands.size() * sizeof(MeshIndirectInfo),
            batch.commands.data(),
            GL_DYNAMIC_DRAW);

        // Set all the active textures for this batch
        for (uint32_t offset = 0; const auto texture : textureHandles[index])
        {
            shader.Set(2 + offset, offset);
            glActiveTexture(GL_TEXTURE0 + offset);
            glBindTexture(GL_TEXTURE_2D, texture);
            offset++;
        }
        
        // Finally, bind the VAO and issue the draw call
        glBindVertexArray(_vao);
        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            nullptr,
            batch.commands.size(),
            sizeof(MeshIndirectInfo));
        // Increment the index to go to the next batch
        index++;
    }
}
```

### CMake
Finally, we have to let CMake know that we need to use more files and more dependencies.
You want to add all these files in `Project/CMakeLists.txt`
```cmake
    set(sourceFiles
        Shader.cpp
        Mesh.cpp
        Model.cpp
        Main.cpp
        ProjectApplication.cpp
    )
    set(headerFiles
        include/Project/ProjectApplication.hpp
        include/Project/Shader.hpp
        include/Project/Mesh.hpp
        include/Project/Model.hpp
    )
```

About the dependencies, here's how you get them with CMake's all new `FetchContent`
This should go in `lib/CMakeLists.txt`
```cmake
#----------------------------------------------------------------------

FetchContent_Declare(
    cgltf
    GIT_REPOSITORY  https://github.com/jkuhlmann/cgltf.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(cgltf)
if(NOT cgltf_POPULATED)
    FetchContent_Populate(cgltf)
    message("Fetching cgltf")

    add_library(cgltf INTERFACE ${cgltf_SOURCE_DIR}/cgltf.h)
    target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})
endif()

#----------------------------------------------------------------------

FetchContent_Declare(
    stb_image
    GIT_REPOSITORY  https://github.com/nothings/stb.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(stb_image)
if(NOT stb_image_POPULATED)
    FetchContent_Populate(stb_image)
    message("Fetching stb_image")

    add_library(stb_image INTERFACE ${stb_image_SOURCE_DIR}/stb_image.h)
    target_include_directories(stb_image INTERFACE ${stb_image_SOURCE_DIR})
endif()
```

If you managed to reach the end, then you should have all the tools necessary to
keep expanding your engine to your liking (make sure to read all the comments),
but if you are very confused, there are many resources that explain every
little detail in Modern OpenGL, you could also decide to completely ditch
Modern OpenGL and instead use only Core GL 3.3 features, like LearnOpenGL.com
does it's all up to you, just remember that your engine will always have room 
for improvement, keep learning.
