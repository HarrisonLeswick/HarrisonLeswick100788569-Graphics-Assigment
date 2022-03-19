#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/EnemyComponent.h"
#include "Gameplay/Components/CharacterMovement.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");

		// Load in the meshes
		MeshResource::Sptr canooMesh = ResourceManager::CreateAsset<MeshResource>("canoo.obj");
		MeshResource::Sptr turtleMesh = ResourceManager::CreateAsset<MeshResource>("turtle.obj");
		MeshResource::Sptr groundMesh = ResourceManager::CreateAsset<MeshResource>("ground.obj");
		MeshResource::Sptr flagMesh = ResourceManager::CreateAsset<MeshResource>("flag.obj");
		MeshResource::Sptr megaKitchenMesh = ResourceManager::CreateAsset<MeshResource>("megaKitchen.obj");
		MeshResource::Sptr trashMesh = ResourceManager::CreateAsset<MeshResource>("Trash.obj");
		MeshResource::Sptr bowlOfFruitMesh = ResourceManager::CreateAsset<MeshResource>("bowlOfFruit.obj");

		// Load in some textures
		Texture2D::Sptr    skyTex   = ResourceManager::CreateAsset<Texture2D>("textures/sky.png");
		Texture2D::Sptr    canooTex      = ResourceManager::CreateAsset<Texture2D>("textures/Cylinder1.png");
		Texture2D::Sptr    turtleTex = ResourceManager::CreateAsset<Texture2D>("textures/turtle.png");
		Texture2D::Sptr    groundTex    = ResourceManager::CreateAsset<Texture2D>("textures/Cube.png");
		Texture2D::Sptr    flagTex = ResourceManager::CreateAsset<Texture2D>("textures/flagUv.png");
		
		Texture2D::Sptr megaKitchenTex = ResourceManager::CreateAsset<Texture2D>("textures/KitchenTexture.png");
		Texture2D::Sptr trashTex = ResourceManager::CreateAsset<Texture2D>("textures/Trash.png");
		Texture2D::Sptr bowlOfFruitTex = ResourceManager::CreateAsset<Texture2D>("textures/FruitBowl.png");

		Texture3D::Sptr lutDumpsterFire = ResourceManager::CreateAsset<Texture3D>("luts/Apocalypsecube.cube");
		Texture3D::Sptr lutCold = ResourceManager::CreateAsset<Texture3D>("luts/Coldcube.cube");
		Texture3D::Sptr lutWarm = ResourceManager::CreateAsset<Texture3D>("luts/Warmcube.cube");


		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>(); 

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");  

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr skyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			skyMaterial->Name = "Sky";
			skyMaterial->Set("u_Material.Diffuse", skyTex);
			skyMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr bowlOfFruitMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			bowlOfFruitMaterial->Name = "Canoo";
			bowlOfFruitMaterial->Set("u_Material.Diffuse", bowlOfFruitTex);
			bowlOfFruitMaterial->Set("u_Material.Shininess", 0.1f);
		}	
		
		Material::Sptr turtleMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			turtleMaterial->Name = "Turtle";
			turtleMaterial->Set("u_Material.Diffuse", turtleTex);
			turtleMaterial->Set("u_Material.Shininess", 0.1f);
		}	
		
		Material::Sptr megaKitchenMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			megaKitchenMaterial->Name = "Ground";
			megaKitchenMaterial->Set("u_Material.Diffuse", megaKitchenTex);
			megaKitchenMaterial->Set("u_Material.Shininess", 0.1f);
		}	
		
		Material::Sptr trashMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			trashMaterial->Name = "Flag";
			trashMaterial->Set("u_Material.Diffuse", trashTex);
			trashMaterial->Set("u_Material.Shininess", 0.1f);
		}

		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(0.0f, 3.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		scene->Lights[1].Position = glm::vec3(1.0f, 3.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		scene->Lights[2].Position = glm::vec3(20.0f, 3.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();
		
		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ 9, -9, 9 });
			camera->SetRotation(glm::vec3(70, 0, 0));
		}
		
	
		GameObject::Sptr kitchen = scene->CreateGameObject("Kitchen");
		{
			// Set position in the scene
			kitchen->SetPostion(glm::vec3(9.63f, 0.04f, 2.66f));
			kitchen->SetRotation(glm::vec3(0.0f, 0.0f, -173.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = kitchen->Add<RenderComponent>();
			renderer->SetMesh(megaKitchenMesh);
			renderer->SetMaterial(megaKitchenMaterial);
		}

		GameObject::Sptr turtle = scene->CreateGameObject("Turtle");
		{
			// Set position in the scene
			turtle->SetPostion(glm::vec3(20.0f, 0.0f, 3.2f));
			turtle->SetRotation(glm::vec3(90.0, 0, 0));
			turtle->SetScale(glm::vec3(0.45, 0.45, 0.45));


			turtle->Add<EnemyComponent>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = turtle->Add<RenderComponent>();
			renderer->SetMesh(turtleMesh);
			renderer->SetMaterial(turtleMaterial);
		}
		GameObject::Sptr trash = scene->CreateGameObject("Trash");
		{
			trash->SetPostion(glm::vec3(1.98f, -3.1f, 2.84f));
			trash->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = trash->Add<RenderComponent>();
			renderer->SetMesh(trashMesh);
			renderer->SetMaterial(trashMaterial);
		}

		GameObject::Sptr bowlOfFruit = scene->CreateGameObject("Bowl of Fruit");
		{
			bowlOfFruit->SetPostion(glm::vec3(15.61f, 5.67f, 6.82f));
			bowlOfFruit->SetRotation(glm::vec3(90, 0, 0));


			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = bowlOfFruit->Add<RenderComponent>();
			renderer->SetMesh(bowlOfFruitMesh);
			renderer->SetMaterial(bowlOfFruitMaterial);
		}


		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
