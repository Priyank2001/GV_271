#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "shader_utils.h"
#include <algorithm>
#include <math.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>



using std::string;




/*
 * Struct Definitions
 */
typedef struct Vt {
	float x,y,z;
}Vertex;

typedef struct Pgn {
	int noSides;
	int *v;
}Polygon;

typedef struct offmodel {
	Vertex *vertices;
	Polygon *polygons;
	int numberOfVertices;
 	int numberOfPolygons;
}OffModel;


struct SliceVert {glm::vec3 pos,nrm;};
struct SliceTri { SliceVert v[3]; };

struct Piece {
    std::vector<SliceTri> tris;
    glm::vec3 offset;
};
/*
 * Globals
 */

OffModel* model = nullptr;
GLuint shaderProgram;
GLuint VAO, VBO, EBO;
GLint uModelLoc, uProjectionLoc;
glm::vec3 modelCenter;
float modelScale;
glm:: vec3 rotAxis;
float rotAngle = 0.0f;
const float rotSpeed = 50.0f;
int lastTime = 0;
std::vector<float > flatVertices;
std::vector<unsigned int> indices;
int winWidth = 1000;
int winHeight = 1000;
float zoomLevel = 1.0f;
const float zoomScale = 1.1f;
const float minZoom = 0.1f;
const float maxZoom = 10.0f;
bool isDarkMode = true;
bool isWireFrame = true;
int fpsFrameCount = 0;
int fpsTimerMs = 0;
double currentFPS = 0.0;
std::vector<float> faceNormals;   
bool useOffCentrePlanes = false;   
GLuint uColorLoc;
GLuint colorVBO;
GLint uUseVertexColorLoc;
bool usevertexColor=true;
std::vector<float> vertexColor;
glm::vec3 boundsMin, boundsMax;

glm::mat4 viewMatrix;
glm::vec3 cameraPos;
GLint uViewLoc, uNormalMatLoc,uViewPosLoc, uLightColorLoc, uLightPosLoc;
GLint uKaLoc, uKdLoc, uKsLoc, uShininessLoc;
GLint uNearDepthLoc, uFarDepthLoc;
GLint uSWedgeNormalALoc ;
GLint uSWedgeNormalBLoc ;
float nearDepth, farDepth;
std::vector<float> normals;
GLuint normalVBO;
GLuint flatVAO = 0, flatPosVBO = 0, flatNrmVBO = 0;
bool useFlatShading = false;

const int MAX_PLANES = 10;
int numActivePlanes = 1;
float wedgeOffsetA[2*MAX_PLANES];
float wedgeOffsetB[2*MAX_PLANES];
float wedgeCutOffset = 0.1f;   

glm::vec3 customPlaneN = glm::vec3(1.0f, 0.0f, 0.0f);
float     customPlaneW = 0.25f;
bool      useCustomPlane = false;

enum NormalMode { NORMAL_AREA_WEIGHTED, NORMAL_ANGLE_WEIGHTED };
NormalMode currentNormalMode = NORMAL_ANGLE_WEIGHTED;





enum RenderMode {MODE_WHOLE, MODE_CPU_SLICE, MODE_GPU_SLICE, MODE_FREE_PLANES};
RenderMode currentMode = MODE_WHOLE;

float sliceOffsetD = 0.6f;
int colormapId = 0;

std::vector<float> preppedPositions;
GLuint preppedVAO,preppedPosVBO;


GLuint sliceGSProgram;
GLint uSModelLoc, uSViewLoc, uSProjLoc, uSNormalMatLoc, uSNearLoc, uSFarLoc
    ,uSOffsetLoc, uSColormapIdLoc, uSUseVertexColorLoc;
GLint uSViewPosLoc, uSLightPosLoc, uSLightColorLoc,
      uSKaLoc, uSKdLoc, uSKsLoc, uSShininessLoc;
GLint uColormapIdLoc;

GLint uSWedgeOffsetALoc, uSWedgeOffsetBLoc;



int numActiveWedges = 2;
glm::vec3 wedgeNormalA[2*MAX_PLANES];
glm::vec3 wedgeNormalB[2*MAX_PLANES];
glm::vec3 wedgeOffsetDir[2*MAX_PLANES];



std::vector<float> wedgePosArr[2*MAX_PLANES], wedgeNrmArr[2*MAX_PLANES];
std::vector<unsigned int> wedgeIdxArr[2*MAX_PLANES];
GLuint wedgeVAOArr[2*MAX_PLANES]={0}, wedgeVBOArr[2*MAX_PLANES]={0},
       wedgeNVBOArr[2*MAX_PLANES]={0}, wedgeEBOArr[2*MAX_PLANES]={0};
/*
 * Helper functions
 */


extern bool benchmarking;
void startBenchmark();
void benchmarkTick();
void keyboard(unsigned char key,int x, int y);
void applyTheme();
void applyPolygonMode();
void createVertexNormals();
void rebuildNormals();
void setupCamera();
void buildPreppedWholeMesh();
void reportBufferUse(const char* where);

static SliceVert lerpVert(const SliceVert& a, const SliceVert& b, float t);
void rebuildWedgeGeometryDefs();
std::vector<SliceTri> clipListAgainstPlane(const std::vector<SliceTri> &in, const glm::vec3 &n,float w);
void buildAllWedgesCPU(const glm::mat4& rotOnly, const glm::mat3& normalRotOnly);
void buildArrangementCPU(const glm::mat4& rotOnly, const glm::mat3& normalRotOnly);
void initFreePlanes();

glm::vec3 freePlaneNormal[MAX_PLANES];
float     freePlaneW[MAX_PLANES];
std::vector<std::vector<float>> pieceP, pieceN;
std::vector<std::vector<unsigned int>> pieceI;
std::vector<GLuint> pieceVAO, pieceVBO, pieceNVBO, pieceEBO;
int numActivePieces = 0;
const int MAX_PIECES = 64;



static void addTri(std::vector<float>& pos, std::vector<float>& nrm, std::vector<unsigned int>& idx,
                    const SliceVert& a, const SliceVert& b, const SliceVert& c, const glm::vec3& offset);

void uploadSliceVAO(GLuint& vao, GLuint& vbo, GLuint& nvbo, GLuint& ebo,
                     const std::vector<float>& pos, const std::vector<float>& nrm,
                     const std::vector<unsigned int>& idx);



static std::string readTextFile(const char* path){
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compileStage(GLenum type, const char* path){
    std::string src = readTextFile(path);
    const char* csrc = src.c_str();
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &csrc, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){
        char log[2048]; glGetShaderInfoLog(s, 2048, nullptr, log);
        std::cerr << "Shader compile error (" << path << "):\n" << log << std::endl;
    }
    return s;
}

GLuint createShaderProgramGS(const char* vsPath, const char* gsPath, const char* fsPath){
    GLuint vs = compileStage(GL_VERTEX_SHADER, vsPath);
    GLuint gs = compileStage(GL_GEOMETRY_SHADER, gsPath);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsPath);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, gs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok){
        char log[2048]; glGetProgramInfoLog(prog, 2048, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }
    glDeleteShader(vs); glDeleteShader(gs); glDeleteShader(fs);
    return prog;
}



void initRandomAxis(){
    srand((unsigned) time(nullptr));
    const glm::vec3 viewDir(0.0f, 0.0f, 1.0f);
    const float cosLimit = cosf(glm::radians(32.0f));
    glm::vec3 axis;
        do {
        axis = glm::vec3(((float)rand()/RAND_MAX)*2-1,
                         ((float)rand()/RAND_MAX)*2-1,
                         ((float)rand()/RAND_MAX)*2-1);
        if(glm::length(axis) < 1e-6f){ axis = glm::vec3(0,0,1); continue; }
        axis = glm::normalize(axis);
    } while(fabsf(glm::dot(axis, viewDir)) > cosLimit);
    rotAxis = axis;
    std::cout << "Rotation Axis: (" << rotAxis.x << ", " << rotAxis.y << ", " << rotAxis.z << ")\n";
}


void timer(int value){
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float delta = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    rotAngle = rotAngle + rotSpeed * delta;
    if(rotAngle > 360.0f) rotAngle -= 360.0f;

    glutPostRedisplay();
    glutTimerFunc(0, timer , 0 );
}

void createVertexBuffers(){
    flatVertices.clear();
    flatVertices.reserve(model->numberOfVertices * 3);
    for(int i = 0 ; i < model->numberOfVertices ; i++){
        Vertex v = model->vertices[i];
        flatVertices.push_back(v.x);
        flatVertices.push_back(v.y);
        flatVertices.push_back(v.z);
    }

    vertexColor.clear();
    vertexColor.reserve(3 * model->numberOfVertices);
    glm::vec3 extent = boundsMax - boundsMin;

    for(int i = 0 ; i < model-> numberOfVertices ; i++){
        Vertex v = model->vertices[i];
        // 1e-6f protects divide by zero error for a perfectly flat mesh along some axis
        float r = (extent.x > 1e-6f) ? (v.x - boundsMin.x) / extent.x : 0.5f;
        float g = (extent.y > 1e-6f) ? (v.y - boundsMin.y) / extent.y : 0.5f;
        float b = (extent.z > 1e-6f) ? (v.z - boundsMin.z) / extent.z : 0.5f;
        vertexColor.push_back(r);
        vertexColor.push_back(g);
        vertexColor.push_back(b);
    }

    indices.clear();
    for(int i = 0 ; i < model->numberOfPolygons ; i++){
        Polygon polygon = model->polygons[i];
        for(int j = 1 ; j < polygon.noSides - 1 ; j++){
            indices.push_back(polygon.v[0]);
            indices.push_back(polygon.v[j]);
            indices.push_back(polygon.v[j+1]);
            
        }
    }
    
    createVertexNormals();
    

    std::cout << "Vertices: " << flatVertices.size()/3 << " Indices: " << indices.size() << std::endl;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);


    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float) , flatVertices.data(),GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE, 3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexColor.size() * sizeof(float), vertexColor.data() , GL_STATIC_DRAW);
    glVertexAttribPointer(1,3,GL_FLOAT, GL_FALSE,3 * sizeof(float) , (void*) 0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(float), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);


    glBindVertexArray(0);

    std::cout << "Buffers Built\n"; 


}


void buildFlatShadedMesh(){
    if(faceNormals.size() != indices.size()){
        std::cerr << "buildFlatShadedMesh: faceNormals/indices mismatch ("
                  << faceNormals.size()/3 << " faces vs " << indices.size()/3
                  << " triangles)\n";
        return;
    }
    std::vector<float> pos, nrm;
    pos.reserve(indices.size()*3); nrm.reserve(indices.size()*3);
    size_t f = 0;
    for(size_t t = 0; t < indices.size(); t += 3, f++){
        for(int k = 0; k < 3; k++){
            unsigned int vi = indices[t+k];
            pos.push_back(preppedPositions[3*vi]);
            pos.push_back(preppedPositions[3*vi+1]);
            pos.push_back(preppedPositions[3*vi+2]);
            nrm.push_back(faceNormals[3*f]);
            nrm.push_back(faceNormals[3*f+1]);
            nrm.push_back(faceNormals[3*f+2]);
        }
    }
    if(flatVAO) glDeleteVertexArrays(1,&flatVAO);
    if(flatPosVBO) glDeleteBuffers(1,&flatPosVBO);
    if(flatNrmVBO) glDeleteBuffers(1,&flatNrmVBO);
    glGenVertexArrays(1,&flatVAO); glBindVertexArray(flatVAO);
    glGenBuffers(1,&flatPosVBO); glBindBuffer(GL_ARRAY_BUFFER, flatPosVBO);
    glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(float), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1,&flatNrmVBO); glBindBuffer(GL_ARRAY_BUFFER, flatNrmVBO);
    glBufferData(GL_ARRAY_BUFFER, nrm.size()*sizeof(float), nrm.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void ComputeModelTransform(){
    float minX, maxX, minY, maxY, minZ, maxZ;
    minX = maxX = model->vertices[0].x;
    minY = maxY = model->vertices[0].y;
    minZ = maxZ = model->vertices[0].z;

    for(int i = 1; i < model->numberOfVertices; i++){
        Vertex v = model->vertices[i];
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
    }

    modelCenter = glm::vec3((minX+maxX)/2.0f, (minY+maxY)/2.0f, (minZ+maxZ)/2.0f);
    boundsMin = glm::vec3(minX,minY,minZ);
    boundsMax = glm::vec3(maxX,maxY,maxZ);

    float extentX = maxX - minX;
    float extentY = maxY - minY;
    float extentZ = maxZ - minZ;
    float maxExtent = std::max(extentX, std::max(extentY, extentZ));
    modelScale = (maxExtent > 0) ? (1.6f / maxExtent) : 1.0f;

    std::cout << "Center: (" << modelCenter.x << ", " << modelCenter.y << ", " << modelCenter.z << ")"
              << " Scale: " << modelScale << std::endl;
}
const char* currentModeName(){
    switch(currentMode){
        case MODE_WHOLE:      return "Whole Mesh";
        case MODE_CPU_SLICE:  return "CPU Slice";
        case MODE_GPU_SLICE:  return "GPU Slice";
        case MODE_FREE_PLANES: return "Free Planes";
    }
    return "Unknown";
}

void renderBitmapString(int x, int y, void* font, const char* str){
    glWindowPos2i(x, y);
    for(const char* c = str; *c != '\0'; c++) glutBitmapCharacter(font, *c);
}

void renderHUD(){
    char hudText[256];
    snprintf(hudText, sizeof(hudText),
             "Mode: %s | Planes: %d | Normals: %s | Shading: %s | FPS: %.1f",
             currentModeName(), numActivePlanes,
             (currentNormalMode == NORMAL_AREA_WEIGHTED ? "Area" : "Angle"),
             (useFlatShading ? "Flat" : "Smooth"),
             currentFPS);

    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);     
    float hudGrey = isDarkMode ? 0.92f : 0.10f;
    glColor3f(hudGrey, hudGrey, hudGrey);   
    renderBitmapString(10, winHeight - 20, GLUT_BITMAP_9_BY_15, hudText);

    glEnable(GL_DEPTH_TEST);    
}


void display(){
    fpsFrameCount++;
    int wedgeCount = useCustomPlane ? 2 : numActiveWedges;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if(nowMs - fpsTimerMs >= 500){   
        currentFPS = 1000.0 * fpsFrameCount / (double)(nowMs - fpsTimerMs);
        fpsFrameCount = 0;
        fpsTimerMs = nowMs;
    }

    if(benchmarking) benchmarkTick();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 rotOnly = glm::rotate(glm::mat4(1.0f), glm::radians(rotAngle), rotAxis);
    glm::mat3 normalRotOnly = glm::mat3(rotOnly); 



    if(currentMode == MODE_WHOLE){
        applyPolygonMode();
        glUseProgram(shaderProgram);
        glUniform1i(uUseVertexColorLoc, usevertexColor);
        if(useFlatShading){
            glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(rotOnly));
            glUniformMatrix3fv(uNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalRotOnly));
            glBindVertexArray(flatVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)indices.size());
        }
        else{
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(rotAngle), rotAxis);
            modelMat = glm::scale(modelMat, glm::vec3(modelScale));
            modelMat = glm::translate(modelMat, -modelCenter);
            glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));
            glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
            glUniformMatrix3fv(uNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        }
    }
    else if(currentMode == MODE_CPU_SLICE){
        buildAllWedgesCPU(rotOnly, normalRotOnly);

        glUseProgram(shaderProgram);
        static const glm::mat4 identity4(1.0f);
        static const glm::mat3 identity3(1.0f);
        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(identity4));
        glUniformMatrix3fv(uNormalMatLoc, 1, GL_FALSE, glm::value_ptr(identity3));
        glUniform1i(uUseVertexColorLoc, GL_TRUE);

        for(int k=0;k<numActiveWedges;k++){
            bool wireframeThis = (numActivePlanes == 1 && k == 1);  
            glPolygonMode(GL_FRONT_AND_BACK, wireframeThis ? GL_LINE : GL_FILL);
            glBindVertexArray(wedgeVAOArr[k]);
            glDrawElements(GL_TRIANGLES, (GLsizei)wedgeIdxArr[k].size(), GL_UNSIGNED_INT, 0);
        }
    }
    else if(currentMode == MODE_FREE_PLANES){
        buildArrangementCPU(rotOnly, normalRotOnly);
        glUseProgram(shaderProgram);
        static const glm::mat4 identity4(1.0f);
        static const glm::mat3 identity3(1.0f);
        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(identity4));
        glUniformMatrix3fv(uNormalMatLoc, 1, GL_FALSE, glm::value_ptr(identity3));
        glUniform1i(uUseVertexColorLoc, GL_TRUE);
        for(int k = 0; k < numActiveWedges; k++){
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(pieceVAO[k]);
            glDrawElements(GL_TRIANGLES, (GLsizei)pieceI[k].size(), GL_UNSIGNED_INT, 0);
        }
    }
    else { // MODE_GPU_SLICE
        glUseProgram(sliceGSProgram);
        glUniformMatrix4fv(uSModelLoc, 1, GL_FALSE, glm::value_ptr(rotOnly));
        glUniformMatrix3fv(uSNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalRotOnly));
        glUniform1i(uSUseVertexColorLoc, GL_TRUE);
        glBindVertexArray(preppedVAO);
        for(int k=0;k<wedgeCount;k++){
            glUniform3fv(uSWedgeNormalALoc, 1, glm::value_ptr(wedgeNormalA[k]));
            glUniform3fv(uSWedgeNormalBLoc, 1, glm::value_ptr(wedgeNormalB[k]));
            glUniform1f(uSWedgeOffsetALoc, wedgeOffsetA[k]); 
            glUniform1f(uSWedgeOffsetBLoc, wedgeOffsetB[k]);
            glUniform3fv(uSOffsetLoc, 1, glm::value_ptr((sliceOffsetD*0.5f) * wedgeOffsetDir[k]));
            bool wireframeThis = (numActivePlanes == 1 && k == 1);
            glPolygonMode(GL_FRONT_AND_BACK, wireframeThis ? GL_LINE : GL_FILL);
            glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    renderHUD();
    glutSwapBuffers();
}


void updateProjection(){
    int h = winHeight == 0 ? 1 : winHeight;
    float aspect = float(winWidth) / float(winHeight);
    float base = 2.0f / zoomLevel;

    glm::mat4 projection;
    if(aspect >= 1.0f){
        projection = glm::ortho(-base * aspect, base *aspect,-base, base , -10.f , 10.0f);
    }
    else{
        projection = glm::ortho(-base,base , -base/aspect , base / aspect , -10.f,10.f);
    }

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(uProjectionLoc, 1 , GL_FALSE, glm::value_ptr(projection));
    glUseProgram(sliceGSProgram);
    glUniformMatrix4fv(uSProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void reshape(int w,int h){
    glViewport(0,0,w,h);
    winWidth = w;
    winHeight = (h == 0) ? 1 : h;
    updateProjection();        
}

OffModel* readOffFile(char * OffFile) {
	FILE * input;
	char type[4]; 
	int noEdges;
	int i,j;
	float x,y,z;
	int n, v;
	int nv, np;
	OffModel *model;
    
    input = fopen(OffFile, "r");  
    if(!input){ fprintf(stderr, "Cannot open OFF file: %s\n", OffFile); exit(1); }
	fscanf(input, "%s", type);
	/* First line should be OFF */
	if(strcmp(type,"OFF")) {
		printf("Not a OFF file");
		exit(1);
	}
	/* Read the no. of vertices, faces and edges */
	fscanf(input, "%d", &nv);
	fscanf(input, "%d", &np);
	fscanf(input, "%d", &noEdges);

	model = (OffModel*)malloc(sizeof(OffModel));
	model->numberOfVertices = nv;
	model->numberOfPolygons = np;
	
	
	/* allocate required data */
	model->vertices = (Vertex*) malloc(nv * sizeof(Vertex));
	model->polygons = (Polygon*) malloc(np * sizeof(Polygon));
	

	/* Read the vertices' location*/	
	for(i = 0;i < nv;i ++) {
		fscanf(input, "%f %f %f", &x,&y,&z);
		(model->vertices[i]).x = x;
		(model->vertices[i]).y = y;
		(model->vertices[i]).z = z;
	}

	/* Read the Polygons */	
	for(i = 0;i < np;i ++) {
		/* No. of sides of the polygon (Eg. 3 => a triangle) */
		fscanf(input, "%d", &n);
		
		(model->polygons[i]).noSides = n;
		(model->polygons[i]).v = (int *) malloc(n * sizeof(int));
		/* read the vertices that make up the polygon */
		for(j = 0;j < n;j ++) {
			fscanf(input, "%d", &v);
			(model->polygons[i]).v[j] = v;
		}
	}

	fclose(input);
	return model;
}

void printOffModel(OffModel* model) {
	int i, j;
    //printf("OFF\n");
	printf("%d %d 0 \n", model->numberOfVertices, model->numberOfPolygons);
	/*
	for(i = 0; i < model->numberOfVertices;i ++) {
		printf("%f %f %f \n", (model->vertices[i]).x, (model->vertices[i]).y, (model->vertices[i]).z);
	}
	for(i = 0;i < model->numberOfPolygons;i ++) {
		printf("%d ", (model->polygons[i]).noSides);
		for(j = 0;j < (model->polygons[i]).noSides;j ++) {
			printf("%d ", (model->polygons[i]).v[j]);
		}
		printf("\n");
	}
	*/

}

int FreeOffModel(OffModel* model) {
	int i,j;
	if(model == NULL){
		return 0;
	}
	free(model->vertices);
	for(i = 0; i < model->numberOfPolygons; ++i ){
		if((model->polygons[i]).v){
			free((model->polygons[i]).v);
		}
	}
	free(model->polygons);
	free(model);
	return 1;
}


void printVertex(Vertex v,std::string start,std::string end){
	std::cout << start << " x : " << v.x << " y : " << v.y << " z : " << v.z << end; 
    
}

void printHead(OffModel* model,int count = 3){
	if(model == nullptr){
		printf("The model is null.\n");
		return;
	}
	int ct = std::min(count,model->numberOfVertices);
	// printing vertices
	printf("Printing top %d vertices\n",ct);
	for(int i = 0 ; i < ct ; i++){
		printVertex(model->vertices[i]," ","\n");
	}
	ct = std::min(count , model->numberOfPolygons);
	printf("Printing top %d polygons\n",ct);
	for(int i = 0 ; i < ct ; i++){
		printf("Polygon number:%d\n",i + 1);
		for(int x = 0 ; x < (model->polygons[i]).noSides ; x++){
			int index = (model->polygons[i]).v[x];
			printVertex(model->vertices[index],"\t","\n");
		}
	}
	return;
}



 
signed main(int argc,char** argv){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <offfile>\n", argv[0]);
        exit(1);
    } 
    

    model = readOffFile(argv[1]);
	printOffModel(model);
	printHead(model);
	glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Mesh Viewer - Step 1: Load & Display");

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW init failed: " << glewGetErrorString(glewErr) << "\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    shaderProgram = createShaderProgram("./shader.vs", "./shader.fs");


    uModelLoc = glGetUniformLocation(shaderProgram,"uModel");
    uProjectionLoc = glGetUniformLocation(shaderProgram,"uProjection");
    uColorLoc = glGetUniformLocation(shaderProgram,"uColor");
    uUseVertexColorLoc = glGetUniformLocation(shaderProgram,"uUseVertexColor");

    uViewLoc       = glGetUniformLocation(shaderProgram, "uView");
    uNormalMatLoc  = glGetUniformLocation(shaderProgram, "uNormalMatrix");
    uViewPosLoc    = glGetUniformLocation(shaderProgram, "uViewPos");
    uLightPosLoc   = glGetUniformLocation(shaderProgram, "uLightPos");
    uLightColorLoc = glGetUniformLocation(shaderProgram, "uLightColor");
    uKaLoc         = glGetUniformLocation(shaderProgram, "uKa");
    uKdLoc         = glGetUniformLocation(shaderProgram, "uKd");
    uKsLoc         = glGetUniformLocation(shaderProgram, "uKs");
    uShininessLoc  = glGetUniformLocation(shaderProgram, "uShininess");
    uNearDepthLoc  = glGetUniformLocation(shaderProgram, "uNearDepth");
    uFarDepthLoc   = glGetUniformLocation(shaderProgram, "uFarDepth");
    
    
    ComputeModelTransform();
    setupCamera();
    initRandomAxis();
    createVertexBuffers();
    
    initFreePlanes();
    buildPreppedWholeMesh();
    buildFlatShadedMesh();
    rebuildWedgeGeometryDefs();
    
    
    sliceGSProgram      = createShaderProgramGS("./slice.vs", "./slice.gs", "./shader.fs");
    uSModelLoc          = glGetUniformLocation(sliceGSProgram, "uModel");
    uSViewLoc           = glGetUniformLocation(sliceGSProgram, "uView");
    uSProjLoc           = glGetUniformLocation(sliceGSProgram, "uProjection");
    uSNormalMatLoc      = glGetUniformLocation(sliceGSProgram, "uNormalMatrix");
    uSNearLoc           = glGetUniformLocation(sliceGSProgram, "uNearDepth");
    uSFarLoc            = glGetUniformLocation(sliceGSProgram, "uFarDepth");
    uSWedgeNormalALoc   = glGetUniformLocation(sliceGSProgram, "uWedgeNormalA");
    uSWedgeNormalBLoc   = glGetUniformLocation(sliceGSProgram, "uWedgeNormalB");
    uSOffsetLoc         = glGetUniformLocation(sliceGSProgram, "uOffset");
    uSColormapIdLoc     = glGetUniformLocation(sliceGSProgram, "uColormapId");
    uColormapIdLoc      = glGetUniformLocation(shaderProgram, "uColormapId");
    uSUseVertexColorLoc = glGetUniformLocation(sliceGSProgram, "uUseVertexColor");
    uSViewPosLoc        = glGetUniformLocation(sliceGSProgram, "uViewPos");
    uSLightPosLoc       = glGetUniformLocation(sliceGSProgram, "uLightPos");
    uSLightColorLoc     = glGetUniformLocation(sliceGSProgram, "uLightColor");
    uSKaLoc             = glGetUniformLocation(sliceGSProgram, "uKa");
    uSKdLoc             = glGetUniformLocation(sliceGSProgram, "uKd");
    uSKsLoc             = glGetUniformLocation(sliceGSProgram, "uKs");
    uSShininessLoc      = glGetUniformLocation(sliceGSProgram, "uShininess");
    uSWedgeOffsetALoc   = glGetUniformLocation(sliceGSProgram, "uWedgeOffsetA");
    uSWedgeOffsetBLoc   = glGetUniformLocation(sliceGSProgram, "uWedgeOffsetB");
    
    glUseProgram(sliceGSProgram);
    glUniform3fv(uSViewPosLoc, 1, glm::value_ptr(cameraPos));
    glUniform3f(uSLightPosLoc, cameraPos.x + 1.0f, cameraPos.y + 2.0f, cameraPos.z + 1.0f);
    glUniform3f(uSLightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform1f(uSKaLoc, 0.15f);
    glUniform1f(uSKdLoc, 0.70f);
    glUniform1f(uSKsLoc, 0.50f);
    glUniform1f(uSShininessLoc, 32.0f);
    glUseProgram(sliceGSProgram);
    glUniformMatrix4fv(uSViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniform1f(uSNearLoc, nearDepth);
    glUniform1f(uSFarLoc, farDepth);
    glUniform1i(uSColormapIdLoc, colormapId);
        


    applyTheme();
    applyPolygonMode();
    glUseProgram(shaderProgram);
    glUniform1i(uUseVertexColorLoc, usevertexColor);
    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniform3fv(uViewPosLoc, 1, glm::value_ptr(cameraPos));
    glUniform1f(uNearDepthLoc, nearDepth);
    glUniform1f(uFarDepthLoc,  farDepth);
    glUniform3f(uLightPosLoc, cameraPos.x + 1.0f, cameraPos.y + 2.0f, cameraPos.z + 1.0f);
    glUniform3f(uLightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform1f(uKaLoc, 0.15f);
    glUniform1f(uKdLoc, 0.70f);
    glUniform1f(uKsLoc, 0.50f);
    glUniform1f(uShininessLoc, 32.0f);
    


    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);



    lastTime = glutGet(GLUT_ELAPSED_TIME);
    fpsTimerMs = lastTime;   
    glutTimerFunc(0,timer,0);
    glutMainLoop();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &colorVBO);
    FreeOffModel(model);
    
    

	return 0;
}

/*
 * Implementations
 */

void keyboard(unsigned char key,int x,int y){
    
    switch(key){
        case '1': currentMode = MODE_WHOLE; break;
        case '2': currentMode = MODE_CPU_SLICE; break;
        case '3': currentMode = MODE_GPU_SLICE; break;
        case '4': currentMode = MODE_FREE_PLANES; break;
        case 'v':
            colormapId = 1 - colormapId;
            glUseProgram(shaderProgram);   glUniform1i(uColormapIdLoc, colormapId);
            glUseProgram(sliceGSProgram);  glUniform1i(uSColormapIdLoc, colormapId);
            break;
        case '+': case '=':
            zoomLevel = std::min(zoomScale*zoomLevel , maxZoom);
            updateProjection();
            glutPostRedisplay();
            break;
        case '-': case '_':
            zoomLevel = std::max(zoomLevel/zoomScale , minZoom);
            updateProjection();
            glutPostRedisplay();
            break;
        case 'd': case 'D':
            isDarkMode = !isDarkMode;
            applyTheme();
            glutPostRedisplay();
            break;
        case 'f': case 'F':
            isWireFrame = !isWireFrame;
            glutPostRedisplay();
            applyPolygonMode();
            break;
        case 'c': case 'C':
            usevertexColor = !usevertexColor;
            glUseProgram(shaderProgram);
            glUniform1i(uUseVertexColorLoc, usevertexColor);  
            glutPostRedisplay();
            break;
        case 'j':
            numActivePlanes = std::max(1, numActivePlanes - 1);
            rebuildWedgeGeometryDefs();
            std::cout << "Active cutting planes: " << numActivePlanes << "\n";
            glutPostRedisplay();
            break;
        case 'k':
            numActivePlanes = std::min(
                (currentMode == MODE_FREE_PLANES) ? 4 : MAX_PLANES,
                numActivePlanes + 1);
            rebuildWedgeGeometryDefs();
            std::cout << "Active cutting planes: " << numActivePlanes << "\n";
            glutPostRedisplay();
            break;
        case 'b': case 'B':
            if(!benchmarking) startBenchmark();
            break;
        case 'n': case 'N':
            currentNormalMode = (currentNormalMode == NORMAL_AREA_WEIGHTED)
                                ? NORMAL_ANGLE_WEIGHTED
                                : NORMAL_AREA_WEIGHTED;
            rebuildNormals();
            glutPostRedisplay();
            break;
        case 'm': case 'M':
            useFlatShading = !useFlatShading;
            std::cout << "Shading: " << (useFlatShading ? "Flat (face normals)"
                                                        : "Smooth (vertex normals)") << "\n";
            glutPostRedisplay();
            break;
        case 'o': case 'O':
            useOffCentrePlanes = !useOffCentrePlanes;
            rebuildWedgeGeometryDefs();
            std::cout << "Cutting planes: "
                      << (useOffCentrePlanes ? "off-centre" : "through centre") << "\n";
            glutPostRedisplay();
            break;
                case 'p': case 'P':
            useCustomPlane = !useCustomPlane;
            std::cout << "Custom plane: " << (useCustomPlane ? "ON" : "OFF") << "\n";
            glutPostRedisplay();
            break;
        
    }
    
};

void applyTheme(){
    if(isDarkMode){
        glClearColor(0.08f, 0.08f, 0.09f, 1.0f);   // near-black background
        glUseProgram(shaderProgram);
        glUniform3f(uColorLoc, 0.92f, 0.92f, 0.92f); // light lines for contrast
    } else {
        glClearColor(0.95f, 0.95f, 0.95f, 1.0f);   // near-white background
        glUseProgram(shaderProgram);
        glUniform3f(uColorLoc, 0.10f, 0.10f, 0.10f); // dark lines for contrast
    }
}

void applyPolygonMode(){
    glPolygonMode(GL_FRONT_AND_BACK, isWireFrame ? GL_LINE : GL_FILL);
}

void createVertexNormals(){
    normals.assign(model->numberOfVertices * 3, 0.0f);
    faceNormals.clear();
    faceNormals.reserve(indices.size());
    for(size_t t = 0; t < indices.size(); t += 3){
        unsigned int i0 = indices[t], i1 = indices[t+1], i2 = indices[t+2];
        glm::vec3 p0(model->vertices[i0].x, model->vertices[i0].y, model->vertices[i0].z);
        glm::vec3 p1(model->vertices[i1].x, model->vertices[i1].y, model->vertices[i1].z);
        glm::vec3 p2(model->vertices[i2].x, model->vertices[i2].y, model->vertices[i2].z);

        glm::vec3 cross = glm::cross(p1 - p0, p2 - p0);
        float area2 = glm::length(cross);   // 2 * triangle area

        if(area2 < 1e-10f){
            faceNormals.push_back(0.0f);
            faceNormals.push_back(1.0f);
            faceNormals.push_back(0.0f);
            continue;
        }
        glm::vec3 faceNormal = cross / area2;   // unit normal (reuse cross/length)

        faceNormals.push_back(faceNormal.x);
        faceNormals.push_back(faceNormal.y);
        faceNormals.push_back(faceNormal.z);

        unsigned int verts[3] = { i0, i1, i2 };
        glm::vec3 pts[3] = { p0, p1, p2 };

        for(int k = 0; k < 3; k++){
            float weight = 1.0f;

            if(currentNormalMode == NORMAL_AREA_WEIGHTED){
                weight = area2 * 0.5f;          // actual triangle area
            }
            else { // NORMAL_ANGLE_WEIGHTED
                glm::vec3 ea = glm::normalize(pts[(k+1)%3] - pts[k]);
                glm::vec3 eb = glm::normalize(pts[(k+2)%3] - pts[k]);
                float cosA = glm::clamp(glm::dot(ea, eb), -1.0f, 1.0f);
                weight = acosf(cosA);           // interior angle at this vertex
            }

            normals[3*verts[k]]     += faceNormal.x * weight;
            normals[3*verts[k] + 1] += faceNormal.y * weight;
            normals[3*verts[k] + 2] += faceNormal.z * weight;
        }
    }

    for(int i = 0; i < model->numberOfVertices; i++){
        glm::vec3 n(normals[3*i], normals[3*i+1], normals[3*i+2]);
        float len = glm::length(n);
        n = (len > 1e-8f) ? n / len : glm::vec3(0, 1, 0);
        normals[3*i] = n.x; normals[3*i+1] = n.y; normals[3*i+2] = n.z;
    }

    std::cout << "Normals built: "
              << (currentNormalMode == NORMAL_AREA_WEIGHTED ? "Area-Weighted" : "Angle-Weighted")
              << "\n";
}

void rebuildNormals(){
    createVertexNormals();
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * sizeof(float), normals.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void setupCamera(){
    float boundingRadius = 0.5f * glm::length(boundsMax - boundsMin) * modelScale;
    float camDist = boundingRadius * 3.0f + 2.0f; 
    cameraPos = glm::vec3(0.0f,0.0f,camDist);
    viewMatrix = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f,1.0f,0.0f));
    nearDepth = camDist - boundingRadius;
    farDepth  = camDist + boundingRadius;
}

void buildPreppedWholeMesh(){
    preppedPositions.resize(flatVertices.size());
    for(size_t i = 0; i < flatVertices.size(); i += 3){
        glm::vec3 v(flatVertices[i], flatVertices[i+1], flatVertices[i+2]);
        glm::vec3 p = modelScale * (v - modelCenter);
        preppedPositions[i] = p.x; preppedPositions[i+1] = p.y; preppedPositions[i+2] = p.z;
    }
    glGenVertexArrays(1, &preppedVAO);
    glBindVertexArray(preppedVAO);

    glGenBuffers(1, &preppedPosVBO);
    glBindBuffer(GL_ARRAY_BUFFER, preppedPosVBO);
    glBufferData(GL_ARRAY_BUFFER, preppedPositions.size()*sizeof(float), preppedPositions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);           
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0); 
    glEnableVertexAttribArray(2);                    

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);     
    glBindVertexArray(0);
}

static SliceVert lerpVert(const SliceVert& a,const SliceVert& b,float t){
    SliceVert r;
    r.pos = glm::mix(a.pos,b.pos,t);
    r.nrm = glm::normalize(glm::mix(a.nrm,b.nrm, t));
    return r;
}

static void addTri(std::vector<float>& pos, std::vector<float>& nrm, std::vector<unsigned int>& idx
    , const SliceVert& a,const SliceVert& b,const SliceVert& c, const glm::vec3& offset){
        unsigned int base = (unsigned int)(pos.size()/3);
        for(const SliceVert* v : {&a, &b, &c}){
            glm::vec3 p = v->pos + offset;

            pos.push_back(p.x);
            pos.push_back(p.y);
            pos.push_back(p.z); 

            nrm.push_back(v->nrm.x);
            nrm.push_back(v->nrm.y);
            nrm.push_back(v->nrm.z);

        }
    idx.push_back(base); idx.push_back(base+1); idx.push_back(base+2);
}

void uploadSliceVAO(GLuint& vao, GLuint& vbo, GLuint& nvbo, GLuint& ebo,
                     const std::vector<float>& pos, const std::vector<float>& nrm,
                     const std::vector<unsigned int>& idx){
    if(vao) glDeleteVertexArrays(1,&vao);
    if(vbo) glDeleteBuffers(1,&vbo);
    if(nvbo) glDeleteBuffers(1,&nvbo);
    if(ebo) glDeleteBuffers(1,&ebo);

    glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(float), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1,&nvbo); glBindBuffer(GL_ARRAY_BUFFER, nvbo);
    glBufferData(GL_ARRAY_BUFFER, nrm.size()*sizeof(float), nrm.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(2);

    glGenBuffers(1,&ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void rebuildWedgeGeometryDefs(){
    int M = numActivePlanes;
    numActiveWedges = 2*M;
    std::vector<float> rayAngles;
    rayAngles.reserve(2*M);
    for(int i=0;i<M;i++){
        float theta = i * (3.14159265f / M);   // M lines, evenly spaced over [0,pi)
        rayAngles.push_back(theta);
        rayAngles.push_back(theta + 3.14159265f);   // opposite ray -> fills [pi,2pi)
    }
    std::sort(rayAngles.begin(), rayAngles.end());   // 2M rays, evenly spaced over [0,2pi)

    for(int k=0;k<2*M;k++){
        float phiK  = rayAngles[k];
        float phiK1 = (k+1 < 2*M) ? rayAngles[k+1] : rayAngles[0] + 2.0f*3.14159265f;
        glm::vec3 rK(cosf(phiK), sinf(phiK), 0.0f);
        glm::vec3 rK1(cosf(phiK1), sinf(phiK1), 0.0f);

        
        wedgeNormalA[k] = glm::vec3(-rK.y,  rK.x, 0.0f);
        wedgeNormalB[k] = glm::vec3( rK1.y, -rK1.x, 0.0f);

        float slide = useOffCentrePlanes ? wedgeCutOffset : 0.0f;
        wedgeOffsetA[k] =  slide * cosf(2.0f * phiK);
        wedgeOffsetB[k] = -slide * cosf(2.0f * phiK1);
        
        float bisector = 0.5f*(phiK + phiK1);
        wedgeOffsetDir[k] = glm::vec3(cosf(bisector), sinf(bisector), 0.0f);
    }
}
std::vector<SliceTri> clipListAgainstPlane(const std::vector<SliceTri> &in, const glm::vec3 &n,float w){
    std::vector<SliceTri> out;
    out.reserve(in.size());
    for (const auto &tri : in){
        float d[3];
        for(int k = 0; k < 3 ; k++)d[k] = glm::dot(tri.v[k].pos , n) - w;
        int numPos = (d[0]>0)+(d[1]>0)+(d[2]>0);
        if(numPos == 3){
            // Completely in posPlane
            out.push_back(tri);
            continue;
        } 
        else if(numPos == 0){
            // completely in negPlane
            continue;
        }
        int lone = (d[0]>0)==(d[1]>0) ? 2 : ((d[0]>0)==(d[2]>0) ? 1 : 0);
        int a=lone,b=(lone+1)%3,c=(lone+2)%3;
        bool loneIsPos = d[a] > 0;
        float tb=d[a]/(d[a]-d[b]); float tc=d[a]/(d[a]-d[c]);
        SliceVert ib_ = lerpVert(tri.v[a], tri.v[b], tb);
        SliceVert ic_ = lerpVert(tri.v[a], tri.v[c], tc);
        if(loneIsPos){
            out.push_back({{ tri.v[a], ib_, ic_ }});
        } else {
            out.push_back({{ ib_, tri.v[b], tri.v[c] }});
            out.push_back({{ ib_, tri.v[c], ic_ }});
        }
    }
    return out;
}


void buildAllWedgesCPU(const glm::mat4& rotOnly, const glm::mat3& normalRotOnly){

    int wedgeCount = useCustomPlane ? 2 : numActiveWedges;
    
    std::vector<SliceTri> allTris;
    allTris.reserve(indices.size()/3);
    for(size_t t=0;t<indices.size();t+=3){
        unsigned int idx3[3] = {indices[t], indices[t+1], indices[t+2]};
        SliceTri tri;
        for(int k=0;k<3;k++){
            glm::vec3 lp(preppedPositions[3*idx3[k]], preppedPositions[3*idx3[k]+1], preppedPositions[3*idx3[k]+2]);
            glm::vec3 ln(normals[3*idx3[k]], normals[3*idx3[k]+1], normals[3*idx3[k]+2]);
            tri.v[k].pos = glm::vec3(rotOnly * glm::vec4(lp,1.0f));
            tri.v[k].nrm = glm::normalize(normalRotOnly * ln);
        }
        allTris.push_back(tri);
    }

      for(int k=0;k<numActiveWedges;k++){
        auto afterA = clipListAgainstPlane(allTris, wedgeNormalA[k], wedgeOffsetA[k]);
        auto afterB = clipListAgainstPlane(afterA,  wedgeNormalB[k], wedgeOffsetB[k]);

        auto& pos = wedgePosArr[k]; auto& nrm = wedgeNrmArr[k]; auto& idx = wedgeIdxArr[k];
        pos.clear(); nrm.clear(); idx.clear();
        glm::vec3 off = (sliceOffsetD*0.5f) * wedgeOffsetDir[k];
        for(auto& tri : afterB) addTri(pos, nrm, idx, tri.v[0], tri.v[1], tri.v[2], off);

        uploadSliceVAO(wedgeVAOArr[k],wedgeVBOArr[k],wedgeNVBOArr[k],wedgeEBOArr[k], pos, nrm, idx);
    }
}
void buildArrangementCPU(const glm::mat4& rotOnly, const glm::mat3& normalRotOnly){
    std::vector<SliceTri> allTris;
    allTris.reserve(indices.size()/3);
    for(size_t t = 0; t < indices.size(); t += 3){
        SliceTri tri;
        for(int k = 0; k < 3; k++){
            unsigned int vi = indices[t+k];
            glm::vec3 lp(preppedPositions[3*vi], preppedPositions[3*vi+1], preppedPositions[3*vi+2]);
            glm::vec3 ln(normals[3*vi], normals[3*vi+1], normals[3*vi+2]);
            tri.v[k].pos = glm::vec3(rotOnly * glm::vec4(lp, 1.0f));
            tri.v[k].nrm = glm::normalize(normalRotOnly * ln);
        }
        allTris.push_back(tri);
    }

   
    std::vector<Piece> pieces;
    pieces.push_back({ std::move(allTris), glm::vec3(0.0f) });

    for(int p = 0; p < numActivePlanes; p++){
        glm::vec3 n = freePlaneNormal[p];
        float step = sliceOffsetD * 0.1f;

        std::vector<Piece> next;
        for(size_t pi = 0; pi < pieces.size(); pi++){
            if((int)next.size() + 2 > MAX_PIECES){
                for(size_t r = pi; r < pieces.size(); r++)
                    next.push_back(std::move(pieces[r]));
                break;
            }
            auto& piece = pieces[pi];
            auto above = clipListAgainstPlane(piece.tris,  n,  freePlaneW[p]);
            auto below = clipListAgainstPlane(piece.tris, -n, -freePlaneW[p]);
            if(!above.empty()) next.push_back({ std::move(above), piece.offset + step * n });
            if(!below.empty()) next.push_back({ std::move(below), piece.offset - step * n });
        }
        pieces.swap(next);
    }
    numActivePieces = std::min((int)pieces.size(), MAX_PIECES);
    pieceP.resize(MAX_PIECES); pieceN.resize(MAX_PIECES); pieceI.resize(MAX_PIECES);
    pieceVAO.resize(MAX_PIECES, 0); pieceVBO.resize(MAX_PIECES, 0);
    pieceNVBO.resize(MAX_PIECES, 0); pieceEBO.resize(MAX_PIECES, 0);

    for(int k = 0; k < numActivePieces; k++){
        glm::vec3 off = pieces[k].offset;
        off.z = 0.0f;   // orthographic view: z separation is invisible

        auto& pos = pieceP[k]; auto& nrm = pieceN[k]; auto& idx = pieceI[k];
        pos.clear(); nrm.clear(); idx.clear();
        for(auto& tri : pieces[k].tris)
            addTri(pos, nrm, idx, tri.v[0], tri.v[1], tri.v[2], off);
        uploadSliceVAO(pieceVAO[k], pieceVBO[k], pieceNVBO[k], pieceEBO[k], pos, nrm, idx);
    }
    
}


void initFreePlanes(){
    for(int i = 0; i < MAX_PLANES; i++){
        float a = i * (3.14159265f / MAX_PLANES);
        freePlaneNormal[i] = glm::vec3(cosf(a), sinf(a), 0.0f);
        freePlaneW[i]      = 0.30f * cosf(i * 2.3f);   // prepped units, mesh spans ~[-0.8, 0.8]
    }
}

void reportBufferUse(const char* where){
    size_t wedgeTris = 0, pieceTris = 0;
    int wedgeVAOs = 0, pieceVAOs = 0;
    for(int k = 0; k < 2*MAX_PLANES; k++){
        if(wedgeVAOArr[k]) wedgeVAOs++;
        wedgeTris += wedgeIdxArr[k].size()/3;
    }
    for(size_t k = 0; k < pieceVAO.size(); k++){
        if(pieceVAO[k]) pieceVAOs++;
        pieceTris += pieceI[k].size()/3;
    }
    std::cout << "[" << where << "] wedge VAOs=" << wedgeVAOs
              << " tris=" << wedgeTris
              << " | piece VAOs=" << pieceVAOs
              << " tris=" << pieceTris << "\n";
}
#include "benchmark.h"