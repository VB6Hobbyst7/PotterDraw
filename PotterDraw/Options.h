// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      05apr17	initial version
		
*/

#pragma once

#include "Properties.h"

class COptions : public CProperties {
public:
// Construction
	COptions();

// Types

// Constants
	enum {	// groups
		#define GROUPDEF(name) GROUP_##name,
		#include "OptionsDef.h"
		GROUPS
	};
	enum {	// properties
		#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) PROP_##name,
		#include "OptionsDef.h"
		PROPERTIES
	};
	static const OPTION_INFO	m_Group[GROUPS];	// group names
	static const PROPERTY_INFO	m_Info[PROPERTIES];	// fixed info for each property

// Data members
	#define PROPDEF(group, proptype, type, name, initval, minval, maxval, itemname, items) type m_##name;
	#include "OptionsDef.h"

// Overrides
	virtual	int		GetGroupCount() const;
	virtual	int		GetPropertyCount() const;
	virtual	const PROPERTY_INFO&	GetPropertyInfo(int iProp) const;
	virtual	void	GetVariants(CVariantArray& Var) const;
	virtual	void	SetVariants(const CVariantArray& Var);
	virtual	CString	GetGroupName(int iGroup) const;

// Helpers
	void	ReadProperties();
	void	WriteProperties() const;
};
