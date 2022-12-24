#include "CViewC64AllGraphicsScreensControl.h"
#include "CViewC64AllGraphics.h"
#include "VID_Main.h"
#include "VID_ImageBinding.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64VicControl.h"
#include "CViewDataDump.h"
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CGuiMain.h"
#include "CViewDataDump.h"
#include "CGuiLockableList.h"
#include "CSlrString.h"
#include "CViewC64StateVIC.h"
#include "CViewC64.h"

// TODO: colour leds are just rectangles with hitbox and lot of copy pasted code, replace them to proper buttons!

CViewC64AllGraphicsScreensControl::CViewC64AllGraphicsScreensControl(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;

	//
	font = viewC64->fontCBMShifted;
	fontScale = 1.5f;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;

	float startX = 5;
	float startY = 5;
	
	float px = startX;
	float py = startY;
	float buttonSizeX = 5.96 * fontSize;
	float buttonSizeY = 2.0f * fontSize;
	float gap = 5.0f;
	float mgap = 2.5f;
	float buttonTextOffsetY = 0.67*fontSize;

	btnModeBitmapColorsBlackWhite = new CGuiButtonSwitch(NULL, NULL, NULL,
										   px, py, posZ, buttonSizeX, buttonSizeY,
										   new CSlrString("B/W"),
										   FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
										   font, fontScale,
										   1.0, 1.0, 1.0, 1.0,
										   1.0, 1.0, 1.0, 1.0,
										   0.3, 0.3, 0.3, 1.0,
										   this);
	btnModeBitmapColorsBlackWhite->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
	this->AddGuiElement(btnModeBitmapColorsBlackWhite);

	px += buttonSizeX + gap;

	btnModeHires = new CGuiButtonSwitch(NULL, NULL, NULL,
														px, py, posZ, buttonSizeX, buttonSizeY,
														new CSlrString("HIRES"),
														FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
														font, fontScale,
														1.0, 1.0, 1.0, 1.0,
														1.0, 1.0, 1.0, 1.0,
														0.3, 0.3, 0.3, 1.0,
														this);
	btnModeHires->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeHires);
	this->AddGuiElement(btnModeHires);

	px += buttonSizeX + gap;
	
	btnModeMulti = new CGuiButtonSwitch(NULL, NULL, NULL,
										px, py, posZ, buttonSizeX, buttonSizeY,
										new CSlrString("MULTI"),
										FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
										font, fontScale,
										1.0, 1.0, 1.0, 1.0,
										1.0, 1.0, 1.0, 1.0,
										0.3, 0.3, 0.3, 1.0,
										this);
	btnModeMulti->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeMulti);
	this->AddGuiElement(btnModeMulti);
	
	// list of charset addresses
	px += buttonSizeX*1.8f + gap;

	char **txtCharsetAddresses = new char *[0x40];
	
	u16 addr = 0x0000;
	for (int i = 0; i < 0x20; i++)
	{
		char *txtCharsetAddr = new char[5];
		sprintf(txtCharsetAddr, "%04x", addr);
		addr += 0x0800;
		
		txtCharsetAddresses[i] = txtCharsetAddr;
	}

	float lstFontSize = fontSize;//4.0f;
	
	this->lstCharsetAddresses = new CGuiLockableList(px, py, posZ+0.01, lstFontSize*6.5f, 65.0f, lstFontSize,
													NULL, 0, false,
													 viewC64->fontDisassembly,
													guiMain->theme->imgBackground, 1.0f,
													this);
	this->lstCharsetAddresses->name = "AllGraphics::lstCharsetAddresses";
	this->lstCharsetAddresses->Init(txtCharsetAddresses, 0x20, true);
	this->lstCharsetAddresses->SetGaps(0.0f, -0.25f);
	this->AddGuiElement(this->lstCharsetAddresses);

	//
	px = startX;
	py += buttonSizeY + gap;

	btnShowRAMorIO = new CGuiButtonSwitch(NULL, NULL, NULL,
												  px, py, posZ, buttonSizeX, buttonSizeY,
												  new CSlrString("ROM"),
												  FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
												  font, fontScale,
												  1.0, 1.0, 1.0, 1.0,
												  1.0, 1.0, 1.0, 1.0,
												  0.3, 0.3, 0.3, 1.0,
												  this);
	btnShowRAMorIO->SetOn(true);
	SetSwitchButtonDefaultColors(btnShowRAMorIO);
	this->AddGuiElement(btnShowRAMorIO);

	px += buttonSizeX + gap;

	btnShowGrid = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("GRID"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
										  font, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
	btnShowGrid->SetOn(true);
	SetSwitchButtonDefaultColors(btnShowGrid);
	this->AddGuiElement(btnShowGrid);
	
	SetupMode();
}

CViewC64AllGraphicsScreensControl::~CViewC64AllGraphicsScreensControl()
{
}

void CViewC64AllGraphicsScreensControl::SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn)
{
	btn->buttonSwitchOffColorR = 0.0f;
	btn->buttonSwitchOffColorG = 0.0f;
	btn->buttonSwitchOffColorB = 0.0f;
	btn->buttonSwitchOffColorA = 1.0f;
	
	btn->buttonSwitchOffColor2R = 0.3f;
	btn->buttonSwitchOffColor2G = 0.3f;
	btn->buttonSwitchOffColor2B = 0.3f;
	btn->buttonSwitchOffColor2A = 1.0f;
	
	btn->buttonSwitchOnColorR = 0.0f;
	btn->buttonSwitchOnColorG = 0.7f;
	btn->buttonSwitchOnColorB = 0.0f;
	btn->buttonSwitchOnColorA = 1.0f;
	
	btn->buttonSwitchOnColor2R = 0.3f;
	btn->buttonSwitchOnColor2G = 0.3f;
	btn->buttonSwitchOnColor2B = 0.3f;
	btn->buttonSwitchOnColor2A = 1.0f;
	
}

void CViewC64AllGraphicsScreensControl::SetLockableButtonDefaultColors(CGuiButtonSwitch *btn)
{
	btn->buttonSwitchOffColorR = 0.0f;
	btn->buttonSwitchOffColorG = 0.0f;
	btn->buttonSwitchOffColorB = 0.0f;
	btn->buttonSwitchOffColorA = 1.0f;
	
	btn->buttonSwitchOffColor2R = 0.3f;
	btn->buttonSwitchOffColor2G = 0.3f;
	btn->buttonSwitchOffColor2B = 0.3f;
	btn->buttonSwitchOffColor2A = 1.0f;
	
	btn->buttonSwitchOnColorR = 0.7f;
	btn->buttonSwitchOnColorG = 0.0f;
	btn->buttonSwitchOnColorB = 0.0f;
	btn->buttonSwitchOnColorA = 1.0f;
	
	btn->buttonSwitchOnColor2R = 0.3f;
	btn->buttonSwitchOnColor2G = 0.3f;
	btn->buttonSwitchOnColor2B = 0.3f;
	btn->buttonSwitchOnColor2A = 1.0f;
	
}

void CViewC64AllGraphicsScreensControl::SetupMode()
{
	guiMain->LockMutex();

	btnModeBitmapColorsBlackWhite->SetVisible(true);
	btnModeHires->SetVisible(true);
	btnModeMulti->SetVisible(true);
	lstCharsetAddresses->SetVisible(true);

	guiMain->UnlockMutex();
}

void CViewC64AllGraphicsScreensControl::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphicsScreensControl::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64AllGraphicsScreensControl::Render()
{
	float ledX = posX + fontSize * 20.1725f;
	float ledY = posY + 6.0f;

	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

	u16 screenAddress;
	vicii_cycle_state_t *viciiState = &viewC64->viciiStateToShow;

	screenAddress = viciiState->vbank_phi2 + ((viciiState->regs[0x18] & 0xf0) << 6);
	screenAddress = (screenAddress & viciiState->vaddr_mask_phi2) | viciiState->vaddr_offset_phi2;

	u16 charsetAddress = (viciiState->regs[0x18] & 0x0E) << 10;
	charsetAddress = (charsetAddress + viciiState->vbank_phi1);
	charsetAddress &= viciiState->vaddr_mask_phi1;
	charsetAddress |= viciiState->vaddr_offset_phi1;

	if (lstCharsetAddresses->isLocked)
	{
		charsetAddress = lstCharsetAddresses->selectedElement * 0x0800;
	}
	else
	{
		bool updatePosition = true;

		if (lstCharsetAddresses->IsInside(guiMain->mousePosX, guiMain->mousePosY))
			updatePosition = false;

		// update controls
		int addrItemNum = charsetAddress / 0x0800;
		lstCharsetAddresses->SetElement(addrItemNum, updatePosition, false);
	}

	//
	if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_NONE)
	{
		u8 mc;

		mc = (viciiState->regs[0x16] & 0x10) >> 4;

		btnModeBitmapColorsBlackWhite->SetOn(false);
		btnModeHires->SetOn(!mc);
		btnModeMulti->SetOn(mc);
	}

	// render color leds for bitmap. todo: convert them to proper buttons
	if (btnModeHires->IsOn())
	{
		float px = ledX;
		float py = ledY;

		// D021
		int i = 0x01;
		u8 color = viewC64->colorsToShow[i];
		bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
		RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
		py += fontSize + step;

		/* D800
		color = viewC64->colorToShowD800;
		isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
		RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
		 */
	}
	else if (btnModeMulti->IsOn())
	{
		float px = ledX;
		float py = ledY;

		// D021-D023
		for (int i = 0x01; i < 0x04; i++)
		{
			u8 color = viewC64->colorsToShow[i];
			bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
			RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
			py += fontSize + step;
		}

		/* D800
		u8 color = viewC64->colorToShowD800;
		bool isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
		RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
		 */
	}

	btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());
	UpdateShowIOButton();

	CGuiView::Render();
}

void CViewC64AllGraphicsScreensControl::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphicsScreensControl::UpdateShowIOButton()
{
	btnShowRAMorIO->SetOn( ! viewC64->isDataDirectlyFromRAM);
}

//@returns is consumed
bool CViewC64AllGraphicsScreensControl::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsScreensControl::DoTap:  x=%f y=%f", x, y);
	
	guiMain->LockMutex();

	// TODO: note this is copy pasted code from C64ViewStateVIC, needs to be generalized
	//       idea is to sync values with VIC state view. Leds should be replaced with proper buttons and callbacks.
	//		 the below is just a temporary POC made in a few minutes

	float ledX = posX + fontSize * 20.1725f;
	float ledY = posY + 6.0f;

	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

	// render color leds for screens when in hires
	if (btnModeHires->IsOn())
	{
		float px = ledX;
		float py = ledY;

		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			// D021
			int i = 0x01;

			if (viewC64->viewC64StateVIC->forceColors[i] == -1)
			{
				viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
			}
			else
			{
				viewC64->viewC64StateVIC->forceColors[i] = -1;
			}
			guiMain->UnlockMutex();
			return true;
		}

		py += fontSize + step;

		/* D800
		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			if (viewC64->viewC64StateVIC->forceColorD800 == -1)
			{
				viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
			}
			else
			{
				viewC64->viewC64StateVIC->forceColorD800 = -1;
			}
			guiMain->UnlockMutex();
			return true;
		}*/
	}
	else if (btnModeMulti->IsOn())
	{
		float px = ledX;
		float py = ledY;

		// D021-D023
		for (int i = 1; i < 4; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					viewC64->viewC64StateVIC->forceColors[i] = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;
		}

		/* D800
		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			if (viewC64->viewC64StateVIC->forceColorD800 == -1)
			{
				viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
			}
			else
			{
				viewC64->viewC64StateVIC->forceColorD800 = -1;
			}
			guiMain->UnlockMutex();
			return true;
		}*/
	}

	guiMain->UnlockMutex();
	
	return CGuiView::DoTap(x, y);
}

bool CViewC64AllGraphicsScreensControl::DoRightClick(float x, float y)
{
	guiMain->LockMutex();

	// TODO: note this is copy pasted code from C64ViewStateVIC, needs to be generalized
	//       idea is to sync values with VIC state view. Leds should be replaced with proper buttons and callbacks.
	//		 the below is just a temporary POC made in few minutes

	float ledX = posX + fontSize * 20.1725f;
	float ledY = posY + 6.0f;

	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

	// render color leds for screens when in hires
	if (btnModeHires->IsOn())
	{
		float px = ledX;
		float py = ledY;

		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			// D021
			int i = 0x01;

			if (viewC64->viewC64StateVIC->forceColors[i] == -1)
			{
				viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
			}
			viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

			guiMain->UnlockMutex();
			return true;
		}

		py += fontSize + step;

		/* D800
		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			if (viewC64->viewC64StateVIC->forceColorD800 == -1)
			{
				viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
			}
			viewC64->viewC64StateVIC->forceColorD800 = (viewC64->viewC64StateVIC->forceColorD800 + 1) & 0x0F;

			guiMain->UnlockMutex();
			return true;
		}*/
	}
	else if (btnModeMulti->IsOn())
	{
		float px = ledX;
		float py = ledY;

		// D021-D023
		for (int i = 1; i < 4; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;
		}

		/* D800
		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			if (viewC64->viewC64StateVIC->forceColorD800 == -1)
			{
				viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
			}
			viewC64->viewC64StateVIC->forceColorD800 = (viewC64->viewC64StateVIC->forceColorD800 + 1) & 0x0F;

			guiMain->UnlockMutex();
			return true;
		}*/
	}

	guiMain->UnlockMutex();
	
	return CGuiView::CGuiElement::DoRightClick(x, y);
}

bool CViewC64AllGraphicsScreensControl::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewC64AllGraphicsScreensControl::ButtonPressed(CGuiButton *button)
{
	LOGD("CViewC64AllGraphicsScreensControl::ButtonPressed");
	
	if (button == btnModeBitmapColorsBlackWhite)
	{
		if (forcedRenderScreenMode != VIEW_C64_ALL_GRAPHICS_FORCED_GRAY)
		{
			btnModeMulti->SetOn(false);
			btnModeHires->SetOn(false);
			btnModeBitmapColorsBlackWhite->SetOn(true);
			forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_GRAY;
			SetLockableButtonDefaultColors(btnModeBitmapColorsBlackWhite);
		}
		else
		{
			ClearGraphicsForcedMode();
		}
		return true;
	}
	else if (button == btnModeHires)
	{
		if (forcedRenderScreenMode != VIEW_C64_ALL_GRAPHICS_FORCED_HIRES)
		{
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeMulti->SetOn(false);
			btnModeHires->SetOn(true);
			forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_HIRES;
			SetLockableButtonDefaultColors(btnModeHires);
		}
		else
		{
			ClearGraphicsForcedMode();
		}
		return true;
	}
	else if (button == btnModeMulti)
	{
		if (forcedRenderScreenMode != VIEW_C64_ALL_GRAPHICS_FORCED_MULTI)
		{
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeHires->SetOn(false);
			btnModeMulti->SetOn(true);
			forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_MULTI;
			SetLockableButtonDefaultColors(btnModeMulti);
		}
		else
		{
			ClearGraphicsForcedMode();
		}
		return true;
	}
	else if (button == btnShowGrid)
	{
		viewC64->viewC64VicControl->btnShowGrid->DoSwitch();
		return true;
	}

	return false;
}

bool CViewC64AllGraphicsScreensControl::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	LOGD("CViewC64AllGraphicsScreensControl::ButtonSwitchChanged");
	
	if (button == btnShowRAMorIO)
	{
		viewC64->SwitchIsDataDirectlyFromRam();
		return true;
	}
	return false;
}

bool CViewC64AllGraphicsScreensControl::ListElementPreSelect(CGuiList *listBox, int elementNum)
{
	LOGD("CViewC64AllGraphicsScreensControl::ListElementPreSelect");
	guiMain->LockMutex();

	CGuiLockableList *list = (CGuiLockableList*)listBox;

	if (list->isLocked)
	{
		// click on the same element - unlock
		if (list->selectedElement == elementNum)
		{
			list->SetListLocked(false);
			guiMain->UnlockMutex();
			return true;
		}
	}

	list->SetListLocked(true);

	guiMain->UnlockMutex();
	
	return true;
}

void CViewC64AllGraphicsScreensControl::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
//	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
//	{
//		ClearGraphicsForcedMode();
//	}
}

void CViewC64AllGraphicsScreensControl::ClearGraphicsForcedMode()
{
	this->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
	SetSwitchButtonDefaultColors(btnModeHires);
	SetSwitchButtonDefaultColors(btnModeMulti);
}

// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphicsScreensControl::ActivateView()
{
	LOGG("CViewC64AllGraphicsScreensControl::ActivateView()");
	UpdateShowIOButton();
}

void CViewC64AllGraphicsScreensControl::DeactivateView()
{
	LOGG("CViewC64AllGraphicsScreensControl::DeactivateView()");
}

