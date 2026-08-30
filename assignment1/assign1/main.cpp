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


GLuint uColorLoc;
GLuint colorVBO;
GLint uUseVertexColorLoc;
bool usevertexColor=false;
std::vector<float> vertexColor;
glm::vec3 boundsMin, boundsMax;

/*
 * functions
 */


void keyboard(unsigned char key,int x, int y);
void applyTheme();
void applyPolygonMode();

void initRandomAxis(){
    srand((unsigned) time(nullptr));
    float x = ((float)rand() / RAND_MAX) * 2 - 1;
    float y = ((float)rand() / RAND_MAX) * 2 - 1;
    float z = ((float)rand() / RAND_MAX) * 2 - 1;
    glm::vec3 axis(x,y,z);
    if(glm::length(axis) < 1e-6f) axis = glm:: vec3(0,1,0);
    rotAxis = glm::normalize(axis);
    std::cout << "Rotation Axis : (" << rotAxis.x << " " << rotAxis.y << " " << rotAxis.z << std::endl;
}


void timer(int value){
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float delta = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    rotAngle = rotAngle + rotSpeed * delta;
    if(rotAngle > 360.0f) rotAngle -= 360.0f;

    glutPostRedisplay();
    glutTimerFunc(16, timer , 0 );
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


    glBindVertexArray(0);

    std::cout << "Buffers Built\n"; 


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

void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    

    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::rotate(modelMat, glm::radians(rotAngle),rotAxis);
    modelMat = glm::scale(modelMat, glm::vec3(modelScale));
    modelMat = glm::translate(modelMat, -modelCenter);

    glUniformMatrix4fv(uModelLoc, 1 ,GL_FALSE, glm::value_ptr(modelMat));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,(GLsizei) indices.size(), GL_UNSIGNED_INT , 0);
    glBindVertexArray(0);
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

    ComputeModelTransform();
    initRandomAxis();


    
    createVertexBuffers();
    
    applyTheme();
    applyPolygonMode();
    glUseProgram(shaderProgram);
    glUniform1i(uUseVertexColorLoc,usevertexColor);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);



    lastTime = glutGet(GLUT_ELAPSED_TIME);
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
            glUniform1i(uUseVertexColorLoc, usevertexColor); // doubt :-> learn this function
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

