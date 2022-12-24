#ifndef _C64SpriteMulti_H_
#define _C64SpriteMulti_H_

#include "C64Sprite.h"

class C64SpriteMulti : public C64Sprite
{
public:
	C64SpriteMulti();
	C64SpriteMulti(CViewC64VicEditor *vicEditor);
	C64SpriteMulti(CViewC64VicEditor *vicEditor, int x, int y, bool isStretchedHorizontally, bool isStretchedVertically, int pointerValue, int pointerAddr);
	C64SpriteMulti(CViewC64VicEditor *vicEditor, CByteBuffer *byteBuffer);
	virtual ~C64SpriteMulti();

	virtual void SetPixel(int x, int y, u8 color);
	virtual u8 GetPixel(int x, int y);
	virtual void DebugPrint();

	//
	virtual u8 PutPixelMultiSprite(bool forceColorReplace, int x, int y, u8 paintColor, u8 replacementColorNum);
	
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	
	virtual u8 GetColorAtPixel(int x, int y);

	//
	virtual void FetchSpriteData(int addr);
	virtual void StoreSpriteData(int addr);
	
	//
	virtual void Clear();

	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};

#endif
