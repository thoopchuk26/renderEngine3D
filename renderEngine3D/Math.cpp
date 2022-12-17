#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <map>
#include "Math.h"

Math::Math() {};
Math::~Math() {};

vec3d Math::matrixMultiplyVector(mat& m, vec3d& i) {
	vec3d v;
	v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
	v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
	v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
	v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
	return v;
}

mat Math::matrixMultiplyMatrix(mat& m1, mat& m2) {
	mat matrix;
	for (int c = 0; c < 4; c++)
		for (int r = 0; r < 4; r++)
			matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
	return matrix;
}

mat Math::matrixMakeIdentity() {
	mat matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat Math::matrixMakeRotationX(float fAngleRad) {
	mat matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[1][2] = sinf(fAngleRad);
	matrix.m[2][1] = -sinf(fAngleRad);
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat Math::matrixMakeRotationY(float fAngleRad) {
	mat matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][2] = sinf(fAngleRad);
	matrix.m[2][0] = -sinf(fAngleRad);
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat Math::matrixMakeRotationZ(float fAngleRad) {
	mat matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][1] = sinf(fAngleRad);
	matrix.m[1][0] = -sinf(fAngleRad);
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat Math::matrixMakeTranslation(float x, float y, float z) {
	mat matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	matrix.m[3][0] = x;
	matrix.m[3][1] = y;
	matrix.m[3][2] = z;
	return matrix;
}

vec3d Math::vectorAdd(vec3d& v1, vec3d& v2) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

vec3d Math::vectorSub(vec3d& v1, vec3d& v2) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

vec3d Math::vectorMul(vec3d& v1, float k) {
	return { v1.x * k, v1.y * k, v1.z * k };
}

vec3d Math::vectorDiv(vec3d& v1, float k) {
	return { v1.x / k, v1.y / k, v1.z / k };
}

float Math::vectorDotProduct(vec3d& v1, vec3d& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

vec3d Math::vectorCrossProduct(vec3d& v1, vec3d& v2) {
	vec3d v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}

float Math::vectorLength(vec3d& v) {
	return sqrtf(vectorDotProduct(v, v));
}

vec3d Math::vectorNormalize(vec3d& v) {
	float l = vectorLength(v);
	return { v.x / l, v.y / l, v.z / l };
}

mat Math::matrixPointAt(vec3d& pos, vec3d& target, vec3d& up) {
	//calculate new forward
	vec3d newForward = vectorSub(target, pos);
	newForward = vectorNormalize(newForward);

	//calculate new up
	vec3d a = vectorMul(newForward, vectorDotProduct(up, newForward));
	vec3d newUp = vectorSub(up, a);
	newUp = vectorNormalize(newUp);

	//calculate new right
	vec3d newRight = vectorCrossProduct(newUp, newForward);

	//create dimensioning and translation matrix
	mat matrix;
	matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
	matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
	matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
	matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
	return matrix;
}

mat Math::matrixQuickInverse(mat& m) { // Only for Rotation/Translation Matrices
	mat matrix;
	matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
	matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
	matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
	matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
	matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
	matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

vec3d Math::vectorIntersectPlane(vec3d& planeP, vec3d& planeN, vec3d& lineStart, vec3d& lineEnd, float& t) {
	planeN = vectorNormalize(planeN);
	float plane_d = -vectorDotProduct(planeN, planeP);
	float ad = vectorDotProduct(lineStart, planeN);
	float bd = vectorDotProduct(lineEnd, planeN);
	t = (-plane_d - ad) / (bd - ad);
	vec3d lineStartToEnd = vectorSub(lineEnd, lineStart);
	vec3d lineToIntersect = vectorMul(lineStartToEnd, t);
	return vectorAdd(lineStart, lineToIntersect);
}