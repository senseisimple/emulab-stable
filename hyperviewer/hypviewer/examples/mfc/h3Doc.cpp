// h3Doc.cpp : implementation of the CH3Doc class
//

#include "stdafx.h"
#include "h3.h"

#include "h3Doc.h"

#include "main.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CH3Doc

IMPLEMENT_DYNCREATE(CH3Doc, CDocument)

BEGIN_MESSAGE_MAP(CH3Doc, CDocument)
	//{{AFX_MSG_MAP(CH3Doc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CH3Doc construction/destruction

CH3Doc::CH3Doc()
{
	// TODO: add one-time construction code here

}

CH3Doc::~CH3Doc()
{
}

BOOL CH3Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CH3Doc serialization

void CH3Doc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CH3Doc diagnostics

#ifdef _DEBUG
void CH3Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void CH3Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CH3Doc commands

BOOL CH3Doc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	// TODO: Add your specialized creation code here
	
	return openfile(lpszPathName);
}
