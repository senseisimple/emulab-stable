// H3View.cpp : implementation of the CH3View class
//

#include "stdafx.h"
#include "h3.h"

#include "h3Doc.h"
#include "H3View.h"

#include "main.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CH3View

IMPLEMENT_DYNCREATE(CH3View, CView)

BEGIN_MESSAGE_MAP(CH3View, CView)
	//{{AFX_MSG_MAP(CH3View)
	ON_WM_CREATE()
	ON_WM_CHAR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CH3View construction/destruction

CH3View::CH3View()
{
	// TODO: add construction code here

}

CH3View::~CH3View()
{
}

BOOL CH3View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CH3View drawing

void CH3View::OnDraw(CDC* pDC)
{
	CH3Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
	draw();
}

/////////////////////////////////////////////////////////////////////////////
// CH3View printing

BOOL CH3View::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CH3View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CH3View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CH3View diagnostics

#ifdef _DEBUG
void CH3View::AssertValid() const
{
	CView::AssertValid();
}

void CH3View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CH3Doc* CH3View::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CH3Doc)));
	return (CH3Doc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CH3View message handlers

int drag = 0;

int CH3View::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here

	RECT lpr;
	GetClientRect(&lpr);
	create(m_hWnd, lpr.right, lpr.bottom);
	return 0;
}

void CH3View::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	keyboard(nChar);
	CView::OnChar(nChar, nRepCnt, nFlags);
}

void CH3View::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnLButtonDown(nFlags, point);
	mouse(0, 0, point.x, point.y, nFlags & MK_SHIFT, nFlags & MK_CONTROL);
	drag = 1;
}

void CH3View::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnLButtonUp(nFlags, point);
	mouse(0, 1, point.x, point.y, nFlags & MK_SHIFT, nFlags & MK_CONTROL);
	drag = 0;
}

void CH3View::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnMouseMove(nFlags, point);
	if (drag)
	  motion(point.x, point.y, nFlags & MK_SHIFT, nFlags & MK_CONTROL);
	else 
	  passive(point.x, point.y, nFlags & MK_SHIFT, nFlags & MK_CONTROL);
}

void CH3View::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnRButtonDown(nFlags, point);
	mouse(2, 0, point.x, point.y, nFlags & MK_SHIFT, nFlags & MK_CONTROL);
	drag = 1;
}

void CH3View::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnRButtonUp(nFlags, point);
	mouse(2, 1, point.x, point.y, nFlags & MK_SHIFT, nFlags & MK_CONTROL);
	drag = 0;
}

void CH3View::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	resize(cx, cy);
}
