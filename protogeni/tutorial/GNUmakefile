ALL=svg2pdf tutorial-handout.pdf tutorial-slides.pdf

CFLAGS=`pkg-config --cflags librsvg-2.0 cairo-pdf` -Wall -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -fno-strict-aliasing
LDFLAGS=`pkg-config --libs librsvg-2.0 cairo-pdf`

all: $(ALL)

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

%.pdf: %.svg svg2pdf
	./svg2pdf $< $@

tutorial-handout.pdf: tutorial-handout.tex
	pdflatex tutorial-handout

tutorial-slides.pdf: tutorial-slides.tex openssh.png putty-1.png putty-2.png \
	slice.pdf tutorial-diagram-1.pdf tutorial-diagram-2.pdf \
	tutorial-diagram-3.pdf tutorial-diagram-4.pdf tutorial-diagram-5.pdf \
	tutorial-diagram-6.pdf tutorial-diagram-7.pdf 
	pdflatex tutorial-slides

clean:
	-rm -f $(ALL) *.o *.pdf
