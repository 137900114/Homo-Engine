#pragma once
#include <string>
#include <vector>
#include "Buffer.h"

/*

the structures below mostly just are copy and paste of the d3d12.h structures
typedef
enum D3D12_COMPARISON_FUNC
    {
        D3D12_COMPARISON_FUNC_NEVER	= 1,
        D3D12_COMPARISON_FUNC_LESS	= 2,
        D3D12_COMPARISON_FUNC_EQUAL	= 3,
        D3D12_COMPARISON_FUNC_LESS_EQUAL	= 4,
        D3D12_COMPARISON_FUNC_GREATER	= 5,
        D3D12_COMPARISON_FUNC_NOT_EQUAL	= 6,
        D3D12_COMPARISON_FUNC_GREATER_EQUAL	= 7,
        D3D12_COMPARISON_FUNC_ALWAYS	= 8
    } 	D3D12_COMPARISON_FUNC;
*/
/*
typedef
enum D3D12_DEPTH_WRITE_MASK
    {
        D3D12_DEPTH_WRITE_MASK_ZERO	= 0,
        D3D12_DEPTH_WRITE_MASK_ALL	= 1
    } 	D3D12_DEPTH_WRITE_MASK;
*/
/*
typedef
enum D3D12_STENCIL_OP
    {
        D3D12_STENCIL_OP_KEEP	= 1,
        D3D12_STENCIL_OP_ZERO	= 2,
        D3D12_STENCIL_OP_REPLACE	= 3,
        D3D12_STENCIL_OP_INCR_SAT	= 4,
        D3D12_STENCIL_OP_DECR_SAT	= 5,
        D3D12_STENCIL_OP_INVERT	= 6,
        D3D12_STENCIL_OP_INCR	= 7,
        D3D12_STENCIL_OP_DECR	= 8
    } 	D3D12_STENCIL_OP;
*/
/*
typedef struct D3D12_DEPTH_STENCILOP_DESC
    {
    D3D12_STENCIL_OP StencilFailOp;
    D3D12_STENCIL_OP StencilDepthFailOp;
    D3D12_STENCIL_OP StencilPassOp;
    D3D12_COMPARISON_FUNC StencilFunc;
    } 	D3D12_DEPTH_STENCILOP_DESC;
*/
/*
typedef struct D3D12_DEPTH_STENCIL_DESC
    {
    BOOL DepthEnable;
    D3D12_DEPTH_WRITE_MASK DepthWriteMask;
    D3D12_COMPARISON_FUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3D12_DEPTH_STENCILOP_DESC FrontFace;
    D3D12_DEPTH_STENCILOP_DESC BackFace;
    } 	D3D12_DEPTH_STENCIL_DESC;

*/
/*
    typedef struct D3D12_RENDER_TARGET_BLEND_DESC
    {
    BOOL BlendEnable;
    BOOL LogicOpEnable;
    D3D12_BLEND SrcBlend;
    D3D12_BLEND DestBlend;
    D3D12_BLEND_OP BlendOp;
    D3D12_BLEND SrcBlendAlpha;
    D3D12_BLEND DestBlendAlpha;
    D3D12_BLEND_OP BlendOpAlpha;
    D3D12_LOGIC_OP LogicOp;
    UINT8 RenderTargetWriteMask;
    } 	D3D12_RENDER_TARGET_BLEND_DESC;

*/
/*
    typedef
enum D3D12_BLEND_OP
    {
        D3D12_BLEND_OP_ADD	= 1,
        D3D12_BLEND_OP_SUBTRACT	= 2,
        D3D12_BLEND_OP_REV_SUBTRACT	= 3,
        D3D12_BLEND_OP_MIN	= 4,
        D3D12_BLEND_OP_MAX	= 5
    } 	D3D12_BLEND_OP;

*/


namespace Game {
	enum ShaderTopologyType {
		SHADER_TOPOLOGY_TYPE_TRIANGLE = 3,
		SHADER_TOPOLOGY_TYPE_LINE = 2
	};

    enum ShaderDepthStencilOperation {
        SHADER_DS_OPERATION_KEEP = 1,
        SHADER_DS_OPERATION_ZERO = 2,
        SHADER_DS_OPERATION_REPLACE = 3,
        SHADER_DS_OPERATION_INCR_SAT = 4,
        SHADER_DS_OPERATION_DECR_SAT = 5,
        SHADER_DS_OPERATION_INVERT = 6,
        SHADER_DS_OPERATION_INCR = 7,
        SHADER_DS_OPERATION_DECR = 8
    };

    enum ShaderBlendOperation
    {
        SHADER_BLEND_OP_ADD = 1,
        SHADER_BLEND_OP_SUBTRACT = 2,
        SHADER_BLEND_OP_REV_SUBTRACT = 3,
        SHADER_BLEND_OP_MIN = 4,
        SHADER_BLEND_OP_MAX = 5
    };


    enum ShaderCamparisionFunction {
        SHADER_COMPARISON_FUNC_NEVER = 1,
        SHADER_COMPARISON_FUNC_LESS = 2,
        SHADER_COMPARISON_FUNC_EQUAL = 3,
        SHADER_COMPARISON_FUNC_LESS_EQUAL = 4,
        SHADER_COMPARISON_FUNC_GREATER = 5,
        SHADER_COMPARISON_FUNC_NOT_EQUAL = 6,
        SHADER_COMPARISON_FUNC_GREATER_EQUAL = 7,
        SHADER_COMPARISON_FUNC_ALWAYS = 8
    };

    enum ShaderBlendState
    {
        SHADER_BLEND_ZERO = 1,
        SHADER_BLEND_ONE = 2,
        SHADER_BLEND_SRC_COLOR = 3,
        SHADER_BLEND_INV_SRC_COLOR = 4,
        SHADER_BLEND_SRC_ALPHA = 5,
        SHADER_BLEND_INV_SRC_ALPHA = 6,
        SHADER_BLEND_DEST_ALPHA = 7,
        SHADER_BLEND_INV_DEST_ALPHA = 8,
        SHADER_BLEND_DEST_COLOR = 9,
        SHADER_BLEND_INV_DEST_COLOR = 10,
        SHADER_BLEND_SRC_ALPHA_SAT = 11,
        SHADER_BLEND_FACTOR = 14,
        SHADER_BLEND_INV_BLEND_FACTOR = 15,
        SHADER_BLEND_SRC1_COLOR = 16,
        SHADER_BLEND_INV_SRC1_COLOR = 17,
        SHADER_BLEND_SRC1_ALPHA = 18,
        SHADER_BLEND_INV_SRC1_ALPHA = 19
    };

    enum ShaderLanguageType {
        SHADER_LANGUAGE_HLSL
        //,GLSL currently we don't support glsl translation
    };

    enum ShaderCullMode {
        SHADER_CULL_MODE_NONE = 1,
        SHADER_CULL_MODE_FRONT = 2,
        SHADER_CULL_MODE_BACK = 3
    };

    enum ShaderFillMode
    {
        SHADER_FILL_MODE_WIREFRAME = 2,
        SHADER_FILL_MODE_SOLID = 3
    };

    enum ShaderEntryType {
        SHADER_ENTRY_TYPE_VS, 
        //SHADER_ENTRY_TYPE_GS, 
        //SHADER_ENTRY_TYPE_HS, 
        //SHADER_ENTRY_TYPE_DS, 
        SHADER_ENTRY_TYPE_PS
    };

    enum ShaderBufferType {
        //SHADER_BUFFER_TYPE_TEXTURE,
        SHADER_BUFFER_TYPE_CONSTANT
    };

    struct ShaderBuffer {
        uint32_t regID;
        ShaderBufferType type;
    };

    struct ShaderParameter {
        enum ShaderParameterType {
            INT = 0,//a 32 bit int number
            FLOAT = 1,//a 32 bit float number
            FLOAT2 = 2,//a 2 dimension 32 bit float vector
            FLOAT3 = 3,//a 3 dimension 32 bit float vector
            FLOAT4 = 4,//a 4 dimension 32 bit float vector
            FLOAT4X4 = 5,//a 4x4 dimension 32 bit float matrix
            TEXTURE2D = 0x10,//a dimension texture
            TEXTURECUBE = 0x11,// a cube texture

            //the owned parameters will be passed to object constant buffer in register slot 0
            OWNED_INT = 0x20,
            OWNED_FLOAT = 0x21,
            OWNED_FLOAT2 = 0x22,
            OWNED_FLOAT3 = 0x23,
            OWNED_FLOAT4 = 0x24,
            OWNED_FLOAT4X4 = 0x25
        } type;

        enum ShaderParameterAttribute {
            SHARED,//the Shader parameter is shadered among all the objects
            OWNED,//the Shader parameter belong to one object.one object may have a different value to one other object 
            INVAILD
        } attribute;

        std::string name;
        size_t offset;//the offset in the buffer for constants
        size_t padding_size;
        size_t size;
        //the register will be used for constant buffer or texture buffer
        //if the register id is -1 system will allocate a new reg id
        int regID;

        ShaderParameter() : attribute(INVAILD) {

        }


        //a shared constant shader parameter constructor
        ShaderParameter(std::string name, size_t offset, size_t padding_size, int regID, ShaderParameterType SPType);
        //shared texture parameter constructor
        ShaderParameter(std::string name, int regID, ShaderParameterType SPType);

        //a owned constant shader parameter constuctor
        ShaderParameter(std::string& name, ShaderParameterType SPType);

        bool isSharedConstant() { return type <= FLOAT4X4; }
        bool isSharedTexture() { return type <= TEXTURECUBE && type >= TEXTURE2D; }

        bool isOwnedConstant() { return type <= OWNED_FLOAT4X4 && type >= OWNED_INT; }
    };

    class ShaderContent {
        friend class ShaderParser;
    public:
        bool GetShaderContent(ShaderEntryType entry, char** data, size_t* size) {
            for (uint32_t i = 0; i != descirptorNum; i++) {
                if (entry == EntryDescriptors[i].type) {
                    *data = EntryDescriptors[i].data;
                    *size = EntryDescriptors[i].size;
                    return true;
                }
            }
            return false;
        }
        inline ShaderLanguageType GetShaderLanguage() {
            return type;
        }

    private:

        struct ShaderEntryDescriptor {
            ShaderEntryType type;
            char* data;
            size_t size;
        };

        Buffer codeData;
        ShaderLanguageType type;
        ShaderEntryDescriptor* EntryDescriptors;
        uint32_t descirptorNum;
    };

	struct Shader {

        Shader(std::string name = ""):name(name) {
            DepthEnable = true, StencilEnable = false;
            
            fileName = "";
            pipelineStateObject = "";

            topologyType = SHADER_TOPOLOGY_TYPE_TRIANGLE;
            DepthStencilDesc.stencilFunc = SHADER_COMPARISON_FUNC_ALWAYS;
            DepthStencilDesc.depthFunc = SHADER_COMPARISON_FUNC_LESS;
            DepthStencilDesc.stencilDepthFail = SHADER_DS_OPERATION_KEEP;
            DepthStencilDesc.stencilFail = SHADER_DS_OPERATION_KEEP;
            DepthStencilDesc.stencilPassOp = SHADER_DS_OPERATION_KEEP;

            BlendEnable = false;
            BlendDesc.blendSrc = SHADER_BLEND_ONE;
            BlendDesc.blendDest = SHADER_BLEND_ZERO;
            BlendDesc.blendOp = SHADER_BLEND_OP_ADD;
            BlendDesc.blendAlphaSrc = SHADER_BLEND_ONE;
            BlendDesc.blendAlphaDest = SHADER_BLEND_ZERO;
            BlendDesc.blendAlphaOp = SHADER_BLEND_OP_ADD;
            

            RasterizerState.cullMode = SHADER_CULL_MODE_BACK;
            RasterizerState.fillMode = SHADER_FILL_MODE_SOLID;
        }

        

        bool isAvalid() {
            return !pipelineStateObject.empty();
        }
        
        std::string fileName;
		std::string name;

        std::string pipelineStateObject;

        ShaderTopologyType topologyType;
        
        bool DepthEnable, StencilEnable;
        struct {
            
            ShaderDepthStencilOperation stencilFail;
            ShaderDepthStencilOperation stencilDepthFail;
            ShaderDepthStencilOperation stencilPassOp;
            ShaderCamparisionFunction   stencilFunc;
            ShaderCamparisionFunction   depthFunc;
        } DepthStencilDesc;

        
        bool BlendEnable;
        struct {    
            ShaderBlendState blendSrc;
            ShaderBlendState blendDest;
            ShaderBlendOperation blendOp;
            ShaderBlendState blendAlphaSrc;
            ShaderBlendState blendAlphaDest;
            ShaderBlendOperation blendAlphaOp;
        }   BlendDesc;


        struct {
            ShaderCullMode cullMode;
            ShaderFillMode fillMode;
        } RasterizerState;

        ShaderContent content;
        std::vector<ShaderParameter> shaderParameters;
        std::vector<ShaderBuffer> shaderBuffers;

        bool lightEnabled;
        bool isVaild;
	};
}