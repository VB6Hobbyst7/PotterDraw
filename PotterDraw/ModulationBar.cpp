// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      04may17	initial version
		01		01sep17	call help directly
		02		20oct17	in ctor, change enable modulation init to true
		
*/

#include "stdafx.h"
#include "PotterDraw.h"
#include "ModulationBar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

#define RK_MODULATION_BAR _T("ModulationBar\\")
#define RK_EXPAND _T("Expand")
#define RK_DESCRIPTION_ROWS _T("DescriptionRows")

CModulationBar::CModulationBar()
{
	m_pInitialProps = NULL;
	m_bEnableModulation = true;	// properties are initially enabled
}

CModulationBar::~CModulationBar()
{
}

void CModulationBar::SetInitialProperties(const CModulationProps& Props)
{
	m_pInitialProps = &Props;
}

void CModulationBar::GetProperties(CModulationProps& Props) const
{
	m_Grid.GetProperties(Props);
}

void CModulationBar::SetProperties(const CModulationProps& Props)
{
	m_Grid.SetProperties(Props);
}

void CModulationBar::InitPropList(const CProperties& Props)
{
	m_Grid.EnableHeaderCtrl(FALSE);
//	m_Grid.EnableDescriptionArea();
	m_Grid.SetVSDotNetLook();
//	m_Grid.MarkModifiedProperties();
	m_Grid.InitPropList(Props);
}

void CModulationBar::CMyPropertiesGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{
	CValidPropertyGridCtrl::OnPropertyChanged(pProp);
	AfxGetMainWnd()->SendMessage(UWM_PROPERTY_CHANGE, pProp->GetData(), reinterpret_cast<LPARAM>(GetParent()));
}

void CModulationBar::EnableModulation(bool bEnable)
{
	if (bEnable == m_bEnableModulation)	// if already in requested state
		return;	// nothing to do
	CMFCPropertyGridProperty	*pGroup = m_Grid.GetProperty(0);
	ASSERT(pGroup != NULL);
	int	nProps = CModulationProps::PROPERTIES;
	for (int iProp = 1; iProp < nProps; iProp++)	// for each property, excluding modulation target
		pGroup->GetSubItem(iProp)->Enable(bEnable);	// enable or disable property as requested
	m_bEnableModulation = bEnable;	// update shadow
}

BEGIN_MESSAGE_MAP(CModulationBar, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CModulationBar::AdjustLayout()
{
	if (GetSafeHwnd () == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect	rc;
	GetClientRect(rc);

	m_Grid.SetWindowPos(NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CModulationBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_Grid.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 0, 0), this, 0))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	if (m_pInitialProps != NULL)
		InitPropList(*m_pInitialProps);

	m_Grid.RestoreGroupExpansion(RK_MODULATION_BAR RK_EXPAND);
	m_Grid.SetDescriptionRows(AfxGetApp()->GetProfileInt(RK_MODULATION_BAR, RK_DESCRIPTION_ROWS, 3));

	AdjustLayout();
	return 0;
}

void CModulationBar::OnDestroy()
{
	CDockablePane::OnDestroy();
	m_Grid.SaveGroupExpansion(RK_MODULATION_BAR RK_EXPAND);
	AfxGetApp()->WriteProfileInt(RK_MODULATION_BAR, RK_DESCRIPTION_ROWS, m_Grid.GetActualDescriptionRows());
}

void CModulationBar::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CModulationBar::OnExpandAllProperties()
{
	m_Grid.ExpandAll();
}

void CModulationBar::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CModulationBar::OnSortProperties()
{
	m_Grid.SetAlphabeticMode(!m_Grid.IsAlphabeticMode());
}

void CModulationBar::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_Grid.IsAlphabeticMode());
}

void CModulationBar::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_Grid.SetFocus();
}

void CModulationBar::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
}

LRESULT CModulationBar::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	int	iProp = m_Grid.GetCurSelIdx();
	int	nID = 0;
	if (iProp >= 0 && iProp < CModulationProps::PROPERTIES)	// if valid property index
		nID = CModulationProps::m_Info[iProp].nNameID;	// get property name resource ID
	else
		nID = IDS_MODULATION_BAR;
	theApp.WinHelp(nID);
	return TRUE;
}
