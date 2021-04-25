#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library


// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Texture Loading Functions
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

//************************************************************
//CAMERA CLASS
//
//LearnOpenGL.com
//
//Modified ProcessKeyboard and ProcessMouseScroll to implement 
//up/down and speed functionality respectively
//************************************************************
#include "camera.h"




using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Michael Delfino :)"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;


    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    //***********************
    // Plane mesh data
    GLMesh gMesh_plane;
    // Cylinder mesh data
    GLMesh gMesh_body;
    GLMesh gMesh_bodyTop;
    GLMesh gMesh_fullCyl;
    // Cup Handle mesh data
    GLMesh gMesh_handle;
    GLMesh gMesh_handleInside;
    GLMesh gMesh_handleOutside;
    // Light Cube mesh data
    GLMesh gMesh_cube;
    glm::vec2 gUVScale(5.0f, 5.0f);
    //**********************

    // Texture data
    GLuint gTextureId_handle;
    GLuint gTextureId_cupBody;
    GLuint gTextureId_carpet;
    GLuint gTextureId_coffee;
    GLuint gTextureId_cupHandle;
    GLuint gTextureId_candleTop;
    GLuint gTextureId_candle;
    GLuint gTextureId_wax;
    GLuint gTextureId_cart;
    GLuint gTextureId_label;
    GLuint gTextureId_book;
    GLuint gTextureId_pages;
    GLuint gTextureId_spine;


    // Shader programs
    GLuint gProgramId;
    GLuint gLampProgramId;
    GLuint gPlaneProgramId;
    GLuint gCandleProgramId;

    //Light color
    glm::vec3 gLightColor(1.0, 1.0f, 0.90f);

    // Light position and scale
    glm::vec3 gLightPosition(-3.5f, 1.5f, 0.0f);
    glm::vec3 gLightScale(0.5f);

    // Mesh and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);

    // Camera constructor & vars initialization
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Perspective var
    bool isOrtho = false;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePosCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UPerspectiveSwitch(GLFWwindow* window, int key, int scancode, int action, int mods);
void UDestroyMesh(GLMesh& mesh);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void UCreateMesh(GLMesh& mesh, int meshChoice);
void URender(GLMesh& mesh_plane, GLMesh& mesh_body, GLMesh& mesh_bodyTop, GLMesh& mesh_handle, GLMesh& mesh_handleInside, GLMesh& mesh_handleOutside, GLMesh& mesh_cube, GLMesh& mesh_fullCyl, float incRotation);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
    layout(location = 2) in vec3 normal; // VAP position 2 for normals
    layout(location = 1) in vec2 textureCoordinate;  // Texture Data from Vertex Attrib Pointer 1

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate; // variable to transfer texture coords to the fragment shader

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;



    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // Variable to hold incoming normal coords
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate; // Variable to hold incoming texture coords from vertex shader

    out vec4 fragmentColor;

    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightColor2;
    uniform vec3 lightPos;
    uniform vec3 lightPos2;
    uniform vec3 viewPosition;

    uniform sampler2D uTexture; // Useful when working with multiple textures

    void main()
    {
        //Phong lighting model calculations to generate ambient, diffuse, and specular components

        //Calculate Ambient lighting
        float ambientStrength1 = 1.0f; // Set ambient or global lighting strength
        vec3 ambient = (ambientStrength1 * lightColor);

        //Calculate Diffuse lighting
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact1 = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = (impact1 * lightColor); // Generate diffuse light color

        //Calculate Specular lighting
        float specularIntensity = 3.0f; // Set specular light strength        
        float highlightSize = 16.0f; // Set specular highlight size        
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector        

        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, (reflectDir)), 0.0), highlightSize);        
        vec3 specular = (specularIntensity * specularComponent * lightColor);

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource_plane = GLSL(440,
    in vec3 vertexNormal; // Variable to hold incoming normal coords
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate; // Variable to hold incoming texture coords from vertex shader

    out vec4 fragmentColor;

    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightColor2;
    uniform vec3 lightPos;
    uniform vec3 lightPos2;
    uniform vec3 viewPosition;

    uniform sampler2D uTexture; // Useful when working with multiple textures

    void main()
    {
        //Phong lighting model calculations to generate ambient, diffuse, and specular components

        //Calculate Ambient lighting
        float ambientStrength1 = 0.5f; // Set ambient or global lighting strength
        vec3 ambient = (ambientStrength1 * lightColor);

        //Calculate Diffuse lighting
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact1 = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = (impact1 * lightColor); // Generate diffuse light color

        //Calculate Specular lighting
        float specularIntensity = 0.5f; // Set specular light strength        
        float highlightSize = 16.0f; // Set specular highlight size        
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector        

        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, (reflectDir)), 0.0), highlightSize);
        vec3 specular = (specularIntensity * specularComponent * lightColor);

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate);
        if (textureColor.a < 0.1)
            discard;

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource_candle = GLSL(440,
    in vec3 vertexNormal; // Variable to hold incoming normal coords
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate; // Variable to hold incoming texture coords from vertex shader

out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightColor2;
uniform vec3 lightPos;
uniform vec3 lightPos2;
uniform vec3 viewPosition;

uniform sampler2D uTexture; // Useful when working with multiple textures

void main()
{
    //Phong lighting model calculations to generate ambient, diffuse, and specular components

    //Calculate Ambient lighting
    float ambientStrength1 = 0.5f; // Set ambient or global lighting strength
    vec3 ambient = (ambientStrength1 * lightColor);

    //Calculate Diffuse lighting
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact1 = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = (impact1 * lightColor); // Generate diffuse light color

    //Calculate Specular lighting
    float specularIntensity = 0.5f; // Set specular light strength        
    float highlightSize = 16.0f; // Set specular highlight size        
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector        

    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, (reflectDir)), 0.0), highlightSize);
    vec3 specular = (specularIntensity * specularComponent * lightColor);

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate);

    

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 0.1f); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
    layout(location = 2) in vec3 normal; // VAP position 1 for normals
    layout(location = 1) in vec2 textureCoordinate;  // Texture Data from Vertex Attrib Pointer 2

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate; // variable to transfer texture coords to the fragment shader

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;



    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
);

/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    in vec2 vertexTextureCoordinate; // Variable to hold incoming texture coords from vertex shader

    out vec4 fragmentColor;

    uniform sampler2D uTexture;

    void main()
    {
        //vec4 texColor = texture(uTexture, vertexTextureCoordinate);
        //if (texColor.a < 0.1)
            //discard;
        fragmentColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);// Sends texture to GPU
    }
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

//*************************************************************************************************************************

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    //***************************************************************
    //Create Individual Meshes
    //***************************************************************
    //Mesh identifiers
    //
    //Plane : 0
    //Cylinder No Top : 1
    //Cup Handle Frame : 2
    //Handle Inside : 3
    //Handle Outside : 4
    //Circle : 5
    //Cube : 6
    //Full Cylinder : 7
    //
    //***************************************************************

    // Create the meshes based on identifier

    UCreateMesh(gMesh_plane, 0);
    UCreateMesh(gMesh_body, 1);
    UCreateMesh(gMesh_handle, 2);
    UCreateMesh(gMesh_handleInside, 3);
    UCreateMesh(gMesh_handleOutside, 4);
    UCreateMesh(gMesh_bodyTop, 5);
    UCreateMesh(gMesh_cube, 6);
    UCreateMesh(gMesh_fullCyl, 7);

    //***************************************************************



    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Create the light cube shader program
    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Create the light cube shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource_plane, gPlaneProgramId))
        return EXIT_FAILURE;

    // Create the light cube shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource_candle, gCandleProgramId))
        return EXIT_FAILURE;

    // Load Textures
    // Transparent Texture
    const char* texFilename = "../resources/textures/transparency.png";
    if (!UCreateTexture(texFilename, gTextureId_handle))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //Cup Texture
    texFilename = "../resources/textures/brown5.jpg";
    if (!UCreateTexture(texFilename, gTextureId_cupBody))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //handle
    texFilename = "../resources/textures/brown4.jpg";
    if (!UCreateTexture(texFilename, gTextureId_cupHandle))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //Carpet
    texFilename = "../resources/textures/carpet.jpg";
    if (!UCreateTexture(texFilename, gTextureId_carpet))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //Coffee
    texFilename = "../resources/textures/coffee2.jpg";
    if (!UCreateTexture(texFilename, gTextureId_coffee))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //Candle Top
    texFilename = "../resources/textures/candleTop.png";
    if (!UCreateTexture(texFilename, gTextureId_candleTop))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //Candle 
    texFilename = "../resources/textures/candle4.png";
    if (!UCreateTexture(texFilename, gTextureId_candle))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCandleProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCandleProgramId, "uTexture"), 0);

    //Wax
    texFilename = "../resources/textures/wax.jpg";
    if (!UCreateTexture(texFilename, gTextureId_wax))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    //cart
    texFilename = "../resources/textures/grey.jpg";
    if (!UCreateTexture(texFilename, gTextureId_cart))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gPlaneProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gPlaneProgramId, "uTexture"), 0);

    //cart Label
    texFilename = "../resources/textures/mario label.png";
    if (!UCreateTexture(texFilename, gTextureId_label))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gPlaneProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gPlaneProgramId, "uTexture"), 0);

    //book cover
    texFilename = "../resources/textures/book.png";
    if (!UCreateTexture(texFilename, gTextureId_book))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gPlaneProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gPlaneProgramId, "uTexture"), 0);

    //pages
    texFilename = "../resources/textures/pages.jpg";
    if (!UCreateTexture(texFilename, gTextureId_pages))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gPlaneProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gPlaneProgramId, "uTexture"), 0);

    //spine
    texFilename = "../resources/textures/spine.jpg";
    if (!UCreateTexture(texFilename, gTextureId_spine)){
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gPlaneProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gPlaneProgramId, "uTexture"), 0);


    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    //incremental rotation variable for camera movement
    //this example doesn't currently increment, stays at 0.0
    float incRotation = 0.0;

    //*************************************************************************************************************
    //RENDER LOOP
    //*************************************************************************************************************

    while (!glfwWindowShouldClose(gWindow))
    {

        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);
   
        // Render this frame
        // Call overloaded URender, passing in multiple meshes and rotation variable 
        // No rotation in this example

        URender(gMesh_plane, gMesh_body, gMesh_bodyTop, gMesh_handle, gMesh_handleInside, gMesh_handleOutside, gMesh_cube, gMesh_fullCyl, incRotation);


        glfwPollEvents();
    }




    // Release mesh data
    UDestroyMesh(gMesh_plane);
    UDestroyMesh(gMesh_body);
    UDestroyMesh(gMesh_bodyTop);
    UDestroyMesh(gMesh_handle);
    UDestroyMesh(gMesh_handleInside);
    UDestroyMesh(gMesh_handleOutside);
    UDestroyMesh(gMesh_cube);
    UDestroyMesh(gMesh_fullCyl);


    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);
    UDestroyShaderProgram(gCandleProgramId);
    UDestroyShaderProgram(gPlaneProgramId);

    // Release texture data
    UDestroyTexture(gTextureId_handle);
    UDestroyTexture(gTextureId_cupBody);
    UDestroyTexture(gTextureId_carpet);
    UDestroyTexture(gTextureId_coffee);
    UDestroyTexture(gTextureId_candleTop);
    UDestroyTexture(gTextureId_candle);
    UDestroyTexture(gTextureId_wax);
    UDestroyTexture(gTextureId_cart);
    UDestroyTexture(gTextureId_label);
    UDestroyTexture(gTextureId_book);
    UDestroyTexture(gTextureId_spine);
    UDestroyTexture(gTextureId_pages);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}



// ****************************************************************************
// WINDOW CREATION & GLFW CONFIGURE
//*****************************************************************************
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    //******************
    //Window Create
    //******************
    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);

    //Initialize mouse location/scroll in window callback
    glfwSetKeyCallback(*window, UPerspectiveSwitch);
    glfwSetCursorPosCallback(*window, UMousePosCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //*******************
    // INITIALIZE GLEW
    // ******************
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}



//********************************************************
//INPUT REGISTER
//*******************************************************
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);    
}



void UMousePosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

//Process Mouse Scroll Wheel in Window
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

void UPerspectiveSwitch(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        if (isOrtho == true) {
            isOrtho = false;
            return;
        }
        else {
            isOrtho = true;
            return;
        }           
}



//***********************************************************************************************************************
//RENDER FUNCTION
//
//Overloaded URender function passing multiple meshes
//Used to render a single frame
//***********************************************************************************************************************


void URender(GLMesh& mesh_plane, GLMesh& mesh_body, GLMesh& mesh_bodyTop, GLMesh& mesh_handle, GLMesh& mesh_handleInside, GLMesh& mesh_handleOutside, GLMesh& mesh_cube, GLMesh& mesh_fullCyl, float incRotation)
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);
    

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);




    //**********************************************************************
    //Plane
    //**********************************************************************

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDisable(GL_CULL_FACE);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for plane
    glm::mat4 scale = glm::scale(glm::vec3(15.0f, 15.0f, 15.0f));
    glm::mat4 XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 translation = glm::translate(glm::vec3(-3.0f, -2.25f, 0.0f));
    glm::mat4 model = translation * XRotation * scale;       

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");    
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");    
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);    
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);    
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_carpet);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);


    //**********************************************************************
    //Lamp 
    //**********************************************************************
    glUseProgram(gLampProgramId);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_cube.vao);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


    // Draws the cube
    glDrawElements(GL_TRIANGLES, mesh_cube.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
    //Book Pages
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(2.0f, .5f, 3.0f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.25f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-3.0f, -2.0f, 5.0f));
    model = translation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_cube.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_pages);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_cube.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
    //Book Cover
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(3.15f, .5f, 2.15f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(1.8208f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-3.3f, -1.746f, 3.55f));
    model = translation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_book);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
    //Book Cover
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(3.15f, .5f, 2.15f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(1.8208f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-3.3f, -2.24f, 3.55f));
    model = translation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_book);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
    //Book Spine
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(.5f, .5f, 3.05f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.25f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(1.5708f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-4.35f, -2.0f, 3.8f));
    model = translation * XRotation *  YRotation * ZRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_spine);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
   //Cartridge Body
   //**********************************************************************

   // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(.25f, 1.0f, 1.2f)); 
    XRotation = glm::rotate(-1.575f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(1.5708f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(-0.6f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-1.7f, -2.13f, 2.5f));
    model = translation * ZRotation * XRotation *  YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_cube.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cart);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_cube.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
   //Cartridge inside wall
   //**********************************************************************

   // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(.25f, 1.0f, 1.2f));
    XRotation = glm::rotate(-1.575f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(1.5708f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(-0.6f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-2.15f, -1.825f, 2.85f));
    model = translation * ZRotation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cart);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
  //Cartridge chip
  //**********************************************************************

  // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(.25f, 1.0f, 1.2f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(1.5708f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(-0.6f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-2.15f, -1.825f, 2.85f));
    model = translation * ZRotation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cupBody);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
    //Cartridge Label
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(0.80f, 0.85f, 0.90f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(1.5708f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(-0.6f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-2.13f, -1.66f, 2.5f));
    model = translation * ZRotation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_plane.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_label);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_plane.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
    //Cartridge Side 1
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(0.25f, 0.25f, 0.999f));
    XRotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-1.7f, -2.13f, 2.0005f));
    model = translation * ZRotation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_fullCyl.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cart);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_fullCyl.nIndices, GL_UNSIGNED_SHORT, NULL);

   //**********************************************************************
   //Cartridge Side 2
   //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gPlaneProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(0.25f, 0.25f, 0.999f));
    XRotation = glm::rotate(-0.005f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-2.68f, -1.462f, 2.0005f));
    model = translation * ZRotation * XRotation * YRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_fullCyl.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cart);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_fullCyl.nIndices, GL_UNSIGNED_SHORT, NULL);


    //**********************************************************************
    //Coffee Cup Body
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    XRotation = glm::rotate(1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(0.0f, -0.24f, 0.0f));
    model =  translation * XRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
    
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_body.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cupBody);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_body.nIndices, GL_UNSIGNED_SHORT, NULL);

   //**********************************************************************
   //Candle Body
   //**********************************************************************

   // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gCandleProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    XRotation = glm::rotate(1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-5.5f, -0.24f, 0.0f));
    model = translation * XRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_body.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_candle);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_body.nIndices, GL_UNSIGNED_SHORT, NULL);

    //**********************************************************************
   //Candle Inside
   //**********************************************************************

   // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(1.8f, 1.5f, 1.8f));
    XRotation = glm::rotate(1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-5.5f, -0.5f, 0.0f));
    model = translation * XRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    //UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_body.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_wax);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_body.nIndices, GL_UNSIGNED_SHORT, NULL);


    //**********************************************************************
    //Coffee Cup Top Texture
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    XRotation = glm::rotate(1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
    model = translation * XRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_bodyTop.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_coffee);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_bodyTop.nIndices, GL_UNSIGNED_SHORT, NULL);


    //**********************************************************************
    //Candle Top Texture
    //**********************************************************************

    // Wireframe Mode (helps with translation & scaling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set the shader to be used
    glUseProgram(gProgramId);

    //Set up scaling and rotation for cup body
    scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    XRotation = glm::rotate(1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    YRotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ZRotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-5.5f, -0.5f, 0.0f));
    model = translation * XRotation * scale;

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_bodyTop.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_candleTop);

    // Draws the coffee cup body
    glDrawElements(GL_TRIANGLES, mesh_bodyTop.nIndices, GL_UNSIGNED_SHORT, NULL);


    
    //**********************************************************************
    //Coffee Cup Handle
    //**********************************************************************

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // [stencil]
    glEnable(GL_STENCIL_TEST);
    // whole stencil=0
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    // turn off color,depth
    glStencilMask(0xFF);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    //Set up scaling and rotations for coffee cup handle
    scale = glm::scale(glm::vec3(1.5f, 1.5f, 0.25f));    
    YRotation = glm::rotate(0.122173f, glm::vec3(0.0f, -1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.9f, -1.25f, 0.0f));
    

    model =  translation * YRotation * scale;

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_handle.vao);

    glBindTexture(GL_TEXTURE_2D, gTextureId_cupHandle);

    // Draws the coffee cup handle
    glDrawElements(GL_TRIANGLES, mesh_handle.nIndices, GL_UNSIGNED_SHORT, NULL);

    
    //**********************************************************************
    //Coffee Cup Handle Inside
    //**********************************************************************

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    //Set up scaling and rotations for coffee cup handle
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 0.25f));
    YRotation = glm::rotate(0.122173f, glm::vec3(0.0f, -1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.9f, -1.25f, 0.0f));


    model = translation * YRotation * scale;

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_handle.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId_cupHandle);


    // Draws the coffee cup handle
    glDrawElements(GL_TRIANGLES, mesh_handleInside.nIndices, GL_UNSIGNED_SHORT, NULL);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


    //Set up scaling and rotations for coffee cup handle
    scale = glm::scale(glm::vec3(1.5f, 1.5f, 0.25f));
    YRotation = glm::rotate(0.122173f, glm::vec3(0.0f, -1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.9f, -1.25f, 0.0f));


    model = translation * YRotation * scale;

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_handle.vao);

    glBindTexture(GL_TEXTURE_2D, gTextureId_cupHandle);

    // Draws the coffee cup handle
    glDrawElements(GL_TRIANGLES, mesh_handle.nIndices, GL_UNSIGNED_SHORT, NULL);

    glDisable(GL_STENCIL_TEST);

    //**********************************************************************
    //Coffee Cup Handle Outside
    //**********************************************************************

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //Set up scaling and rotations for coffee cup handle
    scale = glm::scale(glm::vec3(1.5f, 1.5f, 0.25f));
    YRotation = glm::rotate(0.122173f, glm::vec3(0.0f, -1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.9f, -1.25f, 0.0f));


    model = translation * YRotation * scale;

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh_handleOutside.vao);

    glBindTexture(GL_TEXTURE_2D, gTextureId_cupHandle);

    // Draws the coffee cup handle
    glDrawElements(GL_TRIANGLES, mesh_handleOutside.nIndices, GL_UNSIGNED_SHORT, NULL);

    //***************************************************************************
    //Set up camera perspective
    //****************************************************************************
    //Perspective view settings
    if (isOrtho == false) {
        viewLoc = glGetUniformLocation(gProgramId, "view");
        projLoc = glGetUniformLocation(gProgramId, "projection");


        // camera/view transformation
        view = gCamera.GetViewMatrix();

        // Creates a perspective projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


        // Deactivate the Vertex Array Object
        glBindVertexArray(0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.    
    }
    //Orthographic view settings
    else if (isOrtho == true) {

        viewLoc = glGetUniformLocation(gProgramId, "view");
        projLoc = glGetUniformLocation(gProgramId, "projection");       

        view = glm::lookAt(
            glm::vec3(0, -0.49, 5), //Camera at this location in space
            glm::vec3(0, -.5, 0), //Camera looking at this location
            glm::vec3(0, 1, 0) // Head up or down 
            );

        translation = glm::translate(glm::vec3(3.0f, 0.0f, 0.0f));

        glm::mat4 orthoView = view * translation;
        glm::mat4 orthoProjection = glm::ortho(-10.0f, 10.0f, -7.5f, 7.5f, 0.1f, 100.0f);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(orthoView));

    
    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.  
    }
}



//***************************************************************************************
//CREATE MESH FUNCTION
//
//Overloaded the UCreateMesh function passing in mesh choice to create different meshes
//***************************************************************************************
void UCreateMesh(GLMesh& mesh, int meshChoice)
{
    if (meshChoice == 0) {
        //**********************************************************************
        //Plane Data
        //**********************************************************************

        // Position and Color data
        GLfloat verts[] = {

            // Square Plane
            // Vertex Positions                  //Normals
             0.5f,  0.0f, 0.5f,    1.0f, 1.0f,   0.0f, 1.0f, 0.0f,             // Top Right Corner 0
            -0.5f,  0.0f, 0.5f,    1.0f, 0.0f,   0.0f, 1.0f, 0.0f,             // Top Left Corner 1
             0.5f,  0.0f, -0.5f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,              // Bottom Right Corner 2
            -0.5f,  0.0f, -0.5f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,                 // Bottom Left Corner 3            
        };

        // Index data to share position data
        GLushort indices[] = {            
            0, 1, 2,
            2, 3, 1           
        };

        const GLuint floatsPerTexture = 2;
        const GLuint floatsPerVertex = 3;
        const GLuint floatsPerNormal = 3;


        glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
        glBindVertexArray(mesh.vao);

        // Create 2 buffers: first one for the vertex data; second one for the indices
        glGenBuffers(2, mesh.vbos);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

        mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Strides between vertex coordinates is 8 (x, y, z, tX, tY, nX, nY, nZ). A tightly packed stride is 0.
        GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture + floatsPerNormal);// The number of floats before each

        // Create Vertex Attribute Pointers
        glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerTexture)));
        glEnableVertexAttribArray(2);
    }

    else if (meshChoice == 1)  {
        //**********************************************************************
        //Coffee Cup Body
        //**********************************************************************

        // Position and Color data
        GLfloat verts[] = {

            //Top Circle
            // Vertex Positions    
            // Top Right Quarter
             0.0f,  0.5f, 0.0f,      0.0f,  1.0f,     0.0f, 1.0f, -0.5f,                    // Top Mid Circle Vertex 0

             0.1f,  0.49f, 0.0f,     0.03f, 1.0f,     0.2f, 0.98f, -0.5f,                   // 1
             0.2f,  0.45f, 0.0f,     0.06f, 1.0f,      0.4f, 0.92f, -0.5f,                 // 2
             0.3f,  0.4f, 0.0f,      0.12f, 1.0f,      0.6f, 0.8f, -0.5f,                   // 3
             0.4f,  0.3f, 0.0f,      0.15f, 1.0f,      0.8f, 0.6f, -0.5f,                  // 4
             0.46f,  0.2f, 0.0f,     0.18f, 1.0f,       0.90f, 0.43f, -0.5f,                // 5
             0.49f,  0.1f, 0.0f,     0.21f, 1.0f,      0.98f, 0.2f, -0.5f,                   // 6

             0.5f,  0.0f, 0.0f,      0.25f, 1.0f,      1.0f, 0.0f, -0.5f,                  // 7 Right Mid Circle Vertex

             // Bottom Right Quarter
             0.49f,  -0.1f, 0.0f,    0.28f, 1.0f,      0.98f, -0.2f, -0.5f,                 // 8
             0.45f,  -0.2f, 0.0f,    0.31f, 1.0f,      0.92f, -0.4f, -0.5f,                 // 9
             0.4f,  -0.3f, 0.0f,     0.34f, 1.0f,      0.8f, -0.6f, -0.5f,                  // 10
             0.3f,  -0.4f, 0.0f,     0.37f, 1.0f,      0.6f, -0.8f, -0.5f,                  // 11
             0.2f,  -0.46f, 0.0f,    0.40f, 1.0f,      0.4f, -0.92f, -0.5f,                 // 12
             0.1f,  -0.49f, 0.0f,    0.43f, 1.0f,      0.2f, -0.98f, -0.5f,                 // 13

             0.0f,  -0.5f, 0.0f,     0.5f, 1.0f,       0.0f, -1.0f, -0.5f,                 // 14 Bottom Mid Circle Vertex

             // Bottom Left Quarter
             -0.1f,  -0.49f, 0.0f,   0.53f, 1.0f,      -0.2f, -0.98f, -0.5f,                  // 15
             -0.2f,  -0.45f, 0.0f,   0.56f, 1.0f,       -0.4f, -0.92f, -0.5f,                 // 16
             -0.3f,  -0.4f, 0.0f,    0.59f, 1.0f,       -0.6f, -0.8f, -0.5f,                  // 17
             -0.4f,  -0.3f, 0.0f,    0.63f, 1.0f,       -0.8f, -0.6f, -0.5f,                  // 18
             -0.46f, -0.2f, 0.0f,    0.66f, 1.0f,        -0.90f, -0.43f, -0.5f,               // 19
             -0.49f, -0.1f, 0.0f,    0.70f, 1.0f,       -0.98f, -0.2f, -0.5f,                 // 20

             -0.5f,  -0.0f, 0.0f,    0.75f, 1.0f,       -1.0f, 0.0f, -0.5f,                  // 21 Left Mid Circle Vertex
             // Top Right Quarter
             -0.49f, 0.1f, 0.0f,     0.78f, 1.0f,      -0.98f, 0.2f, -0.5f,                 // 22
             -0.45f, 0.2f, 0.0f,     0.81f, 1.0f,      -0.92f, 0.4f, -0.5f,                 // 23
             -0.4f,  0.3f, 0.0f,     0.84f, 1.0f,      -0.8f, 0.6f, -0.5f,                  // 24
             -0.3f,  0.4f, 0.0f,     0.87f, 1.0f,      -0.6f, 0.8f, -0.5f,                  // 25
             -0.2f,  0.46f, 0.0f,    0.91f, 1.0f,      -0.4f, 0.92f, -0.5f,                 // 26
             -0.1f,  0.49f, 0.0f,    0.93f, 1.0f,      -0.2f, 0.98f, -0.5f,                 // 27

              0.0f,  0.0f, 0.0f,     1.0f, 1.0f,       -0.5f, 1.0f, -0.5f,          // Circle Center 28

            //Bottom Circle
            // Vertex Positions    
            // Top Right Quarter
             0.0f,  0.5f, 1.0f,     0.0f, 0.0f,  0.0f, 1.0f, -0.5f,            // Top Mid Circle Vertex 29

             0.1f,  0.49f, 1.0f,    0.03f, 0.0f, 0.2f, 0.98f, -0.5f,           // 30
             0.2f,  0.45f, 1.0f,    0.06f, 0.0f,  0.4f, 0.92f, -0.5f,          // 31
             0.3f,  0.4f, 1.0f,     0.12f, 0.0f,  0.6f, 0.8f, -0.5f,           // 32
             0.4f,  0.3f, 1.0f,     0.15f, 0.0f,  0.8f, 0.6f, -0.5f,           // 33
             0.46f,  0.2f, 1.0f,    0.18f, 0.0f,   0.90f, 0.43f, -0.5f,        // 34
             0.49f,  0.1f, 1.0f,    0.21f, 0.0f,  0.98f, 0.2f, -0.5f,          // 35

             0.5f,  0.0f, 1.0f,     0.25f, 0.0f,  1.0f, 0.0f, -0.5f,          //36 Right Mid Circle Vertex

             // Bottom Right Quarter
             0.49f,  -0.1f, 1.0f,   0.28f, 0.0f,  0.98f, -0.2f, -0.5f,          // 37
             0.45f,  -0.2f, 1.0f,   0.31f, 0.0f,  0.92f, -0.4f, -0.5f,          // 38
             0.4f,  -0.3f, 1.0f,    0.34f, 0.0f,  0.8f, -0.6f, -0.5f,           // 39
             0.3f,  -0.4f, 1.0f,    0.37f, 0.0f,  0.6f, -0.8f, -0.5f,           // 40
             0.2f,  -0.46f, 1.0f,   0.40f, 0.0f,  0.4f, -0.92f, -0.5f,          // 41
             0.1f,  -0.49f, 1.0f,   0.43f, 0.0f,  0.2f, -0.98f, -0.5f,          // 42

             0.0f,  -0.5f, 1.0f,    0.5f, 0.0f,   0.0f, -1.0f, -0.5f,            //43 Bottom Mid Circle Vertex

             // Bottom Left Quarter
             -0.1f,  -0.49f, 1.0f,  0.53f, 0.0f,   -0.2f, -0.98f, -0.5f,             //44
             -0.2f,  -0.45f, 1.0f,  0.56f, 0.0f,    -0.4f, -0.92f, -0.5f,            //45
             -0.3f,  -0.4f, 1.0f,   0.59f, 0.0f,    -0.6f, -0.8f, -0.5f,             //46
             -0.4f,  -0.3f, 1.0f,   0.63f, 0.0f,    -0.8f, -0.6f, -0.5f,              //47
             -0.46f, -0.2f, 1.0f,   0.66f, 0.0f,     -0.90f, -0.43f, -0.5f,          //48
             -0.49f, -0.1f, 1.0f,   0.70f, 0.0f,    -0.98f, -0.2f, -0.5f,           // 49

             -0.5f,  -0.0f, 1.0f,   0.75f, 0.0f,    -1.0f, 0.0f, -0.5f,              //50 Left Mid Circle Vertex
                                          
             // Top Right Quarter
             -0.49f, 0.1f, 1.0f,    0.78f, 0.0f,  -0.98f, 0.2f, -0.5f,             // 51
             -0.45f, 0.2f, 1.0f,    0.81f, 0.0f,  -0.92f, 0.4f, -0.5f,             //52
             -0.4f,  0.3f, 1.0f,    0.84f, 0.0f,  -0.8f, 0.6f, -0.5f,             // 53
             -0.3f,  0.4f, 1.0f,    0.87f, 0.0f,  -0.6f, 0.8f, -0.5f,              //54
             -0.2f,  0.46f, 1.0f,   0.91f, 0.0f,  -0.4f, 0.92f, -0.5f,             //55
             -0.1f,  0.49f, 1.0f,   0.93f, 0.0f,  -0.2f, 0.98f, -0.5f,             //56

             0.0f,  0.0f, 1.0f,      1.0f, 1.0f,  0.0f, 0.0f, 0.0f,              // Circle Center 57

             0.0f,  0.5f, 0.0f,      1.0f,  1.0f,  0.0f, 0.0f, 0.0f,          // Top Mid Circle Vertex 58
             0.0f,  0.5f, 1.0f,      1.0f,  0.0f,   0.0f, 0.0f, 0.0f,          // bottom Mid Circle Vertex 59

        };

        // Index data to share position data
        GLushort indices[] = {
            //**********************************************************************
            //Coffee Cup Body
            //**********************************************************************
            
            //Bottom Circle 
            //Top Right Quarter
            29, 30, 57,
            30, 31, 57,
            31, 32, 57,
            32, 33, 57,
            33, 34, 57,
            34, 35, 57,
            35, 36, 57,

            //Bottom Right Quarter
            36, 37, 57,
            37, 38, 57,
            38, 39, 57,
            39, 40, 57,
            40, 41, 57,
            41, 42, 57,
            42, 43, 57,

            //Bottom Left Quarter
            43, 44, 57,
            44, 45, 57,
            45, 46, 57,
            46, 47, 57,
            47, 48, 57,
            48, 49, 57,
            49, 50, 57,

            //Top Right Quarter
            50, 51, 57,
            51, 52, 57,
            52, 53, 57,
            53, 54, 57,
            54, 55, 57,
            55, 56, 57,
            56, 29, 57,

            //Cylinder Sides
            0, 29, 30,
            30, 1, 0,

            1, 30, 31,
            31, 2, 1,

            2, 31, 32,
            32, 3, 2,

            3, 32, 33,
            33, 4, 3,

            4, 33, 34,
            34, 5, 4,

            5, 34, 35,
            35, 6, 5,

            6, 35, 36,
            36, 7, 6,

            7, 36, 37,
            37, 8, 7,

            8, 37, 38,
            38, 9, 8,

            9, 38, 39,
            39, 10, 9,

            10, 39, 40,
            40, 11, 10,

            11, 40, 41,
            41, 12, 11,

            12, 41, 42,
            42, 13, 12,

            13, 42, 43,
            43, 14, 13,

            14, 43, 44,
            44, 15, 14,

            15, 44, 45,
            45, 16, 15,

            16, 45, 46,
            46, 17, 16,

            17, 46, 47,
            47, 18, 17,

            18, 47, 48,
            48, 19, 18,

            19, 48, 49,
            49, 20, 19,

            20, 49, 50,
            50, 21, 20,

            21, 50, 51,
            51, 22, 21,

            22, 51, 52,
            52, 23, 22,

            23, 52, 53,
            53, 24, 23,

            24, 53, 54,
            54, 25, 24,

            25, 54, 55,
            55, 26, 25,

            26, 55, 56,
            56, 27, 26,

            27, 58, 59,
            27, 56, 59
        };

        const GLuint floatsPerTexture = 2;
        const GLuint floatsPerVertex = 3;
        const GLuint floatsPerNormal = 3;


        glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
        glBindVertexArray(mesh.vao);

        // Create 2 buffers: first one for the vertex data; second one for the indices
        glGenBuffers(2, mesh.vbos);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

        mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Strides between vertex coordinates is 8 (x, y, z, tX, tY, nX, nY, nZ). A tightly packed stride is 0.
        GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture + floatsPerNormal);// The number of floats before each

        // Create Vertex Attribute Pointers
        glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerTexture)));
        glEnableVertexAttribArray(2);
             
    }

    else if (meshChoice == 2) {
           //**********************************************************************
           //Coffee Cup Handle
           //**********************************************************************
           //Currently using half cylinder mesh scaled and rotated for handle
           //Will eventually try to use a torus once I understand how to render one

            // Position and Color data
            GLfloat verts[] = {

                //Top Circle
                // Vertex Positions    // Colors (r,g,b,a)
                // Top Right Quarter
                 0.0f,  0.5f, 0.0f,   1.0f, 1.0f, // Top Mid Circle Vertex 0

                 0.1f,  0.49f, 0.0f,    1.0f, 1.0f, // 1
                 0.2f,  0.45f, 0.0f,    1.0f, 1.0f, // 2
                 0.3f,  0.4f, 0.0f,    1.0f, 1.0f,// 3
                 0.4f,  0.3f, 0.0f,    1.0f, 1.0f,// 4
                 0.46f,  0.2f, 0.0f,    1.0f, 1.0f,// 5
                 0.49f,  0.1f, 0.0f,    1.0f, 1.0f,// 6

                 0.5f,  0.0f, 0.0f,    1.0f, 1.0f,// 7 Right Mid Circle Vertex

                 // Bottom Right Quarter
                 0.49f,  -0.1f, 0.0f,    1.0f, 1.0f,// 8
                 0.45f,  -0.2f, 0.0f,    1.0f, 1.0f,// 9
                 0.4f,  -0.3f, 0.0f,    1.0f, 1.0f,// 10
                 0.3f,  -0.4f, 0.0f,    1.0f, 1.0f,// 11
                 0.2f,  -0.46f, 0.0f,   1.0f, 1.0f,// 12
                 0.1f,  -0.49f, 0.0f,   1.0f, 1.0f,// 13

                 0.0f,  -0.5f, 0.0f,    1.0f, 1.0f,// 14 Bottom Mid Circle Vertex                  

                 0.0f,  0.0f, 0.0f,     1.0f, 1.0f,// Circle Center 15

                //Bottom Circle
                // Vertex Positions    // Colors (r,g,b,a)
                // Top Right Quarter
                 0.0f,  0.5f, 1.0f,   1.0f, 1.0f,// Top Mid Circle Vertex 16

                 0.1f,  0.49f, 1.0f,   1.0f, 1.0f,// 17
                 0.2f,  0.45f, 1.0f,   1.0f, 1.0f,// 18
                 0.3f,  0.4f, 1.0f,    1.0f, 1.0f,// 19
                 0.4f,  0.3f, 1.0f,   1.0f, 1.0f,// 20
                 0.46f,  0.2f, 1.0f,   1.0f, 1.0f,// 21
                 0.49f,  0.1f, 1.0f,   1.0f, 1.0f,// 22

                 0.5f,  0.0f, 1.0f,    1.0f, 1.0f,// 23 Right Mid Circle Vertex

                 // Bottom Right Quarter
                 0.49f,  -0.1f, 1.0f,    1.0f, 1.0f,// 24
                 0.45f,  -0.2f, 1.0f,   1.0f, 1.0f,// 25
                 0.4f,  -0.3f, 1.0f,   1.0f, 1.0f,// 26
                 0.3f,  -0.4f, 1.0f,    1.0f, 1.0f,// 27
                 0.2f,  -0.46f, 1.0f,    1.0f, 1.0f,// 28
                 0.1f,  -0.49f, 1.0f,    1.0f, 1.0f,// 29

                 0.0f,  -0.5f, 1.0f,    1.0f, 1.0f,// 30 Bottom Mid Circle Vertex                

                 0.0f,  0.0f, 1.0f,    1.0f, 1.0f,// Circle Center 31
            };

            // Index data to share position data
            GLushort indices[] = {

                //Top Circle 
                //Top Right Quarter
                0, 1, 15,
                1, 2, 15,
                2, 3, 15,
                3, 4, 15,
                4, 5, 15,
                5, 6, 15,
                6, 7, 15,

                //Bottom Right Quarter
                7, 8, 15,
                8, 9, 15,
                9, 10, 15,
                10, 11, 15,
                11, 12, 15,
                12, 13, 15,
                13, 14, 15,

                //Bottom Circle 
                //Top Right Quarter
                16, 17, 31,
                17, 18, 31,
                18, 19, 31,
                19, 20, 31,
                20, 21, 31,
                21, 22, 31,
                22, 23, 31,

                //Bottom Right Quarter
                23, 24, 31,
                24, 25, 31,
                25, 26, 31,
                26, 27, 31,
                27, 28, 31,
                28, 29, 31,
                29, 30, 31,

                
                //Cylinder Sides
                0, 16, 1,
                16, 17, 1,

                1, 17, 2,
                17, 18, 2,
                2,	18,	3,

                18,	19,	3,
                3,	19,	4,

                19,	20,	4,
                4,	20,	5,

                20,	21,	5,
                5,	21, 6,

                21,	22,	6,
                6,	22,	7,

                22,	23,	7,
                7,	23,	8,

                23,	24,	8,
                8,	24,	9,

                24,	25, 9,
                9,	25,	10,

                25,	26,	10,
                10, 26, 11,

                26,	27,	11,
                11,	27,	12,

                27,	28,	12,
                12,	28,	13,

                28,	29,	13,
                13,	29,	14,
                
                30,	0,	14,
                0, 16, 30,

                30, 29, 14
                
            };

            const GLuint floatsPerTexture = 2;
            const GLuint floatsPerVertex = 3;


            glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
            glBindVertexArray(mesh.vao);

            // Create 2 buffers: first one for the vertex data; second one for the indices
            glGenBuffers(2, mesh.vbos);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

            mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            // Strides between vertex coordinates is 5 (x, y, z, tX, tY). A tightly packed stride is 0.
            GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

            // Create Vertex Attribute Pointers
            glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
            glEnableVertexAttribArray(0);


            glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
            glEnableVertexAttribArray(2);
}

    else if (meshChoice == 3) {
       //**********************************************************************
       //Coffee Cup Handle Inside
       //**********************************************************************
       //Currently using half cylinder mesh scaled and rotated for handle
       //Will eventually try to use a torus once I understand how to render one

        // Position and Color data
        GLfloat verts[] = {

            //Top Circle
            // Vertex Positions    // Colors (r,g,b,a)
            // Top Right Quarter
             0.0f,  0.5f, 0.0f,   1.0f, 1.0f, // Top Mid Circle Vertex 0

             0.1f,  0.49f, 0.0f,    1.0f, 1.0f, // 1
             0.2f,  0.45f, 0.0f,    1.0f, 1.0f, // 2
             0.3f,  0.4f, 0.0f,    1.0f, 1.0f,// 3
             0.4f,  0.3f, 0.0f,    1.0f, 1.0f,// 4
             0.46f,  0.2f, 0.0f,    1.0f, 1.0f,// 5
             0.49f,  0.1f, 0.0f,    1.0f, 1.0f,// 6

             0.5f,  0.0f, 0.0f,    1.0f, 1.0f,// 7 Right Mid Circle Vertex

             // Bottom Right Quarter
             0.49f,  -0.1f, 0.0f,    1.0f, 1.0f,// 8
             0.45f,  -0.2f, 0.0f,    1.0f, 1.0f,// 9
             0.4f,  -0.3f, 0.0f,    1.0f, 1.0f,// 10
             0.3f,  -0.4f, 0.0f,    1.0f, 1.0f,// 11
             0.2f,  -0.46f, 0.0f,   1.0f, 1.0f,// 12
             0.1f,  -0.49f, 0.0f,   1.0f, 1.0f,// 13

             0.0f,  -0.5f, 0.0f,    1.0f, 1.0f,// 14 Bottom Mid Circle Vertex                  

             0.0f,  0.0f, 0.0f,     1.0f, 1.0f,// Circle Center 15

            //Bottom Circle
            // Vertex Positions    // Colors (r,g,b,a)
            // Top Right Quarter
             0.0f,  0.5f, 1.0f,   1.0f, 1.0f,// Top Mid Circle Vertex 16

             0.1f,  0.49f, 1.0f,   1.0f, 1.0f,// 17
             0.2f,  0.45f, 1.0f,   1.0f, 1.0f,// 18
             0.3f,  0.4f, 1.0f,    1.0f, 1.0f,// 19
             0.4f,  0.3f, 1.0f,   1.0f, 1.0f,// 20
             0.46f,  0.2f, 1.0f,   1.0f, 1.0f,// 21
             0.49f,  0.1f, 1.0f,   1.0f, 1.0f,// 22

             0.5f,  0.0f, 1.0f,    1.0f, 1.0f,// 23 Right Mid Circle Vertex

             // Bottom Right Quarter
             0.49f,  -0.1f, 1.0f,    1.0f, 1.0f,// 24
             0.45f,  -0.2f, 1.0f,   1.0f, 1.0f,// 25
             0.4f,  -0.3f, 1.0f,   1.0f, 1.0f,// 26
             0.3f,  -0.4f, 1.0f,    1.0f, 1.0f,// 27
             0.2f,  -0.46f, 1.0f,    1.0f, 1.0f,// 28
             0.1f,  -0.49f, 1.0f,    1.0f, 1.0f,// 29

             0.0f,  -0.5f, 1.0f,    1.0f, 1.0f,// 30 Bottom Mid Circle Vertex                

             0.0f,  0.0f, 1.0f,    1.0f, 1.0f,// Circle Center 31
        };

        // Index data to share position data
        GLushort indices[] = {

            //Top Circle 
            //Top Right Quarter
            0, 1, 15,
            1, 2, 15,
            2, 3, 15,
            3, 4, 15,
            4, 5, 15,
            5, 6, 15,
            6, 7, 15,

            //Bottom Right Quarter
            7, 8, 15,
            8, 9, 15,
            9, 10, 15,
            10, 11, 15,
            11, 12, 15,
            12, 13, 15,
            13, 14, 15,

            //Bottom Circle 
            //Top Right Quarter
            16, 17, 31,
            17, 18, 31,
            18, 19, 31,
            19, 20, 31,
            20, 21, 31,
            21, 22, 31,
            22, 23, 31,

            //Bottom Right Quarter
            23, 24, 31,
            24, 25, 31,
            25, 26, 31,
            26, 27, 31,
            27, 28, 31,
            28, 29, 31,
            29, 30, 31,


            //Cylinder Sides
            0, 16, 1,
            16, 17, 1,

            1, 17, 2,
            17, 18, 2,
            2,	18,	3,

            18,	19,	3,
            3,	19,	4,

            19,	20,	4,
            4,	20,	5,

            20,	21,	5,
            5,	21, 6,

            21,	22,	6,
            6,	22,	7,

            22,	23,	7,
            7,	23,	8,

            23,	24,	8,
            8,	24,	9,

            24,	25, 9,
            9,	25,	10,

            25,	26,	10,
            10, 26, 11,

            26,	27,	11,
            11,	27,	12,

            27,	28,	12,
            12,	28,	13,

            28,	29,	13,
            13,	29,	14,

            30,	0,	14,
            0, 16, 30,

            30, 29, 14

        };

        const GLuint floatsPerTexture = 2;
        const GLuint floatsPerVertex = 3;


        glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
        glBindVertexArray(mesh.vao);

        // Create 2 buffers: first one for the vertex data; second one for the indices
        glGenBuffers(2, mesh.vbos);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

        mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Strides between vertex coordinates is 5 (x, y, z, tX, tY). A tightly packed stride is 0.
        GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

        // Create Vertex Attribute Pointers
        glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
        glEnableVertexAttribArray(0);


        glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
        glEnableVertexAttribArray(2);
}

    else if (meshChoice == 4) {
    //**********************************************************************
    //Coffee Cup Handle Outside
    //**********************************************************************
    //Currently using half cylinder mesh scaled and rotated for handle
    //Will eventually try to use a torus once I understand how to render one

     // Position and Color data
     GLfloat verts[] = {

         //Top Circle
         // Vertex Positions    // Colors (r,g,b,a)
         // Top Right Quarter
          0.0f,  0.5f, 0.0f,   1.0f, 1.0f, // Top Mid Circle Vertex 0

          0.1f,  0.49f, 0.0f,    1.0f, 1.0f, // 1
          0.2f,  0.45f, 0.0f,    1.0f, 1.0f, // 2
          0.3f,  0.4f, 0.0f,    1.0f, 1.0f,// 3
          0.4f,  0.3f, 0.0f,    1.0f, 1.0f,// 4
          0.46f,  0.2f, 0.0f,    1.0f, 1.0f,// 5
          0.49f,  0.1f, 0.0f,    1.0f, 1.0f,// 6

          0.5f,  0.0f, 0.0f,    1.0f, 1.0f,// 7 Right Mid Circle Vertex

          // Bottom Right Quarter
          0.49f,  -0.1f, 0.0f,    1.0f, 1.0f,// 8
          0.45f,  -0.2f, 0.0f,    1.0f, 1.0f,// 9
          0.4f,  -0.3f, 0.0f,    1.0f, 1.0f,// 10
          0.3f,  -0.4f, 0.0f,    1.0f, 1.0f,// 11
          0.2f,  -0.46f, 0.0f,   1.0f, 1.0f,// 12
          0.1f,  -0.49f, 0.0f,   1.0f, 1.0f,// 13

          0.0f,  -0.5f, 0.0f,    1.0f, 1.0f,// 14 Bottom Mid Circle Vertex                  

          0.0f,  0.0f, 0.0f,     1.0f, 1.0f,// Circle Center 15

         //Bottom Circle
         // Vertex Positions    // Colors (r,g,b,a)
         // Top Right Quarter
          0.0f,  0.5f, 1.0f,   1.0f, 1.0f,// Top Mid Circle Vertex 16

          0.1f,  0.49f, 1.0f,   1.0f, 1.0f,// 17
          0.2f,  0.45f, 1.0f,   1.0f, 1.0f,// 18
          0.3f,  0.4f, 1.0f,    1.0f, 1.0f,// 19
          0.4f,  0.3f, 1.0f,   1.0f, 1.0f,// 20
          0.46f,  0.2f, 1.0f,   1.0f, 1.0f,// 21
          0.49f,  0.1f, 1.0f,   1.0f, 1.0f,// 22

          0.5f,  0.0f, 1.0f,    1.0f, 1.0f,// 23 Right Mid Circle Vertex

          // Bottom Right Quarter
          0.49f,  -0.1f, 1.0f,    1.0f, 1.0f,// 24
          0.45f,  -0.2f, 1.0f,   1.0f, 1.0f,// 25
          0.4f,  -0.3f, 1.0f,   1.0f, 1.0f,// 26
          0.3f,  -0.4f, 1.0f,    1.0f, 1.0f,// 27
          0.2f,  -0.46f, 1.0f,    1.0f, 1.0f,// 28
          0.1f,  -0.49f, 1.0f,    1.0f, 1.0f,// 29

          0.0f,  -0.5f, 1.0f,    1.0f, 1.0f,// 30 Bottom Mid Circle Vertex                

          0.0f,  0.0f, 1.0f,    1.0f, 1.0f,// Circle Center 31
     };

     // Index data to share position data
     GLushort indices[] = {

         /*//Top Circle 
         //Top Right Quarter
         0, 1, 15,
         1, 2, 15,
         2, 3, 15,
         3, 4, 15,
         4, 5, 15,
         5, 6, 15,
         6, 7, 15,

         //Bottom Right Quarter
         7, 8, 15,
         8, 9, 15,
         9, 10, 15,
         10, 11, 15,
         11, 12, 15,
         12, 13, 15,
         13, 14, 15,

         //Bottom Circle 
         //Top Right Quarter
         16, 17, 31,
         17, 18, 31,
         18, 19, 31,
         19, 20, 31,
         20, 21, 31,
         21, 22, 31,
         22, 23, 31,

         //Bottom Right Quarter
         23, 24, 31,
         24, 25, 31,
         25, 26, 31,
         26, 27, 31,
         27, 28, 31,
         28, 29, 31,
         29, 30, 31,*/


         //Cylinder Sides
         0, 16, 1,
         16, 17, 1,

         1, 17, 2,
         17, 18, 2,
         2,	18,	3,

         18,	19,	3,
         3,	19,	4,

         19,	20,	4,
         4,	20,	5,

         20,	21,	5,
         5,	21, 6,

         21,	22,	6,
         6,	22,	7,

         22,	23,	7,
         7,	23,	8,

         23,	24,	8,
         8,	24,	9,

         24,	25, 9,
         9,	25,	10,

         25,	26,	10,
         10, 26, 11,

         26,	27,	11,
         11,	27,	12,

         27,	28,	12,
         12,	28,	13,

         28,	29,	13,
         13,	29,	14,

         30,	0,	14,
         0, 16, 30,

         30, 29, 14

     };

     const GLuint floatsPerTexture = 2;
     const GLuint floatsPerVertex = 3;


     glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
     glBindVertexArray(mesh.vao);

     // Create 2 buffers: first one for the vertex data; second one for the indices
     glGenBuffers(2, mesh.vbos);
     glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
     glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

     mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

     // Strides between vertex coordinates is 5 (x, y, z, tX, tY). A tightly packed stride is 0.
     GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

     // Create Vertex Attribute Pointers
     glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
     glEnableVertexAttribArray(0);


     glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
     glEnableVertexAttribArray(2);
    }

    else if (meshChoice == 5) {
    //**********************************************************************
    //Coffee Cup Texture Top
    //**********************************************************************

    // Position and Color data
    GLfloat verts[] = {

        //Top Circle
        // Vertex Positions    // Colors (r,g,b,a) 
        // Top Right Quarter                        //Normals            
         0.0f,  0.5f, 0.0f,     0.5f, 1.0f,         0.0f, 1.0f,  0.0f,                    // Top Mid Circle Vertex 0
                                                    
         0.1f,  0.49f, 0.0f,    0.6f, 0.99f,         0.0f, 1.0f,  0.0f,                    // 1
         0.2f,  0.45f, 0.0f,    0.7f, 0.95f,         0.0f, 1.0f,  0.0f,                    // 2
         0.3f,  0.4f, 0.0f,     0.8f, 0.9f,          0.0f, 1.0f,  0.0f,                   // 3
         0.4f,  0.3f, 0.0f,     0.9f, 0.8f,          0.0f, 1.0f,  0.0f,                   // 4
         0.46f,  0.2f, 0.0f,    0.96f, 0.7f,         0.0f, 1.0f,  0.0f,                    // 5
         0.49f,  0.1f, 0.0f,    0.99f, 0.6f,         0.0f, 1.0f,  0.0f,                    // 6

         0.5f,  0.0f, 0.0f,     1.0f, 0.5f,       0.0f, 1.0f,  0.0f,                      // 7 Right Mid Circle Vertex

         // Bottom Right Quarter
         0.49f,  -0.1f, 0.0f,   0.99f, 0.4f,      0.0f, 1.0f,  0.0f,                       // 8
         0.45f,  -0.2f, 0.0f,   0.95f, 0.3f,      0.0f, 1.0f,  0.0f,                       // 9
         0.4f,  -0.3f, 0.0f,    0.9f, 0.2f,       0.0f, 1.0f,  0.0f,                      // 10
         0.3f,  -0.4f, 0.0f,    0.8f, 0.1f,       0.0f, 1.0f,  0.0f,                      // 11
         0.2f,  -0.46f, 0.0f,   0.7f, 0.04f,      0.0f, 1.0f,  0.0f,                       // 12
         0.1f,  -0.49f, 0.0f,   0.6f, 0.01f,      0.0f, 1.0f,  0.0f,                       // 13

         0.0f,  -0.5f, 0.0f,    0.5f, 0.0f,      0.0f, 1.0f,  0.0f,                       // 14 Bottom Mid Circle Vertex

         // Bottom Left Quarter
         -0.1f,  -0.49f, 0.0f,  0.4f, 0.01f,    0.0f, 1.0f,  0.0f,                         // 15
         -0.2f,  -0.45f, 0.0f,  0.3f, 0.05f,    0.0f, 1.0f,  0.0f,                         // 16
         -0.3f,  -0.4f, 0.0f,   0.2f, 0.1f,     0.0f, 1.0f,  0.0f,                        // 17
         -0.4f,  -0.3f, 0.0f,   0.1f, 0.2f,     0.0f, 1.0f,  0.0f,                        // 18
         -0.46f, -0.2f, 0.0f,   0.04f, 0.3f,    0.0f, 1.0f,  0.0f,                         // 19
         -0.49f, -0.1f, 0.0f,   0.01f, 0.4f,    0.0f, 1.0f,  0.0f,                         // 20

         -0.5f,  -0.0f, 0.0f,   0.0f, 0.5f,     0.0f, 1.0f,  0.0f,                        // 21 Left Mid Circle Vertex

         // Top Right Quarter
         -0.49f, 0.1f, 0.0f,    0.01f, 0.6f,    0.0f, 1.0f,  0.0f,                         // 22
         -0.45f, 0.2f, 0.0f,    0.05f, 0.7f,    0.0f, 1.0f,  0.0f,                         // 23
         -0.4f,  0.3f, 0.0f,    0.1f, 0.8f,     0.0f, 1.0f,  0.0f,                        // 24
         -0.3f,  0.4f, 0.0f,    0.2f, 0.9f,     0.0f, 1.0f,  0.0f,                        // 25
         -0.2f,  0.46f, 0.0f,   0.3f, 0.96f,    0.0f, 1.0f,  0.0f,                         // 26
         -0.1f,  0.49f, 0.0f,   0.4f, 0.99f,    0.0f, 1.0f,  0.0f,                         // 27

          0.0f,  0.0f, 0.0f,     0.5f, 0.5f,    0.0f, 1.0f,  0.0f,                       // Circle Center 28
    };

    // Index data to share position data
    GLushort indices[] = {
        //**********************************************************************
        //Coffee Cup Body
        //**********************************************************************
        //Top Circle 
        //Top Right Quarter
        0, 1, 28,
        1, 2, 28,
        2, 3, 28,
        3, 4, 28,
        4, 5, 28,
        5, 6, 28,
        6, 7, 28,

        //Bottom Right Quarter
        7, 8, 28,
        8, 9, 28,
        9, 10, 28,
        10, 11, 28,
        11, 12, 28,
        12, 13, 28,
        13, 14, 28,

        //Bottom Left Quarter
        14, 15, 28,
        15, 16, 28,
        16, 17, 28,
        17, 18, 28,
        18, 19, 28,
        19, 20, 28,
        20, 21, 28,

        //Top Right Quarter
        21, 22, 28,
        22, 23, 28,
        23, 24, 28,
        24, 25, 28,
        25, 26, 28,
        26, 27, 28,
        27, 0, 28,
    };

    const GLuint floatsPerTexture = 2;
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;


    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 8 (x, y, z, tX, tY, nX, nY, nZ). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture + floatsPerNormal);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerTexture)));
    glEnableVertexAttribArray(2);
    
    }

    else if (meshChoice == 6) {
    //**********************************************************************
    //Cube Data
    //**********************************************************************

    // Position and Color data
    GLfloat verts[] = {
        //Right Pyramid
        // Vertex Positions   // Texture Coords     //Normals
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f,           0.0f, -0.0f,  1.0f,                     // Top Right Square Vertex 0
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f,           0.0f, -0.0f,  1.0f,                     // Bottom Right Square Vertex 1
         -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,           0.0f, -0.0f,  1.0f,                     // Bottom Left Square Vertex 2
         -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,           0.0f, -0.0f,  1.0f,                     // Top Left Square Vertex 3

         0.5f,  0.5f, -1.0f,  1.0f, 1.0f,           0.0f, -0.0f,  -2.0f, // 4
         0.5f, -0.5f, -1.0f,  1.0f, 0.0f,           0.0f, -0.0f,  -2.0f, // 5
         - 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,          0.0f, -0.0f,  -2.0f, // 6
         - 0.5f,  0.5f, -1.0f, 0.0f, 1.0f,          0.0f, -0.0f,  -2.0f,  // 7

         0.5f,  0.5f, 0.0f,   1.0f, 0.0f,           0.0f, 1.0f,  -0.5f,                     // Top Right Square Vertex 8
         0.5f,  0.5f, -1.0f,  1.0f, 1.0f,           0.0f, 1.0f,  -0.5f, // 9
         -0.5f,  0.5f, -1.0f, 0.0f, 1.0f,           0.0f, 1.0f,  -0.5f,  // 10
         -0.5f,  0.5f, 0.0f,  0.0f, 0.0f,           0.0f, 1.0f,  -0.5f,                     // Top Left Square Vertex 11

         -0.5f,  0.5f, 0.0f,  1.0f, 1.0f,           -1.0f, 0.0f,  -0.5f,                     // Top Left Square Vertex 12
         -0.5f,  0.5f, -1.0f, 0.0f, 1.0f,           -1.0f, -0.0f,  -0.5f,  // 13
         -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,           -1.0f, -0.0f,  -0.5f, // 14
         -0.5f, -0.5f, 0.0f,  1.0f, 0.0f,           -1.0f, -0.0f,  -0.5f,                     // Bottom Left Square Vertex 15

         0.5f,  0.5f, 0.0f,   0.0f, 1.0f,           1.0f, -0.0f,  -0.5f,                     // Top Right Square Vertex 16
         0.5f,  0.5f, -1.0f,  1.0f, 1.0f,           1.0f, -0.0f,  -0.5f, // 17
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f,           1.0f, -0.0f,  -0.5f,                     // Bottom Right Square Vertex 18
         0.5f, -0.5f, -1.0f,  1.0f, 0.0f,           1.0f, -0.0f,  -0.5f, // 19
    };

    // Index data to share position data
    GLushort indices[] = {

        //Square Base
        0, 1, 3,  // Triangle 1
        1, 2, 3,   // Triangle 2

        4, 5, 7,
        5, 6, 7,

        16, 17, 18,
        17, 19, 18,

        12, 13, 14,
        14, 15, 12,

        8, 9, 10,
        10, 11, 8,

        

    };

    const GLuint floatsPerTexture = 2;
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;


    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 8 (x, y, z, tX, tY, nX, nY, nZ). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture + floatsPerNormal);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerTexture)));
    glEnableVertexAttribArray(2);
    }

    else if (meshChoice == 7) {
    //**********************************************************************
    //Full Cylinder
    //**********************************************************************

    // Position and Color data
    GLfloat verts[] = {

        //Top Circle
        // Vertex Positions    
        // Top Right Quarter
         0.0f,  0.5f, 0.0f,      0.0f,  1.0f,     0.0f, 1.0f, -0.5f,                    // Top Mid Circle Vertex 0

         0.1f,  0.49f, 0.0f,     0.03f, 1.0f,     0.2f, 0.98f, -0.5f,                   // 1
         0.2f,  0.45f, 0.0f,     0.06f, 1.0f,      0.4f, 0.92f, -0.5f,                 // 2
         0.3f,  0.4f, 0.0f,      0.12f, 1.0f,      0.6f, 0.8f, -0.5f,                   // 3
         0.4f,  0.3f, 0.0f,      0.15f, 1.0f,      0.8f, 0.6f, -0.5f,                  // 4
         0.46f,  0.2f, 0.0f,     0.18f, 1.0f,       0.90f, 0.43f, -0.5f,                // 5
         0.49f,  0.1f, 0.0f,     0.21f, 1.0f,      0.98f, 0.2f, -0.5f,                   // 6

         0.5f,  0.0f, 0.0f,      0.25f, 1.0f,      1.0f, 0.0f, -0.5f,                  // 7 Right Mid Circle Vertex

         // Bottom Right Quarter
         0.49f,  -0.1f, 0.0f,    0.28f, 1.0f,      0.98f, -0.2f, -0.5f,                 // 8
         0.45f,  -0.2f, 0.0f,    0.31f, 1.0f,      0.92f, -0.4f, -0.5f,                 // 9
         0.4f,  -0.3f, 0.0f,     0.34f, 1.0f,      0.8f, -0.6f, -0.5f,                  // 10
         0.3f,  -0.4f, 0.0f,     0.37f, 1.0f,      0.6f, -0.8f, -0.5f,                  // 11
         0.2f,  -0.46f, 0.0f,    0.40f, 1.0f,      0.4f, -0.92f, -0.5f,                 // 12
         0.1f,  -0.49f, 0.0f,    0.43f, 1.0f,      0.2f, -0.98f, -0.5f,                 // 13

         0.0f,  -0.5f, 0.0f,     0.5f, 1.0f,       0.0f, -1.0f, -0.5f,                 // 14 Bottom Mid Circle Vertex

         // Bottom Left Quarter
         -0.1f,  -0.49f, 0.0f,   0.53f, 1.0f,      -0.2f, -0.98f, -0.5f,                  // 15
         -0.2f,  -0.45f, 0.0f,   0.56f, 1.0f,       -0.4f, -0.92f, -0.5f,                 // 16
         -0.3f,  -0.4f, 0.0f,    0.59f, 1.0f,       -0.6f, -0.8f, -0.5f,                  // 17
         -0.4f,  -0.3f, 0.0f,    0.63f, 1.0f,       -0.8f, -0.6f, -0.5f,                  // 18
         -0.46f, -0.2f, 0.0f,    0.66f, 1.0f,        -0.90f, -0.43f, -0.5f,               // 19
         -0.49f, -0.1f, 0.0f,    0.70f, 1.0f,       -0.98f, -0.2f, -0.5f,                 // 20

         -0.5f,  -0.0f, 0.0f,    0.75f, 1.0f,       -1.0f, 0.0f, -0.5f,                  // 21 Left Mid Circle Vertex
         // Top Right Quarter
         -0.49f, 0.1f, 0.0f,     0.78f, 1.0f,      -0.98f, 0.2f, -0.5f,                 // 22
         -0.45f, 0.2f, 0.0f,     0.81f, 1.0f,      -0.92f, 0.4f, -0.5f,                 // 23
         -0.4f,  0.3f, 0.0f,     0.84f, 1.0f,      -0.8f, 0.6f, -0.5f,                  // 24
         -0.3f,  0.4f, 0.0f,     0.87f, 1.0f,      -0.6f, 0.8f, -0.5f,                  // 25
         -0.2f,  0.46f, 0.0f,    0.91f, 1.0f,      -0.4f, 0.92f, -0.5f,                 // 26
         -0.1f,  0.49f, 0.0f,    0.93f, 1.0f,      -0.2f, 0.98f, -0.5f,                 // 27

          0.0f,  0.0f, 0.0f,     1.0f, 1.0f,       -0.5f, 1.0f, -0.5f,          // Circle Center 28

        //Bottom Circle
        // Vertex Positions    
        // Top Right Quarter
         0.0f,  0.5f, 1.0f,     0.0f, 0.0f,  0.0f, 1.0f, -0.5f,            // Top Mid Circle Vertex 29

         0.1f,  0.49f, 1.0f,    0.03f, 0.0f, 0.2f, 0.98f, -0.5f,           // 30
         0.2f,  0.45f, 1.0f,    0.06f, 0.0f,  0.4f, 0.92f, -0.5f,          // 31
         0.3f,  0.4f, 1.0f,     0.12f, 0.0f,  0.6f, 0.8f, -0.5f,           // 32
         0.4f,  0.3f, 1.0f,     0.15f, 0.0f,  0.8f, 0.6f, -0.5f,           // 33
         0.46f,  0.2f, 1.0f,    0.18f, 0.0f,   0.90f, 0.43f, -0.5f,        // 34
         0.49f,  0.1f, 1.0f,    0.21f, 0.0f,  0.98f, 0.2f, -0.5f,          // 35

         0.5f,  0.0f, 1.0f,     0.25f, 0.0f,  1.0f, 0.0f, -0.5f,          //36 Right Mid Circle Vertex

         // Bottom Right Quarter
         0.49f,  -0.1f, 1.0f,   0.28f, 0.0f,  0.98f, -0.2f, -0.5f,          // 37
         0.45f,  -0.2f, 1.0f,   0.31f, 0.0f,  0.92f, -0.4f, -0.5f,          // 38
         0.4f,  -0.3f, 1.0f,    0.34f, 0.0f,  0.8f, -0.6f, -0.5f,           // 39
         0.3f,  -0.4f, 1.0f,    0.37f, 0.0f,  0.6f, -0.8f, -0.5f,           // 40
         0.2f,  -0.46f, 1.0f,   0.40f, 0.0f,  0.4f, -0.92f, -0.5f,          // 41
         0.1f,  -0.49f, 1.0f,   0.43f, 0.0f,  0.2f, -0.98f, -0.5f,          // 42

         0.0f,  -0.5f, 1.0f,    0.5f, 0.0f,   0.0f, -1.0f, -0.5f,            //43 Bottom Mid Circle Vertex

         // Bottom Left Quarter
         -0.1f,  -0.49f, 1.0f,  0.53f, 0.0f,   -0.2f, -0.98f, -0.5f,             //44
         -0.2f,  -0.45f, 1.0f,  0.56f, 0.0f,    -0.4f, -0.92f, -0.5f,            //45
         -0.3f,  -0.4f, 1.0f,   0.59f, 0.0f,    -0.6f, -0.8f, -0.5f,             //46
         -0.4f,  -0.3f, 1.0f,   0.63f, 0.0f,    -0.8f, -0.6f, -0.5f,              //47
         -0.46f, -0.2f, 1.0f,   0.66f, 0.0f,     -0.90f, -0.43f, -0.5f,          //48
         -0.49f, -0.1f, 1.0f,   0.70f, 0.0f,    -0.98f, -0.2f, -0.5f,           // 49

         -0.5f,  -0.0f, 1.0f,   0.75f, 0.0f,    -1.0f, 0.0f, -0.5f,              //50 Left Mid Circle Vertex

         // Top Right Quarter
         -0.49f, 0.1f, 1.0f,    0.78f, 0.0f,  -0.98f, 0.2f, -0.5f,             // 51
         -0.45f, 0.2f, 1.0f,    0.81f, 0.0f,  -0.92f, 0.4f, -0.5f,             //52
         -0.4f,  0.3f, 1.0f,    0.84f, 0.0f,  -0.8f, 0.6f, -0.5f,             // 53
         -0.3f,  0.4f, 1.0f,    0.87f, 0.0f,  -0.6f, 0.8f, -0.5f,              //54
         -0.2f,  0.46f, 1.0f,   0.91f, 0.0f,  -0.4f, 0.92f, -0.5f,             //55
         -0.1f,  0.49f, 1.0f,   0.93f, 0.0f,  -0.2f, 0.98f, -0.5f,             //56

         0.0f,  0.0f, 1.0f,      1.0f, 1.0f,  0.0f, 0.0f, 0.0f,              // Circle Center 57

         0.0f,  0.5f, 0.0f,      1.0f,  1.0f,  0.0f, 0.0f, 0.0f,          // Top Mid Circle Vertex 58
         0.0f,  0.5f, 1.0f,      1.0f,  0.0f,   0.0f, 0.0f, 0.0f,          // bottom Mid Circle Vertex 59

    };

    // Index data to share position data
    GLushort indices[] = {
        //**********************************************************************
        //Coffee Cup Body
        //**********************************************************************

        //Top Circle 
        //Top Right Quarter
        0, 1, 28,
        1, 2, 28,
        2, 3, 28,
        3, 4, 28,
        4, 5, 28,
        5, 6, 28,
        6, 7, 28,

        //Bottom Right Quarter
        7, 8, 28,
        8, 9, 28,
        9, 10, 28,
        10, 11, 28,
        11, 12, 28,
        12, 13, 28,
        13, 14, 28,

        //Bottom Left Quarter
        14, 15, 28,
        15, 16, 28,
        16, 17, 28,
        17, 18, 28,
        18, 19, 28,
        19, 20, 28,
        20, 21, 28,

        //Top Right Quarter
        21, 22, 28,
        22, 23, 28,
        23, 24, 28,
        24, 25, 28,
        25, 26, 28,
        26, 27, 28,
        27, 0, 28,

        //Bottom Circle 
        //Top Right Quarter
        29, 30, 57,
        30, 31, 57,
        31, 32, 57,
        32, 33, 57,
        33, 34, 57,
        34, 35, 57,
        35, 36, 57,

        //Bottom Right Quarter
        36, 37, 57,
        37, 38, 57,
        38, 39, 57,
        39, 40, 57,
        40, 41, 57,
        41, 42, 57,
        42, 43, 57,

        //Bottom Left Quarter
        43, 44, 57,
        44, 45, 57,
        45, 46, 57,
        46, 47, 57,
        47, 48, 57,
        48, 49, 57,
        49, 50, 57,

        //Top Right Quarter
        50, 51, 57,
        51, 52, 57,
        52, 53, 57,
        53, 54, 57,
        54, 55, 57,
        55, 56, 57,
        56, 29, 57,

        //Cylinder Sides
        0, 29, 30,
        30, 1, 0,

        1, 30, 31,
        31, 2, 1,

        2, 31, 32,
        32, 3, 2,

        3, 32, 33,
        33, 4, 3,

        4, 33, 34,
        34, 5, 4,

        5, 34, 35,
        35, 6, 5,

        6, 35, 36,
        36, 7, 6,

        7, 36, 37,
        37, 8, 7,

        8, 37, 38,
        38, 9, 8,

        9, 38, 39,
        39, 10, 9,

        10, 39, 40,
        40, 11, 10,

        11, 40, 41,
        41, 12, 11,

        12, 41, 42,
        42, 13, 12,

        13, 42, 43,
        43, 14, 13,

        14, 43, 44,
        44, 15, 14,

        15, 44, 45,
        45, 16, 15,

        16, 45, 46,
        46, 17, 16,

        17, 46, 47,
        47, 18, 17,

        18, 47, 48,
        48, 19, 18,

        19, 48, 49,
        49, 20, 19,

        20, 49, 50,
        50, 21, 20,

        21, 50, 51,
        51, 22, 21,

        22, 51, 52,
        52, 23, 22,

        23, 52, 53,
        53, 24, 23,

        24, 53, 54,
        54, 25, 24,

        25, 54, 55,
        55, 26, 25,

        26, 55, 56,
        56, 27, 26,

        27, 58, 59,
        27, 56, 59
    };

    const GLuint floatsPerTexture = 2;
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;


    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 8 (x, y, z, tX, tY, nX, nY, nZ). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture + floatsPerNormal);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerTexture)));
    glEnableVertexAttribArray(2);

    }
}



//**********************************************************
//DESTROY MESH
//**********************************************************
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

//**********************************************************
//CREATE TEXTURE
//**********************************************************
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

//**********************************************************
//DESTROY TEXTURE
//**********************************************************
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

//********************************************************************
//SHADER IMPLEMENTATION
//
// Implements the UCreateShaders function
//********************************************************************
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}
//Destroy shader program
void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}


