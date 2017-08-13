// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda

		revision history:
		rev		date	comments
        00      23mar17	initial version

*/

#include "stdafx.h"
#include "resource.h"
#include "PropertiesGrid.h"
#include "Properties.h"

CValidPropertyGridCtrl::CValidPropertyGridCtrl()
{
	m_bInPreTranslateMsg = false;
	m_bIsDataValidated = false;
}

BOOL CValidPropertyGridCtrl::ValidateItemData(CMFCPropertyGridProperty* pProp)
{
	ASSERT_VALID(pProp);
	if (pProp->IsKindOf(RUNTIME_CLASS(CMFCPropertyGridColorProperty)))	// if color property
		return true;	// special case, ignore it
	CValidPropertyGridProperty	*pMyProp = STATIC_DOWNCAST(CValidPropertyGridProperty, pProp);
	m_bIsDataValidated = true;
	return pMyProp->ValidateData();
}

BOOL CValidPropertyGridCtrl::EndEditItem(BOOL bUpdateData)
{
	if (m_bInPreTranslateMsg && m_bIsDataValidated)
		return false;	// prevent duplicate message boxes
	return CMFCPropertyGridCtrl::EndEditItem(bUpdateData);
}

BOOL CValidPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
{
	m_bInPreTranslateMsg = true;
	m_bIsDataValidated = false;
	if (pMsg->message == WM_MOUSEMOVE) {
		pMsg->wParam = 0x10000;	// prevent base class from calling buggy TrackToolTip
		CPoint ptCursor;
		::GetCursorPos(&ptCursor);
		ScreenToClient(&ptCursor);
		TrackToolTip(ptCursor);	// call patched TrackToolTip instead
	}
	BOOL	bResult = CMFCPropertyGridCtrl::PreTranslateMessage(pMsg);
	m_bInPreTranslateMsg = false;
	return bResult;
}

// same as base class but patched to fix a bug: left-clicking a truncated property name 
// saturates the message loop with set/release capture messages, consuming all idle time
#define AFX_STRETCH_DELTA 2
void CValidPropertyGridCtrl::TrackToolTip(CPoint point)
{
	if (m_bTracking || m_bTrackingDescr)
	{
		return;
	}

	CPoint ptScreen = point;
	ClientToScreen(&ptScreen);

	CRect rectTT;
//	m_IPToolTip.GetWindowRect(&rectTT);	// root cause of bug; should be GetLastRect
	m_IPToolTip.GetLastRect(rectTT);	// tip control resets last rect on deactivation

	if (rectTT.PtInRect(ptScreen) && m_IPToolTip.IsWindowVisible())
	{
		return;
	}

/*	if (!m_IPToolTip.IsWindowVisible())
	{
		rectTT.SetRectEmpty();	// no need, tip control takes care of this
	}*/

	if (::GetCapture() == GetSafeHwnd())
	{
		ReleaseCapture();
	}

	CValidPropertyGridProperty* pProp = STATIC_DOWNCAST(CValidPropertyGridProperty, HitTest(point));
	if (pProp == NULL)
	{
		m_IPToolTip.Deactivate();
		return;
	}

	if (abs(point.x -(m_rectList.left + m_nLeftColumnWidth)) <= AFX_STRETCH_DELTA)
	{
		m_IPToolTip.Deactivate();
		return;
	}

	ASSERT_VALID(pProp);

	if (pProp->IsInPlaceEditing())
	{
		return;
	}

	CString strTipText;
	CRect rectToolTip = pProp->m_Rect;

	BOOL bIsValueTT = FALSE;

	if (point.x < m_rectList.left + m_nLeftColumnWidth)
	{
		if (pProp->IsGroup())
		{
			rectToolTip.left += m_nRowHeight;

			if (point.x <= rectToolTip.left)
			{
				m_IPToolTip.Deactivate();
				return;
			}
		}

		if (pProp->m_bNameIsTruncated)
		{
			if (!m_bAlwaysShowUserTT || pProp->GetNameTooltip().IsEmpty())
			{
				strTipText = pProp->m_strName;
			}
		}
	}
	else
	{
		if (pProp->m_bValueIsTruncated)
		{
			if (!m_bAlwaysShowUserTT || pProp->GetValueTooltip().IsEmpty())
			{
				strTipText = pProp->FormatProperty();
			}
		}

		rectToolTip.left = m_rectList.left + m_nLeftColumnWidth + 1;
		bIsValueTT = TRUE;
	}

	if (!strTipText.IsEmpty())
	{
		ClientToScreen(&rectToolTip);

		if (rectTT.TopLeft() == rectToolTip.TopLeft())
		{
			// Tooltip on the same place, don't show it to prevent flashing
			return;
		}

		m_IPToolTip.SetTextMargin(AFX_TEXT_MARGIN);

		m_IPToolTip.SetFont(bIsValueTT && pProp->IsModified() && m_bMarkModifiedProperties ? &m_fontBold : GetFont());

		m_IPToolTip.Track(rectToolTip, strTipText);
		SetCapture();
	}
	else
	{
		m_IPToolTip.Deactivate();
	}
}

BEGIN_MESSAGE_MAP(CValidPropertyGridCtrl, CMFCPropertyGridCtrl)
END_MESSAGE_MAP()

BOOL CValidPropertyGridProperty::ValidateData()
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pWndInPlace);
	ASSERT_VALID(m_pWndList);
	ASSERT(::IsWindow(m_pWndInPlace->GetSafeHwnd()));
	CString strText;
	m_pWndInPlace->GetWindowText(strText);
	CWnd	*pParent = m_pWndInPlace->GetParent();
	ASSERT_VALID(pParent);
	int	nID = m_pWndInPlace->GetDlgCtrlID();
	COleVariant	dst;
	BOOL	bResult = FALSE;
	bool	bRange = m_vMinVal.vt && m_vMaxVal.vt && m_vMinVal != m_vMaxVal;
	TRY {
		CDataExchange	dx(pParent, TRUE);
		switch (m_varValue.vt) {
		case VT_BSTR:
			break;
		case VT_UI1:
			DDX_Text(&dx, nID, dst.bVal);
			if (bRange)
				DDV_MinMaxByte(&dx, dst.bVal, m_vMinVal.bVal, m_vMaxVal.bVal);
			break;
		case VT_I2:
			DDX_Text(&dx, nID, dst.iVal);
			if (bRange)
				DDV_MinMaxShort(&dx, dst.iVal, m_vMinVal.iVal, m_vMaxVal.iVal);
			break;
		case VT_I4:
		case VT_INT:
			DDX_Text(&dx, nID, dst.intVal);
			if (bRange)
				DDV_MinMaxInt(&dx, dst.intVal, m_vMinVal.intVal, m_vMaxVal.intVal);
			break;
		case VT_UI4:
		case VT_UINT:
			DDX_Text(&dx, nID, dst.uintVal);
			if (bRange)
				DDV_MinMaxUInt(&dx, dst.uintVal, m_vMinVal.uintVal, m_vMaxVal.uintVal);
			break;
		case VT_R4:
			DDX_Text(&dx, nID, dst.fltVal);
			if (bRange)
				DDV_MinMaxFloat(&dx, dst.fltVal, m_vMinVal.fltVal, m_vMaxVal.fltVal);
			break;
		case VT_R8:
			DDX_Text(&dx, nID, dst.dblVal);
			if (bRange)
				DDV_MinMaxDouble(&dx, dst.dblVal, m_vMinVal.dblVal, m_vMaxVal.dblVal);
			break;
		case VT_BOOL:
			break;
		default:
			ASSERT(0);
		}
		bResult = TRUE;
	}
	CATCH (CUserException, e) {
	}
	END_CATCH;
	return bResult;
}

CString CValidPropertyGridProperty::FormatProperty()
{
	COleVariant& var = m_varValue;
	CString strVal;
	if (!m_bIsValueList) {
		switch (var.vt) {
		case VT_R4:
			strVal.Format(_T("%g"), (float)var.fltVal);
			return strVal;
		case VT_R8:
			strVal.Format(_T("%g"), (double)var.dblVal);
			return strVal;
		}
	}
	return CMFCPropertyGridProperty::FormatProperty();
}

void CValidPropertyGridCtrl::SaveGroupExpansion(LPCTSTR szRegKey) const
{
	CWinApp	*pApp = AfxGetApp();
	POSITION	pos = m_lstProps.GetHeadPosition();
	while (pos != NULL) {	// for each top-level property
		const CMFCPropertyGridProperty	*pProp = m_lstProps.GetNext(pos);
		pApp->WriteProfileInt(szRegKey, pProp->GetName(), pProp->IsExpanded());
	}
}

void CValidPropertyGridCtrl::RestoreGroupExpansion(LPCTSTR szRegKey) const
{
	CWinApp	*pApp = AfxGetApp();
	POSITION	pos = m_lstProps.GetHeadPosition();
	while (pos != NULL) {	// for each top-level property
		CMFCPropertyGridProperty	*pProp = m_lstProps.GetNext(pos);
		pProp->Expand(pApp->GetProfileInt(szRegKey, pProp->GetName(), TRUE));
	}
}

int CValidPropertyGridCtrl::GetActualDescriptionRows() const
{
	if (m_nDescrHeight >= 0 && m_nRowHeight > 0)	// if valid heights
		return m_nDescrHeight / m_nRowHeight;
	return m_nDescrRows;
}

CPropertiesGridCtrl::CPropertiesGridCtrl()
{
}

void CPropertiesGridCtrl::GetProperties(CProperties& Props) const
{
	int	nProps = Props.GetPropertyCount();
	CVariantArray	arrVar;
	arrVar.SetSize(nProps);
	for (int iProp = 0; iProp < nProps; iProp++) {	// for each property
		const CMFCPropertyGridProperty* pProp = m_arrProp[iProp];
		switch (Props.GetPropertyType(iProp)) {
		case CProperties::PT_ENUM:
			{
				CString	sVal(pProp->GetValue());
				int	nOptions = Props.GetOptionCount(iProp);
				int	iOption;
				for (iOption = 0; iOption < nOptions; iOption++) {	// for each option
					if (sVal == pProp->GetOption(iOption)) {
						arrVar[iProp] = iOption;
						break;
					}
				}
				ASSERT(iOption < nOptions);	// option not found
			}
			break;
		case CProperties::PT_COLOR:
			arrVar[iProp].uintVal = static_cast<const CMFCPropertyGridColorProperty *>(pProp)->GetColor();
			break;
		default:
			arrVar[iProp] = pProp->GetValue();
		}
	}
	Props.SetVariants(arrVar);
}

void CPropertiesGridCtrl::SetProperties(const CProperties& Props)
{
	CVariantArray	arrVar;
	Props.GetVariants(arrVar);
	int	nProps = Props.GetPropertyCount();
	for (int iProp = 0; iProp < nProps; iProp++) {	// for each property
		CMFCPropertyGridProperty* pProp = m_arrProp[iProp];
		switch (Props.GetPropertyType(iProp)) {
		case CProperties::PT_ENUM:
			{
				int	iOption = arrVar[iProp].intVal;
				LPCTSTR	szOption;
				if (iOption >= 0)
					szOption = pProp->GetOption(iOption);
				else
					szOption = NULL;
				pProp->SetValue(szOption);
			}
			break;
		case CProperties::PT_COLOR:
			static_cast<CMFCPropertyGridColorProperty *>(pProp)->SetColor(arrVar[iProp].uintVal);
			break;
		default:
			pProp->SetValue(arrVar[iProp]);
		}
	}
}

void CPropertiesGridCtrl::InitPropList(const CProperties& Props)
{
	int	nGroups = Props.GetGroupCount();
	m_arrGroup.SetSize(nGroups);
	for (int iGroup = 0; iGroup < nGroups; iGroup++) {	// for each group
		CString	sGroupName(Props.GetGroupName(iGroup));
		CMFCPropertyGridProperty*	pGroup = new CMFCPropertyGridProperty(sGroupName, DWORD_PTR(-1));
		AddProperty(pGroup);
		m_arrGroup[iGroup] = pGroup;
	}
	CVariantArray	arrVar;
	Props.GetVariants(arrVar);
	int	nProps = arrVar.GetSize();
	m_arrProp.SetSize(nProps);
	for (int iProp = 0; iProp < nProps; iProp++) {	// for each property
		CString	sPropName(Props.GetPropertyName(iProp));
		CString	sPropDescrip(Props.GetPropertyDescription(iProp));
		int	iGroup = Props.GetGroup(iProp);
		CMFCPropertyGridProperty*	pGroup = m_arrGroup[iGroup];
		CMFCPropertyGridProperty*	pProp;
		switch (Props.GetPropertyType(iProp)) {
		case CProperties::PT_ENUM:
			{
				pProp = new CMFCPropertyGridProperty(sPropName, _T(""), sPropDescrip, iProp);
				int	nOptions = Props.GetOptionCount(iProp);
				for (int iOption = 0; iOption < nOptions; iOption++)	// for each option
					pProp->AddOption(Props.GetOptionName(iProp, iOption));
				pProp->AllowEdit(false);
			}
			break;
		case CProperties::PT_COLOR:
			{
				CMFCPropertyGridColorProperty	*pColorProp = 
					new CMFCPropertyGridColorProperty(sPropName, arrVar[iProp].uintVal, NULL, sPropDescrip, iProp);
				pColorProp->EnableOtherButton(LDS(IDS_PROPS_MORE_COLORS));
				pProp = pColorProp;
			}
			break;
		case CProperties::PT_FILE:
			{
				CMFCPropertyGridFileProperty	*pFileProp = 
					new CMFCPropertyGridFileProperty(sPropName, TRUE, arrVar[iProp].bstrVal, NULL, 0, NULL, sPropDescrip, iProp);
				pProp = pFileProp;
			}
			break;
		default:
			CValidPropertyGridProperty	*pNumProp;
			pNumProp = new CValidPropertyGridProperty(sPropName, arrVar[iProp], sPropDescrip, iProp);
			Props.GetRange(iProp, pNumProp->m_vMinVal, pNumProp->m_vMaxVal);
			pProp = pNumProp;
		}
		pGroup->AddSubItem(pProp);
		m_arrProp[iProp] = pProp;
	}
}

int CPropertiesGridCtrl::GetCurSelIdx() const
{
	if (m_pSel != NULL)
		return INT64TO32(m_pSel->GetData());
	return -1;
}

void CPropertiesGridCtrl::SetCurSelIdx(int iProp)
{
	CMFCPropertyGridProperty	*pProp;
	if (iProp >= 0)
		pProp = m_arrProp[iProp];
	else
		pProp = NULL;
	SetCurSel(pProp);
}
