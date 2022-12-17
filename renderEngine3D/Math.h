#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <map>
#include <GLFW/glfw3native.h>

struct mat {
	float m[4][4] = {0.0f};
};

struct vec3d {
	float x = 0, y = 0, z = 0, w = 1;

	bool operator==(const vec3d& l) const {
		if (x != l.x || y != l.y || z != l.z) {
			return false;
		}
		return true;
	}
};

class Math {
private:
	GL_FLOAT_VEC4
public:
	Math();
	~Math();
	
	vec3d matrixMultiplyVector(mat& m, vec3d& i);
	mat matrixMultiplyMatrix(mat& m1, mat& m2);
	mat matrixMakeIdentity();
	mat matrixMakeRotationX(float fAngleRad);
	mat matrixMakeRotationY(float fAngleRad);
	mat matrixMakeRotationZ(float fAngleRad);
	mat matrixMakeTranslation(float x, float y, float z);
	vec3d vectorAdd(vec3d& v1, vec3d& v2);
	vec3d vectorSub(vec3d& v1, vec3d& v2);
	vec3d vectorMul(vec3d& v1, float k);
	vec3d vectorDiv(vec3d& v1, float k);
	float vectorDotProduct(vec3d& v1, vec3d& v2);
	vec3d vectorCrossProduct(vec3d& v1, vec3d& v2);
	float vectorLength(vec3d& v);
	vec3d vectorNormalize(vec3d& v);
	mat matrixPointAt(vec3d& pos, vec3d& target, vec3d& up);
	mat matrixQuickInverse(mat& m);
	vec3d vectorIntersectPlane(vec3d& planeP, vec3d& planeN, vec3d& lineStart, vec3d& lineEnd, float& t);
};