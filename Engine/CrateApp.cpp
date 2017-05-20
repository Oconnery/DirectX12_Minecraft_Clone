//***************************************************************************************
// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

//CONOR

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "../../Common/Camera.h"
#include "FrameResource.h"
#include "Windows.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib,"winmm.lib") 

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int {
	Opaque = 0,
	Mirrors,
	Reflected,
	Transparent,
	Shadow,
	Count
};

class CrateApp : public D3DApp
{
public:
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp& operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
	//variables to change when key is pressed and enable different psos
	bool debugMode = false;
	bool cullFront = false;
	bool cullNone = false;
	bool isBuilt = false;


	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	//OISIN
	void backColourChange();
	void changeLightStrength();
	void changeSunStrength();
	void playSounds();
	//

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mOpaquePSO;
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;

	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	Camera mCamera;

	POINT mLastMousePos;

	//OISIN
	float lightAngle = 0.0f; //Starts at 0, x is highest in sky. Ambient should be brightest
	float angleIncrease = 0.01f; //The angle the light changes per frame. Changing this also changes the rate at which the day/night cycle occurs. Lighting depends on this.
	XMFLOAT4 ambientStrength = { 1.0f,1.0f,1.0f,1.0f };// { 0.5f, 0.5f, 0.7f, 1.0f }; //Starts at brightest //Changing this won't affect anything 
	XMFLOAT4 ambientIncrease = { (ambientStrength.x*angleIncrease), (ambientStrength.y * angleIncrease), (ambientStrength.z * angleIncrease), (ambientStrength.w * angleIncrease) };
	// so if +0.01 every frame then it will take 100 frames to reach 1.0f

	XMFLOAT3 sunStrength = { 0.6f, 0.6f, 0.08f };

	float colourChange = 0.5 / angleIncrease; // COLOUR CHANGE PER FRAME -  EACH IF STATEMENT LASTS FOR 0.50

	float red = 0.0f;
	float green = 0.749019623f; 
	float blue = 1.0f;

	//Will switch to true on 'N' key press. Will switch to false on 'L' key press.
	bool lightingOff = false;
	//Temps for when light is off
	XMFLOAT4 lightOffAmbientStrength = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMFLOAT3 lightOffSunStrength = { 0.0f, 0.0f, 0.0f };
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CrateApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CrateApp::CrateApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool CrateApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	//PlaySound(TEXT("water.wav"), NULL, SND_FILENAME);
	//Play intro sound?

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void CrateApp::Update(const GameTimer& gt)
{
	//Called every frame

	OnKeyboardInput(gt);
	//UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);

	changeLightStrength(); //OISIN	
	backColourChange(); //OISIN
	playSounds();

	lightAngle += angleIncrease; //ambientStrength = { 0.0f, 0.0f, 0.0f, 1.0f }; //Max is {0.6f, 0.6f, 0.8f, 1.0f}

	if (lightAngle >= 1.0f)
	{
		//Darkest (midnight)
		lightAngle = -1.0f;
		//ambientStrength = {0.0f, 0.0f, 0.0f, 0.0f};
	}
}

void CrateApp::backColourChange()
{
	//Changes the colour of the buffer
	//Used as skybox

	//Starts mid-day --- 0.0f == midday		0.50f == dusk		1.0f/-1.0f == midnight		-0.50f == Morning

	// R=R || G -=0.203921557f || B-= 0.454901934||
	//0.000000000f, 0.545098066f, 0.545098066f							DARKCYAN

	// R=R || G-= 0.545098066 || B-= 0.545098066F
	// 0,0,0															BLACK

	// R += 0.980392218f || G+=0.980392218f || B+= 0.823529482f
	//0.980392218f, 0.980392218f, 0.823529482f							LIGHTGOLDENYELLOW

	// R-= 0.980392218f || G-= 0.231372595 || B + =	0.176470518										
	// 0.000000000f, 0.749019623f, 1.000000000f							DEEPSKYBLUE


	if (lightAngle >= -0.50f && lightAngle < 0)
	{
		//Morning to noon
		red -= (0.98092218f / colourChange);
		green -= (0.231372595f / colourChange);
		blue += (0.176470518f / colourChange);
	}

	//START HERE --- DEEP SKY BLUE/NOON
	if (lightAngle >= 0.0f && lightAngle < 0.50f)
	{
		//Noon to dusk
		red = red;
		green -= (0.203921557f / colourChange);
		blue -= (0.454901934f / colourChange);
	}
	//NOW ITS DARK CYAN/DUSK

	if (lightAngle >= 0.50f && lightAngle < 1.0f)
	{
		//Dusk to midnight
		red = red;
		green -= (0.545098066f / colourChange);
		blue -= (0.545098066f / colourChange);
	}
	// NOW ITS MIDNIGHT

	if (lightAngle < (-0.50f))
	{
		//Midnight to morning
		red += (0.980392218f / colourChange);
		green += (0.980392218f / colourChange);
		blue += (0.823529482f / colourChange);
	}
}

void CrateApp::changeLightStrength()
{
	//
	//Changes the strength of the ambient and directional light. Called every frame
	//

	//Changing ambient str

	if (lightAngle >= 0.0f) {
		ambientStrength.y -= ambientIncrease.y;
		ambientStrength.x -= ambientIncrease.x;
		ambientStrength.z -= ambientIncrease.z;
	}
	else if (lightAngle < 0.0f) {
		ambientStrength.y += ambientIncrease.y;
		ambientStrength.x += ambientIncrease.x;
		ambientStrength.z += ambientIncrease.z;
	}

	//Change colour/strength of the sun
	changeSunStrength();
}

void CrateApp::changeSunStrength()
{
	//Function to change the colour of the directional light.
	if (lightAngle >= -0.3f && lightAngle <-0.20f)
	{
		//Midday -- A good yellow
		sunStrength.x -= 0.10f * angleIncrease *10.0f;
		sunStrength.y += 0.35f * angleIncrease*10.0f;
		sunStrength.z = 0.08f * angleIncrease*10.0f;

		/*
		sunStrength.x = 0.6f;
		sunStrength.y = 0.6f;
		sunStrength.z = 0.08f;
		*/
		//After the lightAngle changes from 0.3f to 0.2f, the sun Strength will be at these values
	}

	else if (lightAngle >= 0.70f && lightAngle <0.80f)//(lightAngle >= 0.80 || lightAngle<= -0.80f)
	{
		//Pure White
		//Moonlight
		sunStrength.x -= 0.3f * angleIncrease*10.0f;
		sunStrength.y -= 0.3f * angleIncrease*10.0f;
		sunStrength.z += 0.22f * angleIncrease*10.0f;

		/*
		sunStrength.x = 0.3f;
		sunStrength.y = 0.3f;
		sunStrength.z = 0.3f;
		*/
	}

	else if (lightAngle >= -0.75f && lightAngle < -0.65f)
	{
		sunStrength.x += 0.4f*angleIncrease*10.0f;
		sunStrength.y -= 0.05f*angleIncrease*10.0f;
		sunStrength.z -= 0.3f*angleIncrease*10.0f;

		/*
		sunStrength.x = 0.7f;
		sunStrength.y = 0.25f;
		sunStrength.z = 0.0f;
		*/

		//Colors::DarkOrange // - the green is a little bit different than this colour
	}
}

void CrateApp::playSounds()
{
	//Play birds chirping sound during the morning transition
	if (lightAngle >= -0.75f && lightAngle < -0.72f)
	PlaySound(TEXT("bird.wav"), NULL, SND_FILENAME);
}

void CrateApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	//Conor: changing the pso when a key is pressed
		if (debugMode)
		{
			ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO["opaque_wireframe"].Get()));
		}
		//changing the cullmode to cull front
		else if (cullFront)
		{
			ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO["opaque_cullfront"].Get()));
		}
		//changing the cullmode to cull none
		else if (cullNone) {
			ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO["opaque_cullnone"].Get()));
		}
		//when no key is pressed change the pso bck to opaque
		else
		{
			ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO["transparent"].Get()));
			DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);
		}
	//}
	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	/*ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), &mOpaquePSO.Get()));*/

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//Render using the rgb variables
	XMVECTORF32 ABC = { red,green,blue ,1.0f };
	//will change the colour
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), ABC, 0, nullptr);
	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	int cam = 0;

	if (GetAsyncKeyState('E') & 0x8000)
	{
		cam++;
		//mCamera.SetPosition(12, 12, 12);
	}

	if (cam % 2 == 0)
	{
		//JAMES
		//this if statement checks to see if the left mouse button is clicked
		if ((btnState & MK_LBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));
			//Yaw and Pitch is the way of describing the rotation of the camera in 3D Space
			//Pitch represents whenever the camera moves up and down.
			mCamera.Pitch(dy);
			//Rotate is whenvever the camera is moving left to right.
			mCamera.RotateY(dx);
		}
		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
	else if (cam % 2 == 1)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			//sets pixel correspond to 0 meaning it wont move.
			float dx = XMConvertToRadians(0.0f*static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.0f*static_cast<float>(y - mLastMousePos.y));
		}
		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
	
		
}

void CrateApp::OnKeyboardInput(const GameTimer& gt)
{
	//JAMES
	//Delta Time is used in relation to hardware and network responsiveness.
	//Its the time between 2 frame updates
	//Calls timer every frame per second, it holds the time between those 2 frames.
	//The resulting number is the Delta Time. This is used to calculate for example, how far a game character would have
	//travelled during that time. The result is, the character will take the same amount of real world time to move across
	//the screen regardless of the rate of update, whether the delay be caused by lack of processing power, or a slow internet
	//connection. This helps the game be as fair as possible for all users.

	const float dt = gt.DeltaTime();
	//Get AsyncKeyState is a function that determines if a certain key is pressed or not. For this method these keys are W,S,A,D,1,2.
	//This if statement checks to see is if the 'W' key is pressed down, and if so call the camera object with the method '.Walk'.
	if (GetAsyncKeyState('W') & 0x8000)
		//mCamera.Walk(10.0f*dt) is used to move the camera forward 10 pixels whenever the 'W' key is pressed. Multiplies it by DT to ensure its the same everywhere
		mCamera.Walk(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		//this '.Walk' call is set to -10.0f. This is because I wanted the 'S' to move the camera backwards and to do this I take 10pixels away from the current position.
		mCamera.Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f*dt);

	if (GetAsyncKeyState('N') & 0x8000)
		//if (lightingOff = false)//Lighting is on
		//Turn off lighting - Ambient strength == temp0, directional = temp0
		lightingOff = true;

	if (GetAsyncKeyState('L') & 0x8000)
		//if (lightingOff =  true) //Lighting is off
		//Turn on lighting - Ambient strength == normal, directional = normal
		lightingOff = false;

	/*
	Conor: When 1 is pressed a bool variable debugMode is changed to true
	and is switched back to false when let go
	this toggles wireframe
	*/
	if (GetAsyncKeyState('1') & 0x8000)
		debugMode = true;
	else
		debugMode = false;

	/*
	Conor: When 2 is pressed a bool variable cullFront is changed to true
	and is switched back to false when let go
	this toggles cullmode to front culling
	*/
	if (GetAsyncKeyState('2') & 0x8000)
		cullFront = true;
	else
		cullFront = false;

	/*
	Conor: When 3 is pressed a bool variable cullNone is changed to true
	and is switched back to false when let go
	this toggles cullmode to no culling
	*/
	if (GetAsyncKeyState('3') & 0x8000)
		cullNone = true;
	else
		cullNone = false;

	mCamera.UpdateViewMatrix();
}

void CrateApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void CrateApp::AnimateMaterials(const GameTimer& gt)
{
	//Animate water
	// Scroll the water material texture coordinates.

	//Get the material
	auto waterMat = mMaterials["water"].get();

	//Move the texture
	float& a = waterMat->MatTransform(3, 0);
	float& b = waterMat->MatTransform(3, 1);
	a += 0.1f * gt.DeltaTime();
	b += 0.02f * gt.DeltaTime();
	//Reset if = 1.0
	if (a >= 1.0f)
		a -= 1.0f;
	if (b >= 1.0f)
		b -= 1.0f;
	//Set the current MatTransform to a and b
	waterMat->MatTransform(3, 0) = a;
	waterMat->MatTransform(3, 1) = b;
	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void CrateApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMainPassCB(const GameTimer& gt)
{

	//OutputDebugString(L"hello");
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	if (lightingOff == false)
		mMainPassCB.AmbientLight = ambientStrength; // What does the fourth and final float do? -- To find out
	else if (lightingOff == true)
		mMainPassCB.AmbientLight = lightOffAmbientStrength;

	//Directional light (sun)
	mMainPassCB.Lights[0].Direction.x = lightAngle;
	mMainPassCB.Lights[0].Direction.y = -0.45f;
	mMainPassCB.Lights[0].Direction.z = 0.45f;

	//the strength of the light is max whenever x is 0.00f
	//the strength of the light is least whenever x is 1.0f or -1.0f 

	if (lightingOff == false)
		mMainPassCB.Lights[0].Strength = sunStrength; //mMainPassCB.Lights[0].Strength = {0.5f, 0.5f, 0.7f};
	else if (lightingOff == true)
		mMainPassCB.Lights[0].Strength = lightOffSunStrength;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

//Conor
void CrateApp::LoadTextures()
{
	//loading the dirt texture from file and creating a texture object
	auto dirtTex = std::make_unique<Texture>();
	dirtTex->Name = "dirtTex";
	dirtTex->Filename = L"Textures/dirt.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), dirtTex->Filename.c_str(),
		dirtTex->Resource, dirtTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto bedrockTex = std::make_unique<Texture>();
	bedrockTex->Name = "bedrockTex";
	bedrockTex->Filename = L"Textures/bedrock.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bedrockTex->Filename.c_str(),
		bedrockTex->Resource, bedrockTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stoneTex";
	stoneTex->Filename = L"Textures/stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), stoneTex->Filename.c_str(),
		stoneTex->Resource, stoneTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"Textures/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto woodTex = std::make_unique<Texture>();
	woodTex->Name = "woodTex";
	woodTex->Filename = L"Textures/wood.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodTex->Filename.c_str(),
		woodTex->Resource, woodTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto leavesTex = std::make_unique<Texture>();
	leavesTex->Name = "leavesTex";
	leavesTex->Filename = L"Textures/leaves_oak.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), leavesTex->Filename.c_str(),
		leavesTex->Resource, leavesTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto ironTex = std::make_unique<Texture>();
	ironTex->Name = "ironTex";
	ironTex->Filename = L"Textures/iron.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), ironTex->Filename.c_str(),
		ironTex->Resource, ironTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto gravelTex = std::make_unique<Texture>();
	gravelTex->Name = "gravelTex";
	gravelTex->Filename = L"Textures/gravel.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), gravelTex->Filename.c_str(),
		gravelTex->Resource, gravelTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto sandTex = std::make_unique<Texture>();
	sandTex->Name = "sandTex";
	sandTex->Filename = L"Textures/sand.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), sandTex->Filename.c_str(),
		sandTex->Resource, sandTex->UploadHeap));

	//loading the dirt texture from file and creating a texture object
	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"Textures/waterTransparent.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));

	//saving all the textures and naming them in mTextures
	mTextures[dirtTex->Name] = std::move(dirtTex);
	mTextures[bedrockTex->Name] = std::move(bedrockTex);
	mTextures[stoneTex->Name] = std::move(stoneTex);
	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[woodTex->Name] = std::move(woodTex);
	mTextures[leavesTex->Name] = std::move(leavesTex);
	mTextures[ironTex->Name] = std::move(ironTex);
	mTextures[gravelTex->Name] = std::move(gravelTex);
	mTextures[sandTex->Name] = std::move(sandTex);
	mTextures[waterTex->Name] = std::move(waterTex);
}

void CrateApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

//Conor
void CrateApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 10;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//creating the descriptor heap for dirtTex which gives the texture its properties
	auto dirtTex = mTextures["dirtTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = dirtTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = dirtTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(dirtTex.Get(), &srvDesc, hDescriptor);

	//You will need to use the following line to offset the heap to the next position before adding the next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for bedrockTex which gives the texture its properties
	auto bedrockTex = mTextures["bedrockTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv2Desc = {};
	srv2Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv2Desc.Format = bedrockTex->GetDesc().Format;
	srv2Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv2Desc.Texture2D.MostDetailedMip = 0;
	srv2Desc.Texture2D.MipLevels = bedrockTex->GetDesc().MipLevels;
	srv2Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(bedrockTex.Get(), &srv2Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for stoneTex which gives the texture its properties
	auto stoneTex = mTextures["stoneTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv3Desc = {};
	srv3Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv3Desc.Format = stoneTex->GetDesc().Format;
	srv3Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv3Desc.Texture2D.MostDetailedMip = 0;
	srv3Desc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
	srv3Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srv3Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for grassTex which gives the texture its properties
	auto grassTex = mTextures["grassTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv4Desc = {};
	srv4Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv4Desc.Format = grassTex->GetDesc().Format;
	srv4Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv4Desc.Texture2D.MostDetailedMip = 0;
	srv4Desc.Texture2D.MipLevels = grassTex->GetDesc().MipLevels;
	srv4Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srv4Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for woodTex which gives the texture its properties
	auto woodTex = mTextures["woodTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv5Desc = {};
	srv5Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv5Desc.Format = woodTex->GetDesc().Format;
	srv5Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv5Desc.Texture2D.MostDetailedMip = 0;
	srv5Desc.Texture2D.MipLevels = woodTex->GetDesc().MipLevels;
	srv5Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(woodTex.Get(), &srv5Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for leavesTex which gives the texture its properties
	auto leavesTex = mTextures["leavesTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv6Desc = {};
	srv6Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv6Desc.Format = leavesTex->GetDesc().Format;
	srv6Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv6Desc.Texture2D.MostDetailedMip = 0;
	srv6Desc.Texture2D.MipLevels = leavesTex->GetDesc().MipLevels;
	srv6Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(leavesTex.Get(), &srv6Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for ironTex which gives the texture its properties
	auto ironTex = mTextures["ironTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv7Desc = {};
	srv7Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv7Desc.Format = ironTex->GetDesc().Format;
	srv7Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv7Desc.Texture2D.MostDetailedMip = 0;
	srv7Desc.Texture2D.MipLevels = ironTex->GetDesc().MipLevels;
	srv7Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(ironTex.Get(), &srv7Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for gravelTex which gives the texture its properties
	auto gravelTex = mTextures["gravelTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv8Desc = {};
	srv8Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv8Desc.Format = gravelTex->GetDesc().Format;
	srv8Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv8Desc.Texture2D.MostDetailedMip = 0;
	srv8Desc.Texture2D.MipLevels = gravelTex->GetDesc().MipLevels;
	srv8Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(gravelTex.Get(), &srv8Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for sandTex which gives the texture its properties
	auto sandTex = mTextures["sandTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv9Desc = {};
	srv9Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv9Desc.Format = sandTex->GetDesc().Format;
	srv9Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv9Desc.Texture2D.MostDetailedMip = 0;
	srv9Desc.Texture2D.MipLevels = sandTex->GetDesc().MipLevels;
	srv9Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(sandTex.Get(), &srv9Desc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//creating the descriptor heap for waterTex which gives the texture its properties
	auto waterTex = mTextures["waterTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv10Desc = {};
	srv10Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv10Desc.Format = waterTex->GetDesc().Format;
	srv10Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv10Desc.Texture2D.MostDetailedMip = 0;
	srv10Desc.Texture2D.MipLevels = waterTex->GetDesc().MipLevels;
	srv10Desc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srv10Desc, hDescriptor);
}

void CrateApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void CrateApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;


	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

//Conor
void CrateApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	/*opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;*/
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO["opaque"])));

	//
	//PSO for opaque wireframe mode
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mOpaquePSO["opaque_wireframe"])));

	//PSO for cull front
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueCullFrontPsoDesc = opaquePsoDesc;
	opaqueCullFrontPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueCullFrontPsoDesc, IID_PPV_ARGS(&mOpaquePSO["opaque_cullfront"])));

	//PSO for cull none
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueCullNonePsoDesc = opaquePsoDesc;
	opaqueCullNonePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueCullNonePsoDesc, IID_PPV_ARGS(&mOpaquePSO["opaque_cullnone"])));

	//pso for blending
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mOpaquePSO["transparent"])));

}

void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

//Conor
void CrateApp::BuildMaterials()
{
	//Creating the material for the dirt block which sets the physical properties of the block
	auto dirt = std::make_unique<Material>();
	dirt->Name = "dirt";
	dirt->MatCBIndex = 0;
	dirt->DiffuseSrvHeapIndex = 0;
	dirt->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	dirt->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	dirt->Roughness = 0.2f;

	mMaterials["dirt"] = std::move(dirt);

	//Creating the material for the bedrock block which sets the physical properties of the block
	auto bedrock = std::make_unique<Material>();
	bedrock->Name = "bedrock";
	bedrock->MatCBIndex = 1;
	bedrock->DiffuseSrvHeapIndex = 1;
	bedrock->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bedrock->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bedrock->Roughness = 0.2f;

	mMaterials["bedrock"] = std::move(bedrock);

	//Creating the material for the stone block which sets the physical properties of the block
	auto stone = std::make_unique<Material>();
	stone->Name = "stone";
	stone->MatCBIndex = 2;
	stone->DiffuseSrvHeapIndex = 2;
	stone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone->Roughness = 0.2f;

	mMaterials["stone"] = std::move(stone);

	//Creating the material for the grass block which sets the physical properties of the block
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 3;
	grass->DiffuseSrvHeapIndex = 3;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	grass->Roughness = 0.2f;

	mMaterials["grass"] = std::move(grass);

	//Creating the material for the wood block which sets the physical properties of the block
	auto wood = std::make_unique<Material>();
	wood->Name = "wood";
	wood->MatCBIndex = 4;
	wood->DiffuseSrvHeapIndex = 4;
	wood->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wood->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	wood->Roughness = 0.2f;

	mMaterials["wood"] = std::move(wood);

	//Creating the material for the leaves block which sets the physical properties of the block
	auto leaves = std::make_unique<Material>();
	leaves->Name = "leaves";
	leaves->MatCBIndex = 5;
	leaves->DiffuseSrvHeapIndex = 5;
	leaves->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	leaves->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	leaves->Roughness = 0.2f;

	mMaterials["leaves"] = std::move(leaves);

	//Creating the material for the iron block which sets the physical properties of the block
	auto iron = std::make_unique<Material>();
	iron->Name = "iron";
	iron->MatCBIndex = 6;
	iron->DiffuseSrvHeapIndex = 6;
	iron->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	iron->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	iron->Roughness = 0.2f;

	mMaterials["iron"] = std::move(iron);

	//Creating the material for the gravel block which sets the physical properties of the block
	auto gravel = std::make_unique<Material>();
	gravel->Name = "gravel";
	gravel->MatCBIndex = 7;
	gravel->DiffuseSrvHeapIndex = 7;
	gravel->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	gravel->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	gravel->Roughness = 0.2f;

	mMaterials["gravel"] = std::move(gravel);

	//Creating the material for the sand block which sets the physical properties of the block
	auto sand = std::make_unique<Material>();
	sand->Name = "sand";
	sand->MatCBIndex = 8;
	sand->DiffuseSrvHeapIndex = 8;
	sand->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sand->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	sand->Roughness = 0.2f;

	mMaterials["sand"] = std::move(sand);

	//Creating the material for the water block which sets the physical properties of the block
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 9;
	water->DiffuseSrvHeapIndex = 9;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	water->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	water->Roughness = 0.2f;

	mMaterials["water"] = std::move(water);
}

//Conor
void CrateApp::BuildRenderItems()
{
	//auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
	//creating a timer to read in current time and setting the seed for rand function so that a random map is generated each time
	GameTimer gt;
	srand((int)gt.CurrTime());

	//setting a variable to control the amount of trees that increments every time a tree spawns
	int treeCount = 0;
	int index = 0, random = 0;

	//creating a bool array to store the trees x z coordinates so that i can prevent trees spawning too close that the leaves overlap
	//50x50 so trees only spawn in the grass quadrant of the map
	bool ar1[80][50][50];
	for (int i = 0; i < 80; i++) {
		for (int j = 0; j < 50; j++) {
			for (int k = 0; k < 50; k++) {
				//setting all the values to false as no trees have spawned yet, when set to true a tree cant spawn beside it
				//i is the amount of trees allowed to spawn, it is the index number
				//j is the x coordinate of the tree position, it will store true if there is a wood or leaves block on it
				//k is the z coordinate of the tree position, it will store true if there is a wood or leaves block on it
				ar1[i][j][k] = false;
			}
		}
	}
	//mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO["transparent"].Get());
	//grass qaudrant generation 50 x 50 
	for (int z = 0; z < 50; z++) {
		for (int x = 0; x < 50; x++) {

			//creating random number between 1 and 2 to randomize the height of y
			random = rand() % 2 + 1;
			for (int y = 0; y < random + 8; y++) {
				//cheking to see if y is at the bottom so that it spawns bedrock
				if (y == 0)
				{
					//creating a bedrock block at position x,y,z
					auto boxR2item = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxR2item->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxR2item->ObjCBIndex = index;
					boxR2item->Mat = mMaterials["bedrock"].get();
					boxR2item->Geo = mGeometries["boxGeo"].get();
					boxR2item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxR2item->IndexCount = boxR2item->Geo->DrawArgs["box"].IndexCount;
					boxR2item->StartIndexLocation = boxR2item->Geo->DrawArgs["box"].StartIndexLocation;
					boxR2item->BaseVertexLocation = boxR2item->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxR2item));
					index++;
				}
				//creating a layer in a low range so only stone, gravel and iron spawn
				else if (y > 0 && y < 4)
				{
					//creating a random number between 1 and 5 and spawning a gravel block when it equals 2 so there is a 1 in 5 chance to spawn gravel
					if (rand() % 5 + 1 == 2) {
						//creating a gravel block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["gravel"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
					//creating a random number between 1 and 10 and spawning a iron block when it equals 2 so there is a 1 in 10 chance to spawn iron
					else if (rand() % 10 + 1 == 2) {
						//creating a iron block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["iron"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
					//making the rest of the blocks in this range to stone
					else {
						//creating a stone block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["stone"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
				}
				//cheking if the y value is on the top row to spawn grass
				else if ((random == 2 && y == 9) || (random == 1 && y == 8)) {
					//creating a grass block at position x,y,z
					auto boxRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxRitem->ObjCBIndex = index;
					boxRitem->Mat = mMaterials["grass"].get();
					boxRitem->Geo = mGeometries["boxGeo"].get();
					boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
					boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
					boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxRitem));
					index++;

				}
				//making every other block in this range dirt
				else {
					//creating a dirt block at position x,y,z
					auto boxRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxRitem->ObjCBIndex = index;
					boxRitem->Mat = mMaterials["dirt"].get();
					boxRitem->Geo = mGeometries["boxGeo"].get();
					boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
					boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
					boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxRitem));
					index++;
				}
			}

			//checking if there are 80 trees already made
			if (treeCount < 80) {
				//creating random number so tree spawns randomly
				if (rand() % 20 == 4) {
					//cycle ethrough xz coordinates in grass quadrant
					if (x > 0 && x < 50) {
						if (z > 0 && z < 50) {
							//setting variable to false then searching if there is a tree already in the area
							bool isTree = false;
							for (int a = 0; a < 80; a++) {
								if (ar1[a][x][z]) {
									isTree = true;
								}
							}
							//if there is no tree, build one
							if (!isTree) {
								// creating all the tree blocks including wood and leaves
								auto boxRitem = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)random + 8, (float)z));
								boxRitem->ObjCBIndex = index;
								boxRitem->Mat = mMaterials["wood"].get();
								boxRitem->Geo = mGeometries["boxGeo"].get();
								boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
								boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
								boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxRitem));
								index++;

								auto boxR2item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR2item->World, XMMatrixTranslation((float)x, (float)random + 9, (float)z));
								boxR2item->ObjCBIndex = index;
								boxR2item->Mat = mMaterials["wood"].get();
								boxR2item->Geo = mGeometries["boxGeo"].get();
								boxR2item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR2item->IndexCount = boxR2item->Geo->DrawArgs["box"].IndexCount;
								boxR2item->StartIndexLocation = boxR2item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR2item->BaseVertexLocation = boxR2item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR2item));
								index++;

								auto boxR3item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR3item->World, XMMatrixTranslation((float)x, (float)random + 10, (float)z));
								boxR3item->ObjCBIndex = index;
								boxR3item->Mat = mMaterials["wood"].get();
								boxR3item->Geo = mGeometries["boxGeo"].get();
								boxR3item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR3item->IndexCount = boxR3item->Geo->DrawArgs["box"].IndexCount;
								boxR3item->StartIndexLocation = boxR3item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR3item->BaseVertexLocation = boxR3item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR3item));
								index++;

								auto boxR4item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR4item->World, XMMatrixTranslation((float)x + 1, (float)random + 10, (float)z));
								boxR4item->ObjCBIndex = index;
								boxR4item->Mat = mMaterials["leaves"].get();
								boxR4item->Geo = mGeometries["boxGeo"].get();
								boxR4item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR4item->IndexCount = boxR4item->Geo->DrawArgs["box"].IndexCount;
								boxR4item->StartIndexLocation = boxR4item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR4item->BaseVertexLocation = boxR4item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR4item));
								index++;

								auto boxR5item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR5item->World, XMMatrixTranslation((float)x + 1, (float)random + 10, (float)z + 1));
								boxR5item->ObjCBIndex = index;
								boxR5item->Mat = mMaterials["leaves"].get();
								boxR5item->Geo = mGeometries["boxGeo"].get();
								boxR5item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR5item->IndexCount = boxR5item->Geo->DrawArgs["box"].IndexCount;
								boxR5item->StartIndexLocation = boxR5item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR5item->BaseVertexLocation = boxR5item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR5item));
								index++;

								auto boxR6item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR6item->World, XMMatrixTranslation((float)x, (float)random + 10, (float)z + 1));
								boxR6item->ObjCBIndex = index;
								boxR6item->Mat = mMaterials["leaves"].get();
								boxR6item->Geo = mGeometries["boxGeo"].get();
								boxR6item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR6item->IndexCount = boxR6item->Geo->DrawArgs["box"].IndexCount;
								boxR6item->StartIndexLocation = boxR6item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR6item->BaseVertexLocation = boxR6item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR6item));
								index++;

								auto boxR7item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR7item->World, XMMatrixTranslation((float)x - 1, (float)random + 10, (float)z + 1));
								boxR7item->ObjCBIndex = index;
								boxR7item->Mat = mMaterials["leaves"].get();
								boxR7item->Geo = mGeometries["boxGeo"].get();
								boxR7item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR7item->IndexCount = boxR7item->Geo->DrawArgs["box"].IndexCount;
								boxR7item->StartIndexLocation = boxR7item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR7item->BaseVertexLocation = boxR7item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR7item));
								index++;

								auto boxR8item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR8item->World, XMMatrixTranslation((float)x - 1, (float)random + 10, (float)z));
								boxR8item->ObjCBIndex = index;
								boxR8item->Mat = mMaterials["leaves"].get();
								boxR8item->Geo = mGeometries["boxGeo"].get();
								boxR8item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR8item->IndexCount = boxR8item->Geo->DrawArgs["box"].IndexCount;
								boxR8item->StartIndexLocation = boxR8item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR8item->BaseVertexLocation = boxR8item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR8item));
								index++;

								auto boxR9item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR9item->World, XMMatrixTranslation((float)x - 1, (float)random + 10, (float)z - 1));
								boxR9item->ObjCBIndex = index;
								boxR9item->Mat = mMaterials["leaves"].get();
								boxR9item->Geo = mGeometries["boxGeo"].get();
								boxR9item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR9item->IndexCount = boxR9item->Geo->DrawArgs["box"].IndexCount;
								boxR9item->StartIndexLocation = boxR9item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR9item->BaseVertexLocation = boxR9item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR9item));
								index++;

								auto boxR10item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR10item->World, XMMatrixTranslation((float)x, (float)random + 10, (float)z - 1));
								boxR10item->ObjCBIndex = index;
								boxR10item->Mat = mMaterials["leaves"].get();
								boxR10item->Geo = mGeometries["boxGeo"].get();
								boxR10item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR10item->IndexCount = boxR10item->Geo->DrawArgs["box"].IndexCount;
								boxR10item->StartIndexLocation = boxR10item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR10item->BaseVertexLocation = boxR10item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR10item));
								index++;

								auto boxR11item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR11item->World, XMMatrixTranslation((float)x + 1, (float)random + 10, (float)z - 1));
								boxR11item->ObjCBIndex = index;
								boxR11item->Mat = mMaterials["leaves"].get();
								boxR11item->Geo = mGeometries["boxGeo"].get();
								boxR11item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR11item->IndexCount = boxR11item->Geo->DrawArgs["box"].IndexCount;
								boxR11item->StartIndexLocation = boxR11item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR11item->BaseVertexLocation = boxR11item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR11item));
								index++;

								auto boxR12item = std::make_unique<RenderItem>();
								XMStoreFloat4x4(&boxR12item->World, XMMatrixTranslation((float)x, (float)random + 11, (float)z));
								boxR12item->ObjCBIndex = index;
								boxR12item->Mat = mMaterials["leaves"].get();
								boxR12item->Geo = mGeometries["boxGeo"].get();
								boxR12item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
								boxR12item->IndexCount = boxR12item->Geo->DrawArgs["box"].IndexCount;
								boxR12item->StartIndexLocation = boxR12item->Geo->DrawArgs["box"].StartIndexLocation;
								boxR12item->BaseVertexLocation = boxR12item->Geo->DrawArgs["box"].BaseVertexLocation;
								mAllRitems.push_back(std::move(boxR12item));
								index++;

								//adding 1 to the tree count
								treeCount++;
								//setting the bool array to true in the xz coordinates area that the tree encompases
								for (int i = z + 2; i > z - 1; i--) {
									for (int j = x - 2; j < x + 3; j++) {
										ar1[treeCount][j][i] = true;
									}
								}

							}
						}
					}
				}

			}
		}
	}

	//sand quadrant generation
	for (int z = 50; z < 100; z++) {
		for (int x = 50; x < 100; x++) {
			//creating random number between 1 and 10
			random = rand() % 10 + 1;
			//if the random number equals 2 then make the terrain 1 block higher, so 1 in 10 blocks are higher
			if (random == 2) {
				for (int y = 0; y < 10; y++) {
					//cheking to see if y is at the bottom so that it spawns bedrock
					if (y == 0)
					{
						//creating a bedrock block at position x,y,z
						auto boxR2item = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxR2item->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxR2item->ObjCBIndex = index;
						boxR2item->Mat = mMaterials["bedrock"].get();
						boxR2item->Geo = mGeometries["boxGeo"].get();
						boxR2item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxR2item->IndexCount = boxR2item->Geo->DrawArgs["box"].IndexCount;
						boxR2item->StartIndexLocation = boxR2item->Geo->DrawArgs["box"].StartIndexLocation;
						boxR2item->BaseVertexLocation = boxR2item->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxR2item));
						index++;
					}
					//creating a layer in a low range so only stone, gravel and iron spawn
					else if (y > 0 && y < 4)
					{
						//creating a random number between 1 and 5 and spawning a gravel block when it equals 2 so there is a 1 in 5 chance to spawn gravel
						if (rand() % 5 + 1 == 2) {
							//creating a gravel block at position x,y,z
							auto boxRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
							boxRitem->ObjCBIndex = index;
							boxRitem->Mat = mMaterials["gravel"].get();
							boxRitem->Geo = mGeometries["boxGeo"].get();
							boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
							boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
							boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(boxRitem));
							index++;
						}
						//creating a random number between 1 and 10 and spawning a iron block when it equals 2 so there is a 1 in 10 chance to spawn iron
						else if (rand() % 10 + 1 == 2) {
							//creating a iron block at position x,y,z
							auto boxRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
							boxRitem->ObjCBIndex = index;
							boxRitem->Mat = mMaterials["iron"].get();
							boxRitem->Geo = mGeometries["boxGeo"].get();
							boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
							boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
							boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(boxRitem));
							index++;
						}
						//making every other block in this range stone
						else {
							//creating a stone block at position x,y,z
							auto boxRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
							boxRitem->ObjCBIndex = index;
							boxRitem->Mat = mMaterials["stone"].get();
							boxRitem->Geo = mGeometries["boxGeo"].get();
							boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
							boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
							boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(boxRitem));
							index++;
						}
					}
					//making top row sand
					else if (y == 9) {
						//creating a sand block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["sand"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;

					}
					//making every other block in this range dirt
					else {
						//creating a dirt block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["dirt"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
				}
			}
			//lower level sand
			else {
				//height of sand
				for (int y = 0; y < 9; y++) {
					//cheking to see if y is at the bottom so that it spawns bedrock
					if (y == 0)
					{
						//creating a bedrock block at position x,y,z
						auto boxR2item = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxR2item->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxR2item->ObjCBIndex = index;
						boxR2item->Mat = mMaterials["bedrock"].get();
						boxR2item->Geo = mGeometries["boxGeo"].get();
						boxR2item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxR2item->IndexCount = boxR2item->Geo->DrawArgs["box"].IndexCount;
						boxR2item->StartIndexLocation = boxR2item->Geo->DrawArgs["box"].StartIndexLocation;
						boxR2item->BaseVertexLocation = boxR2item->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxR2item));
						index++;
					}
					//creating a layer in a low range so only stone, gravel and iron spawn
					else if (y > 0 && y < 4)
					{
						//creating a random number between 1 and 5 and spawning a gravel block when it equals 2 so there is a 1 in 5 chance to spawn gravel
						if (rand() % 5 + 1 == 2) {
							//creating a gravel block at position x,y,z
							auto boxRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
							boxRitem->ObjCBIndex = index;
							boxRitem->Mat = mMaterials["gravel"].get();
							boxRitem->Geo = mGeometries["boxGeo"].get();
							boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
							boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
							boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(boxRitem));
							index++;
						}
						//creating a random number between 1 and 10 and spawning a iron block when it equals 2 so there is a 1 in 10 chance to spawn iron
						else if (rand() % 10 + 1 == 2) {
							//creating a iron block at position x,y,z
							auto boxRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
							boxRitem->ObjCBIndex = index;
							boxRitem->Mat = mMaterials["iron"].get();
							boxRitem->Geo = mGeometries["boxGeo"].get();
							boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
							boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
							boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(boxRitem));
							index++;
						}
						//making every other block in this range stone
						else {
							//creating a stone block at position x,y,z
							auto boxRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
							boxRitem->ObjCBIndex = index;
							boxRitem->Mat = mMaterials["stone"].get();
							boxRitem->Geo = mGeometries["boxGeo"].get();
							boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
							boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
							boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(boxRitem));
							index++;
						}
					}
					//making top row sand
					else if (y == 8) {
						//creating a sand block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["sand"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;

					}
					//making every other block in this range dirt
					else {
						//creating a dirt block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["dirt"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
				}
			}
		}
	}


	//gravel quadrant generation
	for (int z = 0; z < 50; z++) {
		for (int x = 50; x < 100; x++) {
			//create random number between 3 and 1 so there is a range of 3 in height in this quadrant
			random = rand() % 3 + 1;
			for (int y = 0; y < random + 8; y++) {
				//cheking to see if y is at the bottom so that it spawns bedrock
				if (y == 0)
				{
					//creating a bedrock block at position x,y,z
					auto boxR2item = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxR2item->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxR2item->ObjCBIndex = index;
					boxR2item->Mat = mMaterials["bedrock"].get();
					boxR2item->Geo = mGeometries["boxGeo"].get();
					boxR2item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxR2item->IndexCount = boxR2item->Geo->DrawArgs["box"].IndexCount;
					boxR2item->StartIndexLocation = boxR2item->Geo->DrawArgs["box"].StartIndexLocation;
					boxR2item->BaseVertexLocation = boxR2item->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxR2item));
					index++;
				}
				//creating a layer in a low range so only stone, gravel and iron spawn
				else if (y > 0 && y < 4)
				{
					//creating a random number between 1 and 5 and spawning a gravel block when it equals 2 so there is a 1 in 5 chance to spawn gravel
					if (rand() % 5 + 1 == 2) {
						//creating a gravel block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["gravel"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
					//creating a random number between 1 and 10 and spawning a iron block when it equals 2 so there is a 1 in 10 chance to spawn iron
					else if (rand() % 10 + 1 == 2) {
						//creating a iron block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["iron"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
					//making every other block in this range stone
					else {
						//creating a stone block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["stone"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
				}
				//making the top 3 rows gravel
				else if ((random == 1 && y == 8) || (random == 2 && ((y == 9) || (y == 8))) || (random == 3 && ((y == 10) || (y == 9) || (y == 8)))) {
					//creating a gravel block at position x,y,z
					auto boxRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxRitem->ObjCBIndex = index;
					boxRitem->Mat = mMaterials["gravel"].get();
					boxRitem->Geo = mGeometries["boxGeo"].get();
					boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
					boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
					boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxRitem));
					index++;

				}
				//making every other block in this range dirt
				else {
					//creating a dirt block at position x,y,z
					auto boxRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxRitem->ObjCBIndex = index;
					boxRitem->Mat = mMaterials["dirt"].get();
					boxRitem->Geo = mGeometries["boxGeo"].get();
					boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
					boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
					boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxRitem));
					index++;
				}
			}
		}
	}


	//water quadrant generation
	for (int z = 50; z < 100; z++) {
		for (int x = 0; x < 50; x++) {
			for (int y = 0; y < 9; y++) {
				//cheking to see if y is at the bottom so that it spawns bedrock
				if (y == 0)
				{
					//creating a bedrock block at position x,y,z
					auto boxR2item = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxR2item->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxR2item->ObjCBIndex = index;
					boxR2item->Mat = mMaterials["bedrock"].get();
					boxR2item->Geo = mGeometries["boxGeo"].get();
					boxR2item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxR2item->IndexCount = boxR2item->Geo->DrawArgs["box"].IndexCount;
					boxR2item->StartIndexLocation = boxR2item->Geo->DrawArgs["box"].StartIndexLocation;
					boxR2item->BaseVertexLocation = boxR2item->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxR2item));
					index++;
				}
				//creating a layer in a low range so only stone, gravel and iron spawn
				else if (y > 0 && y < 4)
				{
					//creating a random number between 1 and 5 and spawning a gravel block when it equals 2 so there is a 1 in 5 chance to spawn gravel
					if (rand() % 5 + 1 == 2) {
						//creating a gravel block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["gravel"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
					//creating a random number between 1 and 10 and spawning a iron block when it equals 2 so there is a 1 in 10 chance to spawn iron
					else if (rand() % 10 + 1 == 2) {
						//creating a iron block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["iron"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
					//making every other block in this range stone
					else {
						//creating a stone block at position x,y,z
						auto boxRitem = std::make_unique<RenderItem>();
						XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
						boxRitem->ObjCBIndex = index;
						boxRitem->Mat = mMaterials["stone"].get();
						boxRitem->Geo = mGeometries["boxGeo"].get();
						boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
						boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
						boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
						mAllRitems.push_back(std::move(boxRitem));
						index++;
					}
				}
				//making top layer water
				else if (y == 8) {
					//creating a water block at position x,y,z
					auto boxRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxRitem->ObjCBIndex = index;
					boxRitem->Mat = mMaterials["water"].get();
					boxRitem->Geo = mGeometries["boxGeo"].get();
					boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
					boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
					boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxRitem));
					index++;

				}
				//making every other block in this range dirt
				else {
					//creating a dirt block at position x,y,z
					auto boxRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation((float)x, (float)y, (float)z));
					boxRitem->ObjCBIndex = index;
					boxRitem->Mat = mMaterials["dirt"].get();
					boxRitem->Geo = mGeometries["boxGeo"].get();
					boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
					boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
					boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(boxRitem));
					index++;
				}
			}
		}
	}

	isBuilt = true;
	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	//TODO --- Define the static samplers 

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, //shade register
		D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, //FILTER
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, //shade register
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,								//mipLODBias
		8);									//maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return
	{
		pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp
	};
}