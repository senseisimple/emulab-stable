//---------------------------------------------------------------------------
// Borland C++Builder
// Copyright (c) 1987, 1998 Inprise Corporation. All Rights Reserved.
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include <vcl.h>
#include <math.h>
#pragma hdrstop

#include "draw.h"
#include "hypview.h"
#include "hypdata.h"
#include <stdio.h>
#include <fstream>

//---------------------------------------------------------------------------
#pragma resource "*.dfm"
TOpenGL_Form *OpenGL_Form;
HypView *hv=NULL;
HWND ghWnd;
ifstream f;
bool drag = false;
 string prevsel;


/* This is the OnIdle event handler. It is set in the Form’s OnCreate event handler, so you need only add it as a private method of the form.  Usually it would perform some background processing for the application.  This one simply writes a message to let you know when it’s there. */

void __fastcall TOpenGL_Form::MyIdleHandler(TObject *Sender, bool &Done)
{
  if (hv) {
    int keepgoing = hv->idleCB();
    Done = !keepgoing;
  }
}


void hvSelectCB(const string &id,int,int) {
//            char *id, int /*shift*/, int /*control*/) {
//  if (strchr(id, '|')) {
  if (id.find_first_of ('|', 0) != string::npos) {
    // it's an edge, not a node
    hv->gotoPickPoint(HV_ANIMATE);
  } else {
 //   hv->setSelected(id, 1);
 //   hv->setSelected(prevsel, 0);
    hv->gotoNode(id, HV_ANIMATE);
  }
  hv->redraw();
//  strcpy(prevsel, id);
  prevsel = id;
}

//---------------------------------------------------------------------------
__fastcall TOpenGL_Form::TOpenGL_Form(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                        PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;
    ppfd->iPixelType = PFD_TYPE_RGBA; //PFD_TYPE_COLORINDEX;
    ppfd->cColorBits = 8;
    ppfd->cDepthBits = 32;

    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 )
    {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
    {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    return TRUE;

}

GLvoid TOpenGL_Form::resize( GLsizei width, GLsizei height )
{
    if (hv) {
      hv->reshape(width,height);
      hv->redraw();
    }
}



//---------------------------------------------------------------------------
void __fastcall TOpenGL_Form::FormCreate(TObject *Sender)
{
        ghWnd = Handle;
        ghDC = GetDC(Handle);
        if (!bSetupPixelFormat(ghDC))
            Close();
        ghRC = wglCreateContext(ghDC);
        wglMakeCurrent(ghDC, ghRC);
        Application->OnIdle = MyIdleHandler;

}
//---------------------------------------------------------------------------
void __fastcall TOpenGL_Form::FormResize(TObject *Sender)
{
	resize(ClientRect.Right, ClientRect.Bottom);
}
//---------------------------------------------------------------------------
void __fastcall TOpenGL_Form::FormClose(TObject *Sender, TCloseAction &Action)
{
        if (ghRC)
            wglDeleteContext(ghRC);
        if	(ghDC)
        	ReleaseDC(Handle, ghDC);

        if (hv != NULL) {
          delete hv;
          hv = NULL;
        }

}
//---------------------------------------------------------------------------
void __fastcall TOpenGL_Form::FormKeyDown(TObject *Sender, WORD &Key,
	TShiftState Shift)
{
WORD tk = Key;
if (!Shift.Contains(ssShift)) tk += 32;
//  char buf[31];
  unsigned char key = tk;
  static int gen = 30;
  static int labels = 1;
  static int sphere = 1;
  static int external = 1;
  static int orphan = 1;
  static int media = 1;
  static int group = 0;
  static int incoming = 0;
  static int selincoming = 0;
  static int subincoming = 0;
  static int outgoing = 0;
  static int seloutgoing = 0;
  static int suboutgoing = 0;
  static int rendstyle = 0;
  static int drawlinks = 1;
  static int drawnodes = 1;
  static int showcenter = 0;
  static int negativehide = 0;
  static int selsubtree = 0;
  static int labelright = 0;
  static int backcolor = 0;
  static int keepaspect = 0;
  hv->idle(0);
  if (key >= '0' && key <= '9') {
    gen = key-'0';
    if (rendstyle)
      hv->setGenerationNodeLimit(gen);
    else 
      hv->setGenerationLinkLimit(gen);
  } else if (key == '*') {
      hv->setGenerationNodeLimit(99);
      hv->setGenerationLinkLimit(99);
  } else if (key == 'l') {
    labels++;
    if (labels >= 3) labels = 0;
    hv->setLabels(labels);
  } else if (key == 'L') {
    labelright = !labelright;
    hv->setLabelToRight(labelright);
  } else if (key == 'b') {
    float labsize = hv->getLabelSize();
//    fprintf(stderr, "labsize %f\n", labsize);
    --labsize;
    hv->setLabelSize(labsize);
  } else if (key == 'B') {
    float labsize = hv->getLabelSize();
//    fprintf(stderr, "labsize %f\n", labsize);
    ++labsize;
    hv->setLabelSize(labsize);
  } else if (key == 's') {
    sphere = !(sphere);
    hv->setSphere(sphere);
  } else if (key == 'e') {
    external = !(external);
    hv->setEnableGroup(1, "external", external);
  } else if (key == 'o') {
    orphan = !(orphan);
    hv->setEnableGroup(1, "orphan", orphan);    
  } else if (key == 'i') {
    incoming = !(incoming);
    hv->setDrawBackTo("HV_TOPNODE", incoming, 1);
  } else if (key == 'j') {
    selincoming = !(selincoming);
    hv->setDrawBackTo(prevsel, selincoming, 1);
  } else if (key == 'k') {
    subincoming = !(subincoming);
    hv->setDrawBackTo(prevsel, subincoming, 0);
  } else if (key == 'u') {
    outgoing = !(outgoing);
    hv->setDrawBackFrom("HV_TOPNODE", outgoing, 1);
  } else if (key == 'v') {
    seloutgoing = !(seloutgoing);
    hv->setDrawBackFrom(prevsel, seloutgoing, 1);
  } else if (key == 'w') {
    suboutgoing = !(suboutgoing);
    hv->setDrawBackFrom(prevsel, suboutgoing, 0);
  } else if (key == 'S') {
    selsubtree = !(selsubtree);
    hv->setSelectedSubtree(prevsel, selsubtree);
  } else if (key == 'c') {
    hv->gotoCenterNode(HV_ANIMATE);
  } else if (key == 'C') {
    showcenter = !(showcenter);
    hv->setCenterShow(showcenter);
  } else if (key == 'r') {
    rendstyle = !(rendstyle);
  } else if (key == 'R') {
    backcolor++;
    if (backcolor >= 3) backcolor = 0;
    if (backcolor == 0) {
      hv->setColorBackground(1.0, 1.0, 1.0);
      hv->setColorSphere(.89, .7, .6);
      hv->setColorLabel(0.0, 0.0, 0.0);
    } else if (backcolor == 1) {
      hv->setColorBackground(0.5, 0.5, 0.5);
      hv->setColorSphere(.5, .4, .3);
      hv->setColorLabel(0.0, 0.0, 0.0);
    } else {
      hv->setColorBackground(0.0, 0.0, 0.0);
      hv->setColorSphere(.25, .15, .1);
      hv->setColorLabel(1.0, 1.0, 1.0);
    }
  } else if (key == 'N') {
    drawlinks = !(drawlinks);
    hv->setDrawLinks(drawlinks);
  } else if (key == 'n') {
    drawnodes = !(drawnodes);
    hv->setDrawNodes(drawnodes);
  } else if (key == 'h') {
    negativehide = !(negativehide);
    hv->setNegativeHide(negativehide);
//  } else if (key == 'f') {
//    char buf[1024];
//    if (gets(buf))
//      hv->gotoNode(buf, HV_ANIMATE);
  } else if (key == 'K') {
    keepaspect = !(keepaspect);
    hv->setKeepAspect(keepaspect);
  } else if (key == 'm') {
    media = !(media);
    hv->setEnableGroup(0, "image", media);
    hv->setEnableGroup(0, "text", media);
    hv->setEnableGroup(0, "application", media);
    hv->setEnableGroup(0, "audio", media);
    hv->setEnableGroup(0, "audio", media);
    hv->setEnableGroup(0, "video", media);
    hv->setEnableGroup(0, "other", media);
    //    hv->newLayout(NULL);
  } else if (key == 'g') {
    group = !(group);
    hv->setGroupKey(group);
  }
  hv->redraw();

//	Close();
}
//---------------------------------------------------------------------------
void __fastcall TOpenGL_Form::Start1Click(TObject *Sender)
{
          //Hypview
        hv = new HypView(ghDC,ghWnd);

        hv->bindCallback(HV_LEFT_CLICK, HV_PICK);
        hv->bindCallback(HV_PASSIVE, HV_HILITE);
        hv->bindCallback(HV_LEFT_DRAG, HV_TRANS);
        hv->bindCallback(HV_RIGHT_DRAG, HV_ROT);
        hv->setPickCallback(hvSelectCB);

        hv->clearSpanPolicy();
	hv->addSpanPolicy(HV_SPANFOREST);
	hv->addSpanPolicy(HV_SPANHIER);
	hv->addSpanPolicy(HV_SPANLEX);
 	hv->addSpanPolicy(HV_SPANBFS);
        f.open( "links.txt");

        hv->setLabelFont("Arial",12);

        hv->setGraph(f);
        hv->afterRealize(ghRC, ClientRect.Right, ClientRect.Bottom);
        hv->setSphere(1);
        hv->setLabels(1);
        hv->setNegativeHide(0);
        hv->setEnableGroup(0,"html",1);
        hv->setEnableGroup(0,"image",1);
        hv->setEnableGroup(0,"host",1);
        hv->setEnableGroup(0,"text",1);
        hv->setEnableGroup(0,"application",1);
        hv->setEnableGroup(0,"audio",1);
        hv->setEnableGroup(0,"video",1);
        hv->setEnableGroup(0,"other",1);
        hv->setEnableGroup(0,"vrml",1);
        hv->setEnableGroup(1, "external", 1);
        hv->setEnableGroup(1, "mainbranch", 1);
        hv->setEnableGroup(1, "orphan", 1);
        hv->setEnableGroup(2, "notrail", 1);
  hv->setColorGroup(0, "image", 1.0, 0.0, 1.0);
  hv->setColorGroup(0, "html", 0, 1, 1);
  hv->setColorGroup(0, "text", .90, .35, .05);
  hv->setColorGroup(0, "image", .42, 0, .48);
  hv->setColorGroup(0, "application", .99, .64, .25);
  hv->setColorGroup(0, "audio", .91, .36, .57);
  hv->setColorGroup(0, "video", .91, .36, .57);
  hv->setColorGroup(0, "other", 0, .35, .27);
  hv->setColorGroup(0, "vrml", .09, 0, 1);
  hv->setColorGroup(0, "host", 1.0, 1.0, 1.0);
  hv->setColorGroup(0, "invisible", 0, 0, 0);
  hv->setColorGroup(1, "host", 1.0, 1.0, 1.0);
  hv->setColorGroup(1, "main", 0, 1, 1);
  hv->setColorGroup(1, "orphan", 1.0, 0.0, 1.0);
  hv->setColorGroup(1, "external", 1.0, 1.0, 1.0);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);
  hv->redraw();
}
//---------------------------------------------------------------------------
void __fastcall TOpenGL_Form::ExitClick(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------



void __fastcall TOpenGL_Form::FormClick(TObject *Sender)
{
//        Timer1->Enabled = true;
       if (hv) hv->redraw();

}
//---------------------------------------------------------------------------


void __fastcall TOpenGL_Form::FormMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{

        if (hv != NULL) {
          if (drag) hv->motion(X,Y,0,0);
          else hv->passive(X,Y,0,0);
        }
}
//---------------------------------------------------------------------------

void __fastcall TOpenGL_Form::FormMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
        int i;
        if (Button == mbLeft) i=0;
        if (Button == mbRight) i=2;
        if (hv != NULL) {
           hv->mouse(i,0,X,Y,0,0);
        }
        drag = true;
}
//---------------------------------------------------------------------------

void __fastcall TOpenGL_Form::FormMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
        int i;
        if (Button == mbLeft) i=0;
        if (Button == mbRight) i=2;
        drag = false;
        if (hv != NULL) {
           hv->mouse(i,1,X,Y,0,0);
           char s[256];
        sprintf(s, "MOUSEUP\n");
 //      OutputDebugString(s);

        }

}
//---------------------------------------------------------------------------

void __fastcall TOpenGL_Form::FormPaint(TObject *Sender)
{
        if (hv != NULL) {
           hv->drawFrame();
 /*
           char s[256];
        sprintf(s, "PAINT\n");
       OutputDebugString(s);
 */
        }
}
//---------------------------------------------------------------------------

