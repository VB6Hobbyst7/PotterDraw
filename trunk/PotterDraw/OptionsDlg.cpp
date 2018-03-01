// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05apr17	initial version
        01      01sep17	add property help
		
*/

// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PotterDraw.h"
#include "OptionsDlg.h"

// COptionsDlg dialog

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)

#define RK_EXPAND _T("Options\\Expand")

COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPTIONS_GRID, m_Grid);
}

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	ON_BN_CLICKED(IDC_OPTIONS_RESET_ALL, OnClickedResetAll)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

// COptionsDlg message handlers

BOOL COptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect	rc;
	GetClientRect(rc);
	HDITEM hdItem;
	hdItem.mask = HDI_WIDTH;
	hdItem.cxy = rc.Width() / 2;
	m_Grid.GetHeaderCtrl().SetItem(0, &hdItem);
	m_Grid.EnableDescriptionArea();
	m_Grid.EnableHeaderCtrl(FALSE);
	m_Grid.SetDescriptionRows(2);
	m_Grid.InitPropList(theApp.m_Options);
	m_Grid.RestoreGroupExpansion(RK_EXPAND);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void COptionsDlg::OnOK()
{
	m_Grid.GetProperties(theApp.m_Options);
	m_Grid.SaveGroupExpansion(RK_EXPAND);

	CDialog::OnOK();
}

void COptionsDlg::OnClickedResetAll()
{
	if (AfxMessageBox(IDS_RESET_ALL_OPTIONS, MB_YESNO) == IDYES) {
		COptions	opt;
		m_Grid.SetProperties(opt);
		GotoDlgCtrl(GetDlgItem(IDOK));
	}
}

LRESULT COptionsDlg::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	int	iProp = m_Grid.GetCurSelIdx();
	int	nID = 0;
	if (iProp >= 0 && iProp < COptions::PROPERTIES)	// if valid property index
		nID = COptions::m_Info[iProp].nNameID;	// get property name resource ID
	else
		nID = IDD_OPTIONS;
	theApp.WinHelp(nID);
	return TRUE;
}
