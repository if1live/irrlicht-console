// Ŭnicode please 
#include "stdafx.h"
#include <irrlicht.h>
#include "console/irr_console.h"

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

int main()
{
	IrrlichtDevice *device = createDevice( video::EDT_OPENGL, dimension2d<u32>(640, 480), 16, false, false, false, g_console);

	if (!device) {
		return 1;
	}
	//first, init console
	setUpConsole(device);

	device->setWindowCaption(L"Hello World! - Irrlicht Engine Demo");

	IVideoDriver* driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* guienv = device->getGUIEnvironment();

	IAnimatedMesh* mesh = smgr->getMesh("ext/irrlicht/media/sydney.md2");
	if (!mesh) {
		device->drop();
		return 1;
	}
	IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode( mesh );

	if (node) {
		node->setMaterialFlag(EMF_LIGHTING, false);
		node->setMD2Animation(scene::EMAT_STAND);
		node->setMaterialTexture( 0, driver->getTexture("ext/irrlicht/media/sydney.bmp") );
	}

	smgr->addCameraSceneNode(0, vector3df(0,30,-40), vector3df(0,5,0));

	u32 then = device->getTimer()->getTime();
	while(device->run()) {		
		const u32 now = device->getTimer()->getTime();
		int frameDeltaTime = now - then;	// Time in milliseconds
		then = now;

		driver->beginScene(true, true, SColor(255,100,101,140));

		smgr->drawAll();
		guienv->drawAll();

		g_console->RenderConsole(guienv, driver, frameDeltaTime);

		driver->endScene();
	}

	device->drop();

	return 0;
}