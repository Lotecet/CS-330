///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	m_basicMeshes->LoadPlaneMesh();					// table top
	m_basicMeshes->LoadCylinderMesh();				// jug body
	m_basicMeshes->LoadConeMesh();					// jug neck / lip
	m_basicMeshes->LoadTorusMesh();					// jug handle
	m_basicMeshes->LoadBoxMesh();                   // bread pieces (thin boxes)
	m_basicMeshes->LoadTaperedCylinderMesh();       // cup/bowl shape
	m_basicMeshes->LoadExtraTorusMesh1(0.12f);      // plate rim (thin)
	m_basicMeshes->LoadExtraTorusMesh2(0.22f);      // thicker rim (cup/basket rim)

	CreateGLTexture("textures/wood.jpg", "wood");
	CreateGLTexture("textures/stone.jpg", "stone");
	CreateGLTexture("textures/ceramic.jpg", "ceramic");  
	CreateGLTexture("textures/table.jpg", "table");  
	CreateGLTexture("textures/bread1.jpg", "bread1");  
	CreateGLTexture("textures/bread2.jpg", "bread2");
	CreateGLTexture("textures/basket.jpg", "basket");

	// bind textures to OpenGL texture slots
	BindGLTextures();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	
// two light sources, at least one colored, include a point light
	if (NULL != m_pShaderManager)
	{
		// Enable lighting for the scene (so that plane MUST reflect light)
		m_pShaderManager->setIntValue(g_UseLightingName, true);

		// Main directional light (warm, like sunlight/window light in the reference photo)
		m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.35f, -1.0f, -0.25f));
		m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.20f, 0.18f, 0.14f));
		m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.90f, 0.78f, 0.62f));
		m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.90f, 0.90f, 0.90f));
		m_pShaderManager->setIntValue("directionalLight.bActive", 1);

		// Turn OFF unused point lights 
		for (int i = 1; i < 5; i++)
		{
			std::string activeName = "pointLights[" + std::to_string(i) + "].bActive";
			m_pShaderManager->setIntValue(activeName.c_str(), 0);
		}

		// Fill point light 
		// one colored light + at least one point light.
		m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(4.5f, 6.5f, 4.5f));
		m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.10f, 0.08f, 0.06f));
		m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.85f, 0.55f, 0.30f));   // warm/orange tint
		m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.60f, 0.55f, 0.50f));
		m_pShaderManager->setIntValue("pointLights[0].bActive", 1);

		// Spot light disabled
		m_pShaderManager->setIntValue("spotLight.bActive", 0);
	}
	
	/******************************************************* 
	*    Table plane simple/brown                          *
	********************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	// Turn lighting ON for the plane so it reflects light 
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseLightingName, true);

		// Give the plane a shinier material so reflections are visible
		m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.80f, 0.80f, 0.80f));
		m_pShaderManager->setFloatValue("material.shininess", 64.0f);
	}

	// Change the color
	SetShaderTexture("table");
	SetTextureUVScale(4.0f, 4.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	
	/*******************************************************
	*    Jug Main Body Cylinder                            *
	********************************************************/

	// lighting ON for the jug so it has depth cues 
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseLightingName, true);

		// Less shiny than the plane
		m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.25f, 0.25f, 0.25f));
		m_pShaderManager->setFloatValue("material.shininess", 16.0f);
	}

	// declare the variables for the transformations
	scaleXYZ = glm::vec3(1.7f, 4.0f, 1.7f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Place jug in the middle of the table
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Slightly darker brown for the jug than the table
	SetShaderTexture("stone");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/*******************************************************
	*    Jug upper main Body Cylinder                      *
	********************************************************/

	// declare the variables for the transformations
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.7f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Place jug in the middle of the table
	positionXYZ = glm::vec3(0.0f, 3.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Slightly darker brown for the jug than the table
	SetShaderTexture("stone");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/*******************************************************
	*    Jug Lip Cone                                      *
	********************************************************/

	// declare the variables for the transformations
	// A smaller cone that sits on top of the cylinder body
	scaleXYZ = glm::vec3(1.4f, 2.0f, 1.4f);
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Upper body cone sits above the lower body jug
	positionXYZ = glm::vec3(0.0f, 6.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Slightly darker brown for the jug than the table
	SetShaderTexture("stone");
	SetTextureUVScale(1.0f, 0.8f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	/*******************************************************
	*    Jug Handle Torus                                  *
	*    curved handle on the side of the jug              *
	********************************************************/

	// Thin, tall torus to mimic the curved handle
	scaleXYZ = glm::vec3(0.9f, 1.6f, 0.5f);

	// Rotate so the torus stands vertically like a handle
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// Position handle to the right side of the jug body
	positionXYZ = glm::vec3(1.8f, 4.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Slightly darker brown for the jug than the table
	SetShaderTexture("stone");
	SetTextureUVScale(1.2f, 1.2f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/*******************************************************
	*    Plate (front)                                     *
	********************************************************/

	// Plate base (short cylinder)
	scaleXYZ = glm::vec3(3.0f, 0.10f, 3.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.2f, 0.16f, 5.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Plate rim (thin torus) 
	scaleXYZ = glm::vec3(3.05f, 3.0f, 3.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.2f, 0.16f, 5.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawExtraTorusMesh1();

	/*******************************************************
	*    Cup / bowl (back right of jug)                    *
	********************************************************/

	// Cup body (tapered cylinder)
	scaleXYZ = glm::vec3(1.15f, 1.05f, 1.15f);
	XrotationDegrees = 0.0f;   
	YrotationDegrees = 0.0f;   
	ZrotationDegrees = 180.0f;
	positionXYZ = glm::vec3(4.2f, 1.10f, -2.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);

	// Cup rim 
	scaleXYZ = glm::vec3(0.96f, 0.96f, 0.96f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = -50.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.2f, 0.95f, -2.2f);  

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawExtraTorusMesh2();

	/*******************************************************
	*    Basket (left of jug)                              *
	********************************************************/

	// Basket body 
	scaleXYZ = glm::vec3(2.2f, 1.05f, 2.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.2f, 0.10f, 1.2f);   

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("basket");			                 
	SetTextureUVScale(2.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(false, true, true); // open top 

	// Basket rim (torus) at the top edge
	scaleXYZ = glm::vec3(1.8f, 1.8f, 1.60f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = -50.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.2f, 1.15f, 1.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("basket");
	SetTextureUVScale(2.0f, 1.0f);
	m_basicMeshes->DrawExtraTorusMesh2();

	/*******************************************************
	*    Bread pieces (inside basket)                      *
	********************************************************/

	// Bread piece 1 — angled upward 
	scaleXYZ = glm::vec3(4.5f, 0.40f, 0.60f);     
	XrotationDegrees = 180.0f;                     
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 25.0f;
	positionXYZ = glm::vec3(-4.05f, 1.25f, 1.10f); 

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bread1");                     
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Bread piece 2 — secondary piece
	scaleXYZ = glm::vec3(4.5f, 0.55f, 0.16f);
	XrotationDegrees = -62.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees =25.0f;
	positionXYZ = glm::vec3(-4.30f, 1.18f, 1.30f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bread2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}