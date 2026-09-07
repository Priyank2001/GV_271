#pragma once
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

inline std::string readFile(const char* filePath){
    std:ifstream file(filePath);
    if(!file.is_open()){
        cerr << "Error while opening file " << filePath << "\n";
        return "";
    }
    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline GLuint compileShader(GLenum type, const char* source){
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(!success){
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std:cerr<< "Shader Compile error:\n" << infoLog << "\n";

    }
    return shader;
}

inline GLuint createShaderProgram(const char* vsPath, const char* fsPath){
    string vtSrc = readFile(vsPath);
    string fsSrc = readFile(fsPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vtSrc.c_str());
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str());


    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    GLint Success;
    glGetProgramiv(program, GL_LINK_STATUS, &Success);

    if(!Success){
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        cerr << "Shader Link error: \n" << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragShader);
    return program;
}


