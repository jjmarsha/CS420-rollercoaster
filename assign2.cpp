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

/* total number of splines */
int g_iNumOfSplines;

Pic * planeData;

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
  int scale = 100;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for(int i = 0; i < 99; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < 100; j++) {
      glColor3f(0, 0, 139);
      glVertex3f(scale*(j - 50.0)/100, scale*(i - 50.0)/100, scale*(-0.5));
      glColor3f(0, 0, 139);
      glVertex3f(scale*(j - 50.0)/100, scale*(i - 50.0 + 1)/100, scale*-0.5);
    }
    glEnd();
  }

  for(int i = 0; i < 99; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < 100; j++) {
      glColor3f(i*128, 0, 256);
      glVertex3f(scale*(j - 50.0)/100, scale*-0.5, scale*(i - 50.0)/100);
      glColor3f((i+1)*128, 0, 256);
      glVertex3f(scale*(j - 50.0)/100, scale*-0.5, scale*(i - 50.0 + 1)/100);
    }
    glEnd();
  }

  for(int i = 0; i < 99; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < 100; j++) {
      glColor3f(128, 0, 256);
      glVertex3f(scale*(j - 50.0)/100, scale*(0.5 - 0.01), scale*(i - 50.0)/100);
      glColor3f(128, 0, 256);
      glVertex3f(scale*(j - 50.0)/100, scale*(0.5 - 0.01), scale*(i - 50.0 + 1)/100);
    }
    glEnd();
  }

  for(int i = 0; i < 99; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < 100; j++) {
      glColor3f(128, 0, 0);
      glVertex3f(scale*-0.5, scale*(j - 50.0)/100, scale*(i - 50.0)/100);
      glColor3f(128, 0, 0);
      glVertex3f(scale*-0.5, scale*(j - 50.0 )/100, scale*(i - 50.0 + 1)/100);
    }
    glEnd();
  }

  for(int i = 0; i < 99; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < 100; j++) {
      glColor3f(128, 0, 0);
      glVertex3f(scale*(0.5 - 0.01), scale*(j - 50.0)/100, scale*(i - 50.0)/100);
      glColor3f(128, 0, 0);
      glVertex3f(scale*(0.5 - 0.01), scale*(j - 50.0 )/100, scale*(i - 50.0 + 1)/100);
    }
    glEnd();
  }
    for(int i = 0; i < 99; i++) {
    glBegin(GL_TRIANGLE_STRIP);
    for(int j = 0; j < 100; j++) {
      glColor3f(0, 0, 139);
      glVertex3f(scale*(j - 50.0)/100, scale*(i - 50.0)/100, scale*(0.5 - 0.01));
      glColor3f(0, 0, 139);
      glVertex3f(scale*(j - 50.0)/100, scale*(i - 50.0 + 1)/100, scale*(0.5 - 0.01));
    }
    glEnd();
  }
}

point calcNormTangent(const point* nextPoint) {
  double magnitude = pow((pow(nextPoint->x, 2) + pow(nextPoint->y, 2) + pow(nextPoint->z, 2)), 0.5);
  return {nextPoint->x/magnitude, nextPoint->y/magnitude, nextPoint->z/magnitude};
}

point calculateNormal(point currTangent, point prevBinorm) {
  // std::cout << currTangent.x << std::endl;
  // std::cout << prevBinorm.x << std::endl;
  double x = currTangent.y*prevBinorm.z - currTangent.z*prevBinorm.y;
  double y = currTangent.z*prevBinorm.x - currTangent.x*prevBinorm.z;
  double z = currTangent.x*prevBinorm.y - currTangent.y*prevBinorm.x;
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
      // point thisNorm = calculateNormal(prevB, tangentVectorStart);
      // point thisB = calculateNormal(tangentVectorStart, prevNorm);
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
  
        for(double q = 0; q <= 1; q += 0.005) {
          point currentPoint = splineVertice(&control1, &control2, &control3, &control4, q);
          if(start) {
            start = 0;
            thisTan = calcNormTangent(&currentPoint);
            thisNorm = calculateNormal({3,2,1}, thisTan);
            thisB = calculateNormal(thisTan, thisNorm);
            storeBinormals[CoasterLength] = thisB;
            storeNorms[CoasterLength] = thisNorm;
          } else {
            thisTan = calcNormTangent(&currentPoint);
            thisNorm = calculateNormal(thisB, thisTan);
            thisB = calculateNormal(thisTan, thisNorm);
            storeBinormals[CoasterLength] = thisB;
            storeNorms[CoasterLength] = thisNorm;
          }
          coasterPoints[CoasterLength++] = currentPoint;
        }          
      }
      alreadyGenerated = 1;
    }
    glColor3f(1,1,1);

    for(int i = 0; i < CoasterLength-1; i++) {
        point v0 = calcVectorPoints(i, -1, 1, 0.001, 0);
        point v1 = calcVectorPoints(i, 1, 1, 0.001, 0);
        point v2 = calcVectorPoints(i, 1, -1, 0.001, 0);
        point v3 = calcVectorPoints(i, -1, -1, 0.001, 0);
        point v4 = calcVectorPoints(i+1, -1, 1, 0.001, 0);
        point v5 = calcVectorPoints(i+1, 1, 1, 0.001, 0);
        point v6 = calcVectorPoints(i+1, 1, -1, 0.001, 0);
        point v7 = calcVectorPoints(i+1, -1, -1, 0.001, 0);

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
        point v0 = calcVectorPoints(i, -1, 1, 0.001, 20);
        point v1 = calcVectorPoints(i, 1, 1, 0.001, 20);
        point v2 = calcVectorPoints(i, 1, -1, 0.001, 20);
        point v3 = calcVectorPoints(i, -1, -1, 0.001, 20);
        point v4 = calcVectorPoints(i+1, -1, 1, 0.001, 20);
        point v5 = calcVectorPoints(i+1, 1, 1, 0.001, 20);
        point v6 = calcVectorPoints(i+1, 1, -1, 0.001, 20);
        point v7 = calcVectorPoints(i+1, -1, -1, 0.001, 20);

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
  glLoadIdentity();

  // gluLookAt(coasterPoints[index].x + 0.023*storeNorms[index].x, coasterPoints[index].y + 0.023*storeNorms[index].y, coasterPoints[index].z + 0.023*storeNorms[index].z, coasterPoints[index+1].x, coasterPoints[index+1].y, coasterPoints[index+1].z, storeNorms[index].x, storeNorms[index].y, storeNorms[index].z);
  double scale = 0.003;

  gluLookAt(
    coasterPoints[index].x + scale*storeNorms[index].x + 0.5*0.001*20*storeBinormals[index].x,
    coasterPoints[index].y + scale*storeNorms[index].y + 0.5*0.001*20*storeBinormals[index].y,
    coasterPoints[index].z + scale*storeNorms[index].z + 0.5*0.001*20*storeBinormals[index].z,
    coasterPoints[index+1].x + scale*storeNorms[index+1].x + 0.5*0.001*20*storeBinormals[index+1].x, 
    coasterPoints[index+1].y + scale*storeNorms[index+1].y + 0.5*0.001*20*storeBinormals[index+1].y, 
    coasterPoints[index+1].z + scale*storeNorms[index+1].z + 0.5*0.001*20*storeBinormals[index+1].z, 
    storeNorms[index].x, 
    storeNorms[index].y, 
    storeNorms[index].z
  );

  if(count%5 == 0) {
    index++;
    if(index == CoasterLength) {
      index = 0;
    }
  }
  
  count++;
    mouseRotate();
    mouseTranslate();
    mouseScale();


    // glPushMatrix();
    drawSideplanes();
    // glPopMatrix();


    glPushMatrix();
    glCallList(trackList);
    glPopMatrix();

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
      g_vLandTranslate[2] += 0.1;
      break;
    case 'a':
      g_vLandTranslate[0] += 0.1;
      break;
    case 's':
      g_vLandTranslate[2] -= 0.1;
      break;
    case 'd':
      g_vLandTranslate[0] -= 0.1;
      break;
    case 'q':
      g_vLandTranslate[1] += 0.1;
      break;
    case 'e':
      g_vLandTranslate[1] -= 0.1;
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

  // planeData = jpeg_read("./joji_necklace.jpg", NULL);
  // if (!planeData)
  // {
  //   printf ("error reading %s.\n", argv[1]);
  //   exit(1);
  // }

  loadSplines(argv[1]);

  //Initializaton 
  glutInit(&argc,argv);
  glutInitWindowSize(640, 480);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("SUP BOY!");
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
