#include "viewer.hpp"
#include <iostream>
#include <sstream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <assert.h>
#include "appwindow.hpp"

#define DEFAULT_GAME_SPEED 500
Viewer::Viewer()
{
	
	// Set all rotationAngles to 0
	rotationAngleX = 0;
	rotationAngleY = 0;
	rotationAngleZ = 0;
	rotationSpeed = 0;
	
	// Default draw mode is multicoloured
	currentDrawMode = Viewer::MULTICOLOURED;

	// Default scale factor is 1
	scaleFactor = 1;
	
	// Assume no buttons are held down at start
	shiftIsDown = false;
	mouseB1Down = false;
	mouseB2Down = false;
	mouseB3Down = false;
	rotateAboutX = false;
	rotateAboutY = false;
	rotateAboutZ = false;
	
	linesLeftTillNextLevel = 10;
	
	// Game starts at a slow pace of 500ms
	gameSpeed = 500;
	
	// By default turn double buffer on
	doubleBuffer = false;
	
	gameOver = false;
	
	// Create and initialize all the different side colours
	sideColours = new SideColour[54];
	initSideColours();
	
	Glib::RefPtr<Gdk::GL::Config> glconfig;
	
	// Ask for an OpenGL Setup with
	//  - red, green and blue component colour
	//  - a depth buffer to avoid things overlapping wrongly
	//  - double-buffered rendering to avoid tearing/flickering
	//	- Multisample rendering to smooth out edges
	glconfig = Gdk::GL::Config::create(	Gdk::GL::MODE_RGB |
										Gdk::GL::MODE_DEPTH |
										Gdk::GL::MODE_DOUBLE |
										Gdk::GL::MODE_MULTISAMPLE );
	if (glconfig == 0) {
	  // If we can't get this configuration, die
	  std::cerr << "Unable to setup OpenGL Configuration!" << std::endl;
	  abort();
	}

	// Accept the configuration
	set_gl_capability(glconfig);

	// Register the fact that we want to receive these events
	add_events(	Gdk::BUTTON1_MOTION_MASK	|
				Gdk::BUTTON2_MOTION_MASK    |
				Gdk::BUTTON3_MOTION_MASK    |
				Gdk::BUTTON_PRESS_MASK      | 
				Gdk::BUTTON_RELEASE_MASK    |
				Gdk::KEY_PRESS_MASK 		|
				Gdk::VISIBILITY_NOTIFY_MASK);
		
	// Create Game
	game = new Game(10, 20);
	
	// Start game tick timer
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
	
/*	// Start render timer
	sigc::slot1<bool, GdkEventExpose *> reRender_slot = sigc::mem_fun(*this, &Viewer::on_expose_event);
	Glib::RefPtr<Glib::TimeoutSource> timeout_source = Glib::TimeoutSource::create(100);
	timeout_source->connect(sigc::bind( reRender_slot, new GdkEventExpose() ) );
 	timeout_source->attach(Glib::MainContext::get_default());
*/

/*	rotateTimer = Glib::signal_timeout().connect(sigc::bind(sigc::mem_fun(*this, &Viewer::on_expose_event), (GdkEventExpose *)NULL), 100);
	rotateTimer.disconnect();
*/
}

Viewer::~Viewer()
{
	delete(game);
  // Nothing to do here right now.
}

void Viewer::invalidate()
{
  //Force a rerender
  Gtk::Allocation allocation = get_allocation();
  get_window()->invalidate_rect( allocation, false);
  
}

void Viewer::on_realize()
{
	// Do some OpenGL setup.
	// First, let the base class do whatever it needs to
	Gtk::GL::DrawingArea::on_realize();
	
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	
	if (!gldrawable)
		return;
	
	if (!gldrawable->gl_begin(get_gl_context()))
		return;
	
	// Just enable depth testing and set the background colour.
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.7, 0.7, 1.0, 0.0);
	
	// Let OpenGL normalize vectors for lighting
	glEnable(GL_NORMALIZE);
	
	
	// Initialize lighting settings
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_LIGHTING);
	// Create one light source
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Define properties of light 
	float ambientLight0[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float diffuseLight0[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specularLight0[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	float position0[] = { 5.0f, 0.0f, 0.0f, 1.0f };	
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	
	gldrawable->gl_end();
}

bool Viewer::on_expose_event(GdkEventExpose* event)
{
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	
	if (!gldrawable) return false;

	if (!gldrawable->gl_begin(get_gl_context()))
		return false;

	if (doubleBuffer)
	{
		glDrawBuffer(GL_BACK);	
	}
	else
	{
		glDrawBuffer(GL_FRONT);
	}
		
			
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Modify the current projection matrix so that we move the 
	// camera away from the origin.  We'll draw the game at the
	// origin, and we need to back up to see it.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glTranslated(0.0, 0.0, -40.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Not implemented: set up lighting (if necessary)

	// Scale and rotate the scene
	
	if (scaleFactor != 1)
		glScaled(scaleFactor, scaleFactor, scaleFactor);
		
	if (rotationAngleX != 0)
		glRotated(rotationAngleX, 1, 0, 0);
	
	if (rotationAngleY != 0)
		glRotated(rotationAngleY, 0, 1, 0);

	if (rotationAngleZ != 0)
		glRotated(rotationAngleZ, 0, 0, 1);
	
	if (mouseB1Down || rotateAboutX)
	{
		rotationAngleX += rotationSpeed;
		if (rotationAngleX > 360)
			rotationAngleX -= 360;
	}
	if (mouseB2Down || rotateAboutY)
	{
		rotationAngleY += rotationSpeed;
		if (rotationAngleY > 360)
			rotationAngleY -= 360;
	}
	if (mouseB3Down || rotateAboutZ)
	{
		rotationAngleZ += rotationSpeed;
		if (rotationAngleZ > 360)
			rotationAngleZ -= 360;
	}
	
	// You'll be drawing unit cubes, so the game will have width
	// 10 and height 24 (game = 20, stripe = 4).  Let's translate
	// the game so that we can draw it starting at (0,0) but have
	// it appear centered in the window.
	glTranslated(-5.0, -12.0, 0.0);
	

	
	// Draw Border
	for (int y = -1;y< 20;y++)
	{
		drawCube(y, -1, 7, GL_LINE_LOOP);
		//drawCube(y, -1, 7, GL_QUADS);
		
		drawCube(y, 10, 7, GL_LINE_LOOP);
		//drawCube(y, 10, 7, GL_QUADS);
	}
	for (int x = 0;x < 10; x++)
	{
		drawCube (-1, x, 7, GL_LINE_LOOP);
		//drawCube (-1, x, 7, GL_QUADS);
	}
	
	// Draw current state of tetris
	if (currentDrawMode == Viewer::WIRE)
	{
		for (int i = 23;i>=0;i--) // row
		{
			for (int j = 9; j>=0;j--) // column
			{
				drawCube (i, j, game->get(i, j), GL_LINE_LOOP );
			}
		}
	}
	else if (currentDrawMode == Viewer::MULTICOLOURED)
	{
		for (int i = 23;i>=0;i--) // row
		{
			for (int j = 9; j>=0;j--) // column
			{	
				// Draw outline for cube
				if (game->get(i, j) != -1)
					drawCube(i, j, 8, GL_LINE_LOOP);
					
				drawCube (i, j, game->get(i, j), GL_QUADS, true );
			}
		}
	}
	else if (currentDrawMode == Viewer::FACE)
	{
		for (int i = 23;i>=0;i--) // row
		{
			for (int j = 9; j>=0;j--) // column
			{				
				// Draw outline for cube
				if (game->get(i, j) != -1)
					drawCube(i, j, 8, GL_LINE_LOOP);
					
				drawCube (i, j, game->get(i, j), GL_QUADS );
			}
		}	
	}	
	
	if (gameOver)
	{
		// Some game over animation
	}
 	// We pushed a matrix onto the PROJECTION stack earlier, we 
	// need to pop it.

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Swap the contents of the front and back buffers so we see what we
	// just drew. This should only be done if double buffering is enabled.
	if (doubleBuffer)
	{
		gldrawable->swap_buffers();
	}		
	else
	{
		glFlush();
	}		
		
	gldrawable->gl_end();

	return true;
}

bool Viewer::on_configure_event(GdkEventConfigure* event)
{
  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

  if (!gldrawable) return false;
  
  if (!gldrawable->gl_begin(get_gl_context()))
    return false;

  // Set up perspective projection, using current size and aspect
  // ratio of display

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, event->width, event->height);
  gluPerspective(40.0, (GLfloat)event->width/(GLfloat)event->height, 0.1, 1000.0);

  // Reset to modelview matrix mode
  
  glMatrixMode(GL_MODELVIEW);

  gldrawable->gl_end();

  return true;
}

bool Viewer::on_button_press_event(GdkEventButton* event)
{
	startPos[0] = event->x;
	startPos[1] = event->y;
	mouseDownPos[0] = event->x;
	mouseDownPos[1] = event->y;
	std::cerr << "Stub: Button " << event->button << " pressed" << " at pos " << event->x << ", " << event->y << std::endl;
	
	if ((rotateAboutX || rotateAboutY || rotateAboutZ) && !shiftIsDown)
	{
		rotationSpeed = 0;
		rotateTimer.disconnect();
	}
		
	std::cerr << "Reset Rotation speed " << rotationSpeed << std::endl;
	if (event->button == 1)
		mouseB1Down = true;	
	if (event->button == 2)
		mouseB2Down = true;
	if (event->button == 3)
		mouseB3Down = true;
	
	if (!shiftIsDown)
	{
		rotateAboutX = false;
		rotateAboutY = false;
		rotateAboutZ = false;
	}
	std::cerr << "Button 1 is " << mouseB1Down << " button 2 is " << mouseB2Down << "button 3 is " << mouseB3Down << std::endl;
	return true;
}

bool Viewer::on_button_release_event(GdkEventButton* event)
{
	startScalePos[0] = 0;
	startScalePos[1] = 0;
	//std::cerr << "Stub: Button " << event->button << " released" << " at pos " << event->x << ", " << event->y << std::endl;
	std::cerr << "Mouse Release " << event->x << ", " << event->y << " button: " << event->button << std::endl;		
	if (shiftIsDown)
	{
		return true;
	}

	std::cerr << "mdown time: " << event->time << " last motion time: " << timeOfLastMotionEvent;
	long difference = (long)timeOfLastMotionEvent - (long)event->time;
	std::cerr << " diff time " << difference << std::endl;
	if (difference > -50)
		rotationSpeed = (event->x - mouseDownPos[0]) / 10;

	std::cerr << "Change in x: " << (event->x - mouseDownPos[0]) << ", Rotation speed " << rotationSpeed << std::endl;

	if (event->button == 1)
	{
		mouseB1Down = false;
		rotateAboutX = true;
	}	
	if (event->button == 2)
	{
		mouseB2Down = false;
		rotateAboutY = true;
	}
		
	if (event->button == 3)
	{
		mouseB3Down = false;
		rotateAboutZ = true;
	}
		
	if (mouseB1Down && mouseB2Down && mouseB3Down)
	{
		startPos[0] = 0;
		startPos[1] = 0;
	}
		
	invalidate();
	return true;
}

bool Viewer::on_motion_notify_event(GdkEventMotion* event)
{
	double x2x1;
	if (shiftIsDown) // Start Scaling
	{
		if (startScalePos[0] == 0 && startScalePos[1] == 0)
		{
			startScalePos[0] = event->x;
			startScalePos[1] = event->y;
			return true;
		}
				
		x2x1 = (event->x - startScalePos[0]);
		
		if (x2x1 != 0)
		{
			std::cerr << "Change in x: " << x2x1 << std::endl;
			x2x1 /= 500;
			scaleFactor += x2x1;				
		}
		
		if (scaleFactor < 0.5)
			scaleFactor = 0.5;
		else if (scaleFactor > 2)
			scaleFactor = 2;
			
		std::cerr << "Scale factor " << scaleFactor << std::endl;
		startScalePos[0] = event->x;
		startScalePos[1] = event->y;
		if (rotateTimer.connected())
			return true;
	}
	else // Start rotating
	{
		timeOfLastMotionEvent = event->time;
		x2x1 = event->x - startPos[0];
		x2x1 /= 10;
		
		if (mouseB1Down) // Rotate x
			rotationAngleX += x2x1;
		if (mouseB2Down) // Rotate y
			rotationAngleY += x2x1;
		if (mouseB3Down) // Rotate z
			rotationAngleZ += x2x1;
			
		//rotationSpeed = x2x1

		// Reset the tickTimer
		if (!rotateTimer.connected())
			rotateTimer = Glib::signal_timeout().connect(sigc::bind(sigc::mem_fun(*this, &Viewer::on_expose_event), (GdkEventExpose *)NULL), 100);
	}
	
	std::cerr << "Mouse Motion: " << event->x << ", " << event->y <<  " state " << event->state << std::endl;
	startPos[0] = event->x;
	startPos[1] = event->y;
	invalidate();
	return true;
}

/*void Viewer::drawCube(int y, int x, int colourId, GLenum mode)
{
	if (mode == GL_LINE_LOOP)
		glLineWidth (2);
		
	switch (colourId)
	{
		case 0:
			glColor3d(0.514, 0.839, 0.965); // blue
			break;
		case 1:
			glColor3d(0.553, 0.60 , 0.796); // purple
			break;
		case 2:
			glColor3d(0.988, 0.627, 0.373); // orange
			break;
		case 3:
			glColor3d(0.690, 0.835, 0.529); // green
			break;
		case 4:
			glColor3d(0.976, 0.553, 0.439); // red
			break;
		case 5:
			glColor3d(0.949, 0.388, 0.639); // pink
			break;
		case 6:
			glColor3d(1	 , 0.792, 0.204); // yellow
			break;
		case 7:
			glColor3d(0.0, 0.0, 0.0); // black
			break;
		case 8: 
			glColor3d(0, 0, 0);
			break;
		default:
			return;
	}
	
	double innerXMin = 0;
	double innerYMin = 0;
	double innerXMax = 1;
	double innerYMax = 1;
	double zMax = 1;
	double zMin = 0;
	
	// Front faces
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();

	// Back of front face
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
	glEnd();

	// top face
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();

	// left face
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
	glEnd();

	// bottom face
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
	glEnd();

	// right face
	glBegin(mode);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
	glEnd();
	
	
	innerXMin = 0.25;
	innerYMin = 0.25;
	innerXMax = 0.75;
	innerYMax = 0.75;
	zMin = 0.7;
	zMax = 1;
	
	indexIntoSideColours= colourId*6;
	
	
		// Back of front face
		glBegin(mode);
			glVertex3d(innerXMin + x, innerYMin + y, zMin);
			glVertex3d(innerXMax + x, innerYMin + y, zMin);
			glVertex3d(innerXMax + x, innerYMax + y, zMin);
			glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glEnd();

		// Front of front face		
		glBegin(mode);
			glVertex3d(innerXMin + x, innerYMin + y, zMax);
			glVertex3d(innerXMax + x, innerYMin + y, zMax);
			glVertex3d(innerXMax + x, innerYMax + y, zMax);
			glVertex3d(innerXMin + x, innerYMax + y, zMax);
		glEnd();
		

		indexIntoSideColours++;
		
		// top face
		glColor3d(sideColours[indexIntoSideColours].r, sideColours[indexIntoSideColours].g, sideColours[indexIntoSideColours].b);
		
		glBegin(mode);
			glVertex3d(0 + x, 1 + y, zMin);
			glVertex3d(1 + x, 1 + y, zMin);
			glVertex3d(innerXMax + x, innerYMax + y, zMax);
			glVertex3d(innerXMin + x, innerYMax + y, zMax);
		glEnd();
		

		indexIntoSideColours++;

		// left face
		glColor3d(sideColours[indexIntoSideColours].r, sideColours[indexIntoSideColours].g, sideColours[indexIntoSideColours].b);
		
		glBegin(mode);
			glVertex3d(0 + x, 0 + y, zMin);
			glVertex3d(0 + x, 1 + y, zMin);
			glVertex3d(innerXMin + x, innerYMax+ y, zMax);
			glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glEnd();
		

		indexIntoSideColours++;
		
		//bottom face
		glColor3d(sideColours[indexIntoSideColours].r, sideColours[indexIntoSideColours].g, sideColours[indexIntoSideColours].b);

		glBegin(mode);
			glVertex3d(0 + x, 0 + y, zMin);
			glVertex3d(1 + x, 0 + y, zMin);
			glVertex3d(innerXMax + x, innerYMin + y, zMax);
			glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glEnd();
		

		indexIntoSideColours++;
		
		// right face
		glColor3d(sideColours[indexIntoSideColours].r, sideColours[indexIntoSideColours].g, sideColours[indexIntoSideColours].b);
		
		glBegin(mode);
			glVertex3d(1 + x, 0 + y, zMin);
			glVertex3d(1 + x, 1 + y, zMin);
			glVertex3d(innerXMax + x, innerYMax + y, zMax);
			glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glEnd();
		

}*/

void Viewer::drawCube(int y, int x, int colourId, GLenum mode, bool multiColour)
{
	if (mode == GL_LINE_LOOP)
		glLineWidth (2);
	
	double r, g, b;
	r = 0;
	g = 0;
	b = 0;
	switch (colourId)
	{
		case 0:
			r = 0.514;
			g = 0.839;
			b = 0.965;
			glColor3d(r, g, b); // blue
			break;
		case 1:		
			r = 0.553;
			g = 0.6;
			b = 0.796;
			glColor3d(r, g, b); // purple
			break;
		case 2:
			r = 0.988;
			g = 0.627;
			b = 0.373;
			glColor3d(r, g, b); // orange
			break;
		case 3:
			r = 0.69;
			g = 0.835;
			b = 0.529;
			glColor3d(r, g, b); // green
			break;
		case 4:
			r = 1.00;
			g = 0.453;
			b = 0.339;
			glColor3d(r, g, b); // red
			break;
		case 5:
			r = 0.949;
			g = 0.388;
			b = 0.639;
			glColor3d(r, g, b); // pink
			break;
		case 6:
			r = 1;
			g = 0.792;
			b = 0.204;
			glColor3d(r, g, b); // yellow
			break;
		case 7:
			glColor3d(0.0, 0.0, 0.0); // black
			break;
		case 8: 
			glColor3d(0, 0, 0);
			break;
		default:
			return;
	}
	
	double innerXMin = 0;
	double innerYMin = 0;
	double innerXMax = 1;
	double innerYMax = 1;
	double zMax = 1;
	double zMin = 0;
	// Front faces
	glNormal3d(1, 0, 0);
	glColor3d(r, g, b);
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();
	
	// top face
	if (multiColour)
		glColor3d(g, r, b);
	
//	glNormal3d(0, 1, 0);
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();
	
	// left face
	if (multiColour)
		glColor3d(b, g, r);
//	glNormal3d(0, 0, -1);
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
	glEnd();
	
	// bottom face
	if (multiColour)
		glColor3d(r, b, g);
	
//	glNormal3d(0, -1, 0);
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
	glEnd();
	
	// right face
	if (multiColour)
		glColor3d(b, r, g);
	
//	glNormal3d(0, 0, 1);
	glBegin(mode);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
	glEnd();
	
	// Back of front face
	if (multiColour)
		glColor3d(g, b, r);

//	glNormal3d(-1, 0, 0);
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
	glEnd();
}

void Viewer::setRotationVec(Vector3D newVector)
{
	rotationVec = newVector;
}

Vector3D Viewer::getRotationVec()
{
	return rotationVec;
}

void Viewer::startScale()
{
	shiftIsDown = true;
	disableSpin = true;
}

void Viewer::endScale()
{
	shiftIsDown = false;
	startScalePos[0] = 0;
	startScalePos[1] = 0;
}

void Viewer::setSpeed(Speed newSpeed)
{
	speed = newSpeed;
	switch (newSpeed)
	{
		case SLOW:
			gameSpeed = 500;
			break;
		case MEDIUM:
			gameSpeed = 250;
			break;
		case FAST:
			gameSpeed = 100;
			break;
	}
	
	// Update game tick timer
	tickTimer.disconnect();
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
}

void Viewer::toggleBuffer() 
{ 
	doubleBuffer = !doubleBuffer;
	std::cerr << "double buffer is " << doubleBuffer << std::endl;
	invalidate();
}

void Viewer::setDrawMode(DrawMode mode)
{
	currentDrawMode = mode;
	invalidate();
}

bool Viewer::on_key_press_event( GdkEventKey *ev )
{
	// Don't process movement keys if its game over
	if (gameOver)
		return true;
	
	if (ev->keyval == GDK_Left)
		game->moveLeft();
	else if (ev->keyval == GDK_Right)
		game->moveRight();
	else if (ev->keyval == GDK_Up)
		game->rotateCCW();
	else if (ev->keyval == GDK_Down)
		game->rotateCW();
	else if (ev->keyval == GDK_space)
		game->drop();
	
//	game->dropShadowPiece();
	invalidate();
	return true;
}

bool Viewer::gameTick()
{
	int returnVal = game->tick();
	
	// String streams used to print score and lines cleared	
	std::stringstream scoreStream, linesStream; 
	std::string s;
	
	// Update the score
	scoreStream << game->getScore();
	scoreLabel->set_text("Score:\t" + scoreStream.str());
	
	if (returnVal > 0)
	{
    	linesStream << game->getLinesCleared();
		linesClearedLabel->set_text("Lines Cleared:\t" + linesStream.str());
	}
	
	linesLeftTillNextLevel -= returnVal;
	if (game->getLinesCleared() / 10 > (DEFAULT_GAME_SPEED - gameSpeed) / 50 && gameSpeed > 75)
	{
		linesLeftTillNextLevel += 10;
		gameSpeed -= 50;
		
		std::cerr << game->getLinesCleared() << " cleared " << std::endl; 
		// Update game tick timer
		tickTimer.disconnect();
		tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
	}
	
	if (returnVal < 0)
	{
		gameOver = true;
		tickTimer.disconnect();
		
		// Add animation to spin the game board
	}
	
	invalidate();
	return true;
}

// Initialize colours for every piece
void Viewer::initSideColours()
{
	// Set the colours for each side of every tetris piece
	// Colours inspired from Tetris DS
	// sideColours 0-5 define the blue piece
	// sideColours 6-11 define the purple piece
	// sideColours 12-17 define the orange piece
	// sideColours 18-23 define the green piece
	// sideColours 24-29 define the red piece
	// sideColours 30-35 define the pink piece
	// sideColours 36-41 define the yellow piece
	sideColours[0].r = 51.4;
	sideColours[1].r = 76.1;
	sideColours[2].r = 16.5;
	sideColours[3].r = 28.2;
	sideColours[4].r = 20.8;
	sideColours[5].r = 27.5;
	sideColours[6].r = 55.3;
	sideColours[7].r = 74.9;
	sideColours[8].r = 49.0;
	sideColours[9].r = 42.4;
	sideColours[10].r = 32.9;
	sideColours[11].r = 33.7;
	sideColours[12].r = 98.8;
	sideColours[13].r = 100.0;
	sideColours[14].r = 98.0;
	sideColours[15].r = 95.7;
	sideColours[16].r = 96.1;
	sideColours[17].r = 88.2;
	sideColours[18].r = 69.0;
	sideColours[19].r = 90.2;
	sideColours[20].r = 57.3;
	sideColours[21].r = 43.1;
	sideColours[22].r = 39.2;
	sideColours[23].r = 51.4;
	sideColours[24].r = 97.6;
	sideColours[25].r = 98.4;
	sideColours[26].r = 89.8;
	sideColours[27].r = 91.4;
	sideColours[28].r = 83.9;
	sideColours[29].r = 69.4;
	sideColours[30].r = 94.9;
	sideColours[31].r = 99.6;
	sideColours[32].r = 96.1;
	sideColours[33].r = 45.9;
	sideColours[34].r = 58.4;
	sideColours[35].r = 42.0;
	sideColours[36].r = 100.0;
	sideColours[37].r = 100.0;
	sideColours[38].r = 94.9;
	sideColours[39].r = 51.0;
	sideColours[40].r = 62.0;
	sideColours[41].r = 34.5;
	sideColours[42].r = 99.9;
	
	sideColours[0].g = 83.9;
	sideColours[1].g = 91.4;
	sideColours[2].g = 77.6;
	sideColours[3].g = 67.8;
	sideColours[4].g = 61.6;
	sideColours[5].g = 58.4;
	sideColours[6].g = 60.0;
	sideColours[7].g = 81.2;
	sideColours[8].g = 51.0;
	sideColours[9].g = 40.0;
	sideColours[10].g =38.0;
	sideColours[11].g =37.3;
	sideColours[12].g =62.7;
	sideColours[13].g =81.6;
	sideColours[14].g =56.5;
	sideColours[15].g =45.9;
	sideColours[16].g =41.2;
	sideColours[17].g =45.1;
	sideColours[18].g =83.5;
	sideColours[19].g =93.7;
	sideColours[20].g =80.8;
	sideColours[21].g =76.9;
	sideColours[22].g =75.3;
	sideColours[23].g =74.5;
	sideColours[24].g =55.3;
	sideColours[25].g =79.6;
	sideColours[26].g =38.4;
	sideColours[27].g =29.8;
	sideColours[28].g =9.4 ;
	sideColours[29].g =17.6;
	sideColours[30].g =38.8;
	sideColours[31].g =88.6;
	sideColours[32].g =62.0;
	sideColours[33].g =0.4 ;
	sideColours[34].g =0.8 ;
	sideColours[35].g =1.2 ;
	sideColours[36].g =79.2;
	sideColours[37].g =91.4;
	sideColours[38].g =80.4;
	sideColours[39].g =29.0;
	sideColours[40].g =38.0;
	sideColours[41].g =16.1;
	sideColours[42].g = 99.9;
	
	sideColours[0].b = 96.5;
	sideColours[1].b = 97.3;
	sideColours[2].b = 96.5;
	sideColours[3].b = 88.2;
	sideColours[4].b = 86.3;
	sideColours[5].b = 75.3;
	sideColours[6].b = 79.6;
	sideColours[7].b = 91.0;
	sideColours[8].b = 73.3;
	sideColours[9].b = 69.0;
	sideColours[10].b =68.6;
	sideColours[11].b =58.8;
	sideColours[12].b =37.3;
	sideColours[13].b =55.3;
	sideColours[14].b =32.9;
	sideColours[15].b =29.0;
	sideColours[16].b =11.8;
	sideColours[17].b =15.7;
	sideColours[18].b =52.9;
	sideColours[19].b =68.2;
	sideColours[20].b =51.8;
	sideColours[21].b =49.0;
	sideColours[22].b =41.2;
	sideColours[23].b =42.4;
	sideColours[24].b =43.9;
	sideColours[25].b =70.6;
	sideColours[26].b =33.7;
	sideColours[27].b =21.2;
	sideColours[28].b =12.5;
	sideColours[29].b =12.9;
	sideColours[30].b =63.9;
	sideColours[31].b =94.1;
	sideColours[32].b =78.8;
	sideColours[33].b =24.3;
	sideColours[34].b =32.5;
	sideColours[35].b =25.1;
	sideColours[36].b =20.4;
	sideColours[37].b =72.2;
	sideColours[38].b =35.3;
	sideColours[39].b =0.0 ;
	sideColours[40].b =1.6 ;
	sideColours[41].b =0.0 ;
	
	sideColours[42].r = 60.0;
	sideColours[42].g = 60.0;
	sideColours[42].b = 60.0;
	
	sideColours[43].r = 80.0;
	sideColours[43].g = 80.0;
	sideColours[43].b = 80.0;
	
	sideColours[44].r = 80.0;
	sideColours[44].g = 80.0;
	sideColours[44].b = 80.0;
	
	sideColours[45].r = 33.3;
	sideColours[45].g = 33.3;
	sideColours[45].b = 33.3;
	
	sideColours[46].r = 33.3;
	sideColours[46].g = 33.3;
	sideColours[46].b = 33.3;
	
	sideColours[47].r = 0;
	sideColours[47].g = 0;
	sideColours[47].b = 0;
	
	sideColours[48].r = 0;
	sideColours[48].g = 0;
	sideColours[48].b = 0;
	                    
	sideColours[49].r = 0;
	sideColours[49].g = 0;
	sideColours[49].b = 0;
	                    
	sideColours[50].r = 0;
	sideColours[50].g = 0;
	sideColours[50].b = 0;
	                    
	sideColours[51].r = 0;
	sideColours[51].g = 0;
	sideColours[51].b = 0;
	                    
	sideColours[52].r = 0;
	sideColours[52].g = 0;
	sideColours[52].b = 0;
	
	sideColours[53].r = 0;
	sideColours[53].g = 0;
	sideColours[53].b = 0;
	
	for (int i = 0; i<48;i++)
	{
		sideColours[i].r /= 100;
		sideColours[i].g /= 100;
		sideColours[i].b /= 100;
	}
}                     

void Viewer::resetView()
{
	rotationSpeed = 0;
	rotationAngleX = 0;
	rotationAngleY = 0;
	rotationAngleZ = 0;
	rotateTimer.disconnect();
	
	scaleFactor = 1;
	invalidate();
}

void Viewer::newGame()
{
	gameOver = false;
	game->reset();
	
	// Restore gamespeed to whatever was set in the menu
	setSpeed(speed);
	
	// Reset the tickTimer
	tickTimer.disconnect();
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
	
	std::stringstream scoreStream, linesStream; 
	std::string s;
	
	// Update the score
	scoreStream << game->getScore();
	scoreLabel->set_text("Score:\t" + scoreStream.str());
	linesStream << game->getLinesCleared();
	linesClearedLabel->set_text("Lines Cleared:\t" + linesStream.str());
	
	invalidate();
}

void Viewer::setScoreWidgets(Gtk::Label *score, Gtk::Label *linesCleared)
{
	scoreLabel = score;
	linesClearedLabel = linesCleared;
}
