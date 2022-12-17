#define SDL_MAIN_HANDLED
#include <Windows.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include <strstream>
#include <algorithm>
#include <list>
#include <map>
#include <fstream>
#include <strstream>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <string>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

constexpr int   CHUNK_SIZE = 16,
CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE,
CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

//3d vector struct containing the xyz coords
struct vec3d {
	float x = 0, y = 0, z = 0, w = 1;

	bool operator==(const vec3d& l) const {
		if (x != l.x || y != l.y || z != l.z) {
			return false;
		}
		return true;
	}
};

//2d vector struct containing uv coords
struct vec2d {
	float u = 0;
	float v = 0;
	float w = 1;
};

//struct containing all the points required to create a triangle
struct triangle {
	vec3d p[3];
	vec2d t[3];
	vec3d centroid;
	bool isRendered = true;

	void getCentroid() {
		vec3d temp;
		temp.x = round((p[0].x + p[1].x + p[2].x) / 3);
		temp.y = round((p[0].y + p[1].y + p[2].y) / 3);
		temp.z = round((p[0].z + p[1].z + p[2].z) / 3);
		centroid = temp;
	};

	bool operator==(const triangle& l) const {
		if (centroid.x != l.centroid.x || centroid.y != l.centroid.y || centroid.z != l.centroid.z) {
			return false;
		}

		return true;
	}
};

//struct containing color value split into r g b
struct color {
	int r, g, b;
};

// 4x4 matrix struct
struct mat {
	float m[4][4] = { 0 };
};

//bitmap texture struct that loads in bitmap files
struct bmpTexture {
	std::vector<color> pixels;
	uint32_t width = 0;
	uint32_t height = 0;

	void loadBmp(const char* path) {
		std::fstream f;
		f.open(path, std::ios::in | std::ios::binary);
		if (!f.is_open()) {
			std::cout << "File could not be opened.\n";
			f.close();
			return;
		}

		const int fileHeaderSize = 14;
		const int infoHeaderSize = 40;
		unsigned char fileHeader[fileHeaderSize];
		f.read(reinterpret_cast<char*>(fileHeader), fileHeaderSize);
		unsigned char infoHeader[infoHeaderSize];
		f.read(reinterpret_cast<char*>(infoHeader), infoHeaderSize);

		if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
			f.close();
			std::cout << "File at " << path << "is  not a bitmap image.\n";
			return;
		}

		int fileSize = fileHeader[2] + (fileHeader[3] << 8) + (fileHeader[4] << 16) + (fileHeader[5] << 24);
		width = infoHeader[4] + (infoHeader[5] << 8) + (infoHeader[6] << 16) + (infoHeader[7] << 24);
		height = infoHeader[8] + (infoHeader[9] << 8) + (infoHeader[10] << 16) + (infoHeader[11] << 24);

		pixels.resize(width * height);
		const int paddingAmount = ((4 - (width * 3) % 4) % 4);

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				unsigned char color[3];
				f.read(reinterpret_cast<char*>(color), 3);

				pixels[y * width + x].r = color[2];
				pixels[y * width + x].g = color[1];
				pixels[y * width + x].b = color[0];
			}
			f.ignore(paddingAmount);
		}
		f.close();

		std::reverse(pixels.begin(), pixels.end());
		std::cout << "File read\n";
	};

	//return uint32_t pixel color
	uint32_t getPixelColor(float x, float y) {
		int sx = (int)(x * (float)width);
		int sy = (int)(y * (float)height - 1.0f);
		if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
			return ((0 << 24U) | (0 << 16U) | (0 << 8U) | 255);
		}
		else {
			color col = pixels[sy * width + sx];
			return ((col.r << 24U) | (col.g << 16U) | (col.b << 8U) | 255);
		}
	};

	//return color object pixel color
	color getPixelColorCol(float x, float y) {
		int sx = (int)(x * (float)width);
		int sy = (int)(y * (float)height - 1.0f);
		if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
			return { 0,0,0 };
		}
		else {
			color col = pixels[sy * width + sx];
			return col;
		}
	};
};

//mesh struct containing all the triangles that make up an object, also loads in objects from obj files
struct mesh {
	std::vector<triangle> tris;
	vec3d center = { 0,0,0 };

	bool loadFromObjectFile(std::string sFile) {
		std::ifstream f(sFile);
		if (!f.is_open()) {
			return false;
		}

		std::vector<vec3d> verts;
		std::vector<vec2d> texs;

		while (!f.eof()) {
			char line[128];
			f.getline(line, 128);

			std::strstream s;
			s << line;

			char junk;
			if (line[0] == 'v') {
				if (line[1] == 't') {
					vec2d v;
					s >> junk >> junk >> v.u >> v.v;
					texs.push_back(v);
				}
				else {
					vec3d v;
					s >> junk >> v.x >> v.y >> v.z;
					verts.push_back(v);
				}
			}
			if (line[0] == 'f') {
				std::string str;
				std::getline(s, str);
				for (int i = 0; i < str.length(); i++) {
					if (str[i] == '/') {
						str[i] = ' ';
					}
				}
				str = str.substr(2);
				std::stringstream s(str);
				int f[6];
				s >> f[0] >> f[1] >> f[2] >> f[3] >> f[4] >> f[5];
				triangle temp = { verts[f[0] - 1], verts[f[2] - 1], verts[f[4] - 1], texs[f[1] - 1], texs[f[3] - 1], texs[f[5] - 1] };
				temp.getCentroid();
				tris.push_back(temp);
			}
		}

		return true;
	};

	//shift all vertices of all triangles to the new center point
	void recenter(float x, float y, float z) {
		vec3d newCenter = { x,y,z };
		vec3d difference = { (center.x - x) * (-1), (center.y - y) * (-1), (center.z - z) * (-1) };

		for (int i = 0; i < tris.size(); i++) { 
			vec3d p1, p2, p3;
			p1 = { tris[i].p[0].x + difference.x, tris[i].p[0].y + difference.y, tris[i].p[0].z + difference.z };
			p2 = { tris[i].p[1].x + difference.x, tris[i].p[1].y + difference.y, tris[i].p[1].z + difference.z };
			p3 = { tris[i].p[2].x + difference.x, tris[i].p[2].y + difference.y, tris[i].p[2].z + difference.z };
			triangle newTri = { p1, p2, p3, tris[i].t[0], tris[i].t[1], tris[i].t[2] };
			tris[i] = newTri;
		}


		center = newCenter;
	};
};

//global variables for SDL2, window drawing, and camera manip
SDL_Renderer* render;
SDL_Window* window;
SDL_Texture* windowTexture;

mesh meshCube;
bmpTexture dirtTexture;
mat matProj;

vec3d camera;
vec3d lookDir;
float fTheta = 0.0f;
float fYaw;
float fPitch;

const int screenWidth = 1280;
const int screenHeight = 720;

float* depthBuffer = nullptr;
uint32_t* lockedPixels = nullptr;
int pitch = 0;

//std::vector<std::pair<vec3d, bool>> centers;
vec3d cameraPos = { 0,0,0 };
std::map<vec3d, triangle> triangleMap;

//draw a triangle using 3 points of a specified color
void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color) {
	SDL_SetRenderDrawColor(render, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(render, x1, y1, x2, y2);
	SDL_RenderDrawLine(render, x2, y2, x3, y3);
	SDL_RenderDrawLine(render, x1, y1, x3, y3);
}

vec3d matrixMultiplyVector(mat& m, vec3d& i) {
	vec3d v;
	v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
	v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
	v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
	v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
	return v;
}

mat matrixMultiplyMatrix(mat& m1, mat& m2) {
	mat matrix;
	for (int c = 0; c < 4; c++)
		for (int r = 0; r < 4; r++)
			matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
	return matrix;
}

mat matrixMakeIdentity() {
	mat matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat matrixMakeRotationX(float fAngleRad) {
	mat matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[1][2] = sinf(fAngleRad);
	matrix.m[2][1] = -sinf(fAngleRad);
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat matrixMakeRotationY(float fAngleRad) {
	mat matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][2] = sinf(fAngleRad);
	matrix.m[2][0] = -sinf(fAngleRad);
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat matrixMakeRotationZ(float fAngleRad) {
	mat matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][1] = sinf(fAngleRad);
	matrix.m[1][0] = -sinf(fAngleRad);
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

mat matrixMakeTranslation(float x, float y, float z) {
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

vec3d vectorAdd(vec3d& v1, vec3d& v2) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

vec3d vectorSub(vec3d& v1, vec3d& v2) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

vec3d vectorMul(vec3d& v1, float k) {
	return { v1.x * k, v1.y * k, v1.z * k };
}

vec3d vectorDiv(vec3d& v1, float k) {
	return { v1.x / k, v1.y / k, v1.z / k };
}

float vectorDotProduct(vec3d& v1, vec3d& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

vec3d vectorCrossProduct(vec3d& v1, vec3d& v2) {
	vec3d v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}

float vectorLength(vec3d& v) {
	return sqrtf(vectorDotProduct(v, v));
}

vec3d vectorNormalize(vec3d& v) {
	float l = vectorLength(v);
	return { v.x / l, v.y / l, v.z / l };
}

mat matrixPointAt(vec3d& pos, vec3d& target, vec3d& up) {
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

mat matrixQuickInverse(mat& m) { // Only for Rotation/Translation Matrices
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

vec3d vectorIntersectPlane(vec3d& planeP, vec3d& planeN, vec3d& lineStart, vec3d& lineEnd, float& t) {
	planeN = vectorNormalize(planeN);
	float plane_d = -vectorDotProduct(planeN, planeP);
	float ad = vectorDotProduct(lineStart, planeN);
	float bd = vectorDotProduct(lineEnd, planeN);
	t = (-plane_d - ad) / (bd - ad);
	vec3d lineStartToEnd = vectorSub(lineEnd, lineStart);
	vec3d lineToIntersect = vectorMul(lineStartToEnd, t);
	return vectorAdd(lineStart, lineToIntersect);
}

//figure out what triangles are clipping against the sides of the screen and create new triangles to render instead
int triangleClipPlane(vec3d planeP, vec3d planeN, triangle& inTri, triangle& outTri1, triangle& outTri2) {
	planeN = vectorNormalize(planeN);

	auto dist = [&](vec3d& p) {
		vec3d n = vectorNormalize(p);
		return (planeN.x * p.x + planeN.y * p.y + planeN.z * p.z - vectorDotProduct(planeN, planeP));
	};

	//define inside and outside array to store triangles in for both vectors and textures
	vec3d* insidePoints[3];
	vec3d* outsidePoints[3];
	int insidePointCount = 0;
	int outsidePointCount = 0;

	vec2d* insideTex[3];
	vec2d* outsideTex[3];
	int insideTexCount = 0;
	int outsideTexCount = 0;

	float d0 = dist(inTri.p[0]);
	float d1 = dist(inTri.p[1]);
	float d2 = dist(inTri.p[2]);

	if (d0 >= 0) {
		insidePoints[insidePointCount++] = &inTri.p[0];
		insideTex[insideTexCount++] = &inTri.t[0];
	}
	else {
		outsidePoints[outsidePointCount++] = &inTri.p[0];
		outsideTex[outsideTexCount++] = &inTri.t[0];
	}
	if (d1 >= 0) {
		insidePoints[insidePointCount++] = &inTri.p[1];
		insideTex[insideTexCount++] = &inTri.t[1];
	}
	else {
		outsidePoints[outsidePointCount++] = &inTri.p[1];
		outsideTex[outsideTexCount++] = &inTri.t[1];
	}
	if (d2 >= 0) {
		insidePoints[insidePointCount++] = &inTri.p[2];
		insideTex[insideTexCount++] = &inTri.t[2];
	}
	else {
		outsidePoints[outsidePointCount++] = &inTri.p[2];
		outsideTex[outsideTexCount++] = &inTri.t[2];
	}

	//check number of points within plane and return number of triangles we need
	if (insidePointCount == 0) {
		//no points
		return 0;
	}
	if (insidePointCount == 3) {
		//all points
		outTri1 = inTri;
		return 1;
	}
	if (insidePointCount == 1 && outsidePointCount == 2) {
		outTri1.p[0] = *insidePoints[0];
		outTri1.t[0] = *insideTex[0];

		float t;

		outTri1.p[1] = vectorIntersectPlane(planeP, planeN, *insidePoints[0], *outsidePoints[0], t);
		outTri1.t[1].u = t * (outsideTex[0]->u - insideTex[0]->u) + insideTex[0]->u;
		outTri1.t[1].v = t * (outsideTex[0]->v - insideTex[0]->v) + insideTex[0]->v;
		outTri1.t[1].w = t * (outsideTex[0]->w - insideTex[0]->w) + insideTex[0]->w;

		outTri1.p[2] = vectorIntersectPlane(planeP, planeN, *insidePoints[0], *outsidePoints[1], t);
		outTri1.t[2].u = t * (outsideTex[1]->u - insideTex[0]->u) + insideTex[0]->u;
		outTri1.t[2].v = t * (outsideTex[1]->v - insideTex[0]->v) + insideTex[0]->v;
		outTri1.t[2].w = t * (outsideTex[1]->w - insideTex[0]->w) + insideTex[0]->w;

		return 1;
	}
	if (insidePointCount == 2 && outsidePointCount == 1) {
		outTri1.p[0] = *insidePoints[0];
		outTri1.p[1] = *insidePoints[1];
		outTri1.t[0] = *insideTex[0];
		outTri1.t[1] = *insideTex[1];

		float t;
		outTri1.p[2] = vectorIntersectPlane(planeP, planeN, *insidePoints[0], *outsidePoints[0], t);
		outTri1.t[2].u = t * (outsideTex[0]->u - insideTex[0]->u) + insideTex[0]->u;
		outTri1.t[2].v = t * (outsideTex[0]->v - insideTex[0]->v) + insideTex[0]->v;
		outTri1.t[2].w = t * (outsideTex[0]->w - insideTex[0]->w) + insideTex[0]->w;

		outTri2.p[0] = *insidePoints[1];
		outTri2.t[0] = *insideTex[1];
		outTri2.p[1] = outTri1.p[2];
		outTri2.t[1] = outTri1.t[2];

		outTri2.p[2] = vectorIntersectPlane(planeP, planeN, *insidePoints[1], *outsidePoints[0], t);
		outTri2.t[2].u = t * (outsideTex[0]->u - insideTex[1]->u) + insideTex[1]->u;
		outTri2.t[2].v = t * (outsideTex[0]->v - insideTex[1]->v) + insideTex[1]->v;
		outTri2.t[2].w = t * (outsideTex[0]->w - insideTex[1]->w) + insideTex[1]->w;

		return 2;
	}
}

//draw a texture onto an object
void drawTexture(int x1, int y1, float u1, float v1, float w1,
	int x2, int y2, float u2, float v2, float w2,
	int x3, int y3, float u3, float v3, float w3, bmpTexture blockTexture) {

	//SDL_LockTexture(windowTexture, NULL, (void**) &lockedPixels, &pitch);

	if (y2 < y1) {
		std::swap(y1, y2);
		std::swap(x1, x2);
		std::swap(u1, u2);
		std::swap(v1, v2);
		std::swap(w1, w2);
	}
	if (y3 < y1) {
		std::swap(y1, y3);
		std::swap(x1, x3);
		std::swap(u1, u3);
		std::swap(v1, v3);
		std::swap(w1, w3);
	}
	if (y3 < y2) {
		std::swap(y3, y2);
		std::swap(x3, x2);
		std::swap(u3, u2);
		std::swap(v3, v2);
		std::swap(w3, w2);
	}

	int dy1 = y2 - y1;
	int dx1 = x2 - x1;
	float dv1 = v2 - v1;
	float du1 = u2 - u1;
	float dw1 = w2 - w1;

	int dy2 = y3 - y1;
	int dx2 = x3 - x1;
	float dv2 = v3 - v1;
	float du2 = u3 - u1;
	float dw2 = w3 - w1;

	float texU, texV, texW;

	float daxStep = 0, dbxStep = 0, du1Step = 0, dv1Step = 0, du2Step = 0, dv2Step = 0, dw1Step = 0, dw2Step = 0;

	if (dy1) { daxStep = dx1 / (float)abs(dy1); }
	if (dy2) { dbxStep = dx2 / (float)abs(dy2); }

	if (dy1) { du1Step = du1 / (float)abs(dy1); }
	if (dy1) { dv1Step = dv1 / (float)abs(dy1); }
	if (dy1) { dw1Step = dw1 / (float)abs(dy1); }

	if (dy2) { du2Step = du2 / (float)abs(dy2); }
	if (dy2) { dv2Step = dv2 / (float)abs(dy2); }
	if (dy2) { dw2Step = dw2 / (float)abs(dy2); }

	if (dy1) {
		for (int i = y1; i <= y2; i++) {
			int ax = x1 + (float)(i - y1) * daxStep;
			int bx = x1 + (float)(i - y1) * dbxStep;

			float texStartU = u1 + (float)(i - y1) * du1Step;
			float texStartV = v1 + (float)(i - y1) * dv1Step;
			float texStartW = w1 + (float)(i - y1) * dw1Step;

			float texEndU = u1 + (float)(i - y1) * du2Step;
			float texEndV = v1 + (float)(i - y1) * dv2Step;
			float texEndW = w1 + (float)(i - y1) * dw2Step;

			if (ax > bx) {
				std::swap(ax, bx);
				std::swap(texStartU, texEndU);
				std::swap(texStartV, texEndV);
				std::swap(texStartW, texEndW);
			}

			texU = texStartU;
			texV = texStartV;
			texW = texStartW;

			float tStep = 1.0f / ((float)(bx - ax));
			float t = 0.0f;

			for (int j = ax; j < bx; j++) {
				texU = (1.0f - t) * texStartU + t * texEndU;
				texV = (1.0f - t) * texStartV + t * texEndV;
				texW = (1.0f - t) * texStartW + t * texEndW;

				if (texW > depthBuffer[i * screenWidth + j]) {
					int start = i * screenWidth + j;
					lockedPixels[start] = dirtTexture.getPixelColor(texU / texW, texV / texW);
					depthBuffer[i * screenWidth + j] = texW;
				}
				t += tStep;
			}
		}
	}

	dy1 = y3 - y2;
	dx1 = x3 - x2;
	dv1 = v3 - v2;
	du1 = u3 - u2;
	dw1 = w3 - w2;

	if (dy1) { daxStep = dx1 / (float)abs(dy1); }
	if (dy2) { dbxStep = dx2 / (float)abs(dy2); }

	du1Step = 0, dv1Step = 0;
	if (dy1) { du1Step = du1 / (float)abs(dy1); }
	if (dy1) { dv1Step = dv1 / (float)abs(dy1); }
	if (dy1) { dw1Step = dw1 / (float)abs(dy1); }

	if (dy1) {

		for (int i = y2; i <= y3; i++) {
			int ax = x2 + (float)(i - y2) * daxStep;
			int bx = x1 + (float)(i - y1) * dbxStep;

			float texStartU = u2 + (float)(i - y2) * du1Step;
			float texStartV = v2 + (float)(i - y2) * dv1Step;
			float texStartW = w2 + (float)(i - y2) * dw1Step;

			float texEndU = u1 + (float)(i - y1) * du2Step;
			float texEndV = v1 + (float)(i - y1) * dv2Step;
			float texEndW = w1 + (float)(i - y1) * dw2Step;

			if (ax > bx) {
				std::swap(ax, bx);
				std::swap(texStartU, texEndU);
				std::swap(texStartV, texEndV);
				std::swap(texStartW, texEndW);
			}

			texU = texStartU;
			texV = texStartV;
			texW = texStartW;

			float tStep = 1.0f / ((float)(bx - ax));
			float t = 0.0f;

			for (int j = ax; j < bx; j++) {
				texU = (1.0f - t) * texStartU + t * texEndU;
				texV = (1.0f - t) * texStartV + t * texEndV;
				texW = (1.0f - t) * texStartW + t * texEndW;

				if (texW > depthBuffer[i * screenWidth + j]) {
					int start = i * screenWidth + j;
					lockedPixels[start] = dirtTexture.getPixelColor(texU / texW, texV / texW);
					depthBuffer[i * screenWidth + j] = texW;
				}
				t += tStep;
			}
		}
		//SDL_UnlockTexture(windowTexture);
		SDL_RenderCopy(render, windowTexture, nullptr, nullptr);
	}
}

std::vector<triangle> createTriangles(std::vector<triangle> triBuffer, mat matWorld, mat matView) {
	std::vector<triangle> rasterTriangles;
	for (auto& tri : triBuffer) {
		if (tri.isRendered) {
			triangle triProjected, triTransformed, triView;

			//World matrix Transform
			triTransformed.p[0] = matrixMultiplyVector(matWorld, tri.p[0]);
			triTransformed.p[1] = matrixMultiplyVector(matWorld, tri.p[1]);
			triTransformed.p[2] = matrixMultiplyVector(matWorld, tri.p[2]);
			triTransformed.t[0] = tri.t[0];
			triTransformed.t[1] = tri.t[1];
			triTransformed.t[2] = tri.t[2];

			//Calculate Normal
			vec3d normal, line1, line2;

			//Get lines either side of triangle
			line1 = vectorSub(triTransformed.p[1], triTransformed.p[0]);
			line2 = vectorSub(triTransformed.p[2], triTransformed.p[0]);

			//take cross product to get normal to triangle surface
			normal = vectorCrossProduct(line1, line2);

			//normalize the normal
			normal = vectorNormalize(normal);

			vec3d vCameraRay = vectorSub(triTransformed.p[0], camera);

			if (vectorDotProduct(normal, vCameraRay) < 0.0f) {

				vec3d lightDirection = { 0.0f,1.0f,-1.0f };
				lightDirection = vectorNormalize(lightDirection);
				float dp = max(0.1f, vectorDotProduct(lightDirection, normal));
				//triTransformed.color = getColor(dp);

				//convert world space to view space
				triView.p[0] = matrixMultiplyVector(matView, triTransformed.p[0]);
				triView.p[1] = matrixMultiplyVector(matView, triTransformed.p[1]);
				triView.p[2] = matrixMultiplyVector(matView, triTransformed.p[2]);
				triView.t[0] = triTransformed.t[0];
				triView.t[1] = triTransformed.t[1];
				triView.t[2] = triTransformed.t[2];

				//clip triangles against near plane and put new triangles that creates into triClipped
				int clippedTriangles = 0;
				triangle triClipped[2];
				clippedTriangles = triangleClipPlane({ 0.0f,0.0f,0.1f }, { 0.0f,0.0f,1.0f }, triView, triClipped[0], triClipped[1]);

				//go through new clipped triangles
				for (int j = 0; j < clippedTriangles; j++) {
					//project from 3d to 2d
					triProjected.p[0] = matrixMultiplyVector(matProj, triClipped[j].p[0]);
					triProjected.p[1] = matrixMultiplyVector(matProj, triClipped[j].p[1]);
					triProjected.p[2] = matrixMultiplyVector(matProj, triClipped[j].p[2]);
					triProjected.t[0] = triClipped[j].t[0];
					triProjected.t[1] = triClipped[j].t[1];
					triProjected.t[2] = triClipped[j].t[2];

					triProjected.t[0].u = triProjected.t[0].u / triProjected.p[0].w;
					triProjected.t[1].u = triProjected.t[1].u / triProjected.p[1].w;
					triProjected.t[2].u = triProjected.t[2].u / triProjected.p[2].w;

					triProjected.t[0].v = triProjected.t[0].v / triProjected.p[0].w;
					triProjected.t[1].v = triProjected.t[1].v / triProjected.p[1].w;
					triProjected.t[2].v = triProjected.t[2].v / triProjected.p[2].w;

					triProjected.t[0].w = 1.0f / triProjected.p[0].w;
					triProjected.t[1].w = 1.0f / triProjected.p[1].w;
					triProjected.t[2].w = 1.0f / triProjected.p[2].w;

					//Scale into view
					triProjected.p[0] = vectorDiv(triProjected.p[0], triProjected.p[0].w);
					triProjected.p[1] = vectorDiv(triProjected.p[1], triProjected.p[1].w);
					triProjected.p[2] = vectorDiv(triProjected.p[2], triProjected.p[2].w);

					//un invert X/Y
					triProjected.p[0].x *= -1.0f;
					triProjected.p[1].x *= -1.0f;
					triProjected.p[2].x *= -1.0f;
					triProjected.p[0].y *= -1.0f;
					triProjected.p[1].y *= -1.0f;
					triProjected.p[2].y *= -1.0f;

					//offset vertices into visible space
					vec3d offsetView = { 1,1,0 };
					triProjected.p[0] = vectorAdd(triProjected.p[0], offsetView);
					triProjected.p[1] = vectorAdd(triProjected.p[1], offsetView);
					triProjected.p[2] = vectorAdd(triProjected.p[2], offsetView);
					triProjected.p[0].x *= 0.5f * (float)screenWidth;
					triProjected.p[0].y *= 0.5f * (float)screenHeight;
					triProjected.p[1].x *= 0.5f * (float)screenWidth;
					triProjected.p[1].y *= 0.5f * (float)screenHeight;
					triProjected.p[2].x *= 0.5f * (float)screenWidth;
					triProjected.p[2].y *= 0.5f * (float)screenHeight;

					//store triangle
					rasterTriangles.push_back(triProjected);
				}
			}
		}
	}
	return rasterTriangles;
}

//Everything that is happening on a per frame basis, triangle transformation, camera transformation, clipping, rendering, etc.
void update(float fElapsedTime) {
	mat matRotz, matRotx;
	//rotates
	//fTheta += 1.0f * fElapsedTime;

	//Rotate object
	matRotz = matrixMakeRotationZ(fTheta);
	matRotx = matrixMakeRotationX(fTheta);

	mat matTrans;
	matTrans = matrixMakeTranslation(0.0f, 0.0f, 5.0f);

	mat matWorld;
	matWorld = matrixMakeIdentity();
	matWorld = matrixMultiplyMatrix(matRotz, matRotx);
	matWorld = matrixMultiplyMatrix(matWorld, matTrans);

	//Create "Point At" matrix for the camera
	vec3d up = { 0,1,0 };
	vec3d target = { 0,0,1 };
	mat matCameraRotY = matrixMakeRotationY(fYaw);
	mat matCameraRotX = matrixMakeRotationX(fPitch);
	mat matCameraRot = matrixMultiplyMatrix(matCameraRotY, matCameraRotX);

	lookDir = matrixMultiplyVector(matCameraRot, target);
	target = vectorAdd(camera, lookDir);
	mat matCamera = matrixPointAt(camera, target, up);
	mat matView = matrixQuickInverse(matCamera);

	std::vector<triangle> triBuffer;

	for (auto& i : meshCube.tris) {
		triBuffer.push_back(i);
	}

	meshCube.recenter(2.0f, 2.0f, 0.0f);
	for (auto& i : meshCube.tris) {
		triBuffer.push_back(i);
	}

	meshCube.recenter(0.0f, 0.0f, 0.0f);

	//store the centroid and triangle vector data of all triangles I want to rasterize (ie. only triangles that can be seen by the player
	/*
	
		

	*/

	std::vector<triangle> rasterTriangles = createTriangles(triBuffer, matWorld, matView);

	//sort triangles from front to back, redundant unless translucent textures exist
	//std::sort(rasterTriangles.begin(), rasterTriangles.end(), [](triangle &t1, triangle &t2) {
	//		//sort triangles 
	//		float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
	//		float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
	//		return z1 > z2;
	//	});

	for (auto& triRaster : rasterTriangles) {
		//draw filled triangles
		triangle clipped[2];
		std::list<triangle> listTriangles;
		listTriangles.push_back(triRaster);
		int newTriangles = 1;

		for (int n = 0; n < 4; n++) {
			int trisToAdd = 0;
			while (newTriangles > 0) {
				triangle test = listTriangles.front();
				listTriangles.pop_front();
				newTriangles--;

				switch (n) {
				case 0:
					trisToAdd = triangleClipPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]);
					break;
				case 1:
					trisToAdd = triangleClipPlane({ 0.0f, (float)screenHeight - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]);
					break;
				case 2:
					trisToAdd = triangleClipPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]);
					break;
				case 3:
					trisToAdd = triangleClipPlane({ (float)screenWidth - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]);
					break;
				}
				for (int j = 0; j < trisToAdd; j++) {
					listTriangles.push_back(clipped[j]);
				}
			}
			newTriangles = listTriangles.size();
		}

		for (auto& t : listTriangles) {
			drawTexture(t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w,
				t.p[1].x, t.p[1].y, t.t[1].u, t.t[1].v, t.t[1].w,
				t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w, dirtTexture);
			//drawTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, { 255,255,255,255 });
		}
	}
}

//start of program, mostly SDL2 stuff like initializing the window/renderer and processing events
int main() {
	//Init Application SDL variables and catch errors
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Failed to initialize the SDL2 library\n";
		return -1;
	}
	window = SDL_CreateWindow("SDL2 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, 0);
	if (!window) {
		std::cout << "Failed to create window\n";
		return -1;
	}
	SDL_Event e;
	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	windowTexture = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);





	//Init variables to be used during loop
	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	//load obj file
	meshCube.loadFromObjectFile("assets/block.obj");

	dirtTexture.loadBmp("assets/dirt.bmp");

	//create variables used for camera transformation
	float fNear = 0.1f;
	float fFar = 1000.0f;
	float fov = 90.0F;
	float aspectRatio = (float)screenHeight / (float)screenWidth;
	float fovRad = 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
	matProj.m[0][0] = aspectRatio * fovRad;
	matProj.m[1][1] = fovRad;
	matProj.m[2][2] = fFar / (fFar - fNear);
	matProj.m[3][3] = 0.0f;
	matProj.m[2][3] = 1.0f;
	matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
	depthBuffer = new float[screenWidth * screenHeight];

	//Start our application loop
	bool keepRunning = true;
	float speed = 2.0f;
	int xRel = 0, yRel = 0;
	bool keyPrevState = false;
	bool sensitiviy = 5.0f;
	SDL_bool isRelativeMouse = SDL_TRUE;
	SDL_SetRelativeMouseMode(isRelativeMouse);




	while (keepRunning) {
		uint32_t startTime, frameTime;
		float fps;
		startTime = SDL_GetTicks();
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();
		vec3d forward;
		vec3d sideways;

		//handle all events
		while (SDL_PollEvent(&e) > 0) {
			switch (e.type) {
			case SDL_QUIT:
				SDL_DestroyWindow(window);
				SDL_DestroyRenderer(render);
				keepRunning = false;
				break;
			case SDL_MOUSEMOTION:
				xRel = e.motion.xrel;
				yRel = e.motion.yrel;
			}
		}

		//handle keyboard inputs
		forward = vectorMul(lookDir, (speed * 2.0f) * fElapsedTime);
		const Uint8* keyboardStateArray = SDL_GetKeyboardState(NULL);
		if (keyboardStateArray[SDL_SCANCODE_ESCAPE]) {
			if (isRelativeMouse && !keyPrevState) {
				isRelativeMouse = SDL_FALSE;
				SDL_SetRelativeMouseMode(isRelativeMouse);
			}
			else if (!isRelativeMouse && !keyPrevState) {
				isRelativeMouse = SDL_TRUE;
				SDL_SetRelativeMouseMode(isRelativeMouse);
			}
			keyPrevState = true;
		}
		if (!keyboardStateArray[SDL_SCANCODE_ESCAPE]) {
			keyPrevState = false;
		}
		if (keyboardStateArray[SDL_SCANCODE_UP]) {
			camera.y += speed * fElapsedTime;
		}
		if (keyboardStateArray[SDL_SCANCODE_DOWN]) {
			camera.y -= speed * fElapsedTime;
		}
		if (keyboardStateArray[SDL_SCANCODE_LEFT]) {
			camera.x += speed * fElapsedTime;
		}
		if (keyboardStateArray[SDL_SCANCODE_RIGHT]) {
			camera.x -= speed * fElapsedTime;
		}
		if (keyboardStateArray[SDL_SCANCODE_W]) {
			camera = vectorAdd(camera, forward);
		}
		if (keyboardStateArray[SDL_SCANCODE_S]) {
			camera = vectorSub(camera, forward);
		}
		if (keyboardStateArray[SDL_SCANCODE_A]) {
			
		}
		if (keyboardStateArray[SDL_SCANCODE_D]) {
			
		}
		if (xRel < -1 && isRelativeMouse) {
			fYaw -= std::abs(xRel) * fElapsedTime * sensitiviy;
		}
		else if (xRel > 1 && isRelativeMouse) {
			fYaw += std::abs(xRel) * fElapsedTime * sensitiviy;
		}
		if (yRel < -1 && isRelativeMouse) {
			fPitch -= std::abs(yRel) * fElapsedTime * sensitiviy;
		}
		else if(yRel > 1 && isRelativeMouse) {
			fPitch += std::abs(yRel) * fElapsedTime * sensitiviy;
		}

		//std::cout << "x: " << xRel << " " << "y: " << yRel << "\n";

		//Clear the Render by resetting it to all black
		SDL_SetRenderDrawColor(render, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(render);

		//Clear the Texture by resetting it to all black
		SDL_LockTexture(windowTexture, NULL, (void**)&lockedPixels, &pitch);
		std::vector<uint32_t> pixels(screenWidth * screenHeight, 0x00000000);
		std::copy(pixels.begin(), pixels.end(), lockedPixels);

		//Reset the depth buffer
		std::vector<float> buf(screenWidth * screenHeight, 0.0f);
		std::copy(buf.begin(), buf.end(), depthBuffer);

		//update the screen with all the triangles and textures we want to draw
		update(fElapsedTime);

		//Update the texture and render the renderer object
		SDL_UnlockTexture(windowTexture);
		SDL_RenderPresent(render);

		frameTime = SDL_GetTicks() - startTime;
		fps = (frameTime > 0) ? 1000.0f / frameTime : 0.0f;
		std::cout << fps << "\n";
	}

	return 0;
}
