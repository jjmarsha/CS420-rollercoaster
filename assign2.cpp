/*
  CSCI 480
  Assignment 2
 */

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <iostream>
#include "pic.h"
#include <stdio.h>
#include <math.h>

#define S 0.5

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;

float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
   double xP;
   double yP;
   double zP;
};

GLuint trackList;
GLuint skybox;
GLfloat LightSource[] = {0.2, 0.2, 0.2, 1.0};

double scaleVar = 0.002;


/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

//stores the points of the coaster for the camera to traverse
point storeNorms[100000];
point storeBinormals[100000];
point coasterPoints[100000];
int CoasterLength = 0;
char alreadyGenerated = 0;

int CameraTraverse = 0;

/* the spline array */
struct spline *g_Splines;

GLuint SideTex;

GLuint TopTex;

GLuint BottomTex;

/* total number of splines */
int g_iNumOfSplines;

Pic * planeData;
Pic * sideData;
Pic * topData;

void mouseTranslate() {
      glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
}

void mouseRotate() {
    glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
    glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
    glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
}

void mouseScale() {
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
}

void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.005;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.005;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.005;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1]*0.1;
        g_vLandRotate[1] += vMouseDelta[0]*0.1;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1]*0.1;
      }

      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    case GLUT_ACTIVE_ALT:
      // ++renderType;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

int counter = 0;

void doIdle()
{

  /* do some stuff... */
/* make the screen update */
  if(counter % 100 == 0) {
  }
  counter++;
  glutPostRedisplay();
}

int loadSplines(char *argv) {
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;


  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }
  
  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &g_Splines[j].points[i].x, 
	   &g_Splines[j].points[i].y, 
	   &g_Splines[j].points[i].z) != EOF) {
      i++;
    }
  }

  free(cName);

  return 0;
}

/* vertices of cube about the origin */

point splineVertice(point* c1, point* c2, point* c3, point* c4, double u) {
  double x = u*u*u*(-S*c1->x + (2-S)*c2->x + (S-2)*c3->x + S*c4->x) + u*u*(2*S*c1->x + (S-3)*c2->x + (3-2*S)*c3->x + -S*c4->x) + u*(-S*c1->x + S*c3->x) + (c2->x);
  double xP = 3*u*u*(-S*c1->x + (2-S)*c2->x + (S-2)*c3->x + S*c4->x) + 2*u*(2*S*c1->x + (S-3)*c2->x + (3-2*S)*c3->x + -S*c4->x) + (-S*c1->x + S*c3->x);
  double y = u*u*u*(-S*c1->y + (2-S)*c2->y + (S-2)*c3->y + S*c4->y) + u*u*(2*S*c1->y + (S-3)*c2->y + (3-2*S)*c3->y + -S*c4->y) + u*(-S*c1->y + S*c3->y) + (c2->y); 
  double yP = 3*u*u*(-S*c1->y + (2-S)*c2->y + (S-2)*c3->y + S*c4->y) + 2*u*(2*S*c1->y + (S-3)*c2->y + (3-2*S)*c3->y + -S*c4->y) + (-S*c1->y + S*c3->y);
  double z = u*u*u*(-S*c1->z + (2-S)*c2->z + (S-2)*c3->z + S*c4->z) + u*u*(2*S*c1->z + (S-3)*c2->z + (3-2*S)*c3->z + -S*c4->z) + u*(-S*c1->z + S*c3->z) + (c2->z);
  double zP = 3*u*u*(-S*c1->z + (2-S)*c2->z + (S-2)*c3->z + S*c4->z) + 2*u*(2*S*c1->z + (S-3)*c2->z + (3-2*S)*c3->z + -S*c4->z) + (-S*c1->z + S*c3->z);

  return {x, y, z, xP, yP, zP};
}


void drawSideplanes() {
  double scale = 4;
  int positioning = planeData->nx/(2*scale);
  int height = planeData->nx;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, SideTex);
  glPushMatrix();
  glRotatef(90,0,0,1);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for(int i = 0; i < height-1; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < height; j++) {
      glTexCoord2f(i/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, (i)/scale - positioning, -positioning);
      glTexCoord2f((i+1)/512.0, j/512.0);    
      glVertex3f((j)/scale - positioning, (i +1)/scale - positioning, -positioning);
    }
    glEnd();
  }
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, TopTex);
  glPushMatrix();
  // glRotatef(90,0,0,1);
  for(int i = 0; i < height-1; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < height; j++) {
      glTexCoord2f(i/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, -positioning, (i)/scale-positioning);
      glTexCoord2f((i+1)/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, -positioning, (i+1)/scale - positioning);
    }
    glEnd();
  }
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, BottomTex);
  glPushMatrix();
  for(int i = 0; i < height-1; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < height; j++) {
      glTexCoord2f(i/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, positioning - 1/scale, (i)/scale - positioning);
      glTexCoord2f((i+1)/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, positioning - 1/scale, (i+1)/scale - positioning);
    }
    glEnd();
  }
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, SideTex);
  glPushMatrix();
  // glRotated(0, 1,0,0);
  for(int i = 0; i < height-1; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < height; j++) {
      glTexCoord2f(i/512.0, j/512.0);
      glVertex3f(-positioning + 1/scale, (j)/scale - positioning, (i)/scale - positioning);
      glTexCoord2f((i+1)/512.0, j/512.0);
      glVertex3f(-positioning + 1/scale, (j)/scale - positioning, (i+1)/scale -positioning);
    }
    glEnd();
  }
  glPopMatrix();

  glPushMatrix();
  for(int i = 0; i < height-1; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < height; j++) {
      glTexCoord2f(i/512.0, j/512.0);
      glVertex3f(positioning - 2/scale, (j)/scale - positioning, (i)/scale - positioning);
      glTexCoord2f((i+1)/512.0, j/512.0);
      glVertex3f(positioning - 2/scale, (j)/scale - positioning, (i+1)/scale - positioning);
    }
    glEnd();
  }
  glPopMatrix();

  glPushMatrix();
  glRotated(90, 0,0,1);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for(int i = 0; i < height-1; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < height; j++) {
      unsigned char Red1 = PIC_PIXEL(sideData, j,i,0);
      unsigned char Green1 = PIC_PIXEL(sideData, j,i,1);
      unsigned char Blue1 = PIC_PIXEL(sideData, j,i,2);

      unsigned char Red2 = PIC_PIXEL(sideData, j,i+1,0);
      unsigned char Green2 = PIC_PIXEL(sideData, j,i+1,1);
      unsigned char Blue2 = PIC_PIXEL(sideData, j,i+1,2);



      glTexCoord2f(i/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, (i)/scale - positioning, positioning - 1/scale);
      glTexCoord2f((i+1)/512.0, j/512.0);
      glVertex3f((j)/scale - positioning, (i +1)/scale - positioning, positioning - 1/scale);
    }
    glEnd();
  }
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);

}

point calcNormTangent(const point* nextPoint) {
  double magnitude = pow((pow(nextPoint->xP, 2) + pow(nextPoint->yP, 2) + pow(nextPoint->zP, 2)), 0.5);
  return {nextPoint->xP/magnitude, nextPoint->yP/magnitude, nextPoint->zP/magnitude};
}

point crossProduct(point vector1, point vector2) {
  // std::cout << vector1.x << std::endl;
  // std::cout << vector2.x << std::endl;
  double x = vector1.y*vector2.z - vector1.z*vector2.y;
  double y = vector1.z*vector2.x - vector1.x*vector2.z;
  double z = vector1.x*vector2.y - vector1.y*vector2.x;
  double magnitude = pow((pow(x,2) + pow(y, 2) + pow(z, 2)), 0.5);
  // std::cout << "inside calcnorm " << "x: " << x << " y: "<< y << " z: " << z << std::endl;
  return {x/magnitude, y/magnitude, z/magnitude};
}

  point calcVectorPoints(int vectorIndex, double norm, double bi, double scale, double second) {
      double x = coasterPoints[vectorIndex].x + scale*(norm*storeNorms[vectorIndex].x + bi*storeBinormals[vectorIndex].x) + scale*second*storeBinormals[vectorIndex].x;
      double y = coasterPoints[vectorIndex].y + scale*(norm*storeNorms[vectorIndex].y + bi*storeBinormals[vectorIndex].y) +  scale*second*storeBinormals[vectorIndex].y;
      double z = coasterPoints[vectorIndex].z + scale*(norm*storeNorms[vectorIndex].z + bi*storeBinormals[vectorIndex].z) + scale*second*storeBinormals[vectorIndex].z;
      return {x, y, z};
  }

void coaster() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      // point thisTan = calcNormTangent(&coasterPoints[index]);
      // point thisNorm = crossProduct(prevB, tangentVectorStart);
      // point thisB = crossProduct(tangentVectorStart, prevNorm);
    if(!alreadyGenerated) {
      point thisTan;
      point thisNorm;
      point thisB;
      char start = 1;

      int total_points = g_Splines[0].numControlPoints;
      for(int i = 1; i < total_points-2; i++) {
        int j = i + 1;
        point control1;
        point control2;
        point control3;
        point control4;

        control1 = g_Splines[0].points[i-1];
        control2 = g_Splines[0].points[i];
        control3 = g_Splines[0].points[j];
        control4 = g_Splines[0].points[j+1];
  
        for(double q = 0; q <= 1; q += 0.001) {
          point currentPoint = splineVertice(&control1, &control2, &control3, &control4, q);
          if(start) {
            start = 0;
            thisTan = calcNormTangent(&currentPoint);
            thisNorm = crossProduct({0,2,0}, thisTan);
            thisB = crossProduct(thisTan, thisNorm);
            storeBinormals[CoasterLength] = thisB;
            storeNorms[CoasterLength] = thisNorm;
          } else {
            thisTan = calcNormTangent(&currentPoint);
            thisNorm = crossProduct(thisB, thisTan);
            thisB = crossProduct(thisTan, thisNorm);
            storeBinormals[CoasterLength] = thisB;
            storeNorms[CoasterLength] = thisNorm;
          }
          coasterPoints[CoasterLength++] = currentPoint;
        }          
      }
      alreadyGenerated = 1;
    }
    glColor3f( 0.5, 0.5, 0.5);


    for(int i = 0; i < CoasterLength-1; i++) {
        point v0 = calcVectorPoints(i, -1, 1, scaleVar, 0);
        point v1 = calcVectorPoints(i, 1, 1, scaleVar, 0);
        point v2 = calcVectorPoints(i, 1, -1, scaleVar, 0);
        point v3 = calcVectorPoints(i, -1, -1, scaleVar, 0);
        point v4 = calcVectorPoints(i+1, -1, 1, scaleVar, 0);
        point v5 = calcVectorPoints(i+1, 1, 1, scaleVar, 0);
        point v6 = calcVectorPoints(i+1, 1, -1, scaleVar, 0);
        point v7 = calcVectorPoints(i+1, -1, -1, scaleVar, 0);

        glBegin(GL_QUADS);
          glVertex3f(v0.x, v0.y, v0.z);
          glVertex3f(v1.x, v1.y, v1.z);
          glVertex3f(v5.x, v5.y, v5.z);
          glVertex3f(v4.x, v4.y, v4.z);
        glEnd();
        glBegin(GL_QUADS);
          glVertex3f(v1.x, v1.y, v1.z);
          glVertex3f(v2.x, v2.y, v2.z);
          glVertex3f(v6.x, v6.y, v6.z);
          glVertex3f(v5.x, v5.y, v5.z);
        glEnd();
        glBegin(GL_QUADS);
          glVertex3f(v2.x, v2.y, v2.z);
          glVertex3f(v3.x, v3.y, v3.z);
          glVertex3f(v7.x, v7.y, v7.z);
          glVertex3f(v6.x, v6.y, v6.z);
        glEnd();
        glBegin(GL_QUADS);
          glVertex3f(v3.x, v3.y, v3.z);
          glVertex3f(v0.x, v0.y, v0.z);
          glVertex3f(v4.x, v4.y, v4.z);
          glVertex3f(v7.x, v7.y, v7.z);
        glEnd();
    }

    for(int i = 0; i < CoasterLength-1; i++) {
        point v0 = calcVectorPoints(i, -1, 1, scaleVar, 20);
        point v1 = calcVectorPoints(i, 1, 1, scaleVar, 20);
        point v2 = calcVectorPoints(i, 1, -1, scaleVar, 20);
        point v3 = calcVectorPoints(i, -1, -1, scaleVar, 20);
        point v4 = calcVectorPoints(i+1, -1, 1, scaleVar, 20);
        point v5 = calcVectorPoints(i+1, 1, 1, scaleVar, 20);
        point v6 = calcVectorPoints(i+1, 1, -1, scaleVar, 20);
        point v7 = calcVectorPoints(i+1, -1, -1, scaleVar, 20);

        glBegin(GL_QUADS);
          glVertex3f(v0.x, v0.y, v0.z);
          glVertex3f(v1.x, v1.y, v1.z);
          glVertex3f(v5.x, v5.y, v5.z);
          glVertex3f(v4.x, v4.y, v4.z);
        glEnd();
        glBegin(GL_QUADS);
          glVertex3f(v1.x, v1.y, v1.z);
          glVertex3f(v2.x, v2.y, v2.z);
          glVertex3f(v6.x, v6.y, v6.z);
          glVertex3f(v5.x, v5.y, v5.z);
        glEnd();
        glBegin(GL_QUADS);
          glVertex3f(v2.x, v2.y, v2.z);
          glVertex3f(v3.x, v3.y, v3.z);
          glVertex3f(v7.x, v7.y, v7.z);
          glVertex3f(v6.x, v6.y, v6.z);
        glEnd();
        glBegin(GL_QUADS);
          glVertex3f(v3.x, v3.y, v3.z);
          glVertex3f(v0.x, v0.y, v0.z);
          glVertex3f(v4.x, v4.y, v4.z);
          glVertex3f(v7.x, v7.y, v7.z);
        glEnd();
    }
}

int count = -1;
int index = 0;

void display(void) {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glLoadIdentity();
  GLfloat diffuse[] = {0.8, 0.8, 0.8, 1.0};
  GLfloat position[] = {0,1,0,1};

  glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
  glLightfv(GL_LIGHT0,GL_POSITION, LightSource);


  // gluLookAt(coasterPoints[index].x + 0.023*storeNorms[index].x, coasterPoints[index].y + 0.023*storeNorms[index].y, coasterPoints[index].z + 0.023*storeNorms[index].z, coasterPoints[index+1].x, coasterPoints[index+1].y, coasterPoints[index+1].z, storeNorms[index].x, storeNorms[index].y, storeNorms[index].z);
  double scale = 0.003;

    gluLookAt(
      coasterPoints[index].x + scale*storeNorms[index].x + 0.5*scaleVar*20*storeBinormals[index].x,
      coasterPoints[index].y + scale*storeNorms[index].y + 0.5*scaleVar*20*storeBinormals[index].y,
      coasterPoints[index].z + scale*storeNorms[index].z + 0.5*scaleVar*20*storeBinormals[index].z,
      coasterPoints[index+1].x + scale*storeNorms[index+1].x + 0.5*scaleVar*20*storeBinormals[index+1].x, 
      coasterPoints[index+1].y + scale*storeNorms[index+1].y + 0.5*scaleVar*20*storeBinormals[index+1].y, 
      coasterPoints[index+1].z + scale*storeNorms[index+1].z + 0.5*scaleVar*20*storeBinormals[index+1].z, 
      storeNorms[index].x, 
      storeNorms[index].y, 
      storeNorms[index].z
    );

    std::cout << coasterPoints[index].x << " " << coasterPoints[index].y << " " << coasterPoints[index].z << std::endl;

  if(count%1 == 0) {
    index++;
    if(index == CoasterLength) {
      index = 0;
    }
  }
  
  count++;
    
    mouseRotate();
    mouseTranslate();
    mouseScale();


    glPushMatrix();
    glRotated(90, 1, 0, 0);  
    glCallList(skybox);
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glPushMatrix();
    glCallList(trackList);
    glPopMatrix();
    glDisable(GL_LIGHTING);

  glutSwapBuffers();
}

void myinit()
{
  /* setup gl view here */
  glClearColor(0.0, 0.0, 0.0, 0.0);
  // enable depth buffering
  glEnable(GL_DEPTH_TEST); 
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH);   
  // glPolygonOffset(0.75, 20);

  trackList = glGenLists(1);
  glNewList(trackList, GL_COMPILE);
    coaster();
  glEndList();

  skybox = glGenLists(1);
  glNewList(skybox, GL_COMPILE);
    drawSideplanes();
  glEndList();

  glEnable(GL_LIGHT0);

}

void reshape(int w, int h)
{
    GLfloat aspect = (GLfloat) w / (GLfloat) h;
    glViewport(0, 0, w, h); 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(60, aspect, 0.0001, 1000); 
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 'w':
      g_vLandTranslate[2] += 0.5;
      break;
    case 'a':
      g_vLandTranslate[0] += 0.5;
      break;
    case 's':
      g_vLandTranslate[2] -= 0.5;
      break;
    case 'd':
      g_vLandTranslate[0] -= 0.5;
      break;
    case 'q':
      g_vLandTranslate[1] += 0.5;
      break;
    case 'e':
      g_vLandTranslate[1] -= 0.5;
      break;
  }
}


int main (int argc, char ** argv)
{
  if (argc<2)
  {  
  printf ("usage: %s <trackfile> <texturemap>\n", argv[0]);
  exit(0);
  }

  planeData = jpeg_read("./h.jpg", NULL);
  if (!planeData)
  {
    printf ("error reading %s.\n", argv[1]);
    exit(1);
  }
  sideData = jpeg_read("./wxp.jpeg", NULL);
  if (!sideData)
  {
    printf ("error reading %s.\n", argv[1]);
    exit(1);
  }
  topData = jpeg_read("./sun.jpg", NULL);
  if (!topData)
  {
    printf ("error reading %s.\n", argv[1]);
    exit(1);
  }

  loadSplines(argv[1]);

  //Initializaton 
  glutInit(&argc,argv);
  glutInitWindowSize(640, 480);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("SUP BOY!");
  
  //textures
  glGenTextures(1, &TopTex);
  glBindTexture(GL_TEXTURE_2D, TopTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, topData->pix);

  glGenTextures(1, &SideTex);
  glBindTexture(GL_TEXTURE_2D, SideTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, sideData->pix);

  glGenTextures(1, &BottomTex);
  glBindTexture(GL_TEXTURE_2D, BottomTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, planeData->pix);
  
  glLightfv(GL_LIGHT0,GL_AMBIENT,LightSource);


  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);

  glutIdleFunc(doIdle);

  glutKeyboardFunc(keyboard);

  glutMotionFunc(mousedrag);
  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);
  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);


  myinit();
  glutMainLoop();


  return 0;
}
