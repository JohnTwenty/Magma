#include "App\stdAfx.h"
#include "Foundation\Foundation.h"
#include "Foundation\MemoryManager.h"
#include "Foundation\FileManager.h"
#include "RendererD3D11.h"

#pragma warning(push)
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#pragma warning( disable : 4987)//nonstandard extension used
#pragma warning( disable : 4061)//enum is not explicitly handled by a case label
#pragma warning( disable : 4548) //expression before comma has no effect (malloc.h)

#include <d3dcompiler.h>

#pragma warning( disable : 4917) //GUID can only be associated with a class
#include <d3d11_2.h>
#include <d3d11shader.h>		//for shader reflection used by CreateInputLayoutDescFromVertexShaderSignature
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib") 

#pragma warning(pop)



static const unsigned vertexShaderConstbufferSize = sizeof(float) * 16;	//must be multiples of 16
static const unsigned pixelShaderConstbufferSize = sizeof(float) * 16;

ID3D11Buffer* tilePool = nullptr;

#if defined(_DEBUG)
#define MAGMA_REPORT_D3D11_OBJECTS 0
#endif

TexturePtr::TexturePtr(ID3D11Buffer * b, TextureType::Enum  ty) : type(ty), texture(b), constantBuffer(b) {}


RendererD3D11::RendererD3D11(void) : renderWithCompute(false), device(nullptr), deviceContext(nullptr), swapChain(nullptr), rasterState(nullptr)
//,vertexShaderConstantsBuffer(nullptr), pixelShaderConstantsBuffer(nullptr)
	{

	ID3D11Buffer* b = nullptr;
	ID3D11Resource* texture = nullptr;

	//b = texture;
	texture = b;

	}

void RendererD3D11::shutDown()
	{
	if (tilePool)
		{
		tilePool->Release(); tilePool = nullptr;
		}
	if (sampleState)
		{
		sampleState->Release(); sampleState = nullptr;
		}
	/*
	if (vertexShaderConstantsBuffer)
		{
		vertexShaderConstantsBuffer->Release();	vertexShaderConstantsBuffer = nullptr;
		}
	if (pixelShaderConstantsBuffer)
		{
		pixelShaderConstantsBuffer->Release();	pixelShaderConstantsBuffer = nullptr;
		}
	*/
	if (swapChain)
		{
		swapChain->Release();		swapChain = nullptr;
		}
	//AM: I don't get why we had this stuff here, we have it again later.
	//		deviceContext->ClearState();
	//		deviceContext->Flush();
	if (rasterState)
		{
		rasterState->Release();			rasterState = nullptr;
		}
	if (deviceContext)
		{
		deviceContext->ClearState();
		deviceContext->Flush();
		deviceContext->Release();	deviceContext = nullptr;
		}
	//AM: if fraps is active, the below can crash because it seems the devicecontext also destroys the device...so we have a dangling pointer....
	if (device)
		{
		device->Release();			device = nullptr;
		}
#if MAGMA_REPORT_D3D11_OBJECTS
	if (debug)
		{
		//reporting the ID3D11Device as live is OK because this debug object still holds a reference to it!
		debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
		debug->Release();
		}
#endif
	}

void RendererD3D11::createContext(unsigned width, unsigned height, bool renderWCompute)
	{
	renderWithCompute = renderWCompute;

	//This is now considered a 'lazy' way to buffer and present.  It is not supported for windows store apps. https://docs.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
	//What we should be doing is creating a float buffer with ID3D11Device::CreateTexture2D, use that for HDR rendering, and then resample (with quantization, scaling/multi-samling) 
	//into an 8bit per channel back buffer with DXGI_SWAP_EFFECT_FLIP_DISCARD using ID3D11DeviceContext::ResolveSubresource.

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;					//DXGI_SWAP_EFFECT_FLIP_DISCARD needs us to make this at least 2, but then our approach to read from the buffer for TXAA becomes an issue.... 
	sd.BufferDesc.Width = (UINT)width;
	sd.BufferDesc.Height = (UINT)height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //for some reason 32 bit float render target doesn't work.  DXGI_FORMAT_R16G16B16A16_FLOAT does, but creates precision artifacts.  So we just create a float intermediate buffer for renderings.

	/*
	https://www.gamedev.net/forums/topic/686855-how-to-gamma-correct-10-bit-frame-buffer/
	emitting float4(1.0, 0.5, 0.25, 0.0) in the compute shader results in
	DXGI_FORMAT_R8G8B8A8_UNORM --> 255,127,64
	DXGI_FORMAT_R16G16B16A16_FLOAT --> 255,188,137

	So basically DXGI_FORMAT_R16G16B16A16_FLOAT is automatically gamma corrected for me, while DXGI_FORMAT_R8G8B8A8_UNORM emits directly.
	I am guessing if HDR displays ever become real, we will want to use DXGI_FORMAT_R16G16B16A16_FLOAT.

	assuming gamma = pow( saturate(input), float3(1.0/2.2,1.0/2.2,1.0/2.2) ),
	*/


	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | (renderWithCompute ? DXGI_USAGE_UNORDERED_ACCESS : 0);
	sd.OutputWindow = GetActiveWindow();
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	//The only two non-deprecated options are DXGI_SWAP_EFFECT_FLIP_DISCARD or DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL.  The discard options are confusing to work with because
	//the data isn't practically wiped for performance reasons anyways, but interactions with it have undefined behavior, which makes for tricky debugging. 
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	
	
//DXGI_SWAP_EFFECT_FLIP_DISCARD;	//a warning asked us to use this instead of the default.  Doesn't discard mean that we should not be reading from the back buffer!!?


	D3D_FEATURE_LEVEL  featureLevelsRequested = D3D_FEATURE_LEVEL_12_0; // D3D_FEATURE_LEVEL_11_1;	//Need 11.1 for volume tiled resources, need 11.3 == 12 for read+write multichannel textures. https://learn.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
	UINT               numLevelsRequested = 1;
	D3D_FEATURE_LEVEL  featureLevelsSupported;

	UINT flags = 0;

#if defined(_DEBUG)
	// If the project is in a debug build, enable the debug layer.
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (swapChain)
		return;		//we already have a swap chain, user should not have called this more than once!

	ID3D11DeviceContext* dc;
	ID3D11Device* d;

	if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &featureLevelsRequested, numLevelsRequested,
		D3D11_SDK_VERSION, &sd, &swapChain, &d, &featureLevelsSupported, &dc)))
		{
		foundation.fatal("D3D11CreateDeviceAndSwapChain failed");
		return;
		}
	//upgrade the device to use tiled resources
	if (FAILED(d->QueryInterface(IID_PPV_ARGS(&device))))
		{
		foundation.fatal("ID3D11Device2 interface unavailable on device");
		return;
		}

	d->Release();

	//upgrade to DeviceContext2 to use tiled resources
	if (FAILED(dc->QueryInterface(IID_PPV_ARGS(&deviceContext))))
		{
		foundation.fatal("ID3D11DeviceContext2 interface unavailable on device");
		return;
		}

	dc->Release();



#if MAGMA_REPORT_D3D11_OBJECTS
	//test naming my d3d objects.
	const char c_szName[] = "mydevice";
	device->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);

	device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug));
#endif

	D3D11_VIEWPORT vp[1];
	vp[0].Width = static_cast<float>(width);
	vp[0].Height = static_cast<float>(height);
	vp[0].MinDepth = 0;
	vp[0].MaxDepth = 1;
	vp[0].TopLeftX = 0;
	vp[0].TopLeftY = 0;
	deviceContext->RSSetViewports(1, vp);

	// set up a constant buffer for passing const params like matrices to vertex shaders
/*
	D3D11_BUFFER_DESC cd;

	cd.Usage = D3D11_USAGE_DYNAMIC;
	cd.ByteWidth = vertexShaderConstbufferSize;
	cd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cd.MiscFlags = 0;
	cd.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	if (FAILED(device->CreateBuffer(&cd, nullptr, &vertexShaderConstantsBuffer)))
		{
		foundation.fatal("D3DDevice::CreateBuffer for const buffer failed!");
		}

	cd.ByteWidth = pixelShaderConstbufferSize;
	if (FAILED(device->CreateBuffer(&cd, nullptr, &pixelShaderConstantsBuffer)))
	{
		foundation.fatal("D3DDevice::CreateBuffer for const buffer failed!");
	}

	//set it too, cause we will for the time being always be using it

	// Finanly set the constant buffers
	deviceContext->VSSetConstantBuffers(0, 1, &vertexShaderConstantsBuffer);

	if (!renderWithCompute)
		deviceContext->PSSetConstantBuffers(0, 1, &pixelShaderConstantsBuffer);
	else
		deviceContext->CSSetConstantBuffers(0, 1, &pixelShaderConstantsBuffer);
*/

//I believe this is the default D3D11 render state:

	currentState.flags.set(RenderStateFlags::eFILL);
	currentState.flags.set(RenderStateFlags::eCULL_BACK);

	//for texturing make a default sampler.  
	//TODO: if we want to manipulate this in a data driven way we need to create a texture resource ODS script ... for now just go with some default behavior.
	D3D11_SAMPLER_DESC samplerDesc;

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	// Create the texture sampler state.
	if (FAILED(device->CreateSamplerState(&samplerDesc, &sampleState)))
		{
		foundation.fatal("D3DDevice::CreateSamplerState failed!");
		}

	// Set the sampler state in the pixel shader.
	deviceContext->PSSetSamplers(0, 1, &sampleState);

	}

void RendererD3D11::setRenderState(const RenderState& newState)
	{
	if (currentState.flags != newState.flags)
		{
		if (rasterState != nullptr)
			{
			rasterState->Release();	//TODO: these should get persisted and recycled for perf
			rasterState = nullptr;
			}

		D3D11_RASTERIZER_DESC rastDesc;
		ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
		rastDesc.FillMode = (newState.flags & RenderStateFlags::eFILL) ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
		rastDesc.CullMode = (newState.flags & RenderStateFlags::eCULL_FRONT) ? D3D11_CULL_FRONT : ((newState.flags & RenderStateFlags::eCULL_BACK) ? D3D11_CULL_BACK : D3D11_CULL_NONE);
		device->CreateRasterizerState(&rastDesc, &rasterState);
		deviceContext->RSSetState(rasterState);


		}

	currentState = newState;
	}


TexturePtr RendererD3D11::createSwapChain()
	{
	//this is more like getSwapChain -- since we already created it.
	ID3D11Texture2D* texture;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& texture);

	if (!renderWithCompute)
		{
		// use the back buffer address to create the render target
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//gamma corrected back buffer
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		ID3D11RenderTargetView* backBuffer;
		device->CreateRenderTargetView(texture, &rtvDesc, &backBuffer);
		return TexturePtr(texture, backBuffer);
		}
	else
		{
		//create compute destination -- hopefully I can do this in addition to creating regular back buffer with no penalty.  Normally only one or the other is needed.
		ID3D11UnorderedAccessView* computeBackBuffer;
		device->CreateUnorderedAccessView(texture, nullptr, &computeBackBuffer);
		//deviceContext->CSSetUnorderedAccessViews(0, 1, &computeBackBuffer, 0);
		//computeBackBuffer->Release();	//I don't think I need this anymore.
		return TexturePtr(texture, computeBackBuffer);
		}

	}

TexturePtr RendererD3D11::createRenderTarget()
	{
	D3D11_TEXTURE2D_DESC desc;

	desc.Width = (UINT)1024;	//TODO: let user set this -- note: shader code must match this!
	desc.Height = (UINT)1024;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	ID3D11Texture2D* texture = nullptr;
	ID3D11UnorderedAccessView* accessView = nullptr;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, &texture)))
		{
		foundation.printLine("RendererD3D11::createRenderTarget(): CreateTexture2D failed!");
		return TexturePtr();
		}

	if (FAILED(device->CreateUnorderedAccessView(texture, nullptr, &accessView)))
		{
		foundation.printLine("RendererD3D11::createRenderTarget(): CreateUnorderedAccessView failed!");
		texture->Release();
		return TexturePtr();
		}

	return TexturePtr(texture, accessView);

	}


//unsigned frameCount = 0;

void RendererD3D11::present()
	{
//we need to unbind the structured buffer before presenting to avoid a warning from DX
    ID3D11UnorderedAccessView* nullView = nullptr;
    deviceContext->CSSetUnorderedAccessViews(0, 1, &nullView, nullptr);
    		

	UINT vSync = 0;	//1 or 0
	swapChain->Present(vSync, 0);
	}

void RendererD3D11::compute(unsigned xw, unsigned yw, unsigned zw)
	{
//if (frameCount < 100) 
	deviceContext->Dispatch(xw, yw, zw);
	}

//source: https://takinginitiative.wordpress.com/category/graphics-programming/hlsl/
HRESULT CreateInputLayoutDescFromVertexShaderSignature(const void* pShaderBlobPtr, size_t pShaderBlobSize, ID3D11Device* pD3DDevice, ID3D11InputLayout** pInputLayout)
	{
	// Reflect shader info
	ID3D11ShaderReflection* pVertexShaderReflection = nullptr;
	if (FAILED(D3DReflect(pShaderBlobPtr, pShaderBlobSize, IID_ID3D11ShaderReflection, (void**)& pVertexShaderReflection)))
		return S_FALSE;

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	if (FAILED(pVertexShaderReflection->GetDesc(&shaderDesc)))
		return S_FALSE;


	if (shaderDesc.InputParameters > 16)
		return S_FALSE;	//that can't be right!


	// Read input layout description from shader info
	D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc = (D3D11_INPUT_ELEMENT_DESC*)memoryManager.allocate(sizeof(D3D11_INPUT_ELEMENT_DESC) * shaderDesc.InputParameters);//tbh this could go on stack
	for (unsigned i = 0; i < shaderDesc.InputParameters; i++)
		{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC& elementDesc = inputLayoutDesc[i];
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = i;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1)
			{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
		else if (paramDesc.Mask <= 3)
			{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
		else if (paramDesc.Mask <= 7)
			{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
		else if (paramDesc.Mask <= 15)
			{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

		}

	// Try to create Input Layout
	HRESULT hr = pD3DDevice->CreateInputLayout(&inputLayoutDesc[0], shaderDesc.InputParameters, pShaderBlobPtr, pShaderBlobSize, pInputLayout);

	memoryManager.deallocate(inputLayoutDesc);
	//Free allocation shader reflection memory
	pVertexShaderReflection->Release();
	return hr;
	}


ShaderPtr RendererD3D11::createShader(const void* code, size_t length, const char* name, ShaderType::Enum type, bool source, const char* cacheFName, VertexAttributes* va)
	{
	ID3D10Blob* blob = nullptr;
	ID3D10Blob* errorMessage = nullptr;
	const void* shaderObjCode = nullptr;
	size_t shaderObjCodeSize = 0;

#if D3D_COMPILER_VERSION != 47
#error
#endif


	if (source)
		{
		class CShaderInclude : public ID3DInclude
			{
			public:
				// shaderDir: Directory of current shader file, used by #include "FILE"
				// systemDir: Default shader includes directory, used by #include <FILE>
				CShaderInclude()
					{
					}

				HRESULT __stdcall Open(
					D3D_INCLUDE_TYPE IncludeType,
					LPCSTR pFileName,
					LPCVOID pParentData,
					LPCVOID* ppData,
					UINT* pBytes)
					{
					(void)pParentData; //unreferenced parameter warning
					(void)IncludeType; //unreferenced parameter warning
					size_t lengthOut;
					*ppData = fileManager.loadFile(pFileName, &lengthOut);
					*pBytes = static_cast<UINT>(lengthOut);
					return *ppData ? 0 : ERROR_OPEN_FAILED;
					}


				HRESULT __stdcall Close(LPCVOID pData)
					{
					fileManager.unloadFile(pData);
					return 0;
					}
			} includeObj;


		HRESULT result = D3DCompile(code, length, name, nullptr, &includeObj, "main", type == ShaderType::eVERTEX ? "vs_5_0" : (type == ShaderType::ePIXEL ? "ps_5_0" : "cs_5_0"), D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &errorMessage);

		if (FAILED(result))
			{

			if (errorMessage)
				{
				foundation.printLine("RendererD3D11::createShader() D3DX11Compile() error:", (char*)errorMessage->GetBufferPointer());
				errorMessage->Release();
				}
			if (blob)
				blob->Release();
			return ShaderPtr();
			}

		shaderObjCode = blob->GetBufferPointer();
		shaderObjCodeSize = blob->GetBufferSize();
		//cache the compiled shader 
		if (cacheFName)
			fileManager.saveFile(cacheFName, shaderObjCode, shaderObjCodeSize, false);
		else
			foundation.printLine("RendererD3D11::createShader() D3DX11Compile() warning:", "no path to cache shader provided!");


		}
	else
		{
		//create blob from file
		shaderObjCode = code;
		shaderObjCodeSize = length;

		}


	ASSERT(device);

	if (type == ShaderType::eVERTEX)
		{
		ID3D11InputLayout* inputLayout = nullptr;
		ID3D11VertexShader* vertexShader = nullptr;
		HRESULT result = device->CreateVertexShader(shaderObjCode, shaderObjCodeSize, nullptr, &vertexShader);
		if (FAILED(result))
			foundation.printLine("RendererD3D11::createShader(): CreateVertexShader() failed.");



		//create the input layout for the vertex shader
		//this layout must only be created once for all shaders that share this same layout
		//we make a new one each time which could be optimized later if we end up with a lot of shaders
		if (!va)
			{
			//infer input layout!
			//ATTENTION: using inferred input layouts means the hlsl file input must match the mesh vertex buffer format EXACTLY because the remapping that we otherwise get by providing a mapping that matches the VB is lost!
			if (FAILED(CreateInputLayoutDescFromVertexShaderSignature(shaderObjCode, shaderObjCodeSize, device, &inputLayout)))
				foundation.printLine("RendererD3D11::createShader(): CreateInputLayoutDescFromVertexShaderSignature() failed.");
			}
		else
			{
			D3D11_INPUT_ELEMENT_DESC ied[8];
			ASSERT(va->attribs.size() <= sizeof(ied) / sizeof(D3D11_INPUT_ELEMENT_DESC));//we will be copying to the static sized array above.
			unsigned vertexBufferSlot = 0;
			for (unsigned i = 0; i < va->attribs.size(); i++)
				{
				switch (va->attribs[i])
					{
					case VertexAttributes::ePosition3f:
						ied[i].SemanticName = "POSITION";	//this is the name as it stands in the vertex shader!
						ied[i].SemanticIndex = 0;			//TODO: if we can have more than one of this attrib, this needs to count up!
						ied[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
						ied[i].InputSlot = vertexBufferSlot++;
						ied[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
						ied[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						ied[i].InstanceDataStepRate = 0;
						break;
					case VertexAttributes::eColor4f:
						ied[i].SemanticName = "COLOR";		//TODO: this mapping is a duplicate of attributeMap[]
						ied[i].SemanticIndex = 0;			//TODO: if we can have more than one of this attrib, this needs to count up!
						ied[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
						ied[i].InputSlot = vertexBufferSlot++;
						ied[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
						ied[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						ied[i].InstanceDataStepRate = 0;
						break;
					case VertexAttributes::eNormal3f:
						ied[i].SemanticName = "NORMAL";		//TODO: this mapping is a duplicate of attributeMap[]
						ied[i].SemanticIndex = 0;			//TODO: if we can have more than one of this attrib, this needs to count up!
						ied[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
						ied[i].InputSlot = vertexBufferSlot++;
						ied[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
						ied[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						ied[i].InstanceDataStepRate = 0;
						break;
					default:
						ASSERT(0);//TODO: implement / unknown type
					}
				}

			HRESULT result2 = device->CreateInputLayout(ied, va->attribs.size(), shaderObjCode, shaderObjCodeSize, &inputLayout);
			if (FAILED(result2))
				foundation.printLine("RendererD3D11::createShader(): CreateInputLayout() failed");
			}

		if (blob)
			blob->Release();

		return ShaderPtr(type, vertexShader, inputLayout);
		}
	else if (type == ShaderType::ePIXEL)
		{
		ID3D11PixelShader* pixelShader = nullptr;
		HRESULT result = device->CreatePixelShader(shaderObjCode, shaderObjCodeSize, nullptr, &pixelShader);
		if (FAILED(result))
			foundation.printLine("RendererD3D11::createShader(): CreatePixelShader() failed");
		if (blob)
			blob->Release();
		return ShaderPtr(type, pixelShader);
		}
	else
		{
		ASSERT(type == ShaderType::eCOMPUTE);
		ID3D11ComputeShader* computeShader = nullptr;
		HRESULT result = device->CreateComputeShader(shaderObjCode, shaderObjCodeSize, nullptr, &computeShader);
		if (FAILED(result))
			foundation.printLine("RendererD3D11::createShader(): CreateComputeShader() failed");
		if (blob)
			blob->Release();
		return ShaderPtr(type, computeShader);
		}
	}

void RendererD3D11::releaseShader(ShaderPtr ptr)
	{
	if (ptr.inputLayout)
		ptr.inputLayout->Release();
	if (ptr.shader)
		ptr.shader->Release();
	}

void RendererD3D11::bindShader(ShaderPtr ptr)
	{
	if (ptr.shader)
		{
		if (ptr.type == ShaderType::eVERTEX)
			{
			if (ptr.inputLayout)
				deviceContext->IASetInputLayout(ptr.inputLayout);
			deviceContext->VSSetShader(static_cast<ID3D11VertexShader*>(ptr.shader), 0, 0);
			}
		else if (ptr.type == ShaderType::ePIXEL)
			deviceContext->PSSetShader(static_cast<ID3D11PixelShader*>(ptr.shader), 0, 0);
		else
			{
			ASSERT(ptr.type == ShaderType::eCOMPUTE);
			deviceContext->CSSetShader(static_cast<ID3D11ComputeShader*>(ptr.shader), 0, 0);
			}
		}
	}

GeometryArray RendererD3D11::createVertexArray(const float* vertexData, MxU32 vertexSizeBytes, MxU32 vertexCount)
	{
	ID3D11Buffer* vertexBuffer;
	D3D11_BUFFER_DESC bd;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = vertexSizeBytes * vertexCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA sr;
	sr.pSysMem = vertexData;
	sr.SysMemPitch = 0;
	sr.SysMemSlicePitch = 0;

	HRESULT result = device->CreateBuffer(&bd, &sr, &vertexBuffer);       // create the buffer
	if (FAILED(result))
		{
		foundation.printLine("RendererD3D11::createVertexArray(): CreateBuffer() failed!");
		}
	return GeometryArray(vertexBuffer, vertexSizeBytes, vertexCount);
	}

GeometryArray RendererD3D11::createIndexArray(const MxU32* indexData, MxU32 indexSizeBytes, MxU32 indexCount)
	{
	// Fill in a buffer description.
	ID3D11Buffer* indexBuffer;
	D3D11_BUFFER_DESC bd;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = indexSizeBytes * indexCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	// Define the resource data.
	D3D11_SUBRESOURCE_DATA sr;
	sr.pSysMem = indexData;
	sr.SysMemPitch = 0;
	sr.SysMemSlicePitch = 0;

	HRESULT result = device->CreateBuffer(&bd, &sr, &indexBuffer);
	if (FAILED(result))
		{
		foundation.printLine("RendererD3D11::createIndexArray(): CreateBuffer() failed!");
		}
	return GeometryArray(indexBuffer, indexSizeBytes, indexCount);
	}

void RendererD3D11::releaseGeometryArray(GeometryArray rm)
	{
	if (rm.geometryBuffer)
		rm.geometryBuffer->Release();

	}

void RendererD3D11::setBufferData(TexturePtr tpr, unsigned structSize, unsigned numElems, const void* data)
	{
	if ((tpr.type == TextureType::eCSConstantBuffer) || (tpr.type == TextureType::eVSConstantBuffer) || (tpr.type == TextureType::ePSConstantBuffer))
		{
		D3D11_MAPPED_SUBRESOURCE sr;
		if (FAILED(deviceContext->Map(tpr.texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr)))
			{
			foundation.printLine("RendererD3D11::setConstants(): failed to lock constant array!");
			return;
			}
		memcpy(sr.pData, data, structSize * numElems);
		deviceContext->Unmap(tpr.texture, 0);
		}
	else if (tpr.type == TextureType::eComputeBuffer)
		{
		// For structured buffers, we can use UpdateSubresource directly
		deviceContext->UpdateSubresource(tpr.texture, 0, nullptr, data, structSize * numElems, 0);
		}
	else
		foundation.printLine("RendererD3D11::setConstants(): unexpected buffer type!");
	}


TexturePtr RendererD3D11::createVolumeTexture(MxU32 width, MxU32 height, MxU32 depth, unsigned char* voxels)
	{
	D3D11_TEXTURE3D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.Depth = depth;
	desc.MipLevels = 1;	//1 means no mips. 0 means auto-mips.
	//TODO: for now this is just 8 bit scalar texture.  Add more types later.
	desc.Format = DXGI_FORMAT_R8_UINT;
	desc.Usage = D3D11_USAGE_DEFAULT; //if we update frequently use D3D11_USAGE_DYNAMIC
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	ID3D11Texture3D* texture = nullptr;
	ID3D11ShaderResourceView* shaderResource = nullptr;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = voxels;
	data.SysMemPitch = width;
	data.SysMemSlicePitch = width * height;


	if (device == nullptr)
		{
		foundation.printLine("RendererD3D11::createVolumeTexture(): No device!");
		return TexturePtr();
		}

	if (FAILED(device->CreateTexture3D(&desc, voxels ? &data : nullptr, &texture)))
		{
		foundation.printLine("RendererD3D11::createVolumeTexture(): CreateTexture3D failed!");
		return TexturePtr();
		}

	if (FAILED(device->CreateShaderResourceView(texture, nullptr, &shaderResource)))
		{
		foundation.printLine("RendererD3D11::createTexture(): CreateShaderResourceView failed!");
		texture->Release();
		return TexturePtr();
		}

	return TexturePtr(texture, shaderResource);
	}

void RendererD3D11::allocateTiledVolumeTextureMemory(MxU32 width, MxU32 height, MxU32 depth)
	{
	D3D11_BUFFER_DESC desc;
	//figure out memory we need from size in pixels:

	desc.ByteWidth = width * height * depth; 
	//has to be a multiple of D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES (0x10000), so round up:
	unsigned numPages = 1 + desc.ByteWidth / 0x10000;
	desc.ByteWidth = 0x10000 * numPages;

	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = 0u;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;
	desc.StructureByteStride = 0;

	if (device == nullptr)
		{
		foundation.printLine("RendererD3D11::createTilePool(): No device!");
		return;
		}

	D3D11_FEATURE_DATA_D3D11_OPTIONS2 options;
	device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &options, sizeof(options));
	if (options.TiledResourcesTier < D3D11_TILED_RESOURCES_TIER_3)
		{
		foundation.printLine("RendererD3D11::createTilePool(): Device doesn't have good enough tiled resource support!");
		return;
		}

	if (FAILED(device->CreateBuffer(&desc, nullptr, &tilePool)))
		{
		foundation.printLine("RendererD3D11::createTilePool(): CreateBuffer failed!");
		}
	}

void RendererD3D11::writeToTiledVolumeTexture(TexturePtr tpr, MxU32 x, MxU32 y, MxU32 z, MxU32 width, MxU32 height, MxU32 depth, unsigned char* voxels)
	{
	//there can be two ways to update: Either an entire Tile at a time with UpdateTiles(), or an arbitrary area
	//the entire tile at a time method is probably faster but then we need to have the source memory exactly tile sized
	//for now we go with the second option.
	//write to the virtual area that now has real memory backing:
	/*
	UINT flags = D3D11_TILE_MAPPING_NO_OVERWRITE;
	D3D11_TILED_RESOURCE_COORDINATE coord;
	coord.X = x / tileShape.WidthInTexels;
	coord.Y = y / tileShape.HeightInTexels;
	coord.Z = z / tileShape.DepthInTexels;
	coord.Subresource = 0;

	D3D11_TILE_REGION_SIZE size;
	size.bUseBox = 1;
	size.Width = width  / tileShape.WidthInTexels;
	size.Height = height / tileShape.HeightInTexels;
	size.Depth = depth  / tileShape.DepthInTexels;
	size.NumTiles = size.Width * size.Height * size.Depth;
	//if (voxels)
	//	deviceContext->UpdateTiles(tpr.texture, &coord, &size, voxels, flags);	//no return value
	*/
	D3D11_BOX box;
	box.left = x;
	box.top = y;
	box.front = z;
	box.right = x + width;
	box.bottom = y + height;
	box.back = z + depth;

	//assumes 8bpp, otherwise need to multiply with BYTESpp: 
	UINT rowPitch = width;
	UINT depthPitch = width * height;

	if (tpr.texture)
		deviceContext->UpdateSubresource(tpr.texture, 0, &box, voxels, rowPitch, depthPitch);
	else
		foundation.printLine("RendererD3D11::writeToTiledVolumeTexture(): Can't write to NULL texture!");

	}

TexturePtr RendererD3D11::createTiledVolumeTexture(MxU32 width, MxU32 height, MxU32 depth)
	{
	D3D11_TEXTURE3D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.Depth = depth;
	desc.MipLevels = 1;	//1 means no mip mapping
	//TODO: for now this is just 8 bit scalar texture.  Add more types later.
	desc.Format = DXGI_FORMAT_R8_UINT;
	desc.Usage = D3D11_USAGE_DEFAULT; //TODO: if we update frequently try using D3D11_USAGE_DYNAMIC
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TILED;

	ID3D11Texture3D* texture = nullptr;
	ID3D11ShaderResourceView* shaderResource = nullptr;

	if ((width % 64 != 0) || (height % 32 != 0) || (depth % 32 != 0))
	{
		//stupid limitation because of D3D11_TILE_SHAPE below.  The creation call doesn't fail
		//completely but I think bad things happen later.
		foundation.printLine("RendererD3D11::createVolumeTexture(): Size must be multiple of 64x32x32!");
		return TexturePtr();
	}


	if (device == nullptr)
		{
		foundation.printLine("RendererD3D11::createVolumeTexture(): No device!");
		return TexturePtr();
		}

	if (FAILED(device->CreateTexture3D(&desc, nullptr, &texture)))
		{
		foundation.printLine("RendererD3D11::createVolumeTexture(): CreateTexture3D failed!");
		return TexturePtr();
		}

	if (FAILED(device->CreateShaderResourceView(texture, nullptr, &shaderResource)))
		{
		foundation.printLine("RendererD3D11::createTexture(): CreateShaderResourceView failed!");
		texture->Release();
		return TexturePtr();
		}
	//also allocate the backing memory here densely, since we don't expect to need the spartsity feature yet:
	allocateTiledVolumeTextureMemory(width, height, depth);

	//set up mapping to memory

	//inputs are PIXEL coordinates, but we need to convert to TILE coordinates.
	//Tile sizes are always 32x32x16 for the 32bpp textures that we use, so we always just divide by that.
	//make the logical sparse virtual texture's apropriate tile point to the right place in the buffer:

	UINT numTiles;
	D3D11_PACKED_MIP_DESC  mipDesc;
	D3D11_TILE_SHAPE  tileShape;	//ends up being 64x32x32 for 8bpp tiles!
	UINT numSubresTilings = 0;
	UINT numSubresTilingsToGet = 0;
	D3D11_SUBRESOURCE_TILING tiling;
	device->GetResourceTiling(texture, &numTiles, &mipDesc, &tileShape, &numSubresTilings, numSubresTilingsToGet, &tiling);


	//these are TILE coordinates not pixel coordinates!
	UINT flags = D3D11_TILE_MAPPING_NO_OVERWRITE;
	D3D11_TILED_RESOURCE_COORDINATE coord;
	coord.X = 0; //x / tileShape.WidthInTexels;
	coord.Y = 0; //y / tileShape.HeightInTexels;
	coord.Z = 0; //z / tileShape.DepthInTexels;
	coord.Subresource = 0;

	D3D11_TILE_REGION_SIZE size;
	size.bUseBox = 1;
	size.Width = width / tileShape.WidthInTexels;
	size.Height = height / tileShape.HeightInTexels;
	size.Depth = depth / tileShape.DepthInTexels;
	size.NumTiles = size.Width * size.Height * size.Depth;

	UINT rangeFlags = 0;	// range defines sequential tiles in the tile pool
	UINT startOffsets = 0;	//this is the index of the destination tile in the pool.
	UINT tileCounts = size.Width * size.Height * size.Depth;

	if (FAILED(deviceContext->UpdateTileMappings(texture, 1, &coord, &size, tilePool, 1, &rangeFlags, &startOffsets, &tileCounts, flags)))
		{
		foundation.printLine("RendererD3D11::writeToTiledVolumeTexture(): UpdateTileMappings failed!");
		}

	//write to the virtual area that now has real memory backing:
	//if (voxels)
	//	deviceContext->UpdateTiles(tpr.texture, &coord, &size, voxels, flags);	//no return value

	//writeToTiledVolumeTexture(tpr, 0, 0, 0, width, depth, height, nullptr);

	return TexturePtr(texture, shaderResource);
	}

TexturePtr RendererD3D11::createStructuredBuffer(unsigned structSize, unsigned numElems, const void* data)
	{

	D3D11_BUFFER_DESC desc;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = structSize;
	desc.ByteWidth = structSize * numElems;
	desc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA dataDesc;
	dataDesc.pSysMem = data;
	dataDesc.SysMemPitch = 0;
	dataDesc.SysMemSlicePitch = 0;

	ID3D11Buffer* structuredBuffer = nullptr;
	ID3D11UnorderedAccessView* accessView = nullptr;

	if (FAILED(device->CreateBuffer(&desc, data ? &dataDesc : NULL, &structuredBuffer)))
		{
		foundation.printLine("RendererD3D11::createStructuredBuffer(): CreateBuffer failed!");
		}


	D3D11_UNORDERED_ACCESS_VIEW_DESC accessDesc;
	accessDesc.Format = DXGI_FORMAT_UNKNOWN;
	accessDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	accessDesc.Buffer.FirstElement = 0;
	accessDesc.Buffer.NumElements = numElems;
	accessDesc.Buffer.Flags = 0;

	if (FAILED(device->CreateUnorderedAccessView(structuredBuffer, &accessDesc, &accessView)))
		{
		foundation.printLine("RendererD3D11::createStructuredBuffer(): CreateUnorderedAccessView failed!");
		}

	return TexturePtr(structuredBuffer, accessView);
	}

TexturePtr RendererD3D11::createMappableBuffer(unsigned structSize, unsigned numElems, TextureType::Enum type)
	{
	D3D11_BUFFER_DESC cd;
	ID3D11Buffer* b;

	ASSERT((type == TextureType::eCSConstantBuffer) || (type == TextureType::eVSConstantBuffer) || (type == TextureType::ePSConstantBuffer));

	cd.Usage = D3D11_USAGE_DYNAMIC;
	cd.ByteWidth = structSize * numElems;
	cd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cd.MiscFlags = 0;
	cd.StructureByteStride = 0;

	if (FAILED(device->CreateBuffer(&cd, nullptr, &b)))
		{
		foundation.fatal("D3DDevice::CreateBuffer for const buffer failed!");
		}
	return TexturePtr(b, type);
	}


TexturePtr RendererD3D11::createTexture(unsigned w, unsigned h, void * pixels, unsigned pitch, PixelFormat::Enum pf)
	{
	D3D11_TEXTURE2D_DESC desc;

	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = (pf == PixelFormat::e_Rf32_Gf32_Bf32_Af32) ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;  
	
	//;	//not really but whatevs

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = pixels ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = pixels;
	data.SysMemPitch = pitch;
	data.SysMemSlicePitch = 0;


	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* shaderResource = nullptr;
	/*
	UINT support = 0;
	device->CheckFormatSupport(desc.Format, &support);
    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 cfs2;
    cfs2.InFormat = desc.Format;
    cfs2.OutFormatSupport2 = 0;
	device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &cfs2, sizeof(cfs2));
	*/

	if (FAILED(device->CreateTexture2D(&desc, pixels ? &data : NULL, &texture)))
		{
		foundation.printLine("RendererD3D11::createTexture(): CreateTexture2D failed!");
		return TexturePtr();
		}
	//create a shaderresource which is used to bind the texture as an input to a pixel shader.

	if (FAILED(device->CreateShaderResourceView(texture, nullptr, &shaderResource)))
		{
		foundation.printLine("RendererD3D11::createTexture(): CreateShaderResourceView failed!");
		texture->Release();
		return TexturePtr();
		}

	return TexturePtr(texture, shaderResource);
	}

void RendererD3D11::bindTexture(TexturePtr tp, unsigned slot, bool unbind)
	{
	ID3D11UnorderedAccessView* nullView = nullptr;

	if (tp.type == TextureType::ePSConstantBuffer)
		{
		deviceContext->PSSetConstantBuffers(slot, 1, &tp.constantBuffer);
		}
	else if (tp.type == TextureType::eCSConstantBuffer)
		{
		deviceContext->CSSetConstantBuffers(slot, 1, &tp.constantBuffer);
		}
	else if (tp.type == TextureType::eVSConstantBuffer)
		{
		deviceContext->VSSetConstantBuffers(slot, 1, &tp.constantBuffer);
		}
	else if (tp.type == TextureType::eShaderResource)
		{
		if (renderWithCompute)
			deviceContext->CSSetShaderResources(slot, 1, &(tp.shaderResourceView));
		else
			deviceContext->PSSetShaderResources(slot, 1, &(tp.shaderResourceView));
		}
	else if (tp.type == TextureType::eRenderTarget)
		{
		deviceContext->OMSetRenderTargets(1, &tp.rtView, nullptr);
		}
	else if (tp.type == TextureType::eComputeBuffer)
		{
		deviceContext->CSSetUnorderedAccessViews(slot, 1, unbind ? &nullView : &(tp.accessView), nullptr);
		}
	else
		foundation.printLine("RendererD3D11::bindTexture(): Unknown texture type!");

	}

void RendererD3D11::clearTexture(TexturePtr tpr)
	{
	// clear the back buffer to a deep blue
	FLOAT ColorRGBA[4] = { 0.02f, 0.0f, 0.1f, 1.0f };

	if (tpr.type == TextureType::eComputeBuffer)
		{
		deviceContext->ClearUnorderedAccessViewFloat(tpr.accessView, ColorRGBA);
		}
	else if (tpr.type == TextureType::eRenderTarget)
		deviceContext->ClearRenderTargetView(tpr.rtView, ColorRGBA);
	}


void RendererD3D11::releaseTexture(TexturePtr ptr)
	{
	if (ptr.texture)
		ptr.texture->Release();

	if (ptr.texture != ptr.constantBuffer) //for constant buffers its the same object so don't release twice.
		if (ptr.shaderResourceView)
			ptr.shaderResourceView->Release();
	}

void* RendererD3D11::lockTexture(TexturePtr ptr, unsigned& rowPitch, unsigned& depthPitch)
	{
	// Lock the texture so it can be written to.
	D3D11_MAPPED_SUBRESOURCE sr;
	if (FAILED(deviceContext->Map(ptr.texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr)))
		{
		foundation.printLine("RendererD3D11::lockTexture(): failed to lock texture!");
		return nullptr;
		}
	rowPitch = sr.RowPitch;
	depthPitch = sr.DepthPitch;
	return sr.pData;
	}
void RendererD3D11::unlockTexture(TexturePtr ptr)
	{
	deviceContext->Unmap(ptr.texture, 0);
	}



void RendererD3D11::draw(GeometryArray* vertexArrays, MxU32 numVertexArrays, GeometryArray indexArray)
	{
	ASSERT(vertexArrays);
	ASSERT(numVertexArrays);
	ASSERT(vertexArrays[0].geometryBuffer);

	//TODO: its possible to specify all vertex arrays with a single call to IASetVertexBuffers, though as far as I can tell
	//the vertex array pointers would need to be packed for that.  We could repack from GeometryArray but not sure if thats worth it.
	for (MxU32 i = 0; i < numVertexArrays; i++)
		{
		UINT stride = vertexArrays[i].elementSize;
		UINT offset = 0;
		deviceContext->IASetVertexBuffers(i, 1, &vertexArrays[i].geometryBuffer, &stride, &offset);
		}

	deviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	//TODO: let this be specified!

	if (indexArray.geometryBuffer)
		{
		deviceContext->IASetIndexBuffer(indexArray.geometryBuffer, DXGI_FORMAT_R32_UINT, 0);
		deviceContext->DrawIndexed(indexArray.numElements, 0, 0);
		}
	else
		{
		//draw without indexing
		deviceContext->Draw(vertexArrays[0].numElements, 0);
		}
	}



//compile time choice of renderer 
//aim is to avoid virtual calls at runtime
//while also always compiling all renderer implementations
#if RENDERER_USE_D3D11 == 1

RendererD3D11 gRendererD3D11;
RendererD3D11& renderer = gRendererD3D11;

#endif