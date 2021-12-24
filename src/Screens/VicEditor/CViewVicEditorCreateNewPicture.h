#ifndef _CViewVicEditorCreateNewPicture_H_
#define _CViewVicEditorCreateNewPicture_H_

#include "SYS_Defs.h"
#include "CGuiWindow.h"
#include "CGuiEditHex.h"
#include "CGuiViewFrame.h"
#include "CGuiList.h"
#include "CGuiButtonSwitch.h"
#include "CViewC64Palette.h"
#include "CGuiLabel.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterfaceC64;
class CViewVicEditor;
class CVicEditorLayer;
class CViewC64Palette;

class CViewVicEditorCreateNewPicture : public CGuiWindow, public CGuiListCallback, CGuiButtonSwitchCallback, CViewC64PaletteCallback
{
public:
	CViewVicEditorCreateNewPicture(const char *name, float posX, float posY, float posZ, CViewVicEditor *vicEditor);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	virtual bool DoRightClick(float x, float y);
//	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void Render();
	virtual void DoLogic();
	
	//
	CViewVicEditor *vicEditor;
	
	CSlrFont *font;
	float fontScale;
	
	CGuiButton *btnNewPictureModeTextHires;
	CGuiButton *btnNewPictureModeTextMulti;
	CGuiButton *btnNewPictureModeBitmapHires;
	CGuiButton *btnNewPictureModeBitmapMulti;
//	CGuiButton *btnNewPictureModeHyper;
	
	virtual void ActivateView();

	virtual bool ButtonPressed(CGuiButton *button);

	CViewC64Palette *viewPalette;
	
	void CreateNewPicture(u8 mode, u8 backgroundColor, bool storeUndo);
	
	//
	CGuiLabel *lblPictureMode;
	CGuiLabel *lblBackgroundColor;
	
};


#endif

