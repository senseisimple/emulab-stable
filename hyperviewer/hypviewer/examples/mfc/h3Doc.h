// h3Doc.h : interface of the CH3Doc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H3DOC_H__24E584FF_0D87_11D3_90C4_00A0C996066F__INCLUDED_)
#define AFX_H3DOC_H__24E584FF_0D87_11D3_90C4_00A0C996066F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CH3Doc : public CDocument
{
protected: // create from serialization only
	CH3Doc();
	DECLARE_DYNCREATE(CH3Doc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CH3Doc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CH3Doc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CH3Doc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_H3DOC_H__24E584FF_0D87_11D3_90C4_00A0C996066F__INCLUDED_)
