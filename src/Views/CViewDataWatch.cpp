#include "SYS_Defs.h"
#include "DBG_Log.h"
#include "CViewDataWatch.h"
#include "CViewC64.h"
#include "CViewC64Screen.h"
#include "CImageData.h"
#include "CSlrImage.h"
#include "CViewC64.h"
#include "MTH_Random.h"
#include "VID_ImageBinding.h"
#include "SYS_KeyCodes.h"
#include "CViewDataDump.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "C64Opcodes.h"
#include "CViewDataMap.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CGuiMain.h"
#include "imgui_internal.h"

#include <math.h>

// TODO: Address/Label/Value cell text should be vertically centered against the
// Format combo and trash button on the same row. Attempts using
// AlignTextToFramePadding(), explicit SetCursorPosY with FramePadding.y offset,
// pushing CellPadding.y=0 + TableNextRow(min_row_height), and rendering via
// DrawList->AddText with a reserved InvisibleButton/Dummy slot all FAILED to
// visibly centre the text — ImGui still rendered it anchored to the top of the
// cell for reasons we could not determine. Left as top-aligned for now.

static bool TrashIconButton(const char *id)
{
	ImGuiStyle &style = ImGui::GetStyle();
	const float size = ImGui::GetFrameHeight();
	const ImVec2 padBackup = style.FramePadding;
	style.FramePadding = ImVec2(0.0f, 0.0f);
	const bool clicked = ImGui::Button(id, ImVec2(size, size));
	style.FramePadding = padBackup;

	const ImVec2 bmin = ImGui::GetItemRectMin();
	const ImVec2 bmax = ImGui::GetItemRectMax();
	const float w = bmax.x - bmin.x;
	const float h = bmax.y - bmin.y;
	const float cx = bmin.x + w * 0.5f;
	const float cy = bmin.y + h * 0.5f;
	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	ImDrawList *dl = ImGui::GetWindowDrawList();

	const float bw = w * 0.34f;  // body half-width
	const float bh = h * 0.32f;  // body half-height
	const float lidY = cy - bh * 0.55f;
	const float bodyTop = lidY + 1.5f;
	const float bodyBot = cy + bh;
	const float bodyL = cx - bw * 0.80f;
	const float bodyR = cx + bw * 0.80f;

	// Lid (wider horizontal line)
	dl->AddLine(ImVec2(cx - bw, lidY), ImVec2(cx + bw, lidY), col, 1.5f);
	// Handle (small arch above lid)
	const float handleW = bw * 0.45f;
	const float handleY = lidY - h * 0.10f;
	dl->AddLine(ImVec2(cx - handleW, handleY), ImVec2(cx + handleW, handleY), col, 1.5f);
	dl->AddLine(ImVec2(cx - handleW, handleY), ImVec2(cx - handleW, lidY),   col, 1.5f);
	dl->AddLine(ImVec2(cx + handleW, handleY), ImVec2(cx + handleW, lidY),   col, 1.5f);
	// Body sides (trapezoid-ish — slight taper)
	dl->AddLine(ImVec2(bodyL,                bodyTop), ImVec2(bodyL + w*0.03f, bodyBot), col, 1.5f);
	dl->AddLine(ImVec2(bodyR,                bodyTop), ImVec2(bodyR - w*0.03f, bodyBot), col, 1.5f);
	// Body bottom
	dl->AddLine(ImVec2(bodyL + w*0.03f, bodyBot), ImVec2(bodyR - w*0.03f, bodyBot), col, 1.5f);
	// Vertical slits inside
	dl->AddLine(ImVec2(cx - bw*0.38f, bodyTop + 2), ImVec2(cx - bw*0.38f, bodyBot - 2), col, 1.0f);
	dl->AddLine(ImVec2(cx,            bodyTop + 2), ImVec2(cx,            bodyBot - 2), col, 1.0f);
	dl->AddLine(ImVec2(cx + bw*0.38f, bodyTop + 2), ImVec2(cx + bw*0.38f, bodyBot - 2), col, 1.0f);

	return clicked;
}

CViewDataWatch::CViewDataWatch(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
							   CDebugSymbols *symbols, CViewDataMap *viewMemoryMap)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	viewDataDump = NULL;
	
	this->symbols = symbols;
	this->debugInterface = symbols->debugInterface;
	this->dataAdapter = symbols->dataAdapter;
	this->viewMemoryMap = viewMemoryMap;

	this->addWatchPopupAddr = 0;
	this->addWatchPopupSymbol[0] = 0;
	this->addWatchPopupName[0] = 0;
	this->addWatchPopupFocusNameRequested = false;
	this->imGuiColumnsWidthWorkaroundFrame = 0;
	this->imGuiOpenPopupFrame = 0;

	this->openEditWatchPopupRequested = false;
	this->editWatchPopupOriginalAddr = 0;
	this->editWatchPopupAddr = 0;
	this->editWatchPopupAddrText[0] = 0;
	this->editWatchPopupName[0] = 0;
	this->imGuiOpenEditPopupFrame = 0;
}

void CViewDataWatch::DoLogic()
{
	//
}

void CViewDataWatch::RenderImGui()
{
	PreRenderImGui();

	symbols->LockMutex();
	
	CDebugSymbolsSegment *symbolsSegment = symbols->currentSegment;
	if (!symbolsSegment)
	{
		LOGError("CViewDataWatch::RenderImGui: symbols segment is NULL");
		symbols->UnlockMutex();
		return;
	}
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "##DataWatchTable_%s", dataAdapter->adapterID);

	CDebugSymbolsDataWatch *watchToDelete = NULL;
	CDebugSymbolsDataWatch *watchToEdit = NULL;

	if (ImGui::BeginTable(buf, 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders))
	{
		const float editBtnWidth = ImGui::CalcTextSize("Edit").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float trashBtnWidth = ImGui::GetFrameHeight();
		const float trashColWidth = editBtnWidth + ImGui::GetStyle().ItemSpacing.x + trashBtnWidth
								  + ImGui::GetStyle().CellPadding.x * 2.0f;
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed,   ImGui::CalcTextSize("FFFFFF").x);
		ImGui::TableSetupColumn("Label",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("Format",  ImGuiTableColumnFlags_WidthStretch, 1.6f);
		ImGui::TableSetupColumn("",        ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoSort, trashColWidth);
		ImGui::TableHeadersRow();

		for (std::map<int, CDebugSymbolsDataWatch *>::iterator it = symbolsSegment->watches.begin();
			 it != symbolsSegment->watches.end(); it++)
		{
			CDebugSymbolsDataWatch *watch = it->second;

			ImGui::TableNextColumn();
			sprintf(buf, "%04X", watch->address);
			ImGui::Text(buf);

			ImGui::TableNextColumn();
			if (watch->watchName != NULL && watch->watchName[0] != 0)
			{
				ImGui::TextUnformatted(watch->watchName);
			}
			else
			{
				CDebugSymbolsCodeLabel *codeLabel = symbolsSegment->FindLabel(watch->address);
				if (codeLabel)
				{
					ImGui::TextUnformatted(codeLabel->GetLabelText());
				}
				else
				{
					ImGui::TextUnformatted("");
				}
			}

			ImGui::TableNextColumn();

			if (watch->representation == WATCH_REPRESENTATION_TEXT)
			{
				// TODO: WATCH_REPRESENTATION_TEXT   petsci, atasci?
			}
			else
			{
				// determine byte span of this watch
				int numBytes = 1;
				switch (watch->representation)
				{
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						numBytes = 2; break;
					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						numBytes = 4; break;
					default: break;
				}

				// pick strongest recent-access fade color across the byte range
				if (symbols->memory)
				{
					float hr = 0.0f, hg = 0.0f, hb = 0.0f, ha = 0.0f;
					for (int i = 0; i < numBytes; i++)
					{
						CDebugMemoryCell *c = symbols->memory->GetMemoryCell(watch->address + i);
						if (!c) continue;
						u8 v = dataAdapter->AdapterReadByteModulus(watch->address + i);
						c->UpdateCellColors(v, false, 0);
						if (c->sa > ha) { hr = c->sr; hg = c->sg; hb = c->sb; ha = c->sa; }
					}
					if (ha > 0.0f)
					{
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
							ImGui::ColorConvertFloat4ToU32(ImVec4(hr, hg, hb, ha)));
					}
				}

				u32 value = 0;
				switch(watch->representation)
				{
					default:
					case WATCH_REPRESENTATION_BIN:
					case WATCH_REPRESENTATION_HEX_8:
						value = dataAdapter->AdapterReadByteModulus(watch->address);
						break;
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    )
								| dataAdapter->AdapterReadByteModulus(watch->address + 1) << 8;
						break;
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    ) << 8
								| dataAdapter->AdapterReadByteModulus(watch->address + 1);
						break;

					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    )
								| dataAdapter->AdapterReadByteModulus(watch->address + 1) << 8
								| dataAdapter->AdapterReadByteModulus(watch->address + 2) << 16
								| dataAdapter->AdapterReadByteModulus(watch->address + 3) << 24;
						break;
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    ) << 24
								| dataAdapter->AdapterReadByteModulus(watch->address + 1) << 16
								| dataAdapter->AdapterReadByteModulus(watch->address + 2) << 8
								| dataAdapter->AdapterReadByteModulus(watch->address + 3);
						break;
				}

				// TODO: colors
//				if (cell->isExecuteCode)
//				{
//					colorExecuteCodeR, colorExecuteCodeG, colorExecuteCodeB, colorExecuteCodeA
//				}
//				else if (cell->isExecuteArgument)
//				{
//					colorExecuteArgumentR, colorExecuteArgumentG, colorExecuteArgumentB, colorExecuteArgumentA
//				}
//				ON TOP: cell->sr, cell->sg, cell->sb, cell->sa);


				switch(watch->representation)
				{
					default:
					case WATCH_REPRESENTATION_HEX_8:
						ImGui::Text("%02x", value);
						break;
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
						ImGui::Text("%04x", value);
						break;
					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
						ImGui::Text("%08x", value);
						break;
					case WATCH_REPRESENTATION_UNSIGNED_DEC_8:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
						ImGui::Text("%u", value);
						break;
					case WATCH_REPRESENTATION_SIGNED_DEC_8:
						ImGui::Text("%d", (i8)value);
						break;
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						ImGui::Text("%d", (i16)value);
						break;
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						ImGui::Text("%d", (i32)value);
						break;
					case WATCH_REPRESENTATION_BIN:
						FUN_IntToBinaryStr(value, buf, 8);
						ImGui::Text(buf);
						break;
				}
			}

			ImGui::TableNextColumn();

			sprintf(buf, "##DataWatchCombo%x", watch);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo(buf, &(watch->representation), "Hex 8-bits\0Hex 16-bits LE\0Hex 16-bits BE\0Hex 32-bits LE\0Hex 32-bits BE\0Unsigned Dec 8-bits\0Unsigned Dec 16-bits LE\0Unsigned Dec 16-bits BE\0Unsigned Dec 32-bits LE\0Unsigned Dec 32-bits BE\0Signed Dec 8-bits\0Signed Dec 16-bits LE\0Signed Dec 16-bits BE\0Signed Dec 32-bits LE\0Signed Dec 32-bits BE\0Binary\0\0"); //Text\0\0");

			ImGui::TableNextColumn();
			sprintf(buf, "Edit##edit%x", watch);
			if (ImGui::Button(buf))
			{
				watchToEdit = watch;
			}
			ImGui::SameLine();
			sprintf(buf, "##del%x", watch);
			if (TrashIconButton(buf))
			{
				watchToDelete = watch;
			}
		}
		
		ImGui::EndTable();
	}

	if (watchToDelete)
	{
		symbols->currentSegment->DeleteWatch(watchToDelete->address);
	}

	if (watchToEdit)
	{
		editWatchPopupOriginalAddr = watchToEdit->address;
		editWatchPopupAddr = watchToEdit->address;
		sprintf(editWatchPopupAddrText, "%04X", watchToEdit->address);
		if (watchToEdit->watchName != NULL)
		{
			strncpy(editWatchPopupName, watchToEdit->watchName, sizeof(editWatchPopupName) - 1);
			editWatchPopupName[sizeof(editWatchPopupName) - 1] = 0;
		}
		else
		{
			editWatchPopupName[0] = 0;
		}
		openEditWatchPopupRequested = true;
		imGuiOpenEditPopupFrame = 0;
	}

	if (openEditWatchPopupRequested)
	{
		ImGui::OpenPopup("editWatchPopup");
		openEditWatchPopupRequested = false;
	}

	bool skipClosePopupByEnterPressInThisFrame = false;

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		ImGui::OpenPopup("addWatchPopup");

		addWatchPopupAddr = 0;
		addWatchPopupSymbol[0] = '\0';
		addWatchPopupName[0] = '\0';
		addWatchPopupFocusNameRequested = false;
		comboFilterTextBuf[0] = 0;

		imGuiOpenPopupFrame = 0;
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2(360, 0), ImVec2(FLT_MAX, FLT_MAX));
	if (ImGui::BeginPopup("addWatchPopup"))
	{
		ImGui::Text("Add watch point");
		ImGui::Separator();

		if (imGuiOpenPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenPopupFrame++;
		}

		bool buttonAddClicked = false;
		if (symbols->currentSegment)
		{
			const char **hints = symbols->currentSegment->codeLabelsArray;
			int numHints = symbols->currentSegment->numCodeLabelsInArray;

			sprintf(buf, "##addWatchPopupForm%x", this);
			if (ImGui::BeginTable(buf, 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings))
			{
				ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("##field", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Address:");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				sprintf(buf, "##addWatchPopupAddress%x", this);

				// https://github.com/ocornut/imgui/issues/1658   | IM_ARRAYSIZE(hints)
				if( ComboFilter(buf, comboFilterTextBuf, IM_ARRAYSIZE(comboFilterTextBuf), hints, numHints, comboFilterState, this) )
				{
					LOGD("SELECTED %d comboTextBuf='%s'", comboFilterState.activeIdx, comboFilterTextBuf);
					// Enter (or selecting an entry) in the address combo advances focus to the Name field
					// instead of submitting the watch, so the user can type a label first.
					skipClosePopupByEnterPressInThisFrame = true;
					addWatchPopupFocusNameRequested = true;

					// if that is not address then replace by selected symbol
					if (FUN_IsHexNumber(comboFilterTextBuf))
					{
						FUN_ToUpperCaseStr(comboFilterTextBuf);
						addWatchPopupAddr = FUN_HexStrToValue(comboFilterTextBuf);
					}
					else if (hints != NULL && numHints > 0)
					{
						strcpy(comboFilterTextBuf, hints[comboFilterState.activeIdx]);
					}
				}

				// Tab anywhere in the popup advances focus from the address combo to the Name field.
				// Also close the combo's dropdown popup so it doesn't linger over the Name field.
				if (ImGui::IsKeyPressed(ImGuiKey_Tab, false))
				{
					addWatchPopupFocusNameRequested = true;
					skipClosePopupByEnterPressInThisFrame = true;
					ImGui::ClosePopupsOverWindow(ImGui::GetCurrentWindow(), false);
				}

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Name:");
				ImGui::TableNextColumn();
				if (addWatchPopupFocusNameRequested)
				{
					ImGui::SetKeyboardFocusHere();
					addWatchPopupFocusNameRequested = false;
				}
				ImGui::SetNextItemWidth(-FLT_MIN);
				sprintf(buf, "##addWatchPopupName%x", this);
				if (ImGui::InputText(buf, addWatchPopupName, IM_ARRAYSIZE(addWatchPopupName), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					buttonAddClicked = true;
				}

				ImGui::EndTable();
			}

			if (ImGui::Button("Add"))
			{
				buttonAddClicked = true;
			}
		}

		bool finalizeAddingBreakpoint = buttonAddClicked
			|| (!skipClosePopupByEnterPressInThisFrame && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));

		if (finalizeAddingBreakpoint)
		{
			CDebugSymbolsCodeLabel *label = symbols->currentSegment->FindLabelByText(comboFilterTextBuf);

			char *hexNumberBuf = comboFilterTextBuf;
			if (comboFilterTextBuf[0] == '$' && comboFilterTextBuf[1] != 0)
			{
				hexNumberBuf = comboFilterTextBuf + 1;
			}

			if (label)
			{
				addWatchPopupAddr = label->address;
			}
			else if (FUN_IsHexNumber(hexNumberBuf))
			{
				FUN_ToUpperCaseStr(comboFilterTextBuf);
				addWatchPopupAddr = FUN_HexStrToValue(hexNumberBuf);
			}
			else
			{
				char *buf = SYS_GetCharBuf();
				sprintf(buf, "Invalid address or symbol:\n%s", comboFilterTextBuf);
				guiMain->ShowMessageBox("Can't add watch", buf);
				SYS_ReleaseCharBuf(buf);
				addWatchPopupAddr = -1;
			}

			if (addWatchPopupAddr >= 0)
			{
				addWatchPopupAddr = URANGE(0, addWatchPopupAddr, dataAdapter->AdapterGetDataLength());

				symbols->currentSegment->AddWatch(addWatchPopupAddr, addWatchPopupName);

				ImGui::CloseCurrentPopup();
			}
		}

		////

		ImGui::EndPopup();
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2(360, 0), ImVec2(FLT_MAX, FLT_MAX));
	if (ImGui::BeginPopup("editWatchPopup"))
	{
		ImGui::Text("Edit watch point");
		ImGui::Separator();

		if (imGuiOpenEditPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenEditPopupFrame++;
		}

		sprintf(buf, "##editWatchPopupForm%x", this);
		if (ImGui::BeginTable(buf, 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings))
		{
			ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("##field", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Address:");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			sprintf(buf, "##editWatchPopupAddress%x", this);
			ImGui::InputText(buf, editWatchPopupAddrText, IM_ARRAYSIZE(editWatchPopupAddrText),
							 ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Name:");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			sprintf(buf, "##editWatchPopupName%x", this);
			ImGui::InputText(buf, editWatchPopupName, IM_ARRAYSIZE(editWatchPopupName));

			ImGui::EndTable();
		}

		bool buttonSaveClicked = ImGui::Button("Save");
		ImGui::SameLine();
		bool buttonCancelClicked = ImGui::Button("Cancel");

		if (buttonCancelClicked)
		{
			ImGui::CloseCurrentPopup();
		}
		else if (buttonSaveClicked)
		{
			char *addrText = editWatchPopupAddrText;
			if (addrText[0] == '$' && addrText[1] != 0)
				addrText++;

			if (!FUN_IsHexNumber(addrText))
			{
				char *msgBuf = SYS_GetCharBuf();
				sprintf(msgBuf, "Invalid address:\n%s", editWatchPopupAddrText);
				guiMain->ShowMessageBox("Can't edit watch", msgBuf);
				SYS_ReleaseCharBuf(msgBuf);
			}
			else
			{
				int newAddr = FUN_HexStrToValue(addrText);
				newAddr = URANGE(0, newAddr, dataAdapter->AdapterGetDataLength());

				CDebugSymbolsSegment *seg = symbols->currentSegment;
				std::map<int, CDebugSymbolsDataWatch *>::iterator itWatch = seg->watches.find(editWatchPopupOriginalAddr);
				if (itWatch != seg->watches.end())
				{
					CDebugSymbolsDataWatch *watch = itWatch->second;
					if (newAddr != editWatchPopupOriginalAddr)
					{
						// delete any watch already present at the new address,
						// then re-key this watch under the new address
						std::map<int, CDebugSymbolsDataWatch *>::iterator itExisting = seg->watches.find(newAddr);
						if (itExisting != seg->watches.end() && itExisting->second != watch)
						{
							delete itExisting->second;
							seg->watches.erase(itExisting);
						}
						seg->watches.erase(editWatchPopupOriginalAddr);
						watch->address = newAddr;
						seg->watches[newAddr] = watch;
					}
					watch->SetName(editWatchPopupName);
				}

				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	symbols->UnlockMutex();
	
	SYS_ReleaseCharBuf(buf);

	PostRenderImGui();
}

void CViewDataWatch::SetViewC64DataDump(CViewDataDump *viewDataDump)
{
	this->viewDataDump = viewDataDump;
}

bool CViewDataWatch::ComboFilterShouldOpenPopupCallback(const char *label, char *buffer, int bufferlen,
												const char **hints, int num_hints, ImGui::ComboFilterState *s)
{
	// do not need to open combo popup when number is hex
	if (FUN_IsHexNumber(buffer))
	{
		return false;
	}

	if (hints == NULL)
	{
		return false;
	}

	if (num_hints == 0)
	{
		return false;
	}

	return (buffer[0] != 0) && strcmp(buffer, hints[s->activeIdx]);
}

CViewDataWatch::~CViewDataWatch()
{
	
}
