void create(HWND hWnd, int w, int h);
void afterRealize(HDC cx);
int openfile(const char *fname);
void keyboard(unsigned char key);
void motion(int x, int y, int shift, int control);
void mouse(int b, int s, int x, int y, int shift, int control);
void passive(int x, int y, int shift, int control);
void draw();
void resize(int w, int h);
int onidle();


