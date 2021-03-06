// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      12mar17	initial version
		01		24aug17	add load texture file command
		02		06nov17	add lighting
		03		07nov17	in OnEditUndo and OnEditRedo, set modified flag
		04		24nov17	add save/restore phase for animation undo
		05		20feb18	add secondary modulation

*/

// PotterDrawDoc.cpp : implementation of the CPotterDrawDoc class
//
#include "stdafx.h"
#include "PotterDraw.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"
#include "MainFrm.h"
#include "UndoCodes.h"
#include "FocusEdit.h"
#include "PathStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPotterDrawDoc

IMPLEMENT_DYNCREATE(CPotterDrawDoc, CDocument)

#define ID_SPLINE_CMD_FIRST ID_SPLINE_ADD_NODE	// assumes resource IDs are alpha sorted
#define ID_SPLINE_CMD_LAST ID_SPLINE_ZOOM_RESET

const LPCTSTR CPotterDrawDoc::m_arrTextureFileExt[] = {
	_T(".bmp"), _T(".dds"), _T(".dib"), _T(".hdr"), _T(".jpg"), _T(".pfm"), _T(".png"), _T(".ppm"), _T(".tga")
};
const int CPotterDrawDoc::m_nTextureFileExts = _countof(m_arrTextureFileExt);

// CPotterDrawDoc construction/destruction

CPotterDrawDoc::CPotterDrawDoc()
{
	m_UndoMgr.SetRoot(this);
	SetUndoManager(&m_UndoMgr);
}

CPotterDrawDoc::~CPotterDrawDoc()
{
}

BOOL CPotterDrawDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	SetDefaultPalette();
	m_arrSpline.SetSize(1);	// set default spline
	m_arrSpline.SetStartNode(DPoint(25, -50));
	m_arrSpline.GetAt(0) = CSplineBase::CSegment(DPoint(25, -50), DPoint(25, 50), DPoint(25, 50), 
		CSplineBase::ST_CURVE, CSplineBase::NT_SMOOTH);
	return TRUE;
}

// CPotterDrawDoc serialization

void CPotterDrawDoc::Serialize(class CArchive &)
{
}

// CPotterDrawDoc diagnostics

#ifdef _DEBUG
void CPotterDrawDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPotterDrawDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

// CPotterDrawDoc methods

void CPotterDrawDoc::OnPropertyEdit(int iProp)
{
	CPropertyHint	hint(iProp);
	UpdateAllViews(NULL, CPotterDrawDoc::HINT_PROPERTY, &hint);
	SetModifiedFlag();
}

void CPotterDrawDoc::SavePalette(CUndoState& State, int iColor) const
{
	if (iColor >= 0) {	// if color specified
		State.m_Val.p.x.i = m_arrPalette[iColor];	// save only specified color
	} else {	//  save entire palette and its selection
		CRefPtr<CUndoPalette>	pVal;
		pVal.CreateObj();
		pVal->m_arrPalette = m_arrPalette;
		pVal->m_iCurSel = m_iPaletteCurSel;
		State.SetObj(pVal);
	}
}

void CPotterDrawDoc::RestorePalette(const CUndoState& State, int iColor)
{
	if (iColor >= 0) {	// if color specified
		m_arrPalette[iColor] = State.m_Val.p.x.i;	// restore only specified color
	} else {	// restore entire palette and its selection
		CUndoPalette	*pVal = static_cast<CUndoPalette *>(State.GetObj());
		m_arrPalette = pVal->m_arrPalette;
		m_iPaletteCurSel = pVal->m_iCurSel;
		m_nColors = m_arrPalette.GetSize();
	}
}

void CPotterDrawDoc::SaveString(CUndoState& State, const CString& str) const
{
	CRefPtr<CUndoString>	pVal;
	pVal.CreateObj();
	pVal->m_str = str;
	State.SetObj(pVal);
}

void CPotterDrawDoc::RestoreString(const CUndoState& State, CString& str)
{
	CUndoString	*pVal = static_cast<CUndoString *>(State.GetObj());
	str = pVal->m_str;
}

void CPotterDrawDoc::SavePhases(CUndoState& State)
{
	if (m_bAnimation && State.GetCode() == UCODE_PROPERTY) {	// if animating and property edit
		POSITION	pos = GetFirstViewPosition();	// get first view
		if (pos != NULL) {
			CPotterDrawView	*pView = DYNAMIC_DOWNCAST(CPotterDrawView, GetNextView(pos));
			if (pView != NULL)
				pView->GetAnimationState(*this);	// retrieve current phases from vieew
		}
	}
	CRefPtr<CUndoPhases>	pVal;
	pVal.CreateObj();
	int	nAnimMods = GetAnimatedModulationCountEx();	// extra iteration to get phase array size
	pVal->m_arrPhase.SetSize(nAnimMods);	// allocate phase array
	int	iPhase = 0;
	for (int iProp = 0; iProp < MODULATIONS; iProp++) {	// for each potential primary or secondary modulation
		if (IsAnimatedModulation(iProp)) {	// if property is animated modulation
			pVal->m_arrPhase[iPhase] = m_Mod[iProp].m_fPhase;	// save modulation's phase
			iPhase++;	// bump phase array index
		}
	}
	State.SetObj(pVal);
}

void CPotterDrawDoc::RestorePhases(const CUndoState& State)
{
	CUndoPhases	*pVal = static_cast<CUndoPhases *>(State.GetObj());
	int	iPhase = 0;
	for (int iProp = 0; iProp < MODULATIONS; iProp++) {	// for each potential primary or secondary modulation
		if (IsAnimatedModulation(iProp)) {	// if property is animated modulation
			m_Mod[iProp].m_fPhase = pVal->m_arrPhase[iPhase];	// restore modulation's phase
			iPhase++;	// bump phase array index
		}
	}
}

void CPotterDrawDoc::SaveUndoState(CUndoState& State)
{
	switch (State.GetCode()) {
	case UCODE_PROPERTY:
		{
			int	iProp = State.GetCtrlID();
			switch (iProp) {
			case PROP_sTexturePath:
				SaveString(State, m_sTexturePath);
				break;
			default:
				// only scalars, no strings or pointers
				GetValue(iProp, &State.m_Val, sizeof(State.m_Val));
				switch (iProp) {
				case PROP_nColors:
					SavePalette(State);
					break;
				case PROP_bAnimation:
					SavePhases(State);
					break;
				}
			}
		}
		break;
	case UCODE_PALETTE:
		SavePalette(State, State.GetCtrlID());
		break;
	case UCODE_MODULATION:
		{
			int	iModProp = LOWORD(State.GetCtrlID());
			int	iModObj = HIWORD(State.GetCtrlID());
			m_Mod[iModObj].GetValue(iModProp, &State.m_Val, sizeof(State.m_Val));
		}
		break;
	case UCODE_SPLINE:
		{
			CRefPtr<CUndoSpline>	pVal;
			pVal.CreateObj();
			pVal->m_arrSpline = m_arrSpline;
			State.SetObj(pVal);
		}
		break;
	case UCODE_LIGHTING:
		{
			CRefPtr<CUndoLighting>	pVal;
			pVal.CreateObj();
			pVal->m_vLightDir = m_vLightDir;
			pVal->m_mtrlPot = m_mtrlPot;
			State.SetObj(pVal);
		}
		break;
	case UCODE_STEP_ANIMATION:
	case UCODE_RANDOM_PHASE:
		SavePhases(State);
		break;
	}
}

void CPotterDrawDoc::RestoreUndoState(const CUndoState& State)
{
	switch (State.GetCode()) {
	case UCODE_PROPERTY:
		{
			int	iProp = State.GetCtrlID();
			switch (iProp) {
			case PROP_sTexturePath:
				RestoreString(State, m_sTexturePath);
				break;
			default:
				// only scalars, no strings or pointers
				SetValue(iProp, &State.m_Val, sizeof(State.m_Val));
				switch (iProp) {
				case PROP_nColors:
					RestorePalette(State);
					break;
				case PROP_bAnimation:
					RestorePhases(State);
					break;
				}
			}
			CPropertyHint	hint(iProp);
			UpdateAllViews(NULL, HINT_PROPERTY, &hint);
		}
		break;
	case UCODE_PALETTE:
		RestorePalette(State, State.GetCtrlID());
		UpdateAllViews(NULL, HINT_PALETTE);
		break;
	case UCODE_MODULATION:
		{
			int	iModProp = LOWORD(State.GetCtrlID());
			int	iModObj = HIWORD(State.GetCtrlID());
			m_Mod[iModObj].SetValue(iModProp, &State.m_Val, sizeof(State.m_Val));
			int	iModTarget, iModType = GetModulationType(iModObj, iModTarget);
			m_iModTarget = iModTarget;
			m_iModType = iModType;
			CModulationHint	hint(iModTarget, iModProp);
			UpdateAllViews(NULL, HINT_MODULATION, &hint);
		}
		break;
	case UCODE_SPLINE:
		{
			CUndoSpline	*pVal = static_cast<CUndoSpline *>(State.GetObj());
			m_arrSpline = pVal->m_arrSpline;
			UpdateAllViews(NULL, HINT_SPLINE);
		}
		break;
	case UCODE_LIGHTING:
		{
			CUndoLighting	*pVal = static_cast<CUndoLighting  *>(State.GetObj());
			m_vLightDir = pVal->m_vLightDir;
			m_mtrlPot = pVal->m_mtrlPot;
			UpdateAllViews(NULL, HINT_LIGHTING);
		}
		break;
	case UCODE_STEP_ANIMATION:
	case UCODE_RANDOM_PHASE:
		{
			RestorePhases(State);
			UpdateAllViews(NULL, HINT_MOD_PHASE);
		}
		break;
	}
}

CString CPotterDrawDoc::GetUndoTitle(const CUndoState& State)
{
	CString	sTitle;
	switch (State.GetCode()) {
	case UCODE_PROPERTY:
		{
			int	iProp = State.GetCtrlID();
			sTitle = GetPropertyName(iProp);
		}
		break;
	case UCODE_PALETTE:
		sTitle.LoadString(IDS_PALETTE_BAR);
		break;
	case UCODE_MODULATION:
		{
			int	iMod = LOWORD(State.GetCtrlID());
			sTitle.LoadString(IDS_MODULATION_BAR);
			sTitle += ' ' + m_Mod[0].GetPropertyName(iMod);
		}
		break;
	case UCODE_SPLINE:
		sTitle.LoadString(CSplineBar::GetUndoTitle(State.GetCtrlID()));
		break;
	case UCODE_LIGHTING:
		sTitle.LoadString(IDS_UNDO_LIGHTING);
		break;
	case UCODE_STEP_ANIMATION:
		sTitle.LoadString(IDS_UNDO_STEP_ANIMATION);
		break;
	case UCODE_RANDOM_PHASE:
		sTitle.LoadString(IDS_UNDO_RANDOM_PHASE);
		break;
	}
	return sTitle;
}

void CPotterDrawDoc::RelaySplineCmd(WPARAM nCmdID)
{
	CMainFrame	*pMainFrm = theApp.GetMainFrame();
	pMainFrm->GetSplineWnd().SendMessage(WM_COMMAND, nCmdID);	// relay command to spline window
}

void CPotterDrawDoc::RelaySplineUpdateCmdUI(CCmdUI *pCmdUI)
{
	CMainFrame	*pMainFrm = theApp.GetMainFrame();
	pMainFrm->GetSplineWnd().OnCmdMsg(pCmdUI->m_nID, CN_UPDATE_COMMAND_UI, pCmdUI, NULL);
	if (!pMainFrm->GetSplineBar().IsVisible())	// if bar is hidden
		pCmdUI->Enable(FALSE);	// disable command
}

void CPotterDrawDoc::RelayEditCmd(WPARAM nCmdID)
{
	CMainFrame	*pMainFrm = theApp.GetMainFrame();
	if (pMainFrm->GetPaletteBar().ColorBarHasFocus())	// if palette bar's color bar has focus
		pMainFrm->GetPaletteBar().SendColorBarCommand(nCmdID);	// relay command to palette bar
	else	// palette bar isn't the target
		RelaySplineCmd(nCmdID);
}

void CPotterDrawDoc::RelayEditUpdateCmdUI(CCmdUI *pCmdUI)
{
	CMainFrame	*pMainFrm = theApp.GetMainFrame();
	if (pMainFrm->GetPaletteBar().ColorBarHasFocus())	// if palette bar's color bar has focus
		pCmdUI->Enable(TRUE);	// enable command
	else	// palette bar isn't the target
		RelaySplineUpdateCmdUI(pCmdUI);
}

void CPotterDrawDoc::LoadTexture(LPCTSTR szPath)
{
	NotifyUndoableEdit(PROP_sTexturePath, UCODE_PROPERTY);
	m_sTexturePath = szPath;
	SetModifiedFlag();
	CPropertyHint	hint(PROP_sTexturePath);
	UpdateAllViews(NULL, HINT_PROPERTY, &hint);
}

// CPotterDrawDoc message map

BEGIN_MESSAGE_MAP(CPotterDrawDoc, CDocument)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DESELECT, OnUpdateEditDelete)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_EDIT_DESELECT, OnEditDeselect)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND_RANGE(ID_SPLINE_CMD_FIRST, ID_SPLINE_CMD_LAST, OnSplineCmd)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SPLINE_CMD_FIRST, ID_SPLINE_CMD_LAST, OnUpdateSplineCmd)
	ON_COMMAND(ID_FILE_LOAD_TEXTURE, OnFileLoadTexture)
	ON_COMMAND(ID_VIEW_RANDOM_PHASE, OnViewRandomPhase)
END_MESSAGE_MAP()

// CPotterDrawDoc message handlers

void CPotterDrawDoc::OnEditUndo()
{
	if (theApp.GetMainFrame()->PropertiesBarHasFocus() || !CFocusEdit::Undo()) {
		GetUndoManager()->Undo();
		SetModifiedFlag();	// undo counts as modification
	}
}

void CPotterDrawDoc::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	if (theApp.GetMainFrame()->PropertiesBarHasFocus() || !CFocusEdit::UpdateUndo(pCmdUI)) {
		CString	Text;
		Text.Format(LDS(IDS_EDIT_UNDO_FMT), GetUndoManager()->GetUndoTitle());
		pCmdUI->SetText(Text);
		pCmdUI->Enable(m_UndoMgr.CanUndo());
	}
}

void CPotterDrawDoc::OnEditRedo()
{
	GetUndoManager()->Redo();
	SetModifiedFlag();	// redo counts as modification
}

void CPotterDrawDoc::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	CString	Text;
	Text.Format(LDS(IDS_EDIT_REDO_FMT), GetUndoManager()->GetRedoTitle());
	pCmdUI->SetText(Text);
	pCmdUI->Enable(m_UndoMgr.CanRedo());
}

void CPotterDrawDoc::OnEditCut()
{
	if (!CFocusEdit::Cut()) {
		RelayEditCmd(ID_EDIT_CUT);
	}
}

void CPotterDrawDoc::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	if (!CFocusEdit::UpdateCut(pCmdUI)) {
		RelayEditUpdateCmdUI(pCmdUI);
	}
}

void CPotterDrawDoc::OnEditCopy()
{
	if (!CFocusEdit::Copy()) {
		RelayEditCmd(ID_EDIT_COPY);
	}
}

void CPotterDrawDoc::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	if (!CFocusEdit::UpdateCopy(pCmdUI)) {
		RelayEditUpdateCmdUI(pCmdUI);
	}
}

void CPotterDrawDoc::OnEditPaste()
{
	if (!CFocusEdit::Paste()) {
		RelayEditCmd(ID_EDIT_PASTE);
	}
}

void CPotterDrawDoc::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	if (!CFocusEdit::UpdatePaste(pCmdUI)) {
		RelayEditUpdateCmdUI(pCmdUI);
	}
}

void CPotterDrawDoc::OnEditDelete()
{
	if (!CFocusEdit::Delete()) {
		RelayEditCmd(ID_EDIT_DELETE);
	}
}

void CPotterDrawDoc::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	if (!CFocusEdit::UpdateDelete(pCmdUI)) {
		RelayEditUpdateCmdUI(pCmdUI);
	}
}

void CPotterDrawDoc::OnEditSelectAll()
{
	if (!CFocusEdit::SelectAll()) {
		RelayEditCmd(ID_EDIT_SELECT_ALL);
	}
}

void CPotterDrawDoc::OnEditDeselect()
{
}

void CPotterDrawDoc::OnUpdateEditSelectAll(CCmdUI *pCmdUI)
{
	if (!CFocusEdit::UpdateSelectAll(pCmdUI)) {
		pCmdUI->Enable(GetFocus() == theApp.GetMainFrame()->GetSplineWnd());
	}
}

BOOL CPotterDrawDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	TRY {
		ReadProperties(lpszPathName);
		if (!m_sTexturePath.IsEmpty()) {	// if texture path is specified
			// if texture path is relative and not found, try in document's folder
			if (PathIsRelative(m_sTexturePath) && !PathFileExists(m_sTexturePath)) {
				CPathStr	sTexturePath(lpszPathName);
				sTexturePath.RemoveFileSpec();
				sTexturePath.Append(PathFindFileName(m_sTexturePath));
				if (PathFileExists(sTexturePath))	// if found in document's folder
					m_sTexturePath = sTexturePath;	// update texture path
			}
		}
	}
	CATCH(CFileException, e) {
		e->ReportError();
		return FALSE;
	}
	CATCH(CUserException, e) {
		// invalid format; already reported
		return FALSE;
	}
	END_CATCH
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	return TRUE;
}

BOOL CPotterDrawDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnSaveDocument(lpszPathName))
		return FALSE;
	TRY {
		POSITION	pos = GetFirstViewPosition();
		if (pos != NULL) {
			CPotterDrawView	*pView = DYNAMIC_DOWNCAST(CPotterDrawView, GetNextView(pos));
			if (pView != NULL)
				pView->GetViewState(*this);
		}
		WriteProperties(lpszPathName);
	}
	CATCH(CFileException, e) {
		e->ReportError();
		return FALSE;
	}
	END_CATCH
	return TRUE;
}

void CPotterDrawDoc::OnSplineCmd(UINT nID)
{
	RelaySplineCmd(nID);
}

void CPotterDrawDoc::OnUpdateSplineCmd(CCmdUI *pCmdUI)
{
	RelaySplineUpdateCmdUI(pCmdUI);
}

void CPotterDrawDoc::OnFileLoadTexture()
{
	CString	sExt;
	int	nExts = _countof(m_arrTextureFileExt);
	for (int iExt = 0; iExt < nExts; iExt++) {	// for each texture file extension
		if (iExt)
			sExt += _T("; ");
		sExt += '*';
		sExt += m_arrTextureFileExt[iExt];
	}
	CString	sFilter;
	CString	sTextureFiles((LPCTSTR)IDS_TEXTURE_FILES);
	sFilter.Format(_T("%s (%s)|%s|"), sTextureFiles, sExt, sExt);
	sFilter += LDS(AFX_IDS_ALLFILTER) + _T("|*.*||");
	CFileDialog	fd(true, NULL, NULL, OFN_HIDEREADONLY, sFilter);
	if (fd.DoModal() == IDOK) {
		LoadTexture(fd.GetPathName());
	}
}

void CPotterDrawDoc::OnViewRandomPhase()
{
	NotifyUndoableEdit(0, UCODE_RANDOM_PHASE);
	for (int iProp = 0; iProp < MODULATIONS; iProp++) {	// for each potential primary or secondary modulation
		if (IsAnimatedModulation(iProp))	// if property is animated modulation
			m_Mod[iProp].m_fPhase = rand() / double(RAND_MAX);	// randomize its phase
	}
	UpdateAllViews(NULL, HINT_MOD_PHASE);
	SetModifiedFlag();
}
