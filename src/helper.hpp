// Code by Caleb Biasco (biasc007)
// CSCI 5607, Fall 2016

// Includes
#include "trimesh.hpp"

// Macros
#define PI 4.0*atan(1.0)
#define BUFFER_OFFSET(bytes) ((GLvoid*) (bytes))

class Mat4x4 {
public:

	float m[16];

	Mat4x4(){ // Default: Identity
		m[0] = 1.f;  m[4] = 0.f;  m[8]  = 0.f;  m[12] = 0.f;
		m[1] = 0.f;  m[5] = 1.f;  m[9]  = 0.f;  m[13] = 0.f;
		m[2] = 0.f;  m[6] = 0.f;  m[10] = 1.f;  m[14] = 0.f;
		m[3] = 0.f;  m[7] = 0.f;  m[11] = 0.f;  m[15] = 1.f;
	}

	void make_identity(){
		m[0] = 1.f;  m[4] = 0.f;  m[8]  = 0.f;  m[12] = 0.f;
		m[1] = 0.f;  m[5] = 1.f;  m[9]  = 0.f;  m[13] = 0.f;
		m[2] = 0.f;  m[6] = 0.f;  m[10] = 1.f;  m[14] = 0.f;
		m[3] = 0.f;  m[7] = 0.f;  m[11] = 0.f;  m[15] = 1.f;
	}

	void print(){
		std::cout << m[0] << ' ' <<  m[4] << ' ' <<  m[8]  << ' ' <<  m[12] << "\n";
		std::cout << m[1] << ' ' <<   m[5] << ' ' <<  m[9]  << ' ' <<   m[13] << "\n";
		std::cout << m[2] << ' ' <<   m[6] << ' ' <<  m[10] << ' ' <<   m[14] << "\n";
		std::cout << m[3] << ' ' <<   m[7] << ' ' <<  m[11] << ' ' <<   m[15] << "\n";
	}

	void make_scale(float x, float y, float z){
		make_identity();
		m[0] = x; m[5] = y; m[10] = x;
	}
};

static inline const Vec3f operator*(const Mat4x4 &m, const Vec3f &v){
	Vec3f r( m.m[0]*v[0]+m.m[4]*v[1]+m.m[8]*v[2],
		m.m[1]*v[0]+m.m[5]*v[1]+m.m[9]*v[2],
		m.m[2]*v[0]+m.m[6]*v[1]+m.m[10]*v[2] );
	return r;
}


//
//	Global state variables
//
namespace Globals {
	double cursorX, cursorY; // cursor positions
	float win_width, win_height; // window size
	float aspect;
	GLuint verts_vbo[1], colors_vbo[1], normals_vbo[1], faces_ibo[1], tris_vao;
	TriMesh mesh;

	//  Model, view and projection matrices, initialized to the identity
	Mat4x4 model;
	Mat4x4 view;
	Mat4x4 projection;
	
	// Scene variables
	Vec3f eye;
	Vec3f view_dir;
	Mat4x4 x_rot;
	Mat4x4 y_rot;
	Vec3f up_dir;
	Vec3f right_dir;
	
	// Input variables
	bool key_up; // forward movement
	bool key_w;
	bool key_down; // backward movement
	bool key_s;
	bool key_right; // right strafing
	bool key_d;
	bool key_left; // left strafing
	bool key_a;
	bool key_rshift; // upward movement
	bool key_e;
	bool key_0; // downward movement
	bool key_q;
	bool key_rcontrol; // speed up
	bool key_lshift; 
	
	double theta;
	double phi;
}

// Function to construct viewing transformation matrix
void generateViewing() {
	// Calculate the orthogonal axes based on the viewing parameters
	Vec3f n = Globals::view_dir * (-1.f/Globals::view_dir.len());
	Vec3f u = Globals::up_dir.cross(n);
	u.normalize();
	Vec3f v = n.cross(u);
	
	// Calculate the translation based on the new axes
	float dx = -(Globals::eye.dot(u));
	float dy = -(Globals::eye.dot(v));
	float dz = -(Globals::eye.dot(n));
	
	// Fill in the matrix
	Globals::view.m[0] = u[0];	Globals::view.m[4] = u[1];	Globals::view.m[8] = u[2];	Globals::view.m[12] = dx;
	Globals::view.m[1] = v[0];	Globals::view.m[5] = v[1];	Globals::view.m[9] = v[2];	Globals::view.m[13] = dy;
	Globals::view.m[2] = n[0];	Globals::view.m[6] = n[1];	Globals::view.m[10] = n[2];	Globals::view.m[14] = dz;
	Globals::view.m[3] = 0;		Globals::view.m[7] = 0;		Globals::view.m[11] = 0;	Globals::view.m[15] = 1;
}

// Function to construct perspective projection transformation matrix
void generateProjection(float left, float bottom, float right, float top, float near, float far) {
	Globals::projection.m[0] = 2*near/(right-left);			Globals::projection.m[4] = 0;					
	Globals::projection.m[1] = 0;							Globals::projection.m[5] = 2*near/(top-bottom);
	Globals::projection.m[2] = 0;							Globals::projection.m[6] = 0;
	Globals::projection.m[3] = 0;							Globals::projection.m[7] = 0;
	
	Globals::projection.m[8] = (right+left)/(right-left);	Globals::projection.m[12] = 0;
	Globals::projection.m[9] = (top+bottom)/(top-bottom);	Globals::projection.m[13] = 0;
	Globals::projection.m[10] = -(far+near)/(far-near);		Globals::projection.m[14] = -2*far*near/(far-near);
	Globals::projection.m[11] = -1;							Globals::projection.m[15] = 0;
}

// Function to rotate the viewing transform about the y axis
static const Mat4x4 rotateY(float theta) {
	float t = theta*PI/180.f;
	
	Mat4x4 mat;
	mat.m[0] = cos(t);		mat.m[4] = 0.f;		mat.m[8] = sin(t);		mat.m[12] = 0.f;
	mat.m[1] = 0.f;			mat.m[5] = 1.f;		mat.m[9] = 0.f;			mat.m[13] = 0.f;
	mat.m[2] = -sin(t);		mat.m[6] = 0.f;		mat.m[10] = cos(t);		mat.m[14] = 0.f;
	mat.m[3] = 0.f;			mat.m[7] = 0.f;		mat.m[11] = 0.f;		mat.m[15] = 1.f;
	
	return mat;
}

// Function to rotate the viewing transform about the y axis
static const Mat4x4 rotateX(float phi) {
	float t = phi*PI/180.f;
	
	Mat4x4 mat;
	mat.m[0] = 1.f;		mat.m[4] = 0.f;		mat.m[8] = 0.f;			mat.m[12] = 0.f;
	mat.m[1] = 0.f;		mat.m[5] = cos(t);	mat.m[9] = -sin(t);		mat.m[13] = 0.f;
	mat.m[2] = 0.f;		mat.m[6] = sin(t);	mat.m[10] = cos(t);		mat.m[14] = 0.f;
	mat.m[3] = 0.f;		mat.m[7] = 0.f;		mat.m[11] = 0.f;		mat.m[15] = 1.f;
	
	return mat;
}

// Helper function for getting random fraction for passed in value
static const double random(int max, bool negative) {
	if (negative)
		return (double) (rand() % max*2 - max)/(double) max;
	else 
		return (double) (rand() % max)/(double) max;
}

// Helper function for getting random number in a range
static const float range(float min, float max) {
	return min + (max - min) * random(1000, false);
}

// Helper function for stepping from a number toward another number
static const float step(float start, float end, float step) {
	return start + (end - start) * step;
}

// Helper function for getting the sign of a number
// Credit to user79758 at www.stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-fignum-sgn-in-c-c/
template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}
