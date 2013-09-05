
MAKEINFO = makeinfo
HTML_FLAGS = --html --no-split
TEXI2DVI = texi2dvi
TEXI2DVI_FLAGS = --expand

TEXI_TEMP = *.log *.cp *.fn *.vr *.tp *.aux *~ *.toc *.ky *.pg *.vrs *.cps *.fns

SRC = pycodes.texi installing.texi overview.texi pyLDPC.texi

.PHONY: all help clean 

help:
	@echo "Targets:"  
	@echo " "
	@echo "help:          Prints this message."
	@echo "pycodes.info:     Creates info documentation."
	@echo "pycodes.html:     Creates html documentation."
	@echo "pycodes.dvi:      Creates dvi documentation."
	@echo "all:           Creates all documentation."
	@echo "clean:         Removes all generated files."
	@echo " "

all:
	${MAKE} pycodes.info pycodes.html pycodes.dvi 
	rm -f ${TEXI_TEMP}

clean:
	rm -f pycodes.info pycodes.html pycodes.dvi ${TEXI_TEMP}

pycodes.info: ${SRC}
	${MAKEINFO} pycodes.texi

pycodes.html: ${SRC}
	${MAKEINFO} ${HTML_FLAGS} pycodes.texi

pycodes.dvi: ${SRC}
	${TEXI2DVI} ${TEXI2DVI_FLAGS} pycodes.texi

