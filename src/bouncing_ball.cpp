// Based on example code from: Interactive Computer Graphics: A Top-Down Approach with Shader-Based OpenGL (6th Edition), by Ed Angel


#ifdef USE_GLEW
    #include <GL/glew.h>
#else
    #define GLFW_INCLUDE_GLCOREARB
#endif


#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <sstream>

// This file contains the code that reads the shaders from their files and compiles them
#include "shader.hpp"
// This file contains helper classes and functions for rendering
#include "helper.hpp"

#define DEBUG 0

#define MAXPARTICLES 300000
#define MAXSIZE 100

#define GRAVITY 9.8
#define USINGBUBBLES 0

#define WIN_WIDTH 800
#define WIN_HEIGHT 800

using std::cout;
using std::endl;
using std::min;
using std::max;
using std::string;


//----------------------------------------------------------------------------

// initialize some basic structures and geometry
typedef struct {
	bool active = false;
	GLdouble prev_x;
	GLdouble prev_y;
} MouseInfo;

typedef struct {
	float colorStartRange[2][4];
	float colorEndRange[2][4];
	float colorSpeedRange[2];
	float lifetimeRange[2];
	float sizeRange[2];
	float blurRange[2];
	Vec3f velocityRange[2];
	float lighting;
	int force;
} Particle;

typedef struct {
	string name;
	double sizeX;
	double sizeY;
	double sizeZ;
} Shape;

typedef struct {
	Shape shape;
	Particle properties;
	double genRate;
	Vec3f position;
	Vec3f velocity;
	Vec3f direction;
} Emitter;

// some assorted global variables, defined as such to make life easier
GLuint 	vao,
		vbo_verts,
		vbo_colors,
		vbo_lightings,
		vbo_sizes,
		vbo_blurs;

Vec3f 	lightDir = {1, -1, 1},
		lightAmb = {.2, .2, .2},
		lightCol = {1.0, 1.0, 1.0};

mcl::Shader currentShader;

int numParticles = 0;

MouseInfo mouse;
bool paused = false, addTimeMultiplier = false, subTimeMultiplier = false;
double timeMultiplier = 1.0;

GLFWcursor *hand_cursor, *arrow_cursor;

//----------------------------------------------------------------------------

// Particle info (can change per simulation)
Vec3f particles[MAXPARTICLES];
float colors[MAXPARTICLES][4];
float lightings[MAXPARTICLES];
float sizes[MAXPARTICLES];
float blurs[MAXPARTICLES];

Vec3f velocities[MAXPARTICLES];
float colorChanges[MAXPARTICLES][4];
float colorSpeeds[MAXPARTICLES];
double lifetimes[MAXPARTICLES];
double lifeLimits[MAXPARTICLES];
int forces[MAXPARTICLES];
bool grounded[MAXPARTICLES];

//----------------------------------------------------------------------------
// function that is called whenever an error occurs
static void
error_callback(int error, const char* description){
    fputs(description, stderr);  // write the error description to stderr
}

//----------------------------------------------------------------------------
// function that is called whenever a keyboard event occurs; defines how keyboard input will be handled
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if( action == GLFW_PRESS ) {
		switch ( key ) {
			// Close on escape
			case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;

			// Movement keys trigger booleans to be processed during the graphics loop
			// Forward movement
			case GLFW_KEY_UP: Globals::key_up = true; break;
			case GLFW_KEY_W: Globals::key_w = true; break;

			// Backward movement
			case GLFW_KEY_DOWN: Globals::key_down = true; break;
			case GLFW_KEY_S: Globals::key_s = true; break;

			// Right strafing movement
			case GLFW_KEY_RIGHT: Globals::key_right = true; break;
			case GLFW_KEY_D: Globals::key_d = true; break;

			// Left strafing movement
			case GLFW_KEY_LEFT: Globals::key_left = true; break;
			case GLFW_KEY_A: Globals::key_a = true; break;

			// Upward movement
			case GLFW_KEY_RIGHT_SHIFT: Globals::key_rshift = true; break;
			case GLFW_KEY_E: Globals::key_e = true; break;

			// Downward movement
			case GLFW_KEY_KP_0: Globals::key_0 = true; break;
			case GLFW_KEY_Q: Globals::key_q = true; break;

			// Speed up
			case GLFW_KEY_RIGHT_CONTROL: Globals::key_rcontrol = true; break;
			case GLFW_KEY_LEFT_SHIFT: Globals::key_lshift = true; break;

			// Pause
			case GLFW_KEY_SPACE: paused = !paused; break;

			// Release mouse
			case GLFW_KEY_KP_ENTER: glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); break;

			// Increase/decrease game speed
			case GLFW_KEY_MINUS: subTimeMultiplier = true; break;
			case GLFW_KEY_EQUAL: addTimeMultiplier = true; break;
			case GLFW_KEY_BACKSPACE: timeMultiplier = 1.0; 
			
		}
	}
	else if ( action == GLFW_RELEASE ) {
		switch ( key ) {
			// Movement keys trigger booleans to be processed during the graphics loop
			// Forward movement
			case GLFW_KEY_UP: Globals::key_up = false; break;
			case GLFW_KEY_W: Globals::key_w = false; break;

			// Backward movement
			case GLFW_KEY_DOWN: Globals::key_down = false; break;
			case GLFW_KEY_S: Globals::key_s = false; break;

			// Right strafing movement
			case GLFW_KEY_RIGHT: Globals::key_right = false; break;
			case GLFW_KEY_D: Globals::key_d = false; break;

			// Left strafing movement
			case GLFW_KEY_LEFT: Globals::key_left = false; break;
			case GLFW_KEY_A: Globals::key_a = false; break;

			// Upward movement
			case GLFW_KEY_RIGHT_SHIFT: Globals::key_rshift = false; break;
			case GLFW_KEY_E: Globals::key_e = false; break;

			// Downward movement
			case GLFW_KEY_KP_0: Globals::key_0 = false; break;
			case GLFW_KEY_Q: Globals::key_q = false; break;

			// Speed up
			case GLFW_KEY_RIGHT_CONTROL: Globals::key_rcontrol = false; break;
			case GLFW_KEY_LEFT_SHIFT: Globals::key_lshift = false; break;

			// Increase/decrease game speed
			case GLFW_KEY_MINUS: subTimeMultiplier = false; break;
			case GLFW_KEY_EQUAL: addTimeMultiplier = false;
		}
	}
}

//----------------------------------------------------------------------------
// function that is called whenever a mouse or trackpad button press event occurs
static void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwGetCursorPos(window, &mouse.prev_x, &mouse.prev_y);
		mouse.active = true;
	}
}

//----------------------------------------------------------------------------
// function that is called whenever a cursor motion event occurs
static void
cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!mouse.active)
		return;

	Vec3f view_dir = Vec3f(0.0, 0.0, 1.0);
	Vec3f up_dir = Vec3f(0.0, 1.0, 0.0);
	
	if (xpos != mouse.prev_x) {	
		Globals::theta -= 0.2*(xpos - mouse.prev_x);
		Globals::y_rot = rotateY(Globals::theta);
		mouse.prev_x = xpos;
		glUniform1f(currentShader.uniform("theta"), Globals::theta*PI/180.0);
	}

	if (ypos != mouse.prev_y) {
		Globals::phi += 0.2*(ypos - mouse.prev_y);
		if (Globals::phi > 89)
			Globals::phi = 89;
		else if (Globals::phi < -89)
			Globals::phi = -89;
		Globals::x_rot = rotateX(Globals::phi);
		mouse.prev_y = ypos;
		glUniform1f(currentShader.uniform("phi"), Globals::phi*PI/180.0);
	}
	
	view_dir = Globals::x_rot*view_dir;
	Globals::view_dir = Globals::y_rot*view_dir;

	up_dir = Globals::x_rot*up_dir;
	Globals::up_dir = Globals::y_rot*up_dir;

	Globals::right_dir = Globals::up_dir.cross(Globals::view_dir);;
}

//----------------------------------------------------------------------------
// function for emitting new particles
void spawnParticles(Emitter emitter, double dt) {

	// Determine number to spawn
	float numToSpawn = emitter.genRate * dt;
	float fraction = numToSpawn - (int)numToSpawn;
	numToSpawn = (int)numToSpawn;
	if (random(100, false) < fraction)
		numToSpawn++;

	for (int i = 0; i < numToSpawn; i++) {
		if (numParticles >= MAXPARTICLES) {
			cout << "Particle limit reached!" << endl;
			break;		
		}

		// Spawn location
		if (emitter.shape.name == "disk") {
			float radius = emitter.shape.sizeX*sqrt(random(10000, false));
			float theta = 2*PI*random(10000, false);
			particles[numParticles] = Vec3f(sin(theta)*radius + emitter.position[0],
											emitter.position[1],
											cos(theta)*radius + emitter.position[2]);
		}
		else {
			particles[numParticles] = Vec3f(emitter.position[0],
											emitter.position[1],
											emitter.position[2]);
		}

		// Spawn velocity
		velocities[numParticles][0] = range(emitter.properties.velocityRange[0][0], emitter.properties.velocityRange[1][0]);
		velocities[numParticles][1] = range(emitter.properties.velocityRange[0][1], emitter.properties.velocityRange[1][1]);
		velocities[numParticles][2] = range(emitter.properties.velocityRange[0][2], emitter.properties.velocityRange[1][2]);

		// Spawn color
		colors[numParticles][0] = range(emitter.properties.colorStartRange[0][0], emitter.properties.colorStartRange[1][0]);
		colors[numParticles][1] = range(emitter.properties.colorStartRange[0][1], emitter.properties.colorStartRange[1][1]);
		colors[numParticles][2] = range(emitter.properties.colorStartRange[0][2], emitter.properties.colorStartRange[1][2]);
		colors[numParticles][3] = range(emitter.properties.colorStartRange[0][3], emitter.properties.colorStartRange[1][3]);

		// Final color
		colorChanges[numParticles][0] = range(emitter.properties.colorEndRange[0][0], emitter.properties.colorEndRange[1][0]);
		colorChanges[numParticles][1] = range(emitter.properties.colorEndRange[0][1], emitter.properties.colorEndRange[1][1]);
		colorChanges[numParticles][2] = range(emitter.properties.colorEndRange[0][2], emitter.properties.colorEndRange[1][2]);
		colorChanges[numParticles][3] = range(emitter.properties.colorEndRange[0][3], emitter.properties.colorEndRange[1][3]);

		// Other particle properties
		sizes[numParticles] = range(emitter.properties.sizeRange[0], emitter.properties.sizeRange[1]);
		blurs[numParticles] = range(emitter.properties.blurRange[0], emitter.properties.blurRange[1]);
		colorSpeeds[numParticles] = range(emitter.properties.colorSpeedRange[0], emitter.properties.colorSpeedRange[1]);
		lifetimes[numParticles] = 0.0;
		lifeLimits[numParticles] = range(emitter.properties.lifetimeRange[0], emitter.properties.lifetimeRange[1]);
		lightings[numParticles] = emitter.properties.lighting;
		forces[numParticles] = emitter.properties.force;

		numParticles++;
	}
}

//----------------------------------------------------------------------------
// function for killing a particle
void kill(int index) {
	particles[index] = particles[numParticles-1];
	colors[index][0] = colors[numParticles-1][0];
	colors[index][1] = colors[numParticles-1][1];
	colors[index][2] = colors[numParticles-1][2];
	colors[index][3] = colors[numParticles-1][3];
	lightings[index] = lightings[numParticles-1];
	sizes[index] = sizes[numParticles-1];
	blurs[index] = blurs[numParticles-1];

	velocities[index] = velocities[numParticles-1];
	colorChanges[index][0] = colorChanges[numParticles-1][0];
	colorChanges[index][1] = colorChanges[numParticles-1][1];
	colorChanges[index][2] = colorChanges[numParticles-1][2];
	colorChanges[index][3] = colorChanges[numParticles-1][3];
	colorSpeeds[index] = colorSpeeds[numParticles-1];	
	lifetimes[index] = lifetimes[numParticles-1];
	lifeLimits[index] = lifeLimits[numParticles-1];
	forces[index] = forces[numParticles-1];

	numParticles--;
}

//----------------------------------------------------------------------------

void init( mcl::Shader shader ) {
    int i;
	
	// Initalize all other scene elements (meshes, etc.)
	float mesh_verts[4][3];
	float mesh_colors[4][4];
	mesh_verts[0][0] = -100; mesh_verts[0][1] = 0; mesh_verts[0][2] = 0;
	mesh_verts[1][0] = -100; mesh_verts[1][1] = 0; mesh_verts[1][2] = 200;
	mesh_verts[2][0] = 100; mesh_verts[2][1] = 0; mesh_verts[2][2] = 200;
	mesh_verts[3][0] = 100; mesh_verts[3][1] = 0; mesh_verts[3][2] = 0;

	mesh_colors[0][0] = 0.2; mesh_colors[0][1] = 0.0; mesh_colors[0][2] = 0.0; mesh_colors[0][3] = 1.0;
	mesh_colors[1][0] = 0.0; mesh_colors[1][1] = 0.2; mesh_colors[1][2] = 0.0; mesh_colors[1][3] = 1.0;
	mesh_colors[2][0] = 0.0; mesh_colors[2][1] = 0.0; mesh_colors[2][2] = 0.2; mesh_colors[2][3] = 1.0;
	mesh_colors[3][0] = 0.0; mesh_colors[3][1] = 0.2; mesh_colors[3][2] = 0.2; mesh_colors[3][3] = 1.0;

    // Create the buffer for particle/mesh vertices
    glGenBuffers( 1, &vbo_verts );
    glBindBuffer( GL_ARRAY_BUFFER, vbo_verts );
    glBufferData( GL_ARRAY_BUFFER, sizeof(particles) + sizeof(mesh_verts), particles, GL_DYNAMIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, sizeof(particles), sizeof(mesh_verts), mesh_verts );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	// Create the buffer for particle/mesh colors
	glGenBuffers( 1, &vbo_colors );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_colors );
	glBufferData( GL_ARRAY_BUFFER, sizeof(colors) + sizeof(mesh_colors), colors, GL_DYNAMIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, sizeof(colors), sizeof(mesh_colors), mesh_colors );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	// Create the buffer for lighting information
	glGenBuffers( 1, &vbo_lightings );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_lightings );
	glBufferData( GL_ARRAY_BUFFER, sizeof(lightings), lightings, GL_DYNAMIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	// Create the buffer for particle sizes
	glGenBuffers( 1, &vbo_sizes );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_sizes );
	glBufferData( GL_ARRAY_BUFFER, sizeof(sizes), sizes, GL_DYNAMIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	// Create the buffer for particles blurs
	glGenBuffers( 1, &vbo_blurs );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_blurs );
	glBufferData( GL_ARRAY_BUFFER, sizeof(blurs), blurs, GL_DYNAMIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 ); 

    // Create and bind the vertex array object
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Determine locations of the necessary attributes and matrices used in the vertex shader
	glBindBuffer( GL_ARRAY_BUFFER, vbo_verts );
    glEnableVertexAttribArray( shader.attribute("vertex_position") );
    glVertexAttribPointer( shader.attribute("vertex_position"), 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_colors );
    glEnableVertexAttribArray( shader.attribute("vertex_color") );
    glVertexAttribPointer( shader.attribute("vertex_color"), 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_lightings );
	glEnableVertexAttribArray( shader.attribute("particle_lighting") );
	glVertexAttribPointer( shader.attribute("particle_lighting"), 1, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_sizes );
	glEnableVertexAttribArray( shader.attribute("particle_size") );
	glVertexAttribPointer( shader.attribute("particle_size"), 1, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	glBindBuffer( GL_ARRAY_BUFFER, vbo_blurs );
	glEnableVertexAttribArray( shader.attribute("particle_blur") );
	glVertexAttribPointer( shader.attribute("particle_blur"), 1, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
	
	// Done with the vertex array object for now
    glBindVertexArray( vao );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Define static OpenGL state variables
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); // white, opaque background
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glClearDepth(1.0);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Define some GLFW cursors (in case you want to dynamically change the cursor's appearance)
    // If you want, you can add more cursors, and even define your own cursor appearance
    arrow_cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    hand_cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    
}

//----------------------------------------------------------------------------

int main(int argc, char** argv) {

	int i;
	GLdouble rotation;
    GLFWwindow* window;
	

	// ------------ OpenGL setup -----------------

    // Define the error callback function
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW (performs platform-specific initialization)
    if (!glfwInit()) exit(EXIT_FAILURE);
    
    // Ask for OpenGL 3.2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Use GLFW to open a window within which to display your graphics
	Globals::win_width = WIN_WIDTH;
	Globals::win_height = WIN_HEIGHT;
	window = glfwCreateWindow((int)Globals::win_width, (int)Globals::win_height, "Particle Test", NULL, NULL);
	
    // Verify that the window was successfully created; if not, print error message and terminate
    if (!window)
	{
        printf("GLFW failed to create window; terminating\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
	}
    
	glfwMakeContextCurrent(window); // makes the newly-created context current
    
	glfwSwapInterval(1);  // tells the system to wait to swap buffers until monitor refresh has completed; necessary to avoid tearing

    // Define the keyboard callback function
    glfwSetKeyCallback(window, key_callback);
    // Define the mouse button callback function
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    // Define the mouse motion callback function
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    // If not using a Mac, initialize GLEW
    #ifdef USE_GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        // check for errors in GLEW initialization
        if (err != GLEW_OK) {
            cout << "error initializing GLEW: " << glewGetErrorString(err) << endl;
            exit(EXIT_FAILURE);
        }
    #endif


	// ------------ Shader setup -----------------

	// Create the shaders
	mcl::Shader particle_shader;	

    // Define the names of the shader files
    std::stringstream vshader, fshader;
    vshader << SRC_DIR << "/vshader_particle.glsl";
    fshader << SRC_DIR << "/fshader_particle.glsl";
    
    // Load the shaders and use the resulting shader program
    particle_shader.init_from_files( vshader.str(), fshader.str() );
	particle_shader.enable();
	currentShader = particle_shader;

	// Define and load the shaders for the geometry
	/*vshader.str(std::string());
	vshader << SRC_DIR << "/vshader_geometry.glsl";
	fshader.str(std::string());
	fshader << SRC_DIR << "/fshader_geometry.glsl";
	particle_shader.init_from_files( vshader.str(), fshader.str() );*/

	// Initalize particles and scene geometry
	init(particle_shader);


	// ------------ Scene setup ------------------

	// Initialize scene variables
	Globals::eye = Vec3f(0.0, 3.0, -4.0);
	Globals::view_dir = Vec3f(0.0, 0.0, 1.0);
	Globals::up_dir = Vec3f(0.0, 1.0, 0.0);
	Globals::right_dir = Vec3f(1.0, 0.0, 0.0);
	generateViewing();
	generateProjection(-.1, -.1, .1, .1, 0.1, 500.0);

	glUniform3f( particle_shader.uniform("lightAmbient"), lightAmb[0], lightAmb[1], lightAmb[2] );
	glUniform3f( particle_shader.uniform("lightColor"), lightCol[0], lightCol[1], lightCol[2] );
	glUniform3f( particle_shader.uniform("lightDirection"), lightDir[0], lightDir[1], lightDir[2] );

	uint CUR, PREV;
	CUR = glfwGetTimerValue();
	double timePassed = 0;
	double dt = 0;

	uint frames = 0;
	double counter = 0;

	double movementSpeed = 0.1;

	// Bind the vertex array buffer
	glBindVertexArray( vao );

	// ------------ Simulation setup -------------

	particles[0] = Vec3f(0.0, 15.0, 5.0);
	colors[0][0] = 1.0;
	colors[0][1] = 0.0;
	colors[0][2] = 0.0;
	colors[0][3] = 1.0;
	lightings[0] = 1;
	sizes[0] = MAXSIZE;
	blurs[0] = 0.0;

	velocities[0] = Vec3f(0.0, 1.0, 0.0);
	forces[0] = 1;
	grounded[0] = false;

	numParticles++;

	for (i = 1; i < 5; i++) {
		particles[i][0] = 5.0 * random(10000, true);
		particles[i][1] = 60 + 15  * random(10000, false);
		particles[i][2] = 5.0 * random(10000, false);

		colors[i][0] = 0.1 + random(90, false);
		colors[i][1] = 0.1 + random(90, false);
		colors[i][2] = 0.1 + random(90, false);
		colors[i][3] = 1.0;

		velocities[i][0] = 3 * random(10000, true);
		velocities[i][1] = 3 * random(10000, true);
		velocities[i][2] = 3 * random(10000, true);

		lightings[i] = 1;

		sizes[i] = 25 + (MAXSIZE-25)*random(1000, false);

		blurs[i] = 0.0;
		
		forces[i] = 1;

		grounded[i] = false;
	}

	for (i = 5; i < 25; i++) {
		particles[i][0] = 5.0 * random(10000, true);
		particles[i][1] = 130 + 20  * random(10000, false);
		particles[i][2] = 5.0 * random(10000, false);

		colors[i][0] = 0.1 + random(90, false);
		colors[i][1] = 0.1 + random(90, false);
		colors[i][2] = 0.1 + random(90, false);
		colors[i][3] = 1.0;

		velocities[i][0] = 3 * random(10000, true);
		velocities[i][1] = 3 * random(10000, true);
		velocities[i][2] = 3 * random(10000, true);

		lightings[i] = 1;

		sizes[i] = 25 + (MAXSIZE-25)*random(1000, false);

		blurs[i] = 0.0;
		
		forces[i] = 1;

		grounded[i] = false;
	}

	for (i = 25; i < MAXPARTICLES; i++) {
		particles[i][0] = 100.0 * random(10000, true);
		particles[i][1] = 750 + 2000.0 * random(10000, false);
		particles[i][2] = 200.0 * random(10000, false);

		colors[i][0] = 0.1 + random(90, false);
		colors[i][1] = 0.1 + random(90, false);
		colors[i][2] = 0.1 + random(90, false);
		colors[i][3] = 1.0;

		velocities[i][0] = 10 * random(10000, true);
		velocities[i][1] = 10 * random(10000, true);
		velocities[i][2] = 10 * random(10000, true);

		lightings[i] = 1;

		sizes[i] = 25 + (MAXSIZE-25)*random(1000, false);

		blurs[i] = 0.0;
		
		forces[i] = 1;

		grounded[i] = false;
	}
	numParticles = MAXPARTICLES;

	double timer = 0.0;

	// ------------ Graphics loop ----------------

	while (!glfwWindowShouldClose(window)) {

		// ------------ Physics update ---------------

		PREV = CUR;
		CUR = glfwGetTimerValue();
		timePassed = (double) (CUR - PREV)/glfwGetTimerFrequency();
		
		dt = timePassed*timeMultiplier;
		
		if (!paused) {
			timer += dt;

			for (i = 0; i < numParticles; i++) {
				lifetimes[i] += dt;

				if (!grounded[i]) {
					particles[i][0] += velocities[i][0]*dt;
					particles[i][1] += velocities[i][1]*dt - dt*dt*GRAVITY/2.0;
					particles[i][2] += velocities[i][2]*dt;
					if (abs(particles[i][0]) >= 100.0) {
						particles[i][0] = sgn(particles[i][0])*99.9;
						velocities[i][0] *= -.7;
					}

					if (particles[i][1] < 0.1) {
						particles[i][1] = 0.1;
						velocities[i][1] *= -.3;
						if (std::abs(velocities[i][1]*dt) < dt*dt*GRAVITY) {
							velocities[i][1] = 0.0;
							grounded[i] = true;
						}
					}
					else if (forces[i] == 1) {
						velocities[i][1] -= GRAVITY*dt;
					}

					if (abs(particles[i][2] - 100.0) >= 100.0) {
						particles[i][2] = 100 + sgn(particles[i][2])*99.9;
						velocities[i][2] *= -.7;
					}
				}
				else {
					particles[i][0] += velocities[i][0]*dt;
					particles[i][2] += velocities[i][2]*dt;
					if (abs(particles[i][0]) >= 100.0) {
						particles[i][0] = sgn(particles[i][0])*99.9;
						velocities[i][0] *= -.7;
					}

					if (abs(particles[i][2] - 100.0) >= 100.0) {
						particles[i][2] = 100 + sgn(particles[i][2])*99.9;
						velocities[i][2] *= -.7;
					}
					velocities[i][0] -= sgn(velocities[i][0])*2*dt;
					velocities[i][2] -= sgn(velocities[i][2])*2*dt;

					if (sqrt(velocities[i][0]*velocities[i][0] + velocities[i][2]*velocities[i][2]) < dt) {
						velocities[i] = Vec3f(0.0, 0.0, 0.0);
					}
				}
			}

			glBindBuffer( GL_ARRAY_BUFFER, vbo_verts );
			glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(particles[0])*numParticles, particles );

			glBindBuffer( GL_ARRAY_BUFFER, vbo_colors );
			glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(colors[0])*numParticles, colors );

			glBindBuffer( GL_ARRAY_BUFFER, vbo_lightings );
			glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(lightings[0])*numParticles, lightings );

			glBindBuffer( GL_ARRAY_BUFFER, vbo_sizes );
			glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(sizes[0])*numParticles, sizes );

			glBindBuffer( GL_ARRAY_BUFFER, vbo_blurs );
			glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(sizes[0])*numParticles, blurs );
		
			glBindBuffer( GL_ARRAY_BUFFER, 0 );

		}

		// ------------ Input processing ---------------

		if (addTimeMultiplier)
			timeMultiplier += .01;
		if (subTimeMultiplier)
			timeMultiplier = max(0.01, timeMultiplier - .01);

		if (Globals::key_rcontrol || Globals::key_lshift)
			movementSpeed += .01;
		else
			movementSpeed = .1;

		if (Globals::key_up || Globals::key_w) // Move the camera forward
			Globals::eye += Globals::view_dir*movementSpeed;
		if (Globals::key_down || Globals::key_s) // Move the camera backward
			Globals::eye += Globals::view_dir*(-movementSpeed);
		if (Globals::key_left || Globals::key_a) // Move the camera leftward
			Globals::eye += Globals::right_dir*movementSpeed;
		if (Globals::key_right || Globals::key_d) // Move the camera rightward
			Globals::eye += Globals::right_dir*(-movementSpeed);
		if (Globals::key_rshift || Globals::key_e) // Move the camera upward
			Globals::eye += Globals::up_dir*movementSpeed; 
		if (Globals::key_0 || Globals::key_q) // Move the camera downward
			Globals::eye += Globals::up_dir*(-movementSpeed);

		// Camera rotation is handled entirely in the mouse movement callback function

		// Generate the view transformation matrix
		generateViewing();
		
		// Update the uniform values on the shaders
	glUniformMatrix4fv( particle_shader.uniform("M"), 1, GL_FALSE, Globals::model.m ); // model transformation
	glUniformMatrix4fv( particle_shader.uniform("V"), 1, GL_FALSE, Globals::view.m ); // viewing transformation
	glUniformMatrix4fv( particle_shader.uniform("P"), 1, GL_FALSE, Globals::projection.m ); // projection matrix
	glUniform3f( particle_shader.uniform("eye"), Globals::eye[0], Globals::eye[1], Globals::eye[2] );
	glUniform3f( particle_shader.uniform("viewDirection"), Globals::view_dir[0], Globals::view_dir[1], Globals::view_dir[2] );


		// ------------ Frame rate display ---------

		frames++;
		counter += timePassed;
		if ( counter >= 1.0 ) {
			cout << "FPS: " << frames << endl;
			cout << "--- # of Particles: " << numParticles << endl;
			frames = 0;
			counter -= 1.0;
		}
		

		// ------------ Rendering step ------------ 

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // Fill the window with the background color

		// At least 1 particle needs to be rendered before any other scene geometry in
		// order for the shader to work properly (I don't know why)
		if (!USINGBUBBLES) // Writing to the depth mask is problematic when using bubbles
			glDepthMask(GL_FALSE);
		glUniform1f( particle_shader.uniform("specTerm"), 80 );
		glUniform1i( particle_shader.uniform("renderingPoints"), 1 );
		glDrawArrays( GL_POINTS, 0, 1 );
		glDepthMask(GL_TRUE);

		// Render the ground plane
		glUniform1f( particle_shader.uniform("specTerm"), -1 );
		glUniform1i( particle_shader.uniform("renderingPoints"), 0 );
		glDrawArrays( GL_TRIANGLE_FAN, MAXPARTICLES, 4 );

		// Render the opaque particles first
		glUniform1i( particle_shader.uniform("onlyOpaque"), 1 );
		glUniform1f( particle_shader.uniform("specTerm"), 80 );
		glUniform1i( particle_shader.uniform("renderingPoints"), 1 );
		glDrawArrays( GL_POINTS, 0, numParticles );

		// Then render the translucent particles
		if (!USINGBUBBLES) // Writing to the depth mask is problematic when using bubbles
			glDepthMask(GL_FALSE);
		glUniform1i( particle_shader.uniform("onlyOpaque"), 0 );
		if (numParticles > 1)
			glDrawArrays( GL_POINTS, 1, numParticles );
		glDepthMask(GL_TRUE);

		glFlush();	// Ensure that all OpenGL calls have executed before swapping buffers

        glfwSwapBuffers(window);  // Swap buffers
        glfwPollEvents(); // Process events that have happened since last update

	} // End graphics loop

	// Clean up
	glfwDestroyWindow(window);
	glfwTerminate();  // Destroys any remaining objects, frees resources allocated by GLFW
	exit(EXIT_SUCCESS);

} // end main

