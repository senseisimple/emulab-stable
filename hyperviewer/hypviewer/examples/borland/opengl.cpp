//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
USEFORM("draw.cpp", OpenGL_Form);
USERES("opengl.res");
USEFILE("HypData.h");
USEUNIT("hypview.cpp");
USEUNIT("HypTransform.cpp");
USEUNIT("HypQuat.cpp");
USEUNIT("HypPoint.cpp");
USEUNIT("HypGraph.cpp");
USEUNIT("HypNode.cpp");
USEUNIT("HypLink.cpp");
USEUNIT("HypNodeArray.cpp");
USEUNIT("HypHash.cpp");
USEUNIT("HypViewer.cpp");
USEUNIT("HypGroupArray.cpp");
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	try
	{
		Application->Initialize();
		Application->CreateForm(__classid(TOpenGL_Form), &OpenGL_Form);
                 Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	return 0;
}
//---------------------------------------------------------------------------
