//4490
//program: cyber.cpp
//author:  Eddie Velasco
//date:    Spring 2018
//
//Walk cycle using a sprite sheet.
//images courtesy: http://games.ucla.edu/resource/walk-cycles/
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "log.h"
#include "ppm.h"
#include "fonts.h"
#ifdef USE_OPENAL_SOUND
#include </usr/include/AL/alut.h>
#endif

//defined types
typedef float Flt;
typedef Flt Vec[3];
typedef Flt Matrix[4][4];

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
                      (c)[1]=(a)[1]-(b)[1]; \
                      (c)[2]=(a)[2]-(b)[2]

//constants
const float timeslice = 1.0f;
const float gravity = -0.2f;
const int MAX_BULLETS = 100;
const int MAX_ENEMY = 26;
const int MAX_DRONE = 6;
#ifdef USE_OPENAL_SOUND
ALuint alBuffer[2];
ALuint alSource[2];
#endif //USE_OPENAL_SOUND
#define ALPHA 1
#define GRAVITY -0.5f
//X Windows variables
Display *dpy;
Window win;

//function prototypes
void initXWindows(void);
void initOpengl(void);
void cleanupXWindows(void);
void checkResize(XEvent *e);
void checkMouse(XEvent *e);
void checkKeys(XEvent *e);
void init();
void physics(void);
void render(void);
//Functions by self
void healthBar(void);
void renderBackground(void);
void renderFPS(void);
void StartJump(void);
void EndJump(void);
void bullets(void);
void bulletPhysics(void);
void enemyPhysics(void);
void dronePhysics(void);
void shootbullets(void);
void checkCollision(void);
void tileCollision(Vec *tile);
void emptyCollision(Vec *tile);
void gameOver(void);
void music(void);
void deleteMusic(void);
void beams(void);
void droneHitbox(void);
//-----------------------------------------------------------------------------
//Setup timers
class Timers {
public:
	double physicsRate;
	double oobillion;
	struct timespec timeStart, timeEnd, timeCurrent;
	struct timespec walkTime;
	struct timespec enemyTime;
	struct timespec droneTime;
	struct timespec frameTime;
	struct timespec bulletTime;
	struct timespec httpTime;
	struct timespec gameoverTime;
	Timers() {
		physicsRate = 1.0 / 30.0;
		oobillion = 1.0 / 1e9;
	}
	double timeDiff(struct timespec *start, struct timespec *end) {
		return (double)(end->tv_sec - start->tv_sec ) +
				(double)(end->tv_nsec - start->tv_nsec) * oobillion;
	}
	void timeCopy(struct timespec *dest, struct timespec *source) {
		memcpy(dest, source, sizeof(struct timespec));
	}
	void recordTime(struct timespec *t) {
		clock_gettime(CLOCK_REALTIME, t);
	}
} timers;
//-----------------------------------------------------------------------------

struct Shape {
    public:
        float width, height;
        float radius;
        Vec center;
};

class Bullet {
public:
	Vec pos;
	Vec vel;
	int direction;
	float velocity;
	float color[3];
	struct timespec time;
	Bullet () {	
	    direction = 0;
	    pos[0] = 0;
	    pos[1] = 0;
	}
};

class Sprite {
public:
	int onoff;
	int frame;
	double delay;
	Vec pos;
	Vec vel;
	float spritex;
	float spritey;
	bool onGround;
	float health;
	Ppmimage *image;
	GLuint tex;
	struct timespec time;
	Sprite() {
		onoff = 0;
		frame = 0;
		image = NULL;
		delay = 0.1;
	}
};

void showHitbox(float, float, float, float, Sprite);
void Hitbox(float, float, Sprite*);
void enemyHealthBar(float, float, Sprite*);
void makeBeams(Sprite *);

enum State {
	STATE_NONE,
	STATE_STARTUP,
	STATE_GAMEPLAY,
	STATE_PAUSE,
	STATE_GAMEOVER,
	STATE_COMPLETE,
	STATE_CREDITS
};

class Global {
public:
	unsigned char keys[65536];
	State state;
	int done;
	int xres, yres;
	int movie, movieStep;
	int walk;
	int walkFrame;
	int enemyFrame;
	int droneFrame;
	int bulletFrame;
	int httpFrame;
	double fps;
	int gameoverFrame;
	int countdown;
	double delay;
	double gameDelay;
	Vec box[20];
	Sprite mainChar;
	Sprite enemyChar[MAX_ENEMY];
	Sprite droneChar[MAX_DRONE];
	double speed;
	//camera is centered at (0,0) lower-left of screen. 
	Flt camera[2];
	Flt xc[2];
	Flt yc[2];
	int collisionL;
	int collisionR;
	
	//Bullets
	Bullet *bullets;
	int nbullets;
	float bulletVelocity;
	struct timespec bulletTimer;		
	float bulletWidth;
	float bulletHeight;
	//Beams
	Bullet *beams;
	int nbeams;
	float beamVelocity;
	struct timespec beamTimer;		
	float beamWidth;
	float beamHeight;

	int direction;
	int enemyDirection;
	int droneDirection;
	int underFlag;
	//PPM IMAGES
	Ppmimage *cyberMenuImage;
	Ppmimage *cyberstreetImage;
	Ppmimage *floorImage;
	Ppmimage *platformImage;
	Ppmimage *enemyImage;
	Ppmimage *droneImage;
	Ppmimage *creditsImage;
	Ppmimage *gameoverImage;
	Ppmimage *bluebackImage;
	Ppmimage *endImage;
	//---------------------------
	//TEXTURES
	//GLuint walkTexture;
	GLuint cyberMenuTexture;
	GLuint cyberstreetTexture;
	GLuint floorTexture;
	GLuint platformTexture;
	GLuint enemyTexture;
	GLuint droneTexture;
	GLuint creditsTexture;
	GLuint gameoverTexture;
	GLuint bluebackTexture;
	GLuint endTexture;
	//---------------------------
	~Global() {
		delete [] bullets;
		delete [] beams;
		logClose();
	}
	Global() {
		logOpen();
		collisionR = 0;
		collisionL = 0;
		underFlag = 0;
		state = STATE_STARTUP;
		camera[0] = camera[1] = 0.0;
		done=0;
		movie=0;
		movieStep=2;
		xres=1000;
		yres=600;
		walk=0;
		direction=1;
		enemyDirection=1;
		droneDirection=0;
		httpFrame=0;
		walkFrame=0;
		gameoverFrame=0;
		enemyFrame=0;
		droneFrame=0;
		fps=0;
		countdown = 10;
		speed = 2.25;
		//image variables
		mainChar.image=NULL;
		cyberMenuImage=NULL;
		cyberstreetImage=NULL;
		floorImage=NULL;
		platformImage=NULL;
		enemyImage=NULL;
		droneImage=NULL;
		creditsImage=NULL;
		bluebackImage=NULL;
		gameoverImage=NULL;
		endImage=NULL;
		//
		delay = 0.05;
		gameDelay = 0.01;
		mainChar.pos[1] = 0.0;
		mainChar.pos[2] = 0.0;
		mainChar.vel[0] = 0.0;
		mainChar.vel[1] = 0.0;
		mainChar.onGround = false;
		mainChar.health = 20.0;
		memset(keys, 0, 65536);
		//Bullets
		bullets = new Bullet[MAX_BULLETS];
		bulletVelocity = 10;
		nbullets = 0;
		bulletWidth = 4;
		bulletHeight = 2;
		bulletFrame = 0;

		beams = new Bullet[MAX_BULLETS];
		beamVelocity = 10;
		nbeams = 0;
		beamWidth = 4;
		beamHeight = 4;
	}
} gl;

class Level {
public:
	unsigned char arr[21][700];
	int nrows, ncols;
	int dynamicHeight[780];
	int tilesize[2];
	Flt ftsz[2];
	Flt tile_base;
	Level() {
	    	for (int i=0; i < 780; i++)
		{
		    dynamicHeight[i] = -1;
		}
		//Log("Level constructor\n");
		tilesize[0] = 32;
		tilesize[1] = 32;
		ftsz[0] = (Flt)tilesize[0];
		ftsz[1] = (Flt)tilesize[1];
		tile_base = 0.0;
		//read level
		FILE *fpi = fopen("level1.txt","r");
		if (fpi) {
			nrows=0;
			char line[700];
			while (fgets(line, 700, fpi) != NULL) {
				removeCrLf(line);
				int slen = strlen(line);
				ncols = slen;
				//Log("line: %s\n", line);
				for (int j=0; j<slen; j++) {
					arr[nrows][j] = line[j];
				}
				++nrows;
			}
			fclose(fpi);
			//printf("nrows of background data: %i\n", nrows);
		}
	}
	void removeCrLf(char *str) {
		char *p = str;
		while (*p) {
			if (*p == 10 || *p == 13) {
				*p = '\0';
				break;
			}
			++p;
		}
	}
} lev;


int main(void)
{
	initXWindows();
	initOpengl();
	music();
	while (!gl.done) {
		while (XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			checkResize(&e);
			checkMouse(&e);
			checkKeys(&e);
		}
		if(gl.state == STATE_STARTUP) {
			init();
		}
		if(gl.state == STATE_GAMEPLAY) {	
			physics();
			enemyPhysics();
			dronePhysics();
		}
		render();
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	deleteMusic();
	return 0;
}

void cleanupXWindows(void)
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void setTitle(void)
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 - Walk Cycle");
}

void setupScreenRes(const int w, const int h)
{
	gl.xres = w;
	gl.yres = h;
}

void initXWindows(void)
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
	XSetWindowAttributes swa;
	setupScreenRes(gl.xres, gl.yres);
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		printf("\n\tcannot connect to X server\n\n");
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		printf("\n\tno appropriate visual found\n\n");
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
						StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
							vi->depth, InputOutput, vi->visual,
							CWColormap | CWEventMask, &swa);
	GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
	setTitle();
}

void reshapeWindow(int width, int height)
{
	//window has been resized.
	setupScreenRes(width, height);
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	setTitle();
}

unsigned char *buildAlphaData(Ppmimage *img)
{
	//add 4th component to RGB stream...
	int i;
	unsigned char *newdata, *ptr;
	unsigned char *data = (unsigned char *)img->data;
	newdata = (unsigned char *)malloc(img->width * img->height * 4);
	ptr = newdata;
	unsigned char a,b,c;
	//use the first pixel in the image as the transparent color.
	unsigned char t0 = *(data+0);
	unsigned char t1 = *(data+1);
	unsigned char t2 = *(data+2);
	for (i=0; i<img->width * img->height * 3; i+=3) {
		a = *(data+0);
		b = *(data+1);
		c = *(data+2);
		*(ptr+0) = a;
		*(ptr+1) = b;
		*(ptr+2) = c;
		*(ptr+3) = 1;
		if (a==t0 && b==t1 && c==t2)
			*(ptr+3) = 0;
		//-----------------------------------------------
		ptr += 4;
		data += 3;
	}
	return newdata;
}

void initOpengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, gl.xres, gl.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	//
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	//
	//Clear the screen
	glClearColor(1.0, 1.0, 1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();



	//=====================================
	// Convert Images
	system("convert ./images/cyberMenu.png ./images/cyberMenu.ppm");
	system("convert ./images/cyberstreet.png ./images/cyberstreet.ppm");
	system("convert ./images/floor.png ./images/floor.ppm");
	system("convert ./images/platform.png ./images/platform.ppm");
	system("convert ./images/enemy.gif ./images/enemy.ppm");
	system("convert ./images/drone.gif ./images/drone.ppm");
	system("convert ./images/credits.png ./images/credits.ppm");
	system("convert ./images/gameover.png ./images/gameover.ppm");
	system("convert ./images/blueback.png ./images/blueback.ppm");
	system("convert ./images/end.png ./images/end.ppm");
	//=========================
	// Get Images
	//======================================
	gl.cyberMenuImage = ppm6GetImage("./images/cyberMenu.ppm");
	gl.cyberstreetImage = ppm6GetImage("./images/cyberstreet.ppm");
	gl.floorImage = ppm6GetImage("./images/floor.ppm");
	gl.platformImage = ppm6GetImage("./images/platform.ppm");
	gl.enemyImage = ppm6GetImage("./images/enemy.ppm");
	gl.droneImage = ppm6GetImage("./images/drone.ppm");
	gl.creditsImage = ppm6GetImage("./images/credits.ppm");
	gl.gameoverImage = ppm6GetImage("./images/gameover.ppm");
	gl.bluebackImage = ppm6GetImage("./images/blueback.ppm");
	gl.endImage = ppm6GetImage("./images/end.ppm");
	//=======================================
	// Generate Textures
	glGenTextures(1, &gl.cyberMenuTexture);
	glGenTextures(1, &gl.cyberstreetTexture);
	glGenTextures(1, &gl.floorTexture);
	glGenTextures(1, &gl.platformTexture);
	glGenTextures(1, &gl.enemyTexture);
	glGenTextures(1, &gl.droneTexture);
	glGenTextures(1, &gl.creditsTexture);
	glGenTextures(1, &gl.gameoverTexture);
	glGenTextures(1, &gl.bluebackTexture);
	glGenTextures(1, &gl.endTexture);
	//======================================





	//load the images file into a ppm structure.
	//
	system("convert ./images/walk.gif ./images/walk.ppm");
	gl.mainChar.image = ppm6GetImage("./images/walk.ppm");
	int w = gl.mainChar.image->width;
	int h = gl.mainChar.image->height;
	//
	//create opengl texture elements
	glGenTextures(1, &gl.mainChar.tex);
	//-------------------------------------------------------------------------
	//silhouette
	//this is similar to a sprite graphic
	//
	glBindTexture(GL_TEXTURE_2D, gl.mainChar.tex);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	//
	//must build a new set of data...
	unsigned char *walkData = buildAlphaData(gl.mainChar.image);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, walkData);
	free(walkData);
	unlink("./images/walk.ppm");
	//-------------------------------------------------------------------------
	//Cyber Menu Logo
	w = gl.cyberMenuImage->width;
	h = gl.cyberMenuImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.cyberMenuTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *cyberMenuData = buildAlphaData(gl.cyberMenuImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, cyberMenuData);
	free(cyberMenuData);
	unlink("./images/cyberMenu.ppm");
	//------------------------------------------------------
	//Cyber Street Background
	w = gl.cyberstreetImage->width;
	h = gl.cyberstreetImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.cyberstreetTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *cyberstreetData = buildAlphaData(gl.cyberstreetImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, cyberstreetData);
	free(cyberstreetData);
	unlink("./images/cyberstreet.ppm");
	gl.xc[0] = 0.0;
	gl.xc[1] = 1.0;
	gl.yc[0] = 0.0;
	gl.yc[1] = 1.0;
	//------------------------------------------------------
	//Floor Texture
	w = gl.floorImage->width;
	h = gl.floorImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.floorTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *floorData = buildAlphaData(gl.floorImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, floorData);
	free(floorData);
	unlink("./images/floor.ppm");
	//------------------------------------------------------
	//Platform Texture
	w = gl.platformImage->width;
	h = gl.platformImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.platformTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *platformData = buildAlphaData(gl.platformImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, platformData);
	free(platformData);
	unlink("./images/platform.ppm");
	//------------------------------------------------------
	//Enemy Texture
	w = gl.enemyImage->width;
	h = gl.enemyImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.enemyTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *enemyData = buildAlphaData(gl.enemyImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, enemyData);
	free(enemyData);
	unlink("./images/enemy.ppm");
	//------------------------------------------------------
	//Credits Texture
	w = gl.creditsImage->width;
	h = gl.creditsImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.creditsTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *creditsData = buildAlphaData(gl.creditsImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, creditsData);
	free(creditsData);
	unlink("./images/credits.ppm");
	//------------------------------------------------------
	//Game Over Texture
	w = gl.gameoverImage->width;
	h = gl.gameoverImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.gameoverTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *gameoverData = buildAlphaData(gl.gameoverImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, gameoverData);
	free(gameoverData);
	unlink("./images/gameover.ppm");
	//------------------------------------------------------
	//Game Over Texture
	w = gl.bluebackImage->width;
	h = gl.bluebackImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.bluebackTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *bluebackData = buildAlphaData(gl.bluebackImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, bluebackData);
	free(bluebackData);
	unlink("./images/blueback.ppm");
	//------------------------------------------------------
	//End Game Texture
	w = gl.endImage->width;
	h = gl.endImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.endTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *endData = buildAlphaData(gl.endImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, endData);
	free(endData);
	unlink("./images/end.ppm");
	//------------------------------------------------------
	//Enemy Drone Texture 
	w = gl.droneImage->width;
	h = gl.droneImage->height;
	glBindTexture(GL_TEXTURE_2D, gl.droneTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *droneData = buildAlphaData(gl.droneImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, droneData);
	free(droneData);
	unlink("./images/drone.ppm");
}

void checkResize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != gl.xres || xce.height != gl.yres) {
		//Window size did change.
		reshapeWindow(xce.width, xce.height);
	}
}

void init() {
	gl.xc[0] = 0.0;
	gl.yc[0] = 0.0;
	gl.xc[1] = 1.0;
	gl.yc[1] = 1.0;
	gl.camera[0] = 0.0;
	gl.camera[1] = 0.0;
	gl.mainChar.pos[0] = 0.0;
	gl.mainChar.pos[1] = 0.0;
	gl.mainChar.health = 20.0;
	gl.walkFrame = 0;
	gl.enemyFrame = 0;
	gl.droneFrame = 0;
	gl.gameoverFrame = 0;
	for (int i = 0; i < MAX_ENEMY; i++) {	
		gl.enemyChar[i].vel[0] = 0.0;
		gl.enemyChar[i].vel[1] = 0.0;
		gl.enemyChar[i].onGround = false;
		gl.enemyChar[i].health = 12.0;
	}
	for (int i = 0; i < MAX_DRONE; i++) {
		gl.droneChar[i].vel[0] = 0.0;
		gl.droneChar[i].vel[1] = 0.0;
		gl.droneChar[i].onGround = false;
		gl.droneChar[i].health = 10.0;
	}
	//DRONE
	gl.droneChar[0].pos[0] = 1524;	gl.droneChar[0].pos[1] = 300.0;
	gl.droneChar[1].pos[0] = 2120;	gl.droneChar[1].pos[1] = 300.0;
	gl.droneChar[2].pos[0] = 8182;	gl.droneChar[2].pos[1] = 300.0;
	gl.droneChar[3].pos[0] = 8512;	gl.droneChar[3].pos[1] = 300.0;
	gl.droneChar[4].pos[0] = 8845;	gl.droneChar[4].pos[1] = 300.0;
	gl.droneChar[5].pos[0] = 9252;	gl.droneChar[5].pos[1] = 300.0;
	//ENEMY
	gl.enemyChar[0].pos[0] = 768;	gl.enemyChar[0].pos[1] = 0.0;
	gl.enemyChar[1].pos[0] = 1104;	gl.enemyChar[1].pos[1] = 224.0;
	gl.enemyChar[2].pos[0] = 1424;	gl.enemyChar[2].pos[1] = 0.0;
	gl.enemyChar[3].pos[0] = 1920;	gl.enemyChar[3].pos[1] = 0.0;
	gl.enemyChar[4].pos[0] = 2240;	gl.enemyChar[4].pos[1] = 0.0;
	gl.enemyChar[5].pos[0] = 2560;	gl.enemyChar[5].pos[1] = 0.0;
	gl.enemyChar[6].pos[0] = 3072;	gl.enemyChar[6].pos[1] = 64.0;
	gl.enemyChar[7].pos[0] = 3572;	gl.enemyChar[7].pos[1] = 0.0;
	gl.enemyChar[8].pos[0] = 4072;	gl.enemyChar[8].pos[1] = 0.0;
	gl.enemyChar[9].pos[0] = 4392;	gl.enemyChar[9].pos[1] = 0.0;
	gl.enemyChar[10].pos[0] = 4592;	gl.enemyChar[10].pos[1] = 0.0;
	gl.enemyChar[11].pos[0] = 5216;	gl.enemyChar[11].pos[1] = 0.0;
	gl.enemyChar[12].pos[0] = 5536;	gl.enemyChar[12].pos[1] = 0.0;
	gl.enemyChar[13].pos[0] = 5868;	gl.enemyChar[13].pos[1] = 0.0;
	gl.enemyChar[14].pos[0] = 6260;	gl.enemyChar[14].pos[1] = 0.0;
	gl.enemyChar[15].pos[0] = 6360;	gl.enemyChar[15].pos[1] = 0.0;
	gl.enemyChar[16].pos[0] = 6460;	gl.enemyChar[16].pos[1] = 0.0;
	gl.enemyChar[17].pos[0] = 6560;	gl.enemyChar[17].pos[1] = 0.0;
	gl.enemyChar[18].pos[0] = 6660;	gl.enemyChar[18].pos[1] = 0.0;
	gl.enemyChar[19].pos[0] = 6760;	gl.enemyChar[19].pos[1] = 0.0;
	gl.enemyChar[20].pos[0] = 6860;	gl.enemyChar[20].pos[1] = 0.0;
	gl.enemyChar[21].pos[0] = 6960;	gl.enemyChar[21].pos[1] = 0.0;
	gl.enemyChar[22].pos[0] = 7060;	gl.enemyChar[22].pos[1] = 0.0;
	gl.enemyChar[23].pos[0] = 7160;	gl.enemyChar[23].pos[1] = 0.0;
	gl.enemyChar[24].pos[0] = 7260;	gl.enemyChar[24].pos[1] = 0.0;
	gl.enemyChar[25].pos[0] = 7360;	gl.enemyChar[25].pos[1] = 0.0;
}

void checkMouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		savex = e->xbutton.x;
		savey = e->xbutton.y;
	}
}

void screenCapture()
{
	static int fnum = 0;
	static int vid = 0;
	if (!vid) {
		system("mkdir ./vid");
		vid = 1;
	}
	unsigned char *data = (unsigned char *)malloc(gl.xres * gl.yres * 3);
    glReadPixels(0, 0, gl.xres, gl.yres, GL_RGB, GL_UNSIGNED_BYTE, data);
	char ts[32];
	sprintf(ts, "./vid/pic%03i.ppm", fnum);
	FILE *fpo = fopen(ts,"w");	
	if (fpo) {
		fprintf(fpo, "P6\n%i %i\n255\n", gl.xres, gl.yres);
		unsigned char *p = data;
		//go backwards a row at a time...
		p = p + ((gl.yres-1) * gl.xres * 3);
		unsigned char *start = p;
		for (int i=0; i<gl.yres; i++) {
			for (int j=0; j<gl.xres*3; j++) {
				fprintf(fpo, "%c",*p);
				++p;
			}
			start = start - (gl.xres*3);
			p = start;
		}
		fclose(fpo);
		char s[256];
		sprintf(s, "convert ./vid/pic%03i.ppm ./vid/pic%03i.gif", fnum, fnum);
		system(s);
		unlink(ts);
	}
	++fnum;
}

void checkKeys(XEvent *e)
{
	//keyboard input?
	static int shift=0;
	int key = XLookupKeysym(&e->xkey, 0);
	key = key & 0x0000ffff;
	gl.keys[key]=1;
	if (e->type == KeyRelease) {
		gl.keys[key]=0;
		if (key == XK_Shift_L || key == XK_Shift_R) {
			shift=0;
			return;
		}
	}
	if (e->type == KeyPress) {
		gl.keys[key]=1;
		if (key == XK_Shift_L || key == XK_Shift_R) {
			shift=1;
			return;
		}
	} 
	if (e->type == KeyRelease) {
		gl.keys[key]=0;
		if (key == XK_Up) {
			EndJump();
			return;
		}
	}
	if (e->type == KeyPress) {
		gl.keys[key]=1;
		if (key == XK_Shift_L || key == XK_Shift_R) {
			return;
		}
	} 
	else {
		return;
	}
	if (shift) {}
	switch (key) {
	    	case XK_p:
		    	if (gl.state == STATE_GAMEPLAY) {
			    gl.state = STATE_PAUSE;
			    break;
			}
			if (gl.state == STATE_STARTUP) {
		    	    gl.state = STATE_GAMEPLAY;
			    break;
			}
		    	break;
		case XK_r:
			if (gl.state == STATE_PAUSE) {
			    gl.state = STATE_GAMEPLAY;
			    break;
			}
			break;
		case XK_s:
			screenCapture();
			break;
		case XK_m:
			if (gl.state == STATE_PAUSE) {
			    gl.state = STATE_STARTUP;
			}
			break;
		case XK_w:
			timers.recordTime(&timers.walkTime);
			gl.walk ^= 1;
			break;
		case XK_e:
			break;
		case XK_c:
			if (gl.state == STATE_STARTUP) {
				gl.state = STATE_CREDITS;
				break;
			}
			break;
		case XK_f:
			break;
		case XK_space: {
			timers.recordTime(&timers.timeCurrent);
			double timeSpan = timers.timeDiff(&timers.bulletTime, &timers.timeCurrent);
			if (timeSpan > 0.10) { 
				++gl.bulletFrame;
				if (gl.bulletFrame > 2)
					gl.bulletFrame -= 2;
				timers.recordTime(&timers.bulletTime);
			}
			if (gl.bulletFrame < 2 ) {
			    bullets();
			}
			break;
		}
		case XK_Left:
			break;
		case XK_Right:
			break;
		case XK_Up:
			StartJump();
			//printf("jump\n");
			break;
		case XK_Down:
			break;
		case XK_equal:
			gl.delay -= 0.005;
			if (gl.delay < 0.005)
				gl.delay = 0.005;
			break;
		case XK_minus:
			gl.delay += 0.005;
			break;
		case XK_Escape:
			gl.done=1;
			break;
	}
}

Flt VecNormalize(Vec vec)
{
	Flt len, tlen;
	Flt xlen = vec[0];
	Flt ylen = vec[1];
	Flt zlen = vec[2];
	len = xlen*xlen + ylen*ylen + zlen*zlen;
	if (len == 0.0) {
		MakeVector(0.0,0.0,1.0,vec);
		return 1.0;
	}
	len = sqrt(len);
	tlen = 1.0 / len;
	vec[0] = xlen * tlen;
	vec[1] = ylen * tlen;
	vec[2] = zlen * tlen;
	return(len);
}

void music()
{
#ifdef USE_OPENAL_SOUND
	alutInit(0, NULL);
	if (alGetError() != AL_NO_ERROR) {
		printf("ERROR: alutInit()\n");
		return;
	}
	alGetError();
	float vec[6] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
	alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
	alListenerfv(AL_ORIENTATION,vec);
	alListenerf(AL_GAIN, 1.0f);
	alBuffer[0] = alutCreateBufferFromFile("./sound/cyber.wav");
	alBuffer[1] = alutCreateBufferFromFile("./sound/gunshot.wav");
	alGenSources(2, alSource);
	alSourcei(alSource[0], AL_BUFFER, alBuffer[0]);
	alSourcef(alSource[0], AL_GAIN, 0.3f);
	alSourcef(alSource[0], AL_PITCH, 1.0f);
	alSourcei(alSource[0], AL_LOOPING, AL_TRUE);
	if (alGetError() != AL_NO_ERROR) {
		printf("ERROR: setting source\n");
		return;
	}
	alSourcePlay(alSource[0]);
#endif //USE_OPENAL_SOUND
	return;
}
void deleteMusic() {
#ifdef USE_OPENAL_SOUND
	alDeleteSources(1, &alSource[0]);
    alDeleteBuffers(1, &alBuffer[0]);
	alDeleteSources(1, &alSource[1]);
    alDeleteBuffers(1, &alBuffer[1]);
    ALCcontext *Context = alcGetCurrentContext();
    ALCdevice *Device = alcGetContextsDevice(Context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(Context);
    alcCloseDevice(Device);
#endif //USE_OPENAL_SOUND
}
void StartJump() 
{
	if (gl.mainChar.onGround) {
		gl.mainChar.vel[1] = 12.0;
		gl.mainChar.onGround = false;
	}
}
void EndJump() 
{
	if (gl.mainChar.vel[1] > 6.0)
		gl.mainChar.vel[1] = 10.0;
}

void physics(void)
{
	if (gl.keys[XK_Right] || gl.keys[XK_Left] || gl.keys[XK_Up]) {
		//man is walking...
		//when time is up, advance the frame.
		timers.recordTime(&timers.timeCurrent);
		double timeSpan = timers.timeDiff(&timers.walkTime, &timers.timeCurrent);
		if (timeSpan > 0.15) { //gameDelay orig
			//advance
			++gl.walkFrame;
			if (gl.walkFrame >= 16)
				gl.walkFrame -= 16;
			timers.recordTime(&timers.walkTime);
		}
		if (gl.keys[XK_Left] && gl.collisionL == 0) {
			gl.camera[0] -= gl.speed;
			if (gl.camera[0] < 0.0)
				gl.camera[0] = 0.0;
			gl.xc[0] -= 0.00002;
			gl.xc[1] -= 0.00002;
			for (int i = 0; i < MAX_ENEMY; i++) {
				gl.enemyChar[i].pos[0] += gl.speed;
			}
			for (int i = 0; i < MAX_DRONE; i++) {
				gl.droneChar[i].pos[0] += gl.speed;
			}
			for (int i = 0; i < gl.nbeams; i++) {
				gl.beams[i].pos[0] += gl.speed;
			}
			gl.bullets->direction = 1;
		} else if (gl.keys[XK_Right] && gl.collisionR == 0) {
			gl.camera[0] += gl.speed;
			if (gl.camera[0] < 0.0)
				gl.camera[0] = 0.0;
			gl.xc[0] += 0.0002;
			gl.xc[1] += 0.0002;
			for (int i = 0; i < MAX_ENEMY; i++) {
				gl.enemyChar[i].pos[0] -= gl.speed;
			}
			for (int i = 0; i < MAX_DRONE; i++) {
				gl.droneChar[i].pos[0] -= gl.speed;
			}
			for (int i = 0; i < gl.nbeams; i++) {
				gl.beams[i].pos[0] -= gl.speed;
			}
			gl.bullets->direction = 0;
		}
	}
	//=================================================
	//Height Calculation
	Flt dd = lev.ftsz[0];
	//Flt offy = lev.tile_base;
	//int ncols_to_render = gl.xres / lev.tilesize[0] + 2;
	int col = (int)(gl.camera[0] / dd) + (232.0 /*500 original*// lev.tilesize[0] + 1.0); //(232.0 / lev +1.0) sprite
	col = col % lev.ncols;
	int hgt = 0.0;
	if (lev.dynamicHeight[col] != -1) {
		hgt = lev.dynamicHeight[col];
	} else {	
	for (int i = 0; i < lev.nrows; i++) {
	    if (lev.arr[i][col] != ' ') {
		hgt = i;
		lev.dynamicHeight[col] = i;
	    	break;
	    }
	}
	//	printf("%s %d is %d \n", "Height Calculated for Column: ", col, lev.dynamicHeight[col]);
	}
	for (int i = 0; i < lev.nrows; i++) {
	    if (lev.arr[i][col] != ' ') {
		hgt = i;
	    	break;
	    }
	}
	Flt h = lev.tilesize[1] * (lev.nrows-hgt) + lev.tile_base;
	//End of height calculation
	//==================================================
	
	//================================================================================
	//JUMP PHYSICS
    	gl.mainChar.vel[1] += GRAVITY;
	gl.mainChar.pos[1] += gl.mainChar.vel[1];
	gl.mainChar.pos[0] += gl.mainChar.vel[0];
	if (gl.mainChar.pos[1] < 136) {
		gl.mainChar.pos[1] = 136.0;
		gl.mainChar.vel[1] = 0.0;
		gl.mainChar.onGround = true;
	} else if (gl.mainChar.pos[1] <= h && gl.mainChar.pos[1] > h-lev.ftsz[0]) {
		gl.mainChar.vel[1] = 0.0;
	}
	else if (gl.mainChar.pos[1] <= h+72 && gl.mainChar.pos[1] > h) {
		gl.mainChar.pos[1] = h+72;
		gl.mainChar.vel[1] = 0.0;
		gl.mainChar.onGround = true;
	}
	//=================================================================================
	// Shooting
	checkCollision();
}
void checkCollision () {
	//----------------------------
	Flt dd = lev.ftsz[0];
	Flt offy = lev.tile_base;
	int ncols_to_render = gl.xres / lev.tilesize[0] + 2;
	int col = (int)(gl.camera[0] / dd);
	col = col % lev.ncols;
	//Partial tile offset must be determined here.
	//The leftmost tile might be partially off-screen.
	//cdd: camera position in terms of tiles.
	Flt cdd = gl.camera[0] / dd;
	//flo: just the integer portion
	Flt flo = floor(cdd);
	//dec: just the decimal portion
	Flt dec = (cdd - flo);
	//offx: the offset to the left of the screen to start drawing tiles
	Flt offx = -dec * dd;
	//Log("gl.camera[0]: %lf   offx: %lf\n",gl.camera[0],offx);
	for (int j=0; j<ncols_to_render; j++) {
		int row = lev.nrows-1;
		for (int i=0; i<lev.nrows; i++) {
			if (lev.arr[row][col] == 'w') {
			}
			if (lev.arr[row][col] == 'b') {
				Vec tr = { (Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, 0 };
				tileCollision(&tr);
				break;
			}
			if (lev.arr[row][col] == ' ') {
				Vec tr = { (Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, 0 };
				emptyCollision(&tr);
				break;
			}
			--row;
		}
		++col;
		col = col % lev.ncols;
	}
}

void tileCollision(Vec *tile) {
	int mainCharX = gl.xres/4.0;
	if ((mainCharX >= *tile[0]-lev.ftsz[0]) && (mainCharX <= (*tile[0]+lev.ftsz[0])) && gl.keys[XK_Right]) {
		gl.collisionR = 1;
		if (gl.mainChar.pos[1] > *tile[1]+200-lev.ftsz[1]) {
			gl.collisionR = 0;
		}
	}
	if (gl.keys[XK_Left] && gl.collisionR == 1) {
		gl.collisionR = 0;
	}
	if ((mainCharX <= *tile[0]+lev.ftsz[0]) && (mainCharX >= (*tile[0]-lev.ftsz[0])) && gl.keys[XK_Left]) {
		gl.collisionL = 1;
		if (gl.mainChar.pos[1] > *tile[1]+200-lev.ftsz[1]) {
			gl.collisionL = 0;
		}
	}
	if (gl.keys[XK_Right] && gl.collisionL == 1) {
		gl.collisionL = 0;
	}
	gl.underFlag =0;
}

void emptyCollision(Vec *tile) {
	int mainCharX = gl.xres/4.0;
	if ((mainCharX >= *tile[0]-lev.ftsz[0]/2) && (mainCharX <= *tile[0]+lev.ftsz[0]/2)
		&& gl.mainChar.pos[1] - 30 <= *tile[1]+lev.ftsz[1] 
		&& gl.mainChar.pos[1] - 30>= *tile[1]-lev.ftsz[1]) {
		gl.collisionR = 0;
		gl.collisionL = 0;
	}
	if ((mainCharX <= *tile[0]+lev.ftsz[0]) && (mainCharX >= *tile[0]-lev.ftsz[0])
		&& gl.mainChar.pos[1] - 30<= *tile[1]+lev.ftsz[1] 
		&& gl.mainChar.pos[1] - 30>= *tile[1]-lev.ftsz[1]) {
		gl.collisionR = 0;
		gl.collisionL = 0;
	}
}

void bulletPhysics() {
	for (int i = 0; i < gl.nbullets; i++) {
		glPushMatrix();
		glColor3f(1.0,1.0,1.0);
		glTranslatef(gl.bullets[i].pos[0], gl.bullets[i].pos[1], 0);
		float w = gl.bulletWidth;	
		float h = gl.bulletHeight;
		glBegin(GL_QUADS);
			glVertex2i(-w, -h);	
			glVertex2i(-w, h);	
			glVertex2i(w, h);	
			glVertex2i(w, -h);
		glEnd();
		glPopMatrix();
		if (gl.bullets[i].direction == 1) {
			gl.bullets[i].pos[0] = gl.bullets[i].pos[0] - gl.bullets[i].velocity;
		}	
		if (gl.bullets[i].direction == 0) {
			gl.bullets[i].pos[0] = gl.bullets[i].pos[0] + gl.bullets[i].velocity;
		}
		if (gl.bullets[i].pos[0] < 0 || gl.bullets[i].pos[0] > gl.xres) {
			gl.bullets[i] = gl.bullets[gl.nbullets - 1];
			gl.nbullets--;
		}	
	}
}
void bullets() {
	if (gl.direction == 1) {
		gl.bullets[gl.nbullets].pos[0] = gl.mainChar.pos[0]+290;
	}
	if (gl.direction == 0) {
		gl.bullets[gl.nbullets].pos[0] = gl.mainChar.pos[0]+200;
	}
	gl.bullets[gl.nbullets].pos[1] = gl.mainChar.pos[1]-30;
	gl.bullets[gl.nbullets].velocity = gl.bulletVelocity;
	gl.bullets[gl.nbullets].direction = gl.bullets->direction;
	gl.nbullets++;
#ifdef USE_OPENAL_SOUND
	alSourcei(alSource[1], AL_BUFFER, alBuffer[1]);
	alSourcef(alSource[1], AL_GAIN, 0.3f);
	alSourcef(alSource[1], AL_PITCH, 1.0f);
	alSourcei(alSource[1], AL_LOOPING, AL_FALSE);
	alSourcePlay(alSource[1]);
	//printf("bullet made\n");
#endif
}

void healthBar()
{
	Rect r;
        unsigned int c = 0xa30000;
        r.bot = gl.yres-30;
        r.left = (gl.xres/gl.xres) + 20;
        r.center = 0;
        ggprint8b(&r, 16, c, "HEALTH");
        Shape s;
        Shape box[200];
        for (int i = 0; i < gl.mainChar.health; i++) {
                box[i].width = 3;
                box[i].height = 10;
                box[i].center[0] = 20 + (i*6);
                box[i].center[1] = 555;
                box[i].center[2] = 0;
                s = box[i];
                glPushMatrix();
                glColor3ub(8, 146, 208);
                glTranslatef(s.center[0], s.center[1], s.center[2]);
                float w = s.width;
                float h = s.height;
                glBegin(GL_QUADS);
                        glVertex2i(-w, -h);
                        glVertex2i(-w, h);
                        glVertex2i(w, h);
                        glVertex2i(w, -h);
                        glEnd();
                glPopMatrix();
        }	
}

void Hitbox(float cy, float height, Sprite *enemy) 
{
	float miny = cy - 32;
	float maxy = cy + height - 32;
	int mainCharX = gl.xres/4.0;
	for (int i = 0; i < gl.nbullets; i++) {
		//printf("gl.bullets[i].pos[1]: %f\n", gl.bullets[i].pos[1]);
		//printf("pos1+miny: %f pos1+maxy: %f\n", (*pos1 + miny), (*pos1 + maxy));
		if (gl.bullets[i].pos[0] >= enemy->pos[0]+200 && gl.bullets[i].pos[0] <= enemy->pos[0]+300
			&& gl.bullets[i].pos[1] >= enemy->pos[1] + miny
			&& gl.bullets[i].pos[1] <= enemy->pos[1] + maxy) {
				enemy->health -= 5;
			if (enemy->health <= 0) {
				enemy->pos[0] = -1000000;
				enemy->pos[1] = -1000000;
			}
			gl.bullets[i] = gl.bullets[gl.nbullets - 1];
			gl.nbullets--;
		}
	}
	if (mainCharX >= enemy->pos[0]+200 && mainCharX <= enemy->pos[0]+300
			&& gl.mainChar.pos[1] >= enemy->pos[1] + miny
			&& gl.mainChar.pos[1] <= enemy->pos[1] + maxy) {
		gl.mainChar.health -= 1;
		if (gl.mainChar.health <= 0) {
			gl.state = STATE_GAMEOVER;
		}
	}
}

void droneHitbox() {
	float maxx = gl.xres/4.0+gl.mainChar.spritex/2;
	float minx = (gl.xres/4.0)-gl.mainChar.spritex/2;
	for (int i = 0; i < gl.nbeams; i++) {
		if(gl.beams[i].pos[1] <= gl.mainChar.pos[1]+32 &&
		gl.beams[i].pos[0] >= minx && gl.beams[i].pos[0] <= maxx) {
			if (gl.mainChar.health <= 0) {
				gl.state = STATE_GAMEOVER;
			}
			gl.mainChar.health -= 1;
			gl.beams[i] = gl.beams[gl.nbeams - 1];
			gl.nbeams--;
		} 
	}
}

void showHitbox(float cx, float cy, float height, float width, Sprite sprite) 
{
	float w = width;
	float h = height;
	glPushMatrix();
	glColor3f(1.0, 0.0, 0.0);
	glTranslated(sprite.pos[0], sprite.pos[1], 0);
	glBegin(GL_LINE_LOOP);
		glVertex2i(cx-w, cy-h);
		glVertex2i(cx-w, cy+h);
		glVertex2i(cx+w, cy+h);
		glVertex2i(cx+w, cy-h);
	glEnd();
	glPopMatrix();
}

void enemyPhysics() {
	if (gl.state == STATE_GAMEPLAY) {
		timers.recordTime(&timers.timeCurrent);
		double timeSpan = timers.timeDiff(&timers.enemyTime, &timers.timeCurrent);
		if (timeSpan > 0.18) { //gameDelay orig
		//advance
		++gl.enemyFrame;
		//printf("EnemyFrame: %i\n", gl.enemyFrame);
			if (gl.enemyFrame <= 16) {
				gl.enemyDirection = 1;
				for (int i = 0; i < MAX_ENEMY; i++) {
					gl.enemyChar[i].pos[0] += 16;
				}
			}
			if (gl.enemyFrame < 33 && gl.enemyFrame > 16) {
				gl.enemyDirection = 0;
				for (int i = 0; i < MAX_ENEMY; i++) {
					gl.enemyChar[i].pos[0] -= 16;
				}
			}
			if (gl.enemyFrame >= 33) {
				gl.enemyFrame -= 33;
			}
		timers.recordTime(&timers.enemyTime);
		}
	}
}

void dronePhysics() {
	if (gl.state == STATE_GAMEPLAY) {
		timers.recordTime(&timers.timeCurrent);
		double timeSpan = timers.timeDiff(&timers.droneTime, &timers.timeCurrent);
		if (timeSpan > 0.18) { //gameDelay orig
		//advance
		++gl.droneFrame;
		//printf("EnemyFrame: %i\n", gl.enemyFrame);
			if (gl.droneFrame <= 16) {
				gl.droneDirection = 0;
				for (int i = 0; i < MAX_DRONE; i++) {
					gl.droneChar[i].pos[0] += 16;
				}
			}
			if (gl.droneFrame < 33 && gl.droneFrame > 16) {
				gl.droneDirection = 1;
				for (int i = 0; i < MAX_DRONE; i++) {
					gl.droneChar[i].pos[0] -= 16;
					makeBeams(&gl.droneChar[i]);
				}
			}
			if (gl.droneFrame >= 33) {
				gl.droneFrame -= 33;
			}
		timers.recordTime(&timers.droneTime);
		}
	}
}

void beams () {	
	for (int i = 0; i < gl.nbeams; i++) {
		glPushMatrix();
		glColor3f(1.0,0.0,0.0);
		glTranslatef(gl.beams[i].pos[0], gl.beams[i].pos[1], 0);
		float w = gl.beamWidth;	
		float h = gl.beamHeight;
		glBegin(GL_QUADS);
			glVertex2i(-w, -h);	
			glVertex2i(-w, h);	
			glVertex2i(w, h);	
			glVertex2i(w, -h);
		glEnd();
		glPopMatrix();
		gl.beams[i].pos[1] = gl.beams[i].pos[1] - gl.beams[i].velocity;	
		if (gl.beams[i].pos[1] < 0) {
			gl.beams[i] = gl.beams[gl.nbeams - 1];
			gl.nbeams--;
		}	
	}
}
void makeBeams(Sprite *drone) {
	for (int i=0; i < gl.nbeams; i++) {
	gl.beams[gl.nbeams].pos[0] = drone->pos[0]+232;
	gl.beams[gl.nbeams].pos[1] = drone->pos[1]+128;
	}
	gl.beams[gl.nbeams].velocity = gl.beamVelocity;
	gl.beams[gl.nbeams].direction = gl.beams->direction;
	gl.nbeams++;
}

void renderDrone(Sprite *drone)
{
	float cx = gl.xres/4.0;
	float cy = gl.yres/4 -32; //(gl.yres/gl.yres) to test tiles //gl.xres/4.0 original
	float h = 60.0;
	float w = 48.0;
	glPushMatrix();
	glTranslated(drone->pos[0], drone->pos[1], 0);
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.droneTexture);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	int ix = gl.droneFrame % 16; //8.0
	int iy = 0;
	if (gl.droneFrame >= 4) //8.0
		iy = 1;
	float tx = (float)ix / 4.0; //8.0 orig
	float ty = (float)iy / 2.0;
	glBegin(GL_QUADS);
		if (gl.droneDirection) {
			glTexCoord2f(tx,      ty+.5); glVertex2i(cx-w, cy-h);
			glTexCoord2f(tx,      ty);    glVertex2i(cx-w, cy+h);
			glTexCoord2f(tx+.250, ty);    glVertex2i(cx+w, cy+h); //tx+.125 orig
			glTexCoord2f(tx+.250, ty+.5); glVertex2i(cx+w, cy-h);
		} else {
			glTexCoord2f(tx+.250, ty+.5); glVertex2i(cx-w, cy-h);
			glTexCoord2f(tx+.250, ty);    glVertex2i(cx-w, cy+h);
			glTexCoord2f(tx,      ty);    glVertex2i(cx+w, cy+h);
			glTexCoord2f(tx,      ty+.5); glVertex2i(cx+w, cy-h);
		}
	glEnd();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_ALPHA_TEST);
	showHitbox(cx, cy, h, w, *drone);
	Hitbox(cy, h, drone);
	enemyHealthBar(cx, cy, drone);
	//printf("gl.enemyChar.pos[0]: %f\n", gl.enemyChar.pos[0]);
}

void enemyHealthBar(float cx, float cy, Sprite *enemy)
{
        Shape s;
        Shape box[200];
        for (int i = 0; i < enemy->health; i++) {
                box[i].width = 3;
                box[i].height = 6;
                box[i].center[0] = enemy->pos[0] + (i*6) - 30;
                box[i].center[1] = enemy->pos[1] + 50;
                box[i].center[2] = 0;
                s = box[i];
                glPushMatrix();
                glColor3ub(8, 146, 208);
                glTranslatef(s.center[0], s.center[1], s.center[2]);
                float w = s.width;
                float h = s.height;
                glBegin(GL_QUADS);
                        glVertex2i(cx-w, cy-h);
                        glVertex2i(cx-w, cy+h);
                        glVertex2i(cx+w, cy+h);
                        glVertex2i(cx+w, cy-h);
                        glEnd();
                glPopMatrix();
        }	
}

void renderEnemy(Sprite *enemy)
{
	float cx = gl.xres/4.0;
	float cy = gl.yres/4 -32; //(gl.yres/gl.yres) to test tiles //gl.xres/4.0 original
	float h = 60.0;
	float w = 48.0;
	glPushMatrix();
	glTranslated(enemy->pos[0], enemy->pos[1], 0);
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.enemyTexture);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	int ix = gl.enemyFrame % 16; //8.0
	int iy = 0;
	if (gl.enemyFrame >= 4) //8.0
		iy = 1;
	float tx = (float)ix / 4.0; //8.0 orig
	float ty = (float)iy / 2.0;
	glBegin(GL_QUADS);
		if (gl.enemyDirection) {
			glTexCoord2f(tx,      ty+.5); glVertex2i(cx-w, cy-h);
			glTexCoord2f(tx,      ty);    glVertex2i(cx-w, cy+h);
			glTexCoord2f(tx+.250, ty);    glVertex2i(cx+w, cy+h); //tx+.125 orig
			glTexCoord2f(tx+.250, ty+.5); glVertex2i(cx+w, cy-h);
		} else {
			glTexCoord2f(tx+.250, ty+.5); glVertex2i(cx-w, cy-h);
			glTexCoord2f(tx+.250, ty);    glVertex2i(cx-w, cy+h);
			glTexCoord2f(tx,      ty);    glVertex2i(cx+w, cy+h);
			glTexCoord2f(tx,      ty+.5); glVertex2i(cx+w, cy-h);
		}
	glEnd();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_ALPHA_TEST);
	showHitbox(cx, cy, h, w, *enemy);
	Hitbox(cy, h, enemy);
	enemyHealthBar(cx, cy, enemy);
	droneHitbox();
	//printf("gl.enemyChar.pos[0]: %f\n", gl.enemyChar.pos[0]);
}

void gameOver()
{
	timers.recordTime(&timers.timeCurrent);
	double timeSpan = timers.timeDiff(&timers.gameoverTime, &timers.timeCurrent);
	if (timeSpan > 1.0) {
		++gl.gameoverFrame;
		--gl.countdown;
		timers.recordTime(&timers.gameoverTime);
	}
	if (gl.gameoverFrame <= 10) {
		if (gl.keys[XK_c]) {
			gl.state = STATE_GAMEPLAY;
			init();
			return;
		}
		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glPushMatrix();
		glColor3f(1.0,1.0,1.0);
		glBindTexture(GL_TEXTURE_2D, gl.bluebackTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2i(0, 0);
			glTexCoord2f(0.0, 0.0); glVertex2i(0,  gl.yres);
			glTexCoord2f(1.0, 0.0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1.0, 1.0); glVertex2i(gl.xres,  0);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glPushMatrix();
		glColor3f(1.0,1.0,1.0);
		glBindTexture(GL_TEXTURE_2D, gl.gameoverTexture);
		glEnable(GL_ALPHA_TEST);
		glColor4ub(255,255,255,255);
		//glEnable(GL_TEXTURE_2D);
		glAlphaFunc(GL_GREATER, 0.0f);
		glColor4ub(255,255,255,255);
		glBegin(GL_QUADS);
		        glTexCoord2f(0.0, 1.0); glVertex2i(0,0);
		        glTexCoord2f(0.0, 0.0); glVertex2i(0,gl.yres);
		        glTexCoord2f(1.0, 0.0); glVertex2i(gl.xres,gl.yres);
		        glTexCoord2f(1.0, 1.0); glVertex2i(gl.xres,0);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_ALPHA_TEST);
		
		Rect r;
        	unsigned int c = 0xFFFF00;
        	r.bot = gl.yres/4;
        	r.left = (gl.xres/2);
        	r.center = 0;
        	ggprint8b(&r, 16, c, "CONTINUE? %i", gl.countdown);
        	ggprint8b(&r, 16, c, "PRESS C TO CONTINUE");	
	}
	if (gl.gameoverFrame > 10) {
		gl.state = STATE_STARTUP;
	}
}

void levelOver()
{
	gl.state = STATE_COMPLETE;
	timers.recordTime(&timers.timeCurrent);
	double timeSpan = timers.timeDiff(&timers.gameoverTime, &timers.timeCurrent);
	if (timeSpan > 1.0) {
		++gl.gameoverFrame;
		timers.recordTime(&timers.gameoverTime);
	}
	if (gl.gameoverFrame <= 5) {
		glPushMatrix();
		glBindTexture(GL_TEXTURE_2D, gl.endTexture);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2i(0, 0);
			glTexCoord2f(0.0, 0.0); glVertex2i(0,  gl.yres);
			glTexCoord2f(1.0, 0.0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1.0, 1.0); glVertex2i(gl.xres,  0);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (gl.gameoverFrame > 5) {
		gl.state = STATE_STARTUP;
	}
}
void renderFPS()
{
	Rect r;
	double timeSpan = timers.timeDiff(&timers.frameTime, &timers.timeCurrent);
	gl.fps++;
	if (timeSpan >= 1.0) {
		gl.fps = 0;
		timers.recordTime(&timers.frameTime);
	}
	unsigned int c = 0x00ffff44;
	r.bot = gl.yres - 20;
	r.left = gl.xres-100;
	r.center = 0;
	ggprint8b(&r, 16, c, "fps: %0.2f", gl.fps/timeSpan);
}

void renderBackground()
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.cyberstreetTexture);
	glBegin(GL_QUADS);
		glTexCoord2f(gl.xc[0], gl.yc[1]); glVertex2i(0,0);
		glTexCoord2f(gl.xc[0], gl.yc[0]); glVertex2i(0, gl.yres);
                glTexCoord2f(gl.xc[1], gl.yc[0]); glVertex2i(gl.xres, gl.yres);
                glTexCoord2f(gl.xc[1], gl.yc[1]); glVertex2i(gl.xres, 0);
        glEnd();
        glPopMatrix();
        glBindTexture(GL_TEXTURE_2D, 0);

}

void render(void)
{
	if (gl.mainChar.health <= 0) {
		gl.state = STATE_GAMEOVER;
		gameOver();
	}
	if (gl.state == STATE_GAMEPLAY || gl.state == STATE_COMPLETE) {
	//Clear the screen
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	renderBackground();
	float cx = gl.xres/4.0;
	float cy = (gl.yres/gl.yres)-32; //(gl.yres/gl.yres) to test tiles //gl.xres/4.0 original
	
	//----------------------------
	//Render the level tile system
	//----------------------------
	Flt dd = lev.ftsz[0];
	Flt offy = lev.tile_base;
	int ncols_to_render = gl.xres / lev.tilesize[0] + 2;
	int col = (int)(gl.camera[0] / dd);
	col = col % lev.ncols;
	//Partial tile offset must be determined here.
	//The leftmost tile might be partially off-screen.
	//cdd: camera position in terms of tiles.
	Flt cdd = gl.camera[0] / dd;
	//flo: just the integer portion
	Flt flo = floor(cdd);
	//dec: just the decimal portion
	Flt dec = (cdd - flo);
	//offx: the offset to the left of the screen to start drawing tiles
	Flt offx = -dec * dd;
	//Log("gl.camera[0]: %lf   offx: %lf\n",gl.camera[0],offx);
	for (int j=0; j<ncols_to_render; j++) {
		int row = lev.nrows-1;
		for (int i=0; i<lev.nrows; i++) {
			if (lev.arr[row][col] == 'w') {
				glColor3f(0.8, 0.8, 0.6);
				glPushMatrix();
				glBindTexture(GL_TEXTURE_2D, gl.floorTexture);
				Vec tr = { (Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, 0 };
				glTranslated(tr[0],tr[1],tr[2]);
				int tx = lev.tilesize[0];
				int ty = lev.tilesize[1];
				glBegin(GL_QUADS);
					glTexCoord2f(0.0, 1.0); glVertex2i( 0,  0);
					glTexCoord2f(0.0, 0.0); glVertex2i( 0, ty);
					glTexCoord2f(1.0, 0.0); glVertex2i(tx, ty);
					glTexCoord2f(1.0, 1.0); glVertex2i(tx,  0);
				glEnd();
				glPopMatrix();
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			if (lev.arr[row][col] == 'b') {
				glColor3f(0.9, 0.2, 0.2);
				glPushMatrix();
				glBindTexture(GL_TEXTURE_2D, gl.platformTexture);
				Vec tr = { (Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, 0 };
				glTranslated(tr[0],tr[1],tr[2]);
				int tx = lev.tilesize[0];
				int ty = lev.tilesize[1];
				glBegin(GL_QUADS);
					glTexCoord2f(0.0, 1.0); glVertex2i( 0,  0);
					glTexCoord2f(0.0, 0.0); glVertex2i( 0, ty);
					glTexCoord2f(1.0, 0.0); glVertex2i(tx, ty);
					glTexCoord2f(1.0, 1.0); glVertex2i(tx,  0);
				glEnd();
				glPopMatrix();
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			--row;
		}
		++col;
		col = col % lev.ncols;
	}

	//===================================
	healthBar();
	bulletPhysics();
	beams();

	//===================================
	// CHARACTER SPRITE
	//===================================
	float h = 60.0;
	float w = h * .8;
	gl.mainChar.spritex = h;
	gl.mainChar.spritey = w;
	glPushMatrix();
	glTranslated(gl.mainChar.pos[0],gl.mainChar.pos[1], 0);
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.mainChar.tex);
	//
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	int ix = gl.walkFrame % 16; //8.0
	int iy = 0;
	if (gl.walkFrame >= 4) //8.0
		iy = 1;
	float tx = (float)ix / 4.0; //8.0 orig
	float ty = (float)iy / 2.0;
	if (gl.keys[XK_Right])
		gl.direction=1;
	if (gl.keys[XK_Left])
		gl.direction=0;
	glBegin(GL_QUADS);
		if (gl.direction) {
			glTexCoord2f(tx,      ty+.5); glVertex2i(cx-w, cy-h);
			glTexCoord2f(tx,      ty);    glVertex2i(cx-w, cy+h);
			glTexCoord2f(tx+.250, ty);    glVertex2i(cx+w, cy+h); //tx+.125 orig
			glTexCoord2f(tx+.250, ty+.5); glVertex2i(cx+w, cy-h);
		} else {
			glTexCoord2f(tx+.250, ty+.5); glVertex2i(cx-w, cy-h);
			glTexCoord2f(tx+.250, ty);    glVertex2i(cx-w, cy+h);
			glTexCoord2f(tx,      ty);    glVertex2i(cx+w, cy+h);
			glTexCoord2f(tx,      ty+.5); glVertex2i(cx+w, cy-h);
		}
	glEnd();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_ALPHA_TEST);
	showHitbox(cx, cy, h, w, gl.mainChar);
	for(int i=0; i < MAX_ENEMY; i++) {
		renderEnemy(&gl.enemyChar[i]);
	}
	for(int i=0; i < MAX_DRONE; i++) {
		renderDrone(&gl.droneChar[i]);
	}
	renderFPS();
	if (gl.camera[0] >= 10000) {
		gl.state = STATE_COMPLETE;
		levelOver();
	}
	}



	//=========================================================================
	//STARTUP STATE
	if (gl.state == STATE_STARTUP) {
		Rect r;
		glPushMatrix();
		glColor3f(1.0,1.0,1.0);
		glBindTexture(GL_TEXTURE_2D, gl.bluebackTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2i(0, 0);
			glTexCoord2f(0.0, 0.0); glVertex2i(0,  gl.yres);
			glTexCoord2f(1.0, 0.0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1.0, 1.0); glVertex2i(gl.xres,  0);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);

	    	float h1 = 200;
		float w1 = 200;			
		glPushMatrix();
		glColor3f(0.75,0.75,0.75);
		glTranslated(gl.xres/2, gl.yres/2 + 100, 0);
		glBindTexture(GL_TEXTURE_2D, gl.cyberMenuTexture);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2i(-w1, -h1);
			glTexCoord2f(0.0, 0.0); glVertex2i(-w1,  h1);
			glTexCoord2f(1.0, 0.0); glVertex2i(w1,   h1);
			glTexCoord2f(1.0, 1.0); glVertex2i(w1,  -h1);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_ALPHA_TEST);
		unsigned int c = 0x002d88d8;	
		r.bot = gl.yres/2 - 100;
		r.center = 0;
		r.left = gl.xres/2 - 100;
		ggprint16(&r, 16, c, "PLAY");
		ggprint16(&r, 16, c, "CREDITS");

	
	}
	//===========================================================================
	//PAUSE SCREEN
	if (gl.state == STATE_PAUSE) {
	    	int h = 100.0;
		int w = 200.0;			
		Rect r;
		glPushMatrix();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.58,0.58,0.72,0.8);
		glTranslated(gl.xres/2, gl.yres/2, 0);
		glBegin(GL_QUADS);
			glVertex2i(-w, -h);
			glVertex2i(-w,  h);
			glVertex2i(w,   h);
			glVertex2i(w,  -h);
		glEnd();
		glDisable(GL_BLEND);
		glPopMatrix();
		r.bot = gl.yres/2 + 80;
		r.left = gl.xres/2;
		r.center = 1;
		ggprint8b(&r, 16, 0, "PAUSE SCREEN");
		r.center = 0;
		r.left = gl.xres/2 - 100;
		ggprint8b(&r, 16, 0, "M - Main Menu");
		ggprint8b(&r, 16, 0, "R - Resume");

	
	}
	//===========================================================================
	//Credits Screen
	if (gl.state == STATE_CREDITS) {
		if (gl.keys[XK_m]) {
			gl.state = STATE_STARTUP;
		}
		glPushMatrix();
		glColor3f(1.0,1.0,1.0);
		glBindTexture(GL_TEXTURE_2D, gl.creditsTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2i(0, 0);
			glTexCoord2f(0.0, 0.0); glVertex2i(0,  gl.yres);
			glTexCoord2f(1.0, 0.0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1.0, 1.0); glVertex2i(gl.xres,  0);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
	
	}
}







